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

static bool firstChoice = true;
static bool usingTwoOpt = true;
static bool randomInitial = true;
static bool testing = false;

constexpr int MAX_CHECKED_NODES = 1000; // *n
constexpr int MAX_SIDEWAYS_MOVES = 0;
constexpr int MAX_NEIGHBOUR_GENERATION = 1000;
constexpr double EPS = 1e-9;

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
	RandomIntGenerator rng;
};

struct CurrentInfo {
	long visitedNodes = 0;
	long checkedNodes = 0;
	int sidewaysMoves = 0;
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

static vector<Node> minCostSuccessor(const vector<Node>& current, const Info& info, CurrentInfo& currentInfo) {
	vector<Node> minCostSuccessor;
	double minCost = DBL_MAX;
	for (int i = 0; i < info.n; ++i) {
		for (int j = 0; j < info.n; ++j) {
			if (i != j) {
				vector<Node> successor = current;
				const Node temp = successor[i];
				successor[i] = successor[j];
				successor[j] = temp;
				double successorCost = evaluateCost(successor, info);
				currentInfo.checkedNodes++;
				if (successorCost < minCost) {
					minCost = successorCost;
					minCostSuccessor = successor;
				}
			}
		}
	}
	return minCostSuccessor;
}

static void printSolution(const vector<Node>& solution, Info& info) {
	for (const Node node : solution) {
		info.result << node.index << " ";
	}
	info.result << "\n" << evaluateCost(solution, info)<<"\n";
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

static vector<Node> firstChoiceHcs(const vector<Node>& initialSolution, Info& info, CurrentInfo& currentInfo) {
	vector<Node> current = initialSolution;
	while (true) {
		currentInfo.visitedNodes++;
		if (currentInfo.checkedNodes >= MAX_CHECKED_NODES * info.n) return current;
		//printSolution(current, info);
		const double costCurrent = evaluateCost(current, info);
		bool foundNeighbour = false;
		for (int i = 0; i < MAX_NEIGHBOUR_GENERATION; ++i) {
			vector<Node> neighbour = generateRandNeighbour(current, info);
			currentInfo.checkedNodes++;
			if (currentInfo.checkedNodes >= MAX_CHECKED_NODES * info.n) return current;
			if (evaluateCost(neighbour,info) < costCurrent) {
				current = neighbour;
				foundNeighbour = true;
				break;
			}
		}
		if (!foundNeighbour) return current;
	}
}

static vector<Node> hcs(const vector<Node>& initialSolution, Info& info, CurrentInfo& currentInfo) {
	if (firstChoice) return firstChoiceHcs(initialSolution, info, currentInfo);
	vector<Node> current = initialSolution;
	while (true) {
		currentInfo.visitedNodes++;
		if (currentInfo.checkedNodes >= MAX_CHECKED_NODES * info.n) return current;
		//printSolution(current, info);
		vector<Node> neighbour = minCostSuccessor(current, info, currentInfo);
		const double costNeighbour = evaluateCost(neighbour, info);
		const double costCurrent = evaluateCost(current, info);
		if (abs(costNeighbour - costCurrent) < EPS && currentInfo.sidewaysMoves < MAX_SIDEWAYS_MOVES) {
			current = neighbour;
			currentInfo.sidewaysMoves++;
		}
		else if (costNeighbour < costCurrent) {
			current = neighbour;
			currentInfo.sidewaysMoves = 0;
		}
		else return current;
	}
}

static vector<Node> randInitSolution(vector<Node> solution, Info& info) {
	if (!randomInitial) return solution;
	vector<Node> randSolution;
	while (!solution.empty()) {
		int i = info.rng.get(0, static_cast<int>(solution.size()) - 1);
		randSolution.emplace_back(solution[i]);
		solution.erase(solution.begin()+i);
	}
	return randSolution;
}

int main()
{
	Info info;
	info.result.open("result.txt");
	vector<Node> initialSolution;
	readNodes(initialSolution, info);
	do {
		CurrentInfo currentInfo;
		vector<Node> finalSolution = hcs(randInitSolution(initialSolution, info), info, currentInfo);
		printSolution(finalSolution, info);
		//info.result << "Visited Nodes: " << currentInfo.visitedNodes << "\nChecked Nodes: " << currentInfo.checkedNodes;
	} while (testing);
	
	return 0;
}


