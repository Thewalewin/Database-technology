//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <cmath>
SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){
    graph = g;
}

void SimpleEstimator::prepare() {
	// queryPath.clear();
	// pathLength = 0;
	uint32_t noLabels = graph->getNoLabels();
	labelSums.resize(noLabels);
	labelDistribution.resize(noLabels);

	for (auto source : graph->adj) {
		for (auto pair : source) {
			labelSums[pair.first] = labelSums[pair.first] + 1;
		}
	}

	for (uint32_t i; i < noLabels; i++) {
		labelDistribution[i] = (double)labelSums[i] / (double)graph->getNoEdges();
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
		ep.cardinalityEstimate = labelSums[ep.leftLabel];
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

    return cardStat {labelSums[result.leftLabel], result.cardinalityEstimate, labelSums[result.rightLabel]};
}