#include "stdio.h"
#include <vector>
using std::vector;
#include <string>
using std::string;

//to compile: g++ skeleton.cpp -g -fmax-errors=1 -std=c++14 -o skeleton.exe

string structname;
struct field{
	string name;
};
vector<field> fields;
vector<string> args;
//#define nextarg fprintf(stderr,"argc = %d, *argv = [%s]\n",argc,*argv);argv++;argc--;if(!(argc>0)){goto endloop;}
#define nextarg args.push_back(*argv);argv++;argc--;if(!(argc>0)){goto endloop;}

int main(int argc, char **argv){
	string type;
	string name;
	string str;
	bool nocanary = true;
	bool nomalloc = false;
	if(argc == 0){return 0;}
	if(argc == 1){printf("usage: %s horse boop apples -canary -nomalloc > horse.txt\n",*argv); return 0;}
	nextarg;
	structname = *argv; nextarg;
	mainloop:
		str = *argv; 
		if(str == "-canary"){
			nocanary = false;
			nextarg;
		}
		str = *argv; 
		if(str == "-nomalloc"){
			nomalloc = true;
			nextarg;
		}
		name = *argv; 
		fields.push_back({name});
		nextarg;
		goto mainloop;
	endloop:
	string strcanary;
	if(nocanary){
		strcanary = "";
	}else{
		fields.insert(fields.begin(),(field){"canary"});
		strcanary = "check_canary(this,str_"+structname+");";
	}
	printf("#ifndef %s_GUARD\n#define %s_GUARD\n\n",structname.c_str(),structname.c_str());
	printf("//generated using:\n//");
	for(int I = 0; I < args.size(); I++){
		printf("%s ",args[I].c_str());
	}
	printf("\n\n");
	printf("//struct %s\n",structname.c_str());
	for(int I = 0; I < fields.size(); I++){
		auto f = fields[I];
		//printf("//%d: %s %s\n", I, f.type.c_str(), f.name.c_str());
		printf("//    %d: %s\n", I, f.name.c_str());
	}
	printf("//\n\n");
	if(!nocanary){
		printf("str_%s: db \"%s\",0;\n\n",structname.c_str(),structname.c_str());
		printf("#ifndef CANARY_GUARD\n"
		"#define CANARY_GUARD\n"
		"void error(){int 1;}\n"
		"void check_canary(float this, float canary){\n"
		"   if(this[0] != canary){\n"
		"        error();\n"
		"   }\n"
		"}\n"
		"#endif\n\n");
	}
	
	//sizeof
	printf("float sizeof_%s(){return %d};\n\n",structname.c_str(), fields.size());
	//constructor
	printf("//default constructor\n");
	printf("void %s_constructor(float this){\n", structname.c_str());
	for(int I = 0; I < fields.size(); I++){
		if((I == 0) && !nocanary){
			printf("    this[0] = str_%s;\n",structname.c_str());
		}else{
			printf("    this[%d] = 0;\n",I);
		}
	}
	printf("}\n");
	printf("\n");
	printf("//copy constructor\n");
	printf("void %s_copy_constructor(float this, float that){\n", structname.c_str());
	for(int I = 0; I < fields.size(); I++){
		printf("    this[%d] = that[%d];\n",I,I);
	}
	if(!nocanary){printf("    %s\n",strcanary.c_str());}
	printf("}\n");
	printf("\n");
	printf("//default destructor\n");
	printf("void %s_destructor(float this){%s\n",structname.c_str(),strcanary.c_str());
	if(!nocanary){printf("    %s\n",strcanary.c_str());}
	printf("    //add your code here\n");
	printf("}\n");
	printf("\n");
	
	if(!nomalloc){
		printf("//equivalent to \"new %s\"\n",structname.c_str());
		printf("float %s_new(){\n", structname.c_str());
		printf("    float size = sizeof_%s();\n",structname.c_str());
		printf("    float p = malloc(size);\n");
		printf("    %s_constructor(p);\n",structname.c_str());
		printf("    return p;\n");
		printf("}\n");
		printf("\n");
		printf("//equivalent to \"delete %s\"\n",structname.c_str());
		printf("void %s_delete(float this){\n", structname.c_str());
		printf("    %s_destructor(this);\n",structname.c_str());
		printf("    free(this);\n");
		printf("}\n");
		printf("\n");
	}
	printf("//getters\n");
	for(int I = 0; I < fields.size(); I++){
		auto f = fields[I];
		printf("float %s_get_%s(float this){%sreturn this[%d];}\n",structname.c_str(),f.name.c_str(),strcanary.c_str(),I);
	}
	printf("\n");
	printf("//setters\n");
	for(int I = 0; I < fields.size(); I++){
		auto f = fields[I];
		printf("void %s_set_%s(float this, float N){%sthis[%d] = N;}\n",structname.c_str(),f.name.c_str(),strcanary.c_str(),I);
	}
	printf("\n#endif");
}








