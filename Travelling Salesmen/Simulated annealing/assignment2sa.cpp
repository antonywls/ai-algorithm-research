// ReSharper disable CppClangTidyMiscUseAnonymousNamespace
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <cfloat>
#include <random>

using namespace std;

static bool randomInitial = true;
static bool usingTwoOpt = true;
static bool testing = false;

static double initialTempScale = 100;
static double coolingFactor = 0.99950;

constexpr double MIN_TEMP = 0.0001;
constexpr int MAX_VISITED_NODES = 1000; // *n
constexpr int MEAN_DIFFERENCE_NODES = 100; //Number of nodes to use for checking the average difference.

//testing
constexpr int TEST_AVERAGE_ITERATIONS = 2;
constexpr int TEST_INIT_TEMP_ITERATIONS = 20;
constexpr int TEST_COOLING_FACTOR_ITERATIONS = 20;

constexpr double TEST_COOLING_FACTOR_FACTOR = 0.3;
constexpr double TEST_INIT_TEMP_FACTOR = 2;
constexpr double TEST_INITIAL_COOLING_FACTOR = 0.9;

struct RandomBoolGenerator {
	mt19937 gen;
	uniform_real_distribution<> dist;

	RandomBoolGenerator()
		: gen(random_device{}()), dist(0.0, 1.0) {
	}

	bool get(double probability) {
		return dist(gen) < probability;
	}
};

struct RandomIntGenerator {
	mt19937 gen;

	RandomIntGenerator() : gen(random_device{}()) {}

	int get(const int min, const int max) {
		uniform_int_distribution<> dist(min, max);
		return dist(gen);
	}
};

class Info {
public:
	int n;
	ofstream result;
	RandomBoolGenerator rbg;
	RandomIntGenerator rng;
};

struct CurrentInfo {
	double initTemp;
	double coolingFactor;
	long time = 0;
};

struct Node {
	int index;
	double x;
	double y;
};

static void readNodes(vector<Node>& nodes, Info& info) {
	string line;
	while (getline(cin, line)) {
		istringstream iss(line);
		Node node;
		iss >> node.index >> node.x >> node.y;
		nodes.emplace_back(node);
	}
	info.n = static_cast<int>(nodes.size());
}

static double evaluateCost(const vector<Node>& solution, const Info& info) {
	double totalCost = 0;
	for (int i = 0; i < info.n; i++) {
		const Node current = solution[i];
		const Node next = solution[(i + 1) % info.n];
		totalCost = totalCost + sqrt((current.x - next.x) * (current.x - next.x) + (current.y - next.y) * (current.y - next.y));
	}
	return totalCost;
}

static void printSolution(const vector<Node>& solution, Info& info) {
	for (const Node node : solution) {
		info.result << node.index << " ";
	}
	info.result << "\n" << evaluateCost(solution, info) << "\n";
}

static vector< Node> twoOpt(const vector<Node>& current, Info& info) {
	if (info.n < 4) return current;

	int i = info.rng.get(0, info.n - 1);
	int j = info.rng.get(0, info.n - 2);
	if (i == j) ++j;
	if (i > j)  swap(i, j);
	vector<Node> neighbour = current;
	reverse(neighbour.begin() + i, neighbour.begin() + j + 1);
	return neighbour;
}

static vector<Node> generateRandNeighbour(const vector<Node>& current, Info& info) {
	if (info.n < 2) return current;
	if (usingTwoOpt) return twoOpt(current, info);
	int i = info.rng.get(0, info.n - 1);
	int j = info.rng.get(0, info.n - 2);
	if (i == j) j++;
	vector<Node> neighbour = current;
	swap(neighbour[i], neighbour[j]);
	return neighbour;
}

static vector<Node> randInitSolution(vector<Node> solution, Info& info) {
	if (!randomInitial) return solution;
	vector<Node> randSolution;
	while (!solution.empty()) {
		int i = info.rng.get(0, static_cast<int>(solution.size()) - 1);
		randSolution.emplace_back(solution[i]);
		solution.erase(solution.begin() + i);
	}
	return randSolution;
}

static double getMeanDifference(const vector<Node>& solution, Info& info) {
	double mean = 0;
	for (int i = 0; i < MEAN_DIFFERENCE_NODES; ++i) {
		vector<Node> current = randInitSolution(solution, info);
		vector<Node> neighbour = generateRandNeighbour(current, info);
		mean = mean + abs(evaluateCost(current, info) - evaluateCost(neighbour, info));
	}
	mean = mean / MEAN_DIFFERENCE_NODES;
	return mean; 
}

static double getCurrentTemperature(CurrentInfo& currentInfo) {
	return currentInfo.initTemp * pow(currentInfo.coolingFactor, currentInfo.time);
}

static double getProbability(double deltaE, double temp) {
	return exp(0.0 - (deltaE / temp));
}

static vector<Node> sa(const vector<Node>& initialSolution, Info& info, CurrentInfo& currentInfo) {
	vector<Node> current = initialSolution;
	
	while (true) {
		currentInfo.time++;
		double temp = getCurrentTemperature(currentInfo);
		if (temp <= MIN_TEMP || currentInfo.time >= MAX_VISITED_NODES*info.n) return current;
		vector<Node> next = generateRandNeighbour(current, info);
		double deltaE = evaluateCost(next, info) - evaluateCost(current, info);
		if (deltaE <= 0 || info.rbg.get(getProbability(deltaE, temp))) current = next;
	}
}

int main()
{
	Info info;
	info.result.open("result.txt");
	vector<Node> initialSolution;
	readNodes(initialSolution, info);

	int initTempIterate = 0;
	do {
		if (testing) coolingFactor = TEST_INITIAL_COOLING_FACTOR;
		int coolingFactorIterate = 0;
		do {
			int averageIterate = 0;
			double averageCost = 0;
			int averageNodeVisited = 0;
			do {
				CurrentInfo currentInfo;
				currentInfo.coolingFactor = coolingFactor;
				currentInfo.initTemp = initialTempScale * getMeanDifference(initialSolution, info);
				vector<Node> finalSolution = sa(randInitSolution(initialSolution, info), info, currentInfo);
				if (!testing) {
					printSolution(finalSolution, info);
					//info.result << "Visited Nodes: " << currentInfo.time;
				}else {
					
				}
				averageNodeVisited += currentInfo.time;
				averageCost += evaluateCost(finalSolution, info);
				averageIterate++;
			} while (testing && averageIterate < TEST_AVERAGE_ITERATIONS);
			averageNodeVisited /= TEST_AVERAGE_ITERATIONS;
			averageCost /= TEST_AVERAGE_ITERATIONS;

			if (testing && averageNodeVisited < MAX_VISITED_NODES*info.n) info.result << initialTempScale << "; " << coolingFactor << "; " << averageCost << "; " << averageNodeVisited << "\n";

			coolingFactor += (1.0-coolingFactor)*TEST_COOLING_FACTOR_FACTOR;
			coolingFactorIterate++;
		} while (testing && coolingFactorIterate < TEST_COOLING_FACTOR_ITERATIONS);
		initialTempScale *= TEST_INIT_TEMP_FACTOR;
		initTempIterate++;
	} while (testing && initTempIterate < TEST_INIT_TEMP_ITERATIONS);



	return 0;
}


