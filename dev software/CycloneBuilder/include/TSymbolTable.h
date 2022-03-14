#pragma once
#include "stl.h"
#include "zcpu_types.h"

class TSymbolTable {
public:
	vector<cpu_file*> all_files;
	vector<cpu_func*> all_funcs;
	vector<cpu_var*> all_vars;
	vector<string> defined;
	vector<string> lowdefined;
	vector<struct ref> references;
	vector<string> labels;
	vector<string> lowlabels;
	vector<string> keywords;

	TSymbolTable();

	void print_all_vars();
	void print_all_defines();
	void print_all_funcs();
	void print_file(cpu_file* cpf);
};