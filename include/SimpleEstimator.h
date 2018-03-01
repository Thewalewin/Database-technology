//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"
class SimpleEstimator : public Estimator {    
public:
    using SimpleGraphPointer = std::shared_ptr<SimpleGraph>;
    explicit SimpleEstimator(SimpleGraphPointer &g);

    //a destructor of your estimator in which you'll clean up all the stuff you used during your estimation. No memleaks please!
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;
private:
    SimpleGraphPointer graph;
    SimpleGraphPointer prepareTree(RPQTree *q);
    cardStat computeEstimate(SimpleGraphPointer &graph);
    SimpleGraphPointer project(uint32_t label, bool inverse, SimpleGraphPointer &g);
    SimpleGraphPointer join(SimpleGraphPointer &leftTree, SimpleGraphPointer &rightTree);
};


#endif //QS_SIMPLEESTIMATOR_H
