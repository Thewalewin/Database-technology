//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"
using StringVec = std::vector<std::string>;

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
    if(est != nullptr) est->prepare();

    // prepare other things here.., if necessary

}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {

    cardStat stats {};

    for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->adj[source].empty()) stats.noOut++;
    }

    stats.noPaths = g->getNoDistinctEdges();

    for(int target = 0; target < g->getNoVertices(); target++) {
        if(!g->reverse_adj[target].empty()) stats.noIn++;
    }

    return stats;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    if(!inverse) {
        // going forward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    } else {
        // going backward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->reverse_adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(1);

    for(uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) {
        for (auto labelTarget : left->adj[leftSource]) {

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->adj[leftTarget]) {

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        return SimpleEvaluator::project(label, inverse, graph);
    }

    if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);

    }

    return nullptr;
}

StringVec SimpleEvaluator::getQueryNodeList(RPQTree *query) {
    if (query->isLeaf()){
        return StringVec {query->data};
    }
    if (query->isConcat()) {
        auto leftQueryNodeList = getQueryNodeList(query->left);
        auto rightQueryNodeList = getQueryNodeList(query->right);

        leftQueryNodeList.insert(leftQueryNodeList.end(), rightQueryNodeList.begin(), rightQueryNodeList.end());
        return leftQueryNodeList;
    }
}

void printVector(const StringVec& v) {
    std::cout << "\n";
    for (const auto& i : v) {
        std::cout << i << " ";
    }
    std::cout << "\n";
}

/**
 * Generate all permutations of logical plans possible with the nodes provided
 * a node can be a single vertex in the graph or a subquery. 
 * Returns a Vector with strings that represent a logical plan. This vector contains 
 * duplicate logical plans.
 */
StringVec SimpleEvaluator::makeLogicalPlans(const StringVec& nodes) {
    // Base case, there is only one node left, so no other permutation can be made.
    if (nodes.size() == 1) {
        return nodes;
    }
    else {
        StringVec planList;
        /*
         * For each pair of neighboring nodes, create all the possible plans
         * when you merge these nodes into a single node.
         */
        for (uint32_t i = 0; i < nodes.size() - 1; i++) {
            // Merge neighboring nodes and create new nodeList
            StringVec nodeList(nodes.begin(), nodes.begin() + i);
            std::string s("(" + nodes[i] + "/" + nodes[i+1] + ")");
            nodeList.push_back(s);
            nodeList.insert(nodeList.end(), nodes.begin() + i+2, nodes.end());

            // Recurse with the new "nodes" and append the result to the list of plans
            StringVec permutations = makeLogicalPlans(nodeList);
            planList.insert(planList.end(), permutations.begin(), permutations.end());
        }
        return planList;
    }
}

/**
 * Returns a query tree that represents the cheapest logical plan for the given query.
 */
RPQTree* SimpleEvaluator::getLogicalPlan(const StringVec& queryNodeList) {

    // Create all possible plans, sort them and remove duplicates.
    StringVec logicalPlans = makeLogicalPlans(queryNodeList);
    sort(logicalPlans.begin(), logicalPlans.end());
    logicalPlans.erase(unique(logicalPlans.begin(), logicalPlans.end()), logicalPlans.end());

    // Find best logical plan and return this.
    uint32_t cardinalityEstimate = UINT32_MAX;
    RPQTree* bestPlan = nullptr;
    for (auto plan : logicalPlans) {
        RPQTree* r = RPQTree::strToTree(plan);
        cardStat c = est->estimate(r);
        if (c.noPaths < cardinalityEstimate) {
            bestPlan = r;
            cardinalityEstimate = c.noPaths;
        }
    }    
    return bestPlan;
}

std::string SimpleEvaluator::getQueryString(const StringVec& nodeList){
    std::string queryString;

    for (auto s : nodeList) {
        queryString.append(s);
    }
    return queryString;
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    StringVec queryNodeList = getQueryNodeList(query);

    // First search the cache to see if an answer to this query was already calculated.
    std::string queryString = getQueryString(queryNodeList);
    auto search = cache.find(queryString);
    if ( search != cache.end()) {
        return search->second;
    }
    else {
        RPQTree* logicalPlan = getLogicalPlan(queryNodeList);

        if(logicalPlan == nullptr){
            //Something went wrong
            std::cerr << "Error: getLogicalPlan(query) returned nullptr,";
            std::cerr << " evaluating with default plan.";
            logicalPlan == query;
        }
        auto res = evaluate_aux(logicalPlan);
        cardStat result = SimpleEvaluator::computeStats(res);
        cache.emplace(queryString, result);
        return result;
    }
}