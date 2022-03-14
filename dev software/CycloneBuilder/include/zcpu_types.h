#pragma once
#include "stl.h"

#define REF_EXT 0
#define REF_INT_VAR 1
#define REF_INT_FUNC 2
#define REF_INT_LABEL 3
#define REF_STACK 4

struct cpu_var {
	string name;
	string lowname;
};
struct cpu_func {
	string signature;
	string name;
	string lowname;
	vector<cpu_var*> vars;
};
extern cpu_func* currentFunc;

struct cpu_file {
	string name;
	string lowname;
	vector<cpu_file*> included;
	vector<cpu_func*> funcs;
	vector<cpu_var*> vars;
};

struct ref {
	string name;
	string lowname;
	int type;
	cpu_func* func;
};

