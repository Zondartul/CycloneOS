#pragma once
#include "stl.h"
#include "dual_stream.h"

/*
extern string file_cfg;
extern string file_cfg_dir1;

extern string dir_cpuchip;
extern string file_main;
extern string dir_output;
*/

extern ofstream logs;
extern dual_stream dout;

//void GetCpuDir(string filename);
//void readParams(int argc, char** argv);
void read_config_file();
