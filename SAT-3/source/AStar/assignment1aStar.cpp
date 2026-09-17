//Introduction to Artificial Intelligence: Assignment 1
//Solution: Part 3 - A*-Search
//Student: 412262319 (Wei-Yan-Chen | Antony Wilson)

// ReSharper disable CppClangTidyMiscUseInternalLinkage

#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
#include <sstream>
#include <string>

using namespace std;

double A = 0.87;
constexpr int MAX_SCORE = 100000;

struct Literal {
	char sign;
	int index;
};

class Node {
public:
	vector<bool> solution;
	size_t solutionSize;
	int cost;
	double aStarCost;

	Node(const vector<bool>& solution, const vector<vector<Literal>>& matrix, size_t& m, const int& D);
	Node(const vector<bool>& solution, bool b, const vector<vector<Literal>>& matrix, size_t& m, const int& D);
	void init(const vector<bool>& temp, const vector<vector<Literal>>& matrix, size_t& m, const int& D);
	void h(const vector<vector<Literal>>& matrix);
	double g(const size_t& m, const int& D) const;

	int checkLiteralBool(const Literal& literal);
	bool operator<(const Node& other) const { return aStarCost > other.aStarCost; }
};

void readMatrix(vector<vector<Literal>>& matrix, int& D);
void printSolution(const vector<bool>& solution, ostream& result);
bool aStar(const vector<vector<Literal>>& matrix, vector<bool>& solution, const int& D, size_t& queueMaxSize,
          int& expandedNodes, int& visitedNodes, size_t& m);


int main() {
	ofstream result("result.txt");
	vector<vector<Literal>> matrix;
	int D = 0;
	readMatrix(matrix, D);
	size_t m = matrix.size();
	vector<bool> solution;
	int minimumVisNodes = 10000000;
	double optimalA = A;
	//while (A <= 1) {
		size_t queueMaxSize = 0;
		int expandedNodes = 0;
		int visitedNodes = 0;
		if (aStar(matrix, solution, D, queueMaxSize, expandedNodes, visitedNodes, m))
			printSolution(solution, result);
		else
			result << "No Solution found.";
		//result << "\nQueueMaxSize: " << queueMaxSize << "\nExpandedNodes: " << expandedNodes << "\nVisitedNodes: " <<
		//	visitedNodes<<"\nA: "<<A <<"\n";
		if (visitedNodes < minimumVisNodes) {
			minimumVisNodes = visitedNodes;
			optimalA = A;
		}
		A += 0.02;
	//}
	//result << "Optimal A: " << optimalA;
}

Node::Node(const vector<bool>& solution, const vector<vector<Literal>>& matrix, size_t& m, const int& D) {
	init(solution, matrix, m, D);
}

Node::Node(const vector<bool>& solution, bool b, const vector<vector<Literal>>& matrix, size_t& m, const int& D) {
	vector<bool> temp = solution;
	temp.emplace_back(b);
	init(temp, matrix, m, D);
}

void Node::init(const vector<bool>& temp, const vector<vector<Literal>>& matrix, size_t& m, const int& D) {
	this->solution = temp;
	solutionSize = this->solution.size();
	h(matrix);
	aStarCost = g(m, D) + cost;
}

bool aStar(const vector<vector<Literal>>& matrix, vector<bool>& solution, const int& D, size_t& queueMaxSize,
          int& expandedNodes, int& visitedNodes, size_t& m) {
	priority_queue<Node> pq;
	pq.emplace(vector<bool>{}, matrix, m, D);
	while (!pq.empty()) {
		queueMaxSize = max(queueMaxSize, pq.size());
		Node node = pq.top();
		expandedNodes++;
		pq.pop();

		if (static_cast<int>(node.solutionSize) == D) {
			if (node.cost == 0) {
				solution = node.solution;
				return true;
			}
		}
		else {
			for (const bool b : {false, true}) {
				Node temp(node.solution, b, matrix, m, D);
				visitedNodes++;
				if (temp.cost < MAX_SCORE) //Does not push nodes with maximum cost into the queue.
					pq.push(temp);
			}
		}
	}
	return false;
}

void Node::h(const vector<vector<Literal>>& matrix) {
	cost = static_cast<int>(matrix.size());
	for (const vector<Literal>& clause : matrix) {
		bool isFalse = true;
		for (Literal literal : clause) {
			const int x = checkLiteralBool(literal); //x: 1=True, 0=False, -1=Not assigned.
			if (x != 0) {
				isFalse = false;
				if (x == 1) cost--;
				break;
			}
		}
		if (isFalse) {
			cost = MAX_SCORE;
			break;
		}
	}
}

double Node::g(const size_t& m, const int& D) const {
	return static_cast<double>(solutionSize) * (static_cast<double>(m) / D) *  A;
	//Scales number of assigned variables proportional to the number of clauses.
}

int Node::checkLiteralBool(const Literal& literal) {
	if (literal.index >= static_cast<int>(solutionSize)) return -1; //Value not in Solution.
	if ((literal.sign == '+' && solution[literal.index]) || (literal.sign == '-' && !solution[literal.index])) return 1;
	//Literal = True.
	return 0; //Literal = False.
}

void readMatrix(vector<vector<Literal>>& matrix, int& D) {
	string line;
	while (getline(cin, line)) {
		istringstream iss(line);
		vector<Literal> clause(3);

		for (int i = 0; i < 3; ++i) {
			iss >> clause[i].sign >> clause[i].index;
			D = max(D, clause[i].index);
			clause[i].index--; //Such that indexing goes from 0 until D-1. 
			if (i < 2) iss.ignore();
		}

		matrix.emplace_back(std::move(clause));
	}
}

void printSolution(const vector<bool>& solution, ostream& result) {
	for (bool b : solution) {
		result << b << " ";
	}
}
