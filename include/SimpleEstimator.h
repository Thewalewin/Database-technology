//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;
	
	struct LabelStats {
		uint32_t startNodes;
		uint32_t edges;
		uint32_t endNodes;
	};

	struct EstimatorPair {
		uint32_t leftLabel;
		uint32_t rightLabel;
		uint32_t startNodes;
		uint32_t endNodes;
		uint32_t cardinalityEstimate;
	};
	
	std::vector<LabelStats> labelStats;
	
	EstimatorPair estimate_aux(RPQTree *q);
	uint32_t nrOfVertices;
	
	static bool sortPairs(const std::pair<uint32_t,uint32_t> &a, const std::pair<uint32_t,uint32_t> &b);
public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;

};


#endif //QS_SIMPLEESTIMATOR_H
