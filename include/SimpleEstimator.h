//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

template<int N>
struct Factorial {
	static constexpr uint64_t result = N * Factorial<N-1>::result;
};

template<>
struct Factorial<0> {
	static constexpr uint64_t result = 1; 
};

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;
	static constexpr uint64_t factorials[21] = {
		Factorial<0>::result,
		Factorial<1>::result,
		Factorial<2>::result,
		Factorial<3>::result,
		Factorial<4>::result,
		Factorial<5>::result,
		Factorial<6>::result,
		Factorial<7>::result,
		Factorial<8>::result,
		Factorial<9>::result,
		Factorial<10>::result,
		Factorial<11>::result,
		Factorial<12>::result,
		Factorial<13>::result,
		Factorial<14>::result,
		Factorial<15>::result,
		Factorial<16>::result,
		Factorial<17>::result,
		Factorial<18>::result,
		Factorial<19>::result,
		Factorial<20>::result,
	};
	
	uint32_t nrOfStartVertices = 0;
	uint32_t nrOfEndVertices = 0;
	uint32_t nrOfVertices = 0;
	uint32_t nrOfEdges = 0;
	uint32_t maxOutDegree = 0;
	uint32_t nrOfZeroOutDegree = 0;
	uint32_t pathLength = 0;
	
	uint32_t getMaxOutDegree();
	uint32_t getZeroDegree();
	uint32_t getPathLength(RPQTree *q);
	uint32_t countVertices(uint32_t label, bool inverse);
	uint32_t getNrOfStartVertices(RPQTree *q);
	uint32_t getNrOfEndVertices(RPQTree *q);
	double probLabel(uint32_t pathLength);
	double pathSum(uint32_t pathLength);
public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;

};


#endif //QS_SIMPLEESTIMATOR_H
