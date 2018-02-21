//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    /*
     * a constructor of your estimator in which you will initialize all the stuff needed for 
     * estimation. Do not do heavy stuff here, move it to prepare() instead!
     */ 
    graph = g;
}

void SimpleEstimator::prepare() {

    /* 
     * this is where you prepare your estimator given a graph represented by a SimpleGraph. 
     * This function executes once after loading the graph and before running the query workload. 
     * You might want to use it to construct any auxiliary structures you would need during 
     * the estimation.
     */

}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    /*
     * Given a query tree, produce a cardinality estimate packaged in a cardStat structure.
    */

    return cardStat {0, 0, 0};
}