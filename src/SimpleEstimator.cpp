//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

using SimpleGraphPointer = SimpleEstimator::SimpleGraphPointer;

SimpleEstimator::SimpleEstimator(SimpleGraphPointer &g){

    /*
     * a constructor of your estimator in which you will initialize 
     * all the stuff needed for estimation. Do not do heavy stuff here, 
     * move it to prepare() instead!
     */ 
    graph = g;
}

void SimpleEstimator::prepare() {
    /* 
     * this is where you prepare your estimator given a graph represented 
     * by a SimpleGraph. This function executes once after loading the 
     * graph and before running the query workload. You might want to use 
     * it to construct any auxiliary structures you would need during 
     * the estimation.
     * FIXME: In this naive approach (just executing the query and 
     * returning that as an estimate) there is no preparation required.
     */
    return;
}

SimpleGraphPointer SimpleEstimator::project(uint32_t projectLabel, bool inverse, SimpleGraphPointer &in) {

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

SimpleGraphPointer SimpleEstimator::join(SimpleGraphPointer &leftTree, SimpleGraphPointer &rightTree) {
    SimpleGraphPointer out = std::make_shared<SimpleGraph>(leftTree->getNoVertices());
    out->setNoLabels(1);

    for (uint32_t leftSource = 0; leftSource < leftTree->getNoVertices(); leftSource++) {
        for (auto labelTarget : leftTree->adj[leftSource]) {

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : rightTree->adj[leftTarget]) {

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }

    return out;    
}

SimpleGraphPointer SimpleEstimator::prepareTree(RPQTree *q) {
    if (q == nullptr) {
        return nullptr;
    }
    if (q->isLeaf()) {
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;
        uint32_t label;
        bool inverse;

        if (std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if (std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        return SimpleEstimator::project(label, inverse, graph);
    }

    if (q->isConcat()) {
        SimpleGraphPointer leftGraph = SimpleEstimator::prepareTree(q->left);
        SimpleGraphPointer rightGraph = SimpleEstimator::prepareTree(q->right);

        return SimpleEstimator::join(leftGraph, rightGraph);
    }

    return nullptr;
}

cardStat SimpleEstimator::computeEstimate (SimpleGraphPointer &graph) {

    cardStat stats {};
    if (graph == nullptr) {
        return stats;
    }
    for(int source = 0; source < graph->getNoVertices(); source++) {
        if(!graph->adj[source].empty()) stats.noOut++;
    }

    stats.noPaths = graph->getNoDistinctEdges();

    for(int target = 0; target < graph->getNoVertices(); target++) {
        if(!graph->reverse_adj[target].empty()) stats.noIn++;
    }

    return stats;
    //return stats;
}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    /*
     * Given a query tree, produce a cardinality estimate packaged in 
     * a cardStat structure.
     * In the naive approach where we return the evaluation we start by
     * adapting the Graph recursively. Once this is done the stats are computed
     */
    SimpleGraphPointer graph = prepareTree(q);
    return computeEstimate(graph);
}