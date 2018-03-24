//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <cmath>
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
	// queryPath.clear();
	// pathLength = 0;
	uint32_t noLabels = graph->getNoLabels();
	labelStats.resize(noLabels);
	labelDistribution.resize(noLabels);

	for (auto source : graph->adj) {
        uint32_t prevLabel = 0;
        bool first = true;

		std::sort(source.begin(), source.end(), SimpleEstimator::sortPairs);

		for (const auto &pair : source) {
			if (first || !(prevLabel == pair.first)){
				first = false;
				prevLabel = pair.first;
				labelStats[pair.first].startNodes++;
			}
			labelStats[pair.first].edges++;
		}
	}

	for (auto destination : graph->reverse_adj) {
		uint32_t prevLabel = 0;
        bool first = true;

		std::sort(destination.begin(), destination.end(), SimpleEstimator::sortPairs);
		for (const auto &pair : destination) {
			if (first || !(prevLabel == pair.first)) {
				first = false;
				prevLabel = pair.first;
				labelStats[pair.first].endNodes++;
			}
		}
	}
	
	for (uint32_t i=0; i < noLabels; i++) {
		labelDistribution[i] = (double)labelStats[i].edges / (double)graph->getNoEdges();
	}
}

/*
 * Retreive the queried path and put it into a vector, which makes it
 * easier to perform the estimations based on the path. 
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
		return ep;
	}

	if(q->isConcat()){
		EstimatorPair left = estimate_aux(q->left);
		EstimatorPair right = estimate_aux(q->right);
		EstimatorPair join;
		
		uint32_t Tleft = left.cardinalityEstimate;
		double Vleft = 1.0f;
		uint32_t Tright = right.cardinalityEstimate;
		double Vright = labelDistribution[right.leftLabel];

		join.leftLabel = left.leftLabel;
		join.rightLabel = right.rightLabel;
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