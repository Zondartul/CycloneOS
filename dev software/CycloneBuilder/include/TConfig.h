#pragma once
#include "stl.h"

struct TCfgDirs {
	string file_cfg = "cycloneBuilder.txt";
	string file_cfg_dir1 = "bin/";

	string dir_cpuchip; //directory where all cpu projects are stored
	string file_main;   //name of the main file
	string dir_folder;  //folder where the main file lies
	string dir_output;  //folder where output will be written
};

struct TConfig {
	TCfgDirs dirs;
};
