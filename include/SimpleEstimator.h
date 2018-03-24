//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;

	std::vector<uint32_t> labelSums;
	std::vector<double> labelDistribution;
	
	struct EstimatorPair {
		uint32_t leftLabel;
		uint32_t rightLabel;
		uint32_t cardinalityEstimate;
	};

	EstimatorPair estimate_aux(RPQTree *q);
public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;

};


#endif //QS_SIMPLEESTIMATOR_H
