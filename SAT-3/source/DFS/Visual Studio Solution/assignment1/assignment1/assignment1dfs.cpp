//Introduction to Artificial Intelligence: Assignment 1
//Solution: Part 1 - Depth-First-Search
//Student: 412262319 (Wei-Yan-Chen | Antony Wilson)

// ReSharper disable CppClangTidyMiscUseInternalLinkage
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

using namespace std;

struct Literal {
	char sign;
	int index;
};

bool checkLiteralBool(const Literal& literal, const vector<bool>& solution);
void readMatrix(vector<vector<Literal>>& matrix, int& D);
//Only call with solution vector size = D.
bool checkSolution(const vector<vector<Literal>>& matrix, const vector<bool>& solution);
void dfs(const vector<vector<Literal>>& matrix, vector<bool>& solution, bool& foundSolution, const int D, int& visitedNodes);
void printSolution(const vector<bool>& solution, ostream& result);

int main() {
	ofstream result("result.txt");

	vector<vector<Literal>> matrix;
	vector<bool> solution;
	int D = 0;
	int visitedNodes = 0;
	readMatrix(matrix, D);
	bool foundSolution = false;
	dfs(matrix, solution, foundSolution, D, visitedNodes);
	if (foundSolution) 
		printSolution(solution, result);
	else 
		result << "No Solution found.";
	//result << "\nVisitedNodes: " << visitedNodes;
	return 0;
}

void printSolution(const vector<bool>& solution, ostream &result) {
	for (bool b : solution) {
		result << b << " ";
	}
}

void dfs(const vector<vector<Literal>>& matrix, vector<bool>& solution, bool& foundSolution, const int D, int& visitedNodes) {
	if (foundSolution) return;
	if (static_cast<int>(solution.size()) == D) {
		if (checkSolution(matrix, solution)) {
			foundSolution = true;
		}
		return;
	}

	for (bool b : {false, true}) {
		solution.push_back(b);
		visitedNodes++;
		dfs(matrix, solution, foundSolution, D, visitedNodes);
		if (foundSolution) return;
		solution.pop_back();
	}
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

bool checkLiteralBool(const Literal& literal, const vector<bool>& solution) {
	if ((literal.sign == '+' && solution[literal.index]) || (literal.sign == '-' && !solution[literal.index])) return
		true;
	return false;
}

//Only call with solution vector size = D.
bool checkSolution(const vector<vector<Literal>>& matrix, const vector<bool>& solution) {
	for (const vector<Literal>& clause : matrix) {
		bool clauseVal = false;
		//cout << "Clause: \n";
		for (Literal literal : clause) {
			if (checkLiteralBool(literal, solution)) clauseVal = true;
			//cout << "    in "<< literal.sign << literal.index << " of " << solution[literal.index] << " to " << checkLiteralBool(literal, solution) << "\n";
		}
		//cout << "    evaluates to: " << clauseVal << "\n";
		//cout << "Checked solution: ";
		//printSolution(solution);
		//cout << clauseVal << "\n";
		if (!clauseVal) return false;
	}
	return true;
}