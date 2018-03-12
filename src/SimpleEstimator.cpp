//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <cmath>
SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
}

uint32_t SimpleEstimator::getMaxOutDegree() {
	uint32_t max = 0;
	for (const auto &l : graph->adj) {
		max = std::max(max, static_cast<uint32_t>(l.size()));
	}
	return max;
}

uint32_t SimpleEstimator::getZeroDegree() {
	//The nr of vertices with zero outgoing edges, is the nr of vertices - the vertices with at least one outgoing edge
	// return graph->getNoVertices() - graph->adj.size();
	return 0; // Apparently there cannot be vertices with zero out degree
}

void SimpleEstimator::prepare() {
	// Initialize the statistics that depend on the graph.
	nrOfVertices = graph->getNoVertices();
	nrOfEdges = graph->getNoEdges();
	maxOutDegree = getMaxOutDegree();
	nrOfZeroOutDegree = getZeroDegree();
}

//Get the path length of the query. This can be done using some regex tricks on the RPQTree string.
uint32_t SimpleEstimator::getPathLength(RPQTree *q) {
	if (q->isLeaf()){
		std::regex directLabel (R"((\d+)\+)");
		std::regex inverseLabel (R"((\d+)\-)");
		
		std::smatch matches;
		uint32_t length = 0;
		
		if(std::regex_search(q->data, matches, directLabel)) {
			length = matches.size();
		} else if(std::regex_search(q->data, matches, inverseLabel)) {
			length = matches.size();
		}
		return length;
	}
	if (q->isConcat()) {
		uint32_t length = 0;
		length += getPathLength(q->left);
		length += getPathLength(q->right);
		return length;
	}
	return 0;
}

//Counts the amount of vertices with the given label
uint32_t SimpleEstimator::countVertices(uint32_t label, bool inverse) {
	uint32_t count = 0;
	if(!inverse) {
		//going forward
		for (uint32_t source = 0; source < graph->getNoVertices(); source++){
			for (auto labelTarget : graph->adj[source]) {
				if(labelTarget.first == label) {
					count++;
					break; // We break because we only count the amount of vertices.
				}
			}
		}
	} else {
		//going backward
		for (uint32_t source = 0; source < graph->getNoVertices(); source++){
			for (auto labelTarget : graph->reverse_adj[source]) {
				if (labelTarget.first == label) {
					count++;
					break; // We break because we only count the amount of vertices.
				}
			}
		}
	}
	return count;
}

//Gets the nr of vertices that match the first query label
uint32_t SimpleEstimator::getNrOfStartVertices(RPQTree *q) {
	RPQTree *tree = q;
	if(tree->left != nullptr){
		while (!tree->left->isLeaf()) {
			tree = tree->left;
		}
		if (!tree->isLeaf()){
			tree = tree->left;
		}
	}
	std::regex directLabel (R"((\d+)\+)");
	std::regex inverseLabel (R"((\d+)\-)");

	std::smatch matches;

	uint32_t label;
	bool inverse;

	if(std::regex_search(tree->data, matches, directLabel)) {
		label = (uint32_t) std::stoul(matches[1]);
		inverse = false;
	} else if(std::regex_search(tree->data, matches, inverseLabel)) {
		label = (uint32_t) std::stoul(matches[1]);
		inverse = true;
	} else {
		std::cerr << "Label parsing failed!" << std::endl;
	}
	return countVertices(label, inverse);
}

//gets the nr of vertices that match the last query label
uint32_t SimpleEstimator::getNrOfEndVertices(RPQTree *q) {
	RPQTree *tree = q;
	if (tree->right != nullptr){
		while (!tree->right->isLeaf()) {
			tree = tree->right;
		}
		if(!tree->isLeaf()){
			tree = tree->right;
		}
	}
	std::regex directLabel (R"((\d+)\+)");
	std::regex inverseLabel (R"((\d+)\-)");

	std::smatch matches;

	uint32_t label;
	bool inverse;

	if(std::regex_search(tree->data, matches, directLabel)) {
		label = (uint32_t) std::stoul(matches[1]);
		inverse = false;
	} else if(std::regex_search(tree->data, matches, inverseLabel)) {
		label = (uint32_t) std::stoul(matches[1]);
		inverse = true;
	} else {
		std::cerr << "Label parsing failed!" << std::endl;
	}
	return countVertices(label, inverse);
}

constexpr uint64_t SimpleEstimator::factorials[];

double SimpleEstimator::probLabel(uint32_t pathLength){
	uint32_t distinctLabels = graph->getNoLabels();
	double avgPerLabel = (1.0f / distinctLabels) * nrOfEdges;
	double prob = 0;
	for (uint32_t i = 1; i <= pathLength; i++) {
		double fact = (factorials[pathLength]/ (factorials[i] * factorials[pathLength - i]));
		double j = std::pow(avgPerLabel/nrOfEdges, i);
		double k = std::pow((nrOfEdges-avgPerLabel)/nrOfEdges, pathLength - i);
		prob += fact * j * k;
	}
	if (prob < 0.0 || prob > 1.0){
		std::cout << "Error probability incorrect: p = " << prob;
		return 0;
	}
	return prob;
}

double SimpleEstimator::pathSum(uint32_t pathLength) {
	double sum = 0;
	double avg = nrOfEdges / nrOfVertices;
	for (uint32_t i = 1; i <= pathLength; i++) {
		sum += (pathLength + 1) * std::pow(avg + (avg - maxOutDegree)/(nrOfVertices-1), i) * (1 -probLabel(pathLength));
	}
	return sum;
}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    // perform your estimation here
	pathLength = getPathLength(q)/2;
	nrOfStartVertices = getNrOfStartVertices(q);
	nrOfEndVertices = getNrOfEndVertices(q);
	uint32_t pathCardinality = nrOfStartVertices * nrOfEndVertices * pathSum(pathLength);
    return cardStat {nrOfStartVertices, pathCardinality, nrOfEndVertices};
}