// ReSharper disable CppClangTidyMiscUseAnonymousNamespace
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <cfloat>
#include <random>
#include <numeric>

using namespace std;

static bool randomInitial = true;
static bool usingTwoOpt = true;
static bool testing = false;

static double kScale = 0.5;
static int maxGenerations = 100;


struct RandomIntGenerator {
	mt19937 gen;

	RandomIntGenerator() : gen(random_device{}()) {}

	int get(const int min, const int max) {
		uniform_int_distribution<> dist(min, max);
		return dist(gen);
	}
};

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

struct RandomWeightedGenerator {
	mt19937 gen;

	RandomWeightedGenerator()
		: gen(std::random_device{}()) {
	}

	int get(const vector<double>& weights) {
		double total = 0;
		for (double w : weights) total += w;

		uniform_real_distribution<> dist(0.0, total);
		double r = dist(gen);

		double cumulative = 0;
		for (int i = 0, n = weights.size(); i < n; ++i) {
			cumulative += weights[i];
			if (r < cumulative)
				return i;
		}

		return static_cast<int>(weights.size()) - 1;
	}
};

class Info {
public:
	int n;
	ofstream result;
	RandomIntGenerator rng;
	RandomBoolGenerator rbg;
	RandomWeightedGenerator rwg;
};

struct CurrentInfo {
	int time = 0;
	int k;
	long visitedNodes = 0;
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
		node.index--;
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
		info.result << node.index+1 << " ";
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
	int i = info.rng.get(0, info.n-1);
	int j = info.rng.get(0, info.n-2);
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
		solution.erase(solution.begin()+i);
	}
	return randSolution;
}

static vector<double> calculateFitness(const vector<vector<Node>>& population, const Info& info, CurrentInfo& currentInfo){
	vector<double> costRatio;
	costRatio.reserve(population.size());
	for (const vector<Node>& solution : population) {
		costRatio.push_back(1.0 / evaluateCost(solution, info));
		currentInfo.visitedNodes++;
	}
	return costRatio;
}

static vector<Node> reproduce(const vector<Node>& x, const vector<Node>& y, const vector<Node>& initialSolution, Info& info, CurrentInfo& currentInfo) {
	vector<vector<int>> neighbourTable(x.size());
	for (int i = 0; i < static_cast<int>(x.size()); ++i) {
		neighbourTable[x[i].index].push_back(x[(i + 1) % x.size()].index);
		neighbourTable[x[i].index].push_back(x[(i + x.size() - 1) % x.size()].index);

		neighbourTable[y[i].index].push_back(y[(i + 1) % y.size()].index);
		neighbourTable[y[i].index].push_back(y[(i + y.size() - 1) % y.size()].index);
	}
	vector<bool> used(x.size(), false);

	vector<Node> child;
	child.push_back(x[0]);
	used[x[0].index] = true;
	for (const int neighbour : neighbourTable[x[0].index]) {
		neighbourTable[neighbour].erase(remove(neighbourTable[neighbour].begin(), neighbourTable[neighbour].end(), x[0].index), neighbourTable[neighbour].end());
	}

	for (int i = 1; i < x.size(); ++i) {
		int last = child[i - 1].index;
		if (!neighbourTable[last].empty()) {
			int nextIdx = *min_element( 
				neighbourTable[last].begin(),
				neighbourTable[last].end());

			child.push_back(initialSolution[nextIdx]);
			used[nextIdx] = true;
		}else {
			for (int i = 0; i < x.size(); ++i) {
				if (!used[i]) {
					child.push_back(initialSolution[i]);
					break;
				}
			}
		}
		used[child[i].index] = true;
		for (const int neighbour : neighbourTable[child[i].index]) {
			neighbourTable[neighbour].erase(remove(neighbourTable[neighbour].begin(), neighbourTable[neighbour].end(), child[i].index), neighbourTable[neighbour].end());
		}
	}
	return child;
}

static vector<Node> minCostIndividual(const vector<vector<Node>>& population, Info& info, CurrentInfo& currentInfo) {
	vector<Node> minCostSolution = population[0];
	for (vector<Node> solution : population) {
		if (evaluateCost(solution, info) < evaluateCost(minCostSolution,info)) {
			minCostSolution = solution;
		}
	}
	return minCostSolution;
}

static vector<Node> ga(const vector<Node>& initialSolution, Info& info, CurrentInfo& currentInfo) {
	vector<vector<Node>> population;
	population.reserve(currentInfo.k);
	for (int i = 0; i < currentInfo.k; ++i) {
		population.emplace_back(randInitSolution(initialSolution, info));
	}

	while (currentInfo.time < maxGenerations) {
		currentInfo.time++;
		vector<double> fitness = calculateFitness(population, info, currentInfo);

		vector<vector<Node>> newPopulation;
		for (int i = 0; i < currentInfo.k; ++i) {
			vector<Node> x = population[info.rwg.get(fitness)];
			vector<Node> y = population[info.rwg.get(fitness)];
			vector<Node> child = reproduce(x, y, initialSolution, info, currentInfo);
			child = generateRandNeighbour(child, info);
			newPopulation.emplace_back(child);
		}
		population = newPopulation;
	}
	return minCostIndividual(population, info, currentInfo);
}

int main()
{
	Info info;
	info.result.open("result.txt");
	vector<Node> initialSolution;
	readNodes(initialSolution, info);
	do {
		CurrentInfo currentInfo;
		currentInfo.k = static_cast<int>(static_cast<double>(info.n) * kScale);
		vector<Node> finalSolution = ga(initialSolution, info, currentInfo);
		printSolution(finalSolution, info);
		//info.result << "Visited Nodes: " << currentInfo.visitedNodes;
	} while (testing);

	
	
	return 0;
}


