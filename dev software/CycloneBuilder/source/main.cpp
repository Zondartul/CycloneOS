#include "stdio.h"

//27 feb 2022: moved to VS, git.
// needs some work because for some reason only a few functions make it into the table.
// also, needs a better indication of what failed and what succeeded.
//27 feb x2 - builder is split into several files.
// next: either A) read the code and krump it old-style until it works
//          or B) convert to fancy OOP style

//cyclone build stuff:
//- follow #includes
//- hardware address calculator
//- function parsing to a symbol table
//- function instrumentation
//- code/data separation
//- put result in a new "kernel.txt" file?
//- build dependency record
//- possibly replace . by struct access? .get .set, vtable dynamic typing or something

//document all the generated tables
/*
	Tables:
	func_table.txt
	- contains the names and addresses of all globally defined variables and functions
	
	func_export_table.txt
	- contains only the addresses of all globally defined variables and functions
	
	func_import_table.txt
	- declarations of all globally defined variables and functions (for inclusion in client code)

	reference_table.txt
	- declarations of all references to external (not defined) functions and variables, for inclusion in client code.

*/

#include "stl.h"
#include <ctime>
#include "builder_cli.h"
#include "parser.h"
#include "symbol_table.h"
#include "TBuilder.h"

using namespace std;

int main(int argc, char **argv){
	time_t t1 = time(0);
	logs.open("_CycloneBuilder_log.txt");
	cout << "hello world" << endl;
	//dout << "hello " << "world!" << endl;
	TBuilder builder;
	builder.readParams(argc, argv);
	//readParams(argc,argv);
	
	//initRegexes();
	//keywordsLower();
	
	//stage 1 - parse the file and all includes.
	builder.parse();
	//builder.printSummary();
	builder.printSummary2();
	//stage 2 - output
	
	builder.output();
	
	time_t t2 = time(0);
	int run_time = t2-t1;
	//dout << "execution took "<<run_time/60 <<" min "<<run_time%60<<" s."<< endl;
	//dout << "done\n";
	cout << "done" << endl;
	return 0;
}