//Introduction to Artificial Intelligence: Assignment 1
//Solution: Part 2 - Greedy-Best-First-Search
//Student: 412262319 (Wei-Yan-Chen | Antony Wilson)

// ReSharper disable CppClangTidyMiscUseInternalLinkage

#include <fstream>
#include <iostream>
#include <vector>
#include <queue>	
#include <sstream>
#include <string>
#include <climits>
#include <iostream>

using namespace std;

struct Literal {
	char	sign;
	int		index;
};

class Node {
public:
	vector<bool>	solution;
	size_t			solutionSize;
	int				cost;

	Node(const vector<bool>& solution, const vector<vector<Literal>>& matrix);
	Node(const vector<bool>& solution, bool b, const vector<vector<Literal>>& matrix);
	void h(const vector<vector<Literal>>& matrix);
	int checkLiteralBool(const Literal& literal);
	bool operator<(const Node& other) const {return cost > other.cost;}
};

void readMatrix	(vector<vector<Literal>>& matrix, int& D);
void printSolution(const vector<bool>& solution, ostream& result);
bool gbfs(const vector<vector<Literal>>& matrix, vector<bool>& solution, const int& D, size_t& queueMaxSize, int& expandedNodes, int& visitedNodes);


int main() {
	ofstream result("result.txt");
	vector<vector<Literal>> matrix;
	int D = 0;
	size_t queueMaxSize = 0;
	int expandedNodes = 0;
	int visitedNodes = 0;
	readMatrix(matrix, D);
	vector<bool> solution;
	if (gbfs(matrix, solution, D, queueMaxSize, expandedNodes, visitedNodes)) 
		printSolution(solution, result);
	else
		result << "No Solution found.";
	//result << "\nQueueMaxSize: " << queueMaxSize << "\nExpandedNodes: " << expandedNodes << "\nVisitedNodes: " << visitedNodes;

}

Node::Node(const vector<bool>& solution, const vector<vector<Literal>>& matrix) {
	this->solution = solution;
	solutionSize = this->solution.size();
	h(matrix);
}

Node::Node(const vector<bool>& solution, bool b, const vector<vector<Literal>>& matrix) {
	this->solution = solution;
	this->solution.emplace_back(b);
	solutionSize = this->solution.size();
	h(matrix);
}

bool gbfs(const vector<vector<Literal>>& matrix, vector<bool>& solution, const int& D, size_t& queueMaxSize, int& expandedNodes, int&visitedNodes) {
	priority_queue<Node> pq;
	pq.emplace(vector<bool>{}, matrix);
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
		}else {
			for (const bool b : {false, true}) {
				Node temp(node.solution, b, matrix);
				visitedNodes++;
				if (temp.cost<INT_MAX) //Does not push nodes with maximum cost into the queue.
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
			cost = INT_MAX;
			break;
		}
	}
}

int Node::checkLiteralBool(const Literal& literal) {
	if (literal.index >= solutionSize) return -1; //Value not in Solution.
	if ((literal.sign == '+' && solution[literal.index]) || (literal.sign == '-' && !solution[literal.index])) return 1; //Literal = True.
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
