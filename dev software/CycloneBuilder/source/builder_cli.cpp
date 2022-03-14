#include "builder_cli.h"
#include "dual_stream.h"
#include <regex>
using namespace std;



ofstream logs;
dual_stream dout(cout, logs);



/*
void read_config_file() {
	ifstream fs_cfg;
	fs_cfg.open(file_cfg, ios::in);
	if (!fs_cfg.is_open()) { fs_cfg.open(file_cfg_dir1 + file_cfg, ios::in); }
	if (!fs_cfg.is_open()) { dout << "can't open file " << file_cfg << endl; exit(0); }
	smatch sm;
	string S;
	int n = 0;
	do {
		S = "";
		getline(fs_cfg, S);

		smatch sm;
		if (regex_match(S, sm, regex("cpu_root: (.*)"))) {
			dout << "cpu_root = [" << sm[1].str() << "]" << endl;
			dir_cpuchip = sm[1].str();
		}
		if (regex_match(S, sm, regex("main: (.*)"))) {
			dout << "main = [" << sm[1].str() << "]" << endl;
			file_main = sm[1].str();
		}
		n++;
	} while (!fs_cfg.eof());

	fs_cfg.close();
}
*/
