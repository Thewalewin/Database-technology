//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <cmath>
#include <set>
SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){
    graph = g;
}

// sort on the second item in the pair, then on the first (ascending order)
bool SimpleEstimator::sortPairs(const std::pair<uint32_t,uint32_t> &a, const std::pair<uint32_t,uint32_t> &b) {
    if (a.second < b.second) return true;
    if (a.second == b.second) return a.first < b.first;
    return false;
}

void SimpleEstimator::prepare() {
	uint32_t noLabels = graph->getNoLabels();
	nrOfVertices = graph->getNoVertices();
	labelStats.resize(noLabels);
	std::vector<std::set<uint32_t>> nodes(noLabels);
	
	/*
	 * Count the amount of distinct edges (v1,v2) per label.
	 * Per label, count the amount of distinct start and end nodes
	 */
	for (auto &source : graph->adj) {
        uint32_t prevLabel = 0;
		uint32_t prevTarget = 0;
        bool first = true;

		std::sort(source.begin(), source.end(), SimpleEstimator::sortPairs);
		for (const auto &pair : source) {
			if (first || !(prevLabel == pair.first && prevTarget == pair.second)){
				first = false;
				prevLabel = pair.first;
				prevTarget = pair.second;
				labelStats[pair.first].edges++;

				auto it = nodes[pair.first].find(pair.second);
				if(it == nodes[pair.first].end()){
					// node was not in set yet.
					nodes[pair.first].emplace(pair.second);
				}
			}	
		}
	}
	// Save nr of endnodes per label
	for (uint32_t i = 0; i < noLabels; i++) {
		labelStats[i].endNodes = nodes[i].size();
		nodes[i].clear();
	}

	for (auto &destination : graph->reverse_adj) {
		uint32_t prevLabel = 0;
		uint32_t prevSource = 0;
        bool first = true;

		std::sort(destination.begin(), destination.end(), SimpleEstimator::sortPairs);
		for (const auto &pair : destination) {
			if (first || !(prevLabel == pair.first && prevSource == pair.second)) {
				first = false;
				prevLabel = pair.first;
				prevSource = pair.second;

				auto it = nodes[pair.first].find(pair.second);
				if (it == nodes[pair.first].end()){
					//Node was not in set yet
					nodes[pair.first].emplace(pair.second);
				}
			}
		}
	}

	// Save nr of startnodes per label
	for (uint32_t i = 0; i < noLabels; i++) {
		labelStats[i].startNodes = nodes[i].size();
	}
}

/*
 * Recurse over the querytree and calculate the cardinality estimate for each subtree. 
 */
SimpleEstimator::EstimatorPair SimpleEstimator::estimate_aux(RPQTree *q) {
	if(q == nullptr) {
		return {0,0,0};
	}
	if(q->isLeaf()) {
		std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        EstimatorPair ep;

        if(std::regex_search(q->data, matches, directLabel)) {
            ep.leftLabel = (uint32_t) std::stoul(matches[1]);
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            ep.leftLabel = (uint32_t) std::stoul(matches[1]);
        } else {
			ep.leftLabel = 0;
            std::cerr << "Label parsing failed!" << std::endl;
        }
		ep.rightLabel = ep.leftLabel;
		ep.cardinalityEstimate = labelStats[ep.leftLabel].edges;
		std::cout << "\nEstimating " << ep.cardinalityEstimate << " possible edges for label " << ep.leftLabel;
		return ep;
	}

	if(q->isConcat()){
		EstimatorPair left = estimate_aux(q->left);
		EstimatorPair right = estimate_aux(q->right);
		EstimatorPair join;
		
		uint32_t Tleft = left.cardinalityEstimate;
		double Vleft = (double) labelStats[left.rightLabel].endNodes / (double) nrOfVertices;
		uint32_t Tright = right.cardinalityEstimate;
		double Vright = (double) labelStats[right.leftLabel].startNodes / (double) nrOfVertices;

		join.leftLabel = left.leftLabel;
		join.rightLabel = right.rightLabel;
		std::cout << "\n";
		std::cout << "TL: " << Tleft << ", ";
		std::cout << "VL: " << Vleft << ", ";
		std::cout << "TR: " << Tright << ",";
		std::cout << "VR: " << Vright << "\n";

		join.cardinalityEstimate = std::min(Tright * Tleft * Vleft, 
                                            Tleft * Tright * Vright);
		return join;
	}
}

cardStat SimpleEstimator::estimate(RPQTree *q) {
	// queryPath.clear();
	// pathLength = 0;
	EstimatorPair result = estimate_aux(q);

    return cardStat {labelStats[result.leftLabel].startNodes, result.cardinalityEstimate, labelStats[result.rightLabel].endNodes};
}