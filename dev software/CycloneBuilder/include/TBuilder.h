#pragma once
#include <string>
#include "TParser.h"
#include "TSymbolTable.h"
#include "TOutput.h"
using std::string;



class TBuilder {
public:
	TBuilder();
	TConfig cfg;
	//cli
	void readParams(int argc, char** argv);
	void GetCpuDir(string filename);
	//parse
	TParser parser;
	TSymbolTable symTable;
	void parse();
	void printSummary();
	void printSummary2();
	//output
	TOutput out;
	void output();

	//data
	cpu_file* cpf = 0;
};