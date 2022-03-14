#include "TSymbolTable.h"
#include "builder_cli.h"

TSymbolTable::TSymbolTable() {
	//vector<cpu_file*> all_files;
	//vector<cpu_func*> all_funcs;
	//vector<cpu_var*> all_vars;
	defined = { "CYCLONE_BUILDER" };
	labels = {
		"func_table","func_table_end",
		"var_table","var_table_end",
		"func_name_table","func_name_table_end",
		"var_name_table","var_name_table_end",
		"func_export_table","func_export_table_end",
		"var_export_table","var_export_table_end",
		"func_import_table","func_import_table_end",
		"var_import_table","var_import_table_end",
		"reference_table","reference_table_end",
		"reference_name_table","reference_name_table_end"
	};
	lowlabels = labels;
	//extern vector<string> keywords;
}


void TSymbolTable::print_all_vars() {
	dout << "\n\n" << all_vars.size() << " vars found:" << endl;
	for (auto I = all_vars.begin(); I != all_vars.end(); I++) {
		dout << (*I)->name << endl;
	}
}

void TSymbolTable::print_all_defines() {
	dout << "\n\n" << defined.size() << " defined symbols:" << endl;
	for (auto I = defined.begin(); I != defined.end(); I++) {
		dout << *I << endl;
	}
}


void TSymbolTable::print_all_funcs() {
	dout << all_funcs.size() << " functions found" << endl;
	for (auto I = all_funcs.begin(); I != all_funcs.end(); I++) {
		dout << (*I)->name << endl;
	}
	dout << endl;
}

void TSymbolTable::print_file(cpu_file* cpf) {
	dout << "file " << cpf->name << ": " << cpf->funcs.size() << " funcs" << endl;
	for (auto I = cpf->funcs.begin(); I != cpf->funcs.end(); I++) {
		dout << (*I)->name << "\t\t(" << (*I)->signature << ")" << endl;
	}
	for (auto I = cpf->included.begin(); I != cpf->included.end(); I++) {
		dout << "\nincluded from " << cpf->name << ":" << endl;
		print_file(*I);
	}
}
