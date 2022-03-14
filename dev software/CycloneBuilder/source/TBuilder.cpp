#include "TBuilder.h"
#include "builder_cli.h" //for dout
#include "stl.h"
#include "zcpu_types.h"
#include <regex>
using namespace std;
//---- init ---------
TBuilder::TBuilder() :parser(cfg, symTable),out(cfg,symTable) {}

//----- builder_cli -----------
void TBuilder::readParams(int argc, char** argv) {
	if (argc <= 1) {
		printf("usage: drag-and-drop the file of interest into this executable,\n"
			"all outputs will go into the /generated folder nearby\n");
	}
	else {
		string fullname = argv[1];
		//dout << "fullname: " << fullname << endl;
		GetCpuDir(fullname);
	}
}

void TBuilder::GetCpuDir(string filename) {
	regex reg_cpuchip = regex("(.*\\|/cpuchip\\|/).*");
	smatch sm;

	size_t len = strlen("/cpuchip/");
	size_t pos = filename.find("/cpuchip/");
	if (pos == -1) { pos = filename.find("\\cpuchip\\"); }
	if (pos == -1) {
		dout << "dir not found" << endl;
		exit(1);
	}
	else {
		cfg.dirs.dir_cpuchip = filename.substr(0, pos + len);
		//cfg.dirs.file_main = filename.substr(pos + len, -1);
		cfg.dirs.dir_folder = filename.substr(0, filename.find_last_of("\\/") + 1);
		cfg.dirs.file_main = filename.substr(filename.find_last_of("\\/") + 1, -1);

		//cfg.dirs.dir_output = filename.substr(0, filename.find_last_of("\\/") + 1) + "generated\\";
		cfg.dirs.dir_output = cfg.dirs.dir_folder + "generated\\";

		cout << "dir_cpuchip [" << cfg.dirs.dir_cpuchip << "]" << endl;
		cout << "file_main [" << cfg.dirs.file_main << "]" << endl;
		cout << "dir_folder [" << cfg.dirs.dir_folder << "]" << endl;
		cout << "dir_output [" << cfg.dirs.dir_output << "]" << endl;
		//dout << "dir_cpuchip [" << cfg.dirs.dir_cpuchip << "]" << endl;
		//dout << "file_main [" << cfg.dirs.file_main << "]" << endl;
		//dout << "dir_output [" << cfg.dirs.dir_output << "]" << endl;
	}
}

//-------parser --------------------

void TBuilder::parse() {
	err_data erdata;
	cpf = parser.read_cpu_file(cfg.dirs.file_main, erdata); //this is the main function
	if (!cpf) { dout << "can't read main file\n"; exit(0); }

	parser.retypeRefs();
}

void TBuilder::printSummary() {
	symTable.print_file(cpf);
	dout << "\n\n";
	symTable.print_all_funcs();
	symTable.print_all_vars();
	symTable.print_all_defines();
}

void TBuilder::printSummary2() {
	cout << "--- TBuilder summary ---" << endl;

	cout << "parsed " << symTable.all_files.size() << " files:" << endl;
	for (auto *file : symTable.all_files) {
		if (file) {
			cout << "  file [" << file->name << "]" << endl;
		}
		else {
			cout << "  NULL" << endl;
		}
	}

	cout << "--- end summary ---" << endl;
}

//------ output --------------

void TBuilder::output(){
	out.write_symbol_table();
}