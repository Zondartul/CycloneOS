#include "stdio.h"

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


#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <algorithm>
#include <ctime>
using namespace std;
using std::string;

string file_cfg = "cycloneBuilder.txt";
string file_cfg_dir1 = "bin/";

string r_ref = "\\**";
string r_type = "(?:(?:void|float|char)"+r_ref+")";
string r_ident = "(?:[_[:alpha:]][_[:alnum:]]*)";
string r_sp = "[[:space:]]*";
string r_spp = "[[:space:]]+";
string r_param = "(?:"+r_sp+"(?:"+r_type+" )?"+r_sp+r_ref+r_ident+r_sp+")";
string r_param_list = r_param+"?"+"(?:"+","+r_param+")*";
string r_func_signature = r_type+"[[:space:]]+"+r_ref+"("+r_ident+")"+"\\("+r_param_list+"\\)";
//init_hardware doesn't register

string dir_cpuchip;
string file_main;
//string filename_cpufile;
string dir_output;

struct cpu_var{
	string name;
	string lowname;
};
struct cpu_func{
	string signature;
	string name;
	string lowname;
	vector<cpu_var*> vars;
};
cpu_func *currentFunc = 0;

struct cpu_file{
	string name;
	string lowname;
	vector<cpu_file*> included;
	vector<cpu_func*> funcs;
	vector<cpu_var*> vars;
};

struct ref{
	string name;
	string lowname;
	int type;
	cpu_func *func;
};

#define REF_EXT 0
#define REF_INT_VAR 1
#define REF_INT_FUNC 2
#define REF_INT_LABEL 3
#define REF_STACK 4

//stolen from http://www.cplusplus.com/forum/general/20554/
class dual_stream {
public:
    dual_stream(std::ostream& os1, std::ostream& os2) : os1(os1), os2(os2) {}

    template<class T>
    dual_stream& operator<<(const T& x) {
        os1 << x;
        os2 << x;
        return *this;
    }
	typedef ostream& (*ostreamManip)(ostream&);
	dual_stream& operator<<(ostreamManip manip){
		manip(os1);
		manip(os2);
		return *this;
	}
	void flush(){
		os1.flush();
		os2.flush();
	}
private:
    std::ostream& os1;
    std::ostream& os2;
};

ofstream logs;
dual_stream dout(cout,logs);

void read_config_file();
cpu_file *read_cpu_file(string file);
void print_file(cpu_file *cpf);
void print_all_funcs();
void print_all_vars();
void print_all_defines();
void write_symbol_table();
void readParams(int argc, char **argv);
void initRegexes();
void addRef(string id);
void retypeRefs();
void keywordsLower();
//void initDefines();
string toLower(string S);
bool validRef(string id);
void parseFunctionSignature(cpu_func *f);
//void initLabels();

vector<cpu_file*> all_files;
vector<cpu_func*> all_funcs;
vector<cpu_var*> all_vars;
vector<string> defined = {"CYCLONE_BUILDER"};
vector<string> lowdefined;
vector<struct ref> references;
vector<string> labels = {
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
vector<string> lowlabels = labels;
extern vector<string> keywords;
regex reg_define;
regex reg_ifndef;
regex reg_else;
regex reg_ifdef;
regex reg_endif;
regex reg_include;
regex reg_func_signature;
regex reg_var;
regex reg_var_extra;
regex reg_pragma;
regex reg_string;
regex reg_ident;
regex reg_char;
regex reg_label;

int main(int argc, char **argv){
	//dout << r_func_signature << endl;
	//dout << "size: " << r_func_signature.length() << endl;
	time_t t1 = time(0);
	logs.open("_CycloneBuilder_log.txt");
	dout << "hello " << "world!" << endl;
	readParams(argc,argv);
	
	initRegexes();
	keywordsLower();
	//initDefines();
	//initLabels();
	//read_config_file();
	
	//stage 1 - parse the file and all includes.
	cpu_file *cpf = read_cpu_file(file_main); //this is the main function
	if(!cpf){dout << "can't read main file\n"; exit(0);}
	
	//stage 2 - output
	print_file(cpf);
	dout << "\n\n";
	print_all_funcs();
	print_all_vars();
	print_all_defines();
	
	retypeRefs();
	write_symbol_table();
	
	time_t t2 = time(0);
	int run_time = t2-t1;
	dout << "execution took "<<run_time/60 <<" min "<<run_time%60<<" s."<< endl;
	dout << "done\n";
	return 0;
}

void GetCpuDir(string filename){
	regex reg_cpuchip = regex("(.*\\|/cpuchip\\|/).*");
	smatch sm;
	// if(regex_match(filename, sm, reg_cpuchip)){
		// *dir = sm[0].str()
		// *file = filename.substr(sm.position(0)+sm.size(0),-1);
		// dout << "found dir ["<<*dir<<"] and file ["<<*file<<"]"<<endl;
	// }
	int len = strlen("/cpuchip/");
	int pos = filename.find("/cpuchip/");
	if(pos == -1){pos = filename.find("\\cpuchip\\");}
	if(pos == -1){
		dout << "dir not found"<<endl;
		exit(1);
	}else{
		dir_cpuchip = filename.substr(0,pos+len);
		file_main = filename.substr(pos+len,-1);
		dir_output = filename.substr(0,filename.find_last_of("\\/")+1)+"generated\\";
		dout << "dir_cpuchip ["<<dir_cpuchip<<"]"<<endl;
		dout << "file_main ["<<file_main<<"]"<<endl;
		dout << "dir_output ["<<dir_output<<"]"<<endl;
	}
}

void readParams(int argc, char **argv){
	if(argc <= 1){
		printf("usage: drag-and-drop the file of interest into this executable,\n"
		"all outputs will go into the /generated folder nearby\n");
	}else{
		string fullname = argv[1];
		dout << "fullname: "<<fullname <<endl;
		GetCpuDir(fullname);
	}
}

void initRegexes(){
	reg_define = regex("#define ("+r_ident+").*");
	reg_ifndef = regex("#ifndef ("+r_ident+").*");
	reg_else = regex("#else.*");
	reg_ifdef = regex("#ifdef ("+r_ident+").*");
	reg_endif = regex("#endif.*");
	reg_include = regex("#include <([^>]+)>");
	reg_pragma = regex("#pragma ([^[:space:]]+)(?: ([^[:space:]]+))?.*");
	reg_func_signature = regex(r_func_signature);
	//reg_var = regex(r_type+r_spp+"("+r_ident+")");
	reg_var = regex("(?:register )?"+r_type+r_sp+r_ref+"("+r_ident+")");
	reg_var_extra = regex(r_sp+","+r_sp+r_ref+"("+r_ident+")");
	reg_string = regex("\"(?:[^\\\\]|\\\\.)*?\"");
	reg_char = regex("\'(?:[^\\\\']|\\\\[^'])?\'");
	reg_ident = regex(r_ident);
	reg_label = regex("("+r_ident+"):");
}



void keywordsLower(){
	for(auto I = keywords.begin(); I != keywords.end(); I++){
		*I = toLower(*I);
	}
}

string toLower(string S){
	char C;
	for(int I = 0; I < S.length(); I++){
		C = S[I];
		if(C <= 'Z' && C >= 'A'){
			C = C - ('Z' - 'z');
		}
		S[I] = C;
	}
	return S;
}

void print_all_vars(){
	dout << "\n\n"<< all_vars.size() << " vars found:" << endl;
	for(auto I = all_vars.begin(); I != all_vars.end(); I++){
		dout << (*I)->name << endl;
	}
}

void print_all_defines(){
	dout << "\n\n" << defined.size() << " defined symbols:" << endl;
	for(auto I = defined.begin(); I != defined.end(); I++){
		dout << *I << endl;
	}
}

void write_symbol_table(){
	ofstream fs;
	
	// function and variable table, with names
	string filename = dir_output+"func_table.txt";
	fs.open(filename);
	if(!fs.is_open()){dout << "can't open "<<filename<<endl; exit(1);}
	fs << "#ifndef CYCLONE_BUILDER" << endl;
	fs << "func_table:" << endl;
	for(auto I = all_funcs.begin(); I != all_funcs.end(); I++){
		fs << "db " << (*I)->name << ", db str_" << (*I)->name << ";" << endl;
	}
	fs << "func_table_end:" << endl;
	fs << endl;
	fs << "var_table:" << endl;
	for(auto I = all_vars.begin(); I != all_vars.end(); I++){
		fs << "db &" << (*I)->name << ", db str_" << (*I)->name << ";" << endl;
	}
	fs << "var_table_end:" << endl;
	fs << endl;
	fs << "func_name_table:" << endl;
	for(auto I = all_funcs.begin(); I != all_funcs.end(); I++){
		fs << "str_" << (*I)->name << ": db \"" << (*I)->name << "\", db 0;" << endl;
	}
	fs << "func_name_table_end:" << endl;
	fs << endl;
	fs << "var_name_table:" << endl;
	for(auto I = all_vars.begin(); I != all_vars.end(); I++){
		fs << "str_" << (*I)->name << ": db \"" << (*I)->name << "\", db 0;" << endl;
	}
	fs << "var_name_table_end:" << endl;
	fs << "#endif" << endl;
	
	if(fs.is_open()){dout << "func_table written" << endl;}else{dout << "func_table: error" << endl;}
	fs.close();
	
	// symbol export table
	filename = dir_output+"func_export_table.txt";
	fs.open(filename);
	if(!fs.is_open()){dout << "can't open "<<filename<<endl; exit(1);}
	fs << "#ifndef CYCLONE_BUILDER" << endl;
	fs << "func_export_table:" << endl;
	for(auto I = all_funcs.begin(); I != all_funcs.end(); I++){
		fs << "db " << (*I)->name << ";" << endl;
	}
	fs << "func_export_table_end:" << endl;
	fs << endl;
	fs << "var_export_table:" << endl;
	for(auto I = all_vars.begin(); I != all_vars.end(); I++){
		fs << "db &" << (*I)->name << ";" << endl;	//vars are exported by ptr
	}
	fs << "var_export_table_end:" << endl;
	fs << "#endif" << endl;
	
	if(fs.is_open()){dout << "func_export_table written" << endl;}else{dout << "func_export_table: error" << endl;}
	fs.close();
	
	
	// symbol import table
	filename = dir_output+"func_import_table.txt";
	fs.open(filename);
	if(!fs.is_open()){dout << "can't open "<<filename<<endl; exit(1);}
	
	fs << "#ifndef CYCLONE_BUILDER" << endl;
	fs << "func_import_table:" << endl;
	for(auto I = all_funcs.begin(); I != all_funcs.end(); I++){
		fs << "float " << (*I)->name << ";" << endl;
	}
	fs << "func_import_table_end:" << endl;
	fs << endl;
	fs << "var_import_table:" << endl;
	for(auto I = all_vars.begin(); I != all_vars.end(); I++){
		fs << "float " << (*I)->name << ";" << endl;	//vars are ptrs, remember
	}
	fs << "var_import_table_end:" << endl;
	fs << "#endif" << endl;
	if(fs.is_open()){dout << "func_import_table written" << endl;}else{dout << "func_import_table: error" << endl;}
	fs.close();
	
	//reverse reference table
	filename = dir_output+"reference_table.txt";
	fs.open(filename);
	if(!fs.is_open()){dout << "can't open "<<filename<<endl; exit(1);}
	
	fs << "#ifndef CYCLONE_BUILDER" << endl;
	fs << "reference_table:" << endl;
	//int count = 700001;
	for(auto I = references.begin(); I != references.end(); I++){
		int type = I->type;
		string name = I->name;
		
		if(type == REF_STACK){continue;} //we don't handle stack vars. But we could.
		if(type == REF_EXT){fs << "float " << name << "= 0; ";}//": db __PTR__";}
		if(type == REF_INT_VAR){continue;}//{fs << "db &" << name <<",";} //we also don't handle
		if(type == REF_INT_FUNC){continue;}//{fs << "db " << name <<",";} //internals cause they
		if(type == REF_INT_LABEL){continue;}//{fs << "db "<< name <<",";} //just get relocated
		fs <<"db str2_"<< name<<";"<<endl;							     
	}
	//we also don't handle internals because
	//of autogenerated labels like 
	//...
	//je __5;
	//"else"
	//__6:
	//...
	//and because of const-expressions
	//like ...float arg = Arr+1
	//though that can be fixed by making them all floats.
	
	
	fs << "reference_table_end:"<<endl;
	fs << endl;
	fs << "reference_name_table:" << endl;
	for(auto I = references.begin(); I != references.end(); I++){
		int type = I->type;
		string name = I->name;
		
		if(type == REF_STACK){continue;} 
		if(type == REF_INT_VAR){continue;}
		if(type == REF_INT_FUNC){continue;}
		if(type == REF_INT_LABEL){continue;}
		fs << "str2_" << name << ": db \""<< name<<"\",0;"<<endl;
	}
	fs << "reference_name_table_end:" << endl;
	fs << "#endif" << endl;
	if(fs.is_open()){dout << "reference_table written" << endl;}else{dout << "reference_table: error" << endl;}
	fs.close();
	
}

void print_all_funcs(){
	dout << all_funcs.size() << " functions found" << endl;
	for(auto I = all_funcs.begin(); I != all_funcs.end(); I++){
		dout << (*I)->name << endl;
	}
	dout << endl;
}

void print_file(cpu_file *cpf){
	dout << "file " << cpf->name << ": " << cpf->funcs.size() << " funcs" << endl;
	for(auto I = cpf->funcs.begin(); I != cpf->funcs.end(); I++){
		dout << (*I)->name << "\t\t(" << (*I)->signature << ")" << endl;
	}
	for(auto I = cpf->included.begin(); I != cpf->included.end(); I++){
		dout << "\nincluded from " << cpf->name << ":" << endl;
		print_file(*I);
	}
}

#define advancebymatch(S2,sm) (S2 = S2.substr(sm.position(0)+sm.length(0),-1))

//read a HL-ZASM source code file 
//<file> - relative address of the file
//returns a structure with everything gathered from the file
cpu_file *read_cpu_file(string file){
	ifstream fs;
	fs.open(dir_cpuchip+"/"+file, ios::in);
	if(!fs.is_open()){return 0;}
	dout << "reading file " << file << endl;
	
	cpu_file *cpf = new cpu_file();
	cpf->name = file;
	
	string S;
	int line = 1;					//file line counter, for reporting
	bool if_skip = false;			//are we currently skipping an if-else block?
	int bracketLevel = 0;			//how deep are we in the nested brackets?
	bool multilineComment = false;	//are we currently skipping a multiline comment?
	do{
		getline(fs,S);
		dout << endl << "line "<< line << ": [" << S << "]" << endl;
		int brk_found = 0;
		int pos = -1;
		smatch sm;
		
		//proprocessor stuff!
		
		//string literal removal
		while(regex_search(S,sm,reg_string)){
			//dout << "line     ["<<S<<"]"<<endl;
			dout << "removed string literal \"" << sm[0].str() << "\"\n";
			S = S.substr(0,sm.position(0))+"0"+S.substr(sm.position(0)+sm.length(0),-1);
			//dout << "redacted ["<<S<<"]"<<endl;
		}
		
		//character literal removal
		while(regex_search(S,sm,reg_char)){
			dout << "removed character literal \"" << sm[0].str() << "\"\n";
			S = S.substr(0,sm.position(0))+"0"+S.substr(sm.position(0)+sm.length(0),-1);
		}
		
		//comment removal
		//relies on string remover for "//" case.
		pos = S.find("//");
		if(pos != -1){
			string Snew = S.substr(0,pos);
			//dout << "line:       ["<<S<<"]\n" << "no comment: ["<<Snew<<"]"<<endl;
			S = Snew;
		}
		//multiline comment removal
		pos = S.find("/*");
		if(pos != -1){
			string Snew = S.substr(0,pos);
			multilineComment = true;
			pos = S.find("*/");
			if(pos != -1){
				Snew = Snew + S.substr(pos,S.length());
				multilineComment = false;
			}
			S = Snew;
		}
		
		if(multilineComment){
			pos = S.find("*/");
			string Snew;
			if(pos != -1){
				Snew = S.substr(pos,S.length());
				multilineComment = false;
			}
			S = Snew;
		}
		
		//bracket counting
		//relies on string remover for "//" and "{" cases.
		//just dumbly counts the opening and closing brackets on a line
		if(!if_skip){
			brk_found = -1;
			brk_loop:
				brk_found = S.find("{",brk_found+1);
				if(brk_found != -1){
					bracketLevel++;
					//dout << "'{' found at " << brk_found << endl;
					goto brk_loop;
				}
			brk_found = -1;
			brk_loop2:
				brk_found = S.find("}",brk_found+1);
				if(brk_found != -1){
					bracketLevel--;
					//dout << "'}' found at " << brk_found << endl;
					if(bracketLevel == 0){
						//we check falling edge, cause opening bracket
						//may be away from func def
						currentFunc = 0;
						//also funcs inside funcs are not allowed
					}
					if(bracketLevel < 0){
						dout << "error: mismatched closing bracket" << endl; 
						dout << "line: "<<line<<", file: "<<file<<endl;
						exit(0);
					}
					goto brk_loop2;
				}
		}
		//dout << S << endl;
		//as per zCPU compiler limitation, ifdefs do not stack.
		if(regex_match(S,sm,reg_pragma)){ 
			if(sm[1].str() == "no_export"){
				dout << "#pragma no_export, ";
				//dout << "\nsm.size = " << sm.size() << endl;
				string flavor = sm[2].str();
				if(flavor != ""){
					string flavor = sm[2].str();
					dout << "[" << flavor << "] flavor;" << endl;
				}else{
					dout << "default flavour" << endl;
				}
			}//was I going to do something here?
			goto nextline;
		}
		if(if_skip){
			//if we are in an "#ifdef / #ifndef" block,
			//we ignore everything but "#endif".
			if(regex_match(S,sm,reg_endif)){
				if_skip = false;
			}
		}else{
			if(regex_match(S,sm,reg_define)){
				string def = sm[1].str();
				dout << "#defined \""<<def<<"\""<<endl;
				defined.push_back(def);
				lowdefined.push_back(toLower(def));
				//ref catching allowed here
			}
			if(regex_match(S,sm,reg_ifndef)){
				string def = sm[1].str();
				dout << "#ifndef \""<<def<<"\" ";
				if(find(defined.begin(),defined.end(),def) != defined.end()){
					dout << "(defined)" << endl;
					if_skip = true;
				}else{
					dout << "(undefined)" << endl;
				}
				goto nextline;
			}
			if(regex_match(S,sm,reg_else)){
				dout << "#else" << endl;
				if_skip = !if_skip;
				goto nextline;
			}
			if(regex_match(S,sm,reg_ifdef)){
				string def = sm[1].str();
				dout << "ifdef \""<<def<<"\" ";
				if(find(defined.begin(),defined.end(),def) != defined.end()){
					dout << "(defined)" << endl;
				}else{
					dout << "(undefined)" << endl;
					if_skip = true;
				}
				goto nextline;
			}
			if(regex_match(S,sm,reg_endif)){
				//we're not in an if-skip, so ignore
				goto nextline;
			}
			
			if(regex_match(S,sm,reg_include)){
				if(if_skip){
					dout << "found include <"<<sm[1].str() << ">, if-skipping." << endl;
				}else{
					//dout << sm[1].str() << endl;
					dout << "found include <"<<sm[1].str() << ">" << endl;
					string file_inc = sm[1].str();
					cpu_file *cpf2 = read_cpu_file(file_inc);
					if(cpf2){
						cpf->included.push_back(cpf2);
					}else{
						dout << "can't read included file " << file_inc << endl;
						exit(0);
					}
					goto nextline;
				}
			}
			
			//labels
			if(regex_search(S,sm,reg_label)){
				if(if_skip){
					dout << "found label "<<sm[1].str()<<", if-skipping." << endl;
				}else{
					dout << "found label "<<sm[1].str()<<endl;
					string name = sm[1].str();
					labels.push_back(name);
					lowlabels.push_back(toLower(name));
					advancebymatch(S,sm);
				}
			}
			
			 if(regex_search(S,sm,reg_func_signature)){
				if(if_skip){
					dout << "found func "<<sm[1].str()<<", if-skipping." << endl;
				}else{
					cpu_func *f = new cpu_func();
					f->signature = sm[0].str();
					f->name = sm[1].str();
					f->lowname = toLower(f->name);
					cpf->funcs.push_back(f);
					all_funcs.push_back(f);
					dout << "found func "<<sm[1].str()<< endl;
					
					parseFunctionSignature(f);
					currentFunc = f;
					//dout << "\n" << "from file " << file << ":" << endl;
					//dout << sm[0].str() << endl;
					//dout << "function: [" << sm[1].str() << "]" << endl;
					//dout << endl;
					advancebymatch(S,sm);
				}
			}else{
				//can't have var def on same line as func def, else regex borks.
				if(regex_search(S,sm,reg_var)){
					if(if_skip){
						dout << "found var " << sm[1].str() << ", if-skipping." << endl;
					}else{
						if(bracketLevel){
							dout << "found local var " << sm[1].str() << " (BL = " << bracketLevel << ")"<<endl;
							cpu_var *v = new cpu_var();
							v->name = sm[1].str();
							v->lowname = toLower(v->name);
							if(!currentFunc){dout << "ERROR: local var but no local func" << endl; exit(1);}
							currentFunc->vars.push_back(v);
						}else{
							dout << "found global var " << sm[1].str() << endl;
							cpu_var *v = new cpu_var();
							v->name = sm[1].str();
							v->lowname = toLower(v->name);
							cpf->vars.push_back(v);
							all_vars.push_back(v);
						}
					}
					advancebymatch(S,sm);
					while(regex_search(S,sm,reg_var_extra)){
						if(if_skip){
							dout << "found var " << sm[1].str() << ", if-skipping." << endl;
						}else{
							if(bracketLevel){
								dout << "found local var " << sm[1].str() << " (BL = " << bracketLevel << ")"<<endl;
								cpu_var *v = new cpu_var();
								v->name = sm[1].str();
								v->lowname = toLower(v->name);
								if(!currentFunc){dout << "ERROR: local var but no local func" << endl; exit(1);}
								currentFunc->vars.push_back(v);
							}else{
								dout << "found global var " << sm[1].str() << endl;
								cpu_var *v = new cpu_var();
								v->name = sm[1].str();
								v->lowname = toLower(v->name);
								cpf->vars.push_back(v);
								all_vars.push_back(v);
							}
						}
						advancebymatch(S,sm);
					}
				}
			} 
			/* 
			if(regex_search(S,sm,reg_func_signature)){
				if(if_skip){
					dout << "found func "<<sm[1].str()<<", if-skipping." << endl;
				}else{
					cpu_func *f = new cpu_func();
					f->signature = sm[0].str();
					f->name = sm[1].str();
					f->lowname = toLower(f->name);
					cpf->funcs.push_back(f);
					all_funcs.push_back(f);
					dout << "found func "<<sm[1].str()<< endl;
					
					parseFunctionSignature(f);
					currentFunc = f;
					//dout << "\n" << "from file " << file << ":" << endl;
					//dout << sm[0].str() << endl;
					//dout << "function: [" << sm[1].str() << "]" << endl;
					//dout << endl;
					advancebymatch(S,sm);
				}
			}
			//does regex still bork?
			//if var def on same line as func def?
			if(regex_search(S,sm,reg_var)){
				if(if_skip){
					dout << "found var " << sm[1].str() << ", if-skipping." << endl;
				}else{
					if(bracketLevel){
						dout << "found local var " << sm[1].str() << " (BL = " << bracketLevel << ")"<<endl;
						cpu_var *v = new cpu_var();
						v->name = sm[1].str();
						v->lowname = toLower(v->name);
						if(!currentFunc){dout << "ERROR: local var but no local func" << endl; exit(1);}
						currentFunc->vars.push_back(v);
					}else{
						dout << "found global var " << sm[1].str() << endl;
						cpu_var *v = new cpu_var();
						v->name = sm[1].str();
						v->lowname = toLower(v->name);
						cpf->vars.push_back(v);
						all_vars.push_back(v);
					}
				}
				advancebymatch(S,sm);
				while(regex_search(S,sm,reg_var_extra)){
					if(if_skip){
						dout << "found var " << sm[1].str() << ", if-skipping." << endl;
					}else{
						if(bracketLevel){
							dout << "found local var " << sm[1].str() << " (BL = " << bracketLevel << ")"<<endl;
							cpu_var *v = new cpu_var();
							v->name = sm[1].str();
							v->lowname = toLower(v->name);
							if(!currentFunc){dout << "ERROR: local var but no local func" << endl; exit(1);}
							currentFunc->vars.push_back(v);
						}else{
							dout << "found global var " << sm[1].str() << endl;
							cpu_var *v = new cpu_var();
							v->name = sm[1].str();
							v->lowname = toLower(v->name);
							cpf->vars.push_back(v);
							all_vars.push_back(v);
						}
					}
					advancebymatch(S,sm);
				}
			}
			 */
			
			//reference collection
			//after everything else cause we don't want to
			//accidentally grab definitions
			string S2 = S;
			while(regex_search(S2,sm,reg_ident)){
				string id = sm[0].str();
				if(validRef(id)){
					if(if_skip){
						//dout << "found reference to "<<sm[0].str()<<", if-skipping." << endl;
					}else{
						dout << "found reference to "<<sm[0].str()<< endl;
						addRef(id);
					}	
				}
				advancebymatch(S2,sm);
				//S2 = S2.substr(sm.position(0)+sm.length(0),-1);
			}
			
		}
		nextline:
		line++;
	}while(!fs.eof());
	fs.close();
	
	all_files.push_back(cpf);
	return cpf;
}

/*       () (ident) (float ident) (float ident, x)             */
void parseFunctionSignature(cpu_func *f){
	dout << endl << endl << endl;
	string sig = f->signature;
	dout << "sig1 = " << sig << endl;
	sig = sig.substr(sig.find("("),-1);
	dout << "sig2 = " << sig << endl;
	
	smatch sm;
	regex reg_param = regex("(?:"+r_sp+"(?:"+r_type+" )?"+r_sp+r_ref+"("+r_ident+")"+r_sp+")");
	while(regex_search(sig,sm,reg_param)){
		string name = sm[1];
		dout << "found param " << name << endl;
		//cpu_var *v = new cpu_var();
		//v->name = name;
		cpu_var *v = new cpu_var();
		v->name = name;
		v->lowname = toLower(name);
		f->vars.push_back(v);
		advancebymatch(sig,sm);
	}
	//exit(0); //debug exit
}

bool validRef(string ident){
	string ident_lower = toLower(ident);
	//note we are not searching for positions of references
	//- that would require full preprocessing with macro-expansion
	
	//not a reference if defined away;
	for(auto I = lowdefined.begin(); I != lowdefined.end(); I++){
		if(*I == ident_lower){goto vr_isinvalid;}
	}
	for(auto I = keywords.begin(); I != keywords.end(); I++){
		if(*I == ident_lower){goto vr_isinvalid;}
	}
	for(auto I = references.begin(); I != references.end(); I++){
		if((I->type == REF_STACK) && (I->func != currentFunc)){continue;} //ignore refs to other locals
		if(I->lowname == ident_lower){goto vr_isinvalid;}
	}
	
	vr_isvalid:
	//dout << "ref "<<ident<<" is Valid" << endl;
	return true;
	
	vr_isinvalid:
	//dout << "ref "<<ident<<" is invalid"<<endl;
	return false;
}

void addRef(string name){
	//ident is okay to add to references
	//dout << "found ident "<< ident << endl;
	int type = REF_EXT;
	struct ref R;
	string lowname = toLower(name);
	R.name = name;
	R.lowname = lowname;
	//if a stack variable with that name is already defined,
	//immediately make this a stack reference
	if(currentFunc){
		dout << "currentFunc = " << currentFunc->name << endl;
		auto &vars = currentFunc->vars;
		for(auto I = vars.begin(); I != vars.end(); I++){
			if((*I)->lowname == lowname){type = REF_STACK;}
		}
	}else{
		dout << "no currentFunc" << endl;
	}
	R.type = type;
	dout << "ref " << name << " is " << (type == REF_STACK? "REF_STACK" : "REF_EXT") << endl;
	R.func = currentFunc;
	references.push_back(R);
}

void retypeRefs(){
	for(auto J = references.begin(); J != references.end(); J++){
		string lowname = J->lowname;
		int type = J->type;
		if(type != REF_STACK){ //stack refs shadow other refs
			for(auto I = lowlabels.begin(); I != lowlabels.end(); I++){
				if(*I == lowname){type = REF_INT_LABEL;}
			}
			for(auto I = all_funcs.begin(); I != all_funcs.end(); I++){
				if((*I)->lowname == lowname){type = REF_INT_FUNC;}
			}
			for(auto I = all_vars.begin(); I != all_vars.end(); I++){
				if((*I)->lowname == lowname){type = REF_INT_VAR;}
			}
		}
		//handled at creation
		//if(J->func){
		//	auto &vars = J->func->vars;
		//	for(auto I = vars.begin(); I != vars.end(); I++){
		//		if((*I)->name == name){type == REF_STACK;}
		//	}				
		//}
		J->type = type;
	}
}


void read_config_file(){
	ifstream fs_cfg;
	fs_cfg.open(file_cfg, ios::in);
	if(!fs_cfg.is_open()){fs_cfg.open(file_cfg_dir1+file_cfg, ios::in);}
	if(!fs_cfg.is_open()){dout << "can't open file " << file_cfg << endl; exit(0);}
	smatch sm;
	string S;
	int n = 0;
	do{
		S = "";
		//fs_cfg >> S;
		getline(fs_cfg,S);
		//dout << n << ": [" << S << "]" << endl;
		
		smatch sm;
		if(regex_match(S,sm,regex("cpu_root: (.*)"))){
			dout << "cpu_root = [" << sm[1].str() << "]" << endl;
			dir_cpuchip = sm[1].str();
		}
		if(regex_match(S,sm,regex("main: (.*)"))){
			dout << "main = [" << sm[1].str() << "]" << endl;
			file_main = sm[1].str();
		}
		n++;
	}while(!fs_cfg.eof());
	
	fs_cfg.close();
}

vector<string> keywords = {
	"mov",
	"div",
	"frnd",
	"out",
	"int",
	"min",
	"mcopy",
	"sub",
	"add",
	"ret",
	"inc",
	"rstack",
	"fint",
	"fpwr",
	"mod",
	"FFRAC",
	"FLN",
	"FE",
	"FPI",
	"FLOG10",
	"FABS",
	"FCEIL",
	"max",
	"rand",
	"mul",
	"FSIN",
	"FCOS",
	"FTAN",
	"FINV",
	"FASIN",
	"FACOS",
	"FATAN",
	"timer",
	"NOP",
	"push",
	"call",
	"pop",
	"enter",
	"cpuget",
	"in",
	"jmp",
	"CPUSET",
	"STM",
	"CLM",
	"lidtr",
	"leave",
	"iret",
	"clef",
	"stef",
	"sti",
	"cli", //opcodes end
	"programsize",
	"__PROGRAMSIZE__",
	"__DATE_YEAR__",  
	"__DATE_MONTH__", 
	"__DATE_DAY__",   
	"__DATE_HOUR__",  
	"__DATE_MINUTE__",
	"__DATE_SECOND__",
	"regClk",           
	"regReset",         
	"regHWClear",       
	"regVertexMode",    
	"regHalt",          
	"regRAMReset",      
	"regAsyncReset",    
	"regAsyncClk",      
	"regAsyncFreq",     
	"regIndex",         
	"regHScale",        
	"regVScale",        
	"regHWScale",       
	"regRotation",      
	"regTexSize",       
	"regTexDataPtr",    
	"regTexDataSz",     
	"regRasterQ",       
	"regTexBuffer",     
	"regWidth",         
	"regHeight",        
	"regRatio",         
	"regParamList",     
	"regCursorX",       
	"regCursorY",       
	"regCursor",        
	"regCursorButtons", 
	"regBrightnessW",   
	"regBrightnessR",   
	"regBrightnessG",   
	"regBrightnessB",   
	"regContrastW",     
	"regContrastR",     
	"regContrastG",     
	"regContrastB",     
	"regCircleQuality", 
	"regOffsetX",       
	"regOffsetY",       
	"regRotation",      
	"regScale",         
	"regCenterX",       
	"regCenterY",       
	"regCircleStart",   
	"regCircleEnd",     
	"regLineWidth",     
	"regScaleX",        
	"regScaleY",        
	"regFontAlign",     
	"regFontHalign",    
	"regZOffset",       
	"regFontValign",    
	"regCullDistance",  
	"regCullMode",      
	"regLightMode",     
	"regVertexArray",   
	"regTexRotation",   
	"regTexScale",      
	"regTexCenterU",    
	"regTexCenterV",    
	"regTexOffsetU",    
	"regTexOffsetV",    
	"GOTO",
	"FOR",
	"IF",
	"ELSE",	
	"WHILE",	
	"DO",	
	"SWITCH",	
	"CASE",	
	"CONST",	
	"RETURN",	
	"BREAK",	
	"CONTINUE",	
	"EXPORT",	
	"INLINE",	
	"FORWARD",	
	"REGISTER",	
	"STRUCT",	
	"DB",	
	"ALLOC",
	"SCALAR","VECTOR1F","VECTOR2F","UV","VECTOR3F",
	"VECTOR4F","COLOR","VEC1F","VEC2F","VEC3F","VEC4F","MATRIX",	
	"STRING",	
	"DB",		
	"DEFINE",	
	"CODE",		
	"DATA",		
	"ORG",		
	"OFFSET",	
	"VOID","FLOAT","CHAR","INT48","VECTOR",	
	"PRESERVE",	
	"ZAP",		
	"EAX","EBX","ECX","EDX","ESI","EDI","ESP","EBP",
	"CS","SS","DS","ES","GS","FS","KS","LS",		
	"PORT0",
	"PORT1",
	"PORT2",
	"PORT3",
	"PORT4",
	"PORT5",
	"PORT6",
	"PORT7",
	"PORT8",
	"PORT9",
	"PORT10",
	"PORT11",
	"PORT12",
	"PORT13",
	"PORT14",
	"PORT15",
	"PORT16",
	"PORT17",
	"PORT18",
	"PORT19",
	"PORT20",
	"PORT21",
	"PORT22",
	"PORT23",
	"PORT24",
	"PORT25",
	"PORT26",
	"PORT27",
	"PORT28",
	"PORT29",
	"PORT30",
	"PORT31",
	"PORT32",
	"PORT33",
	"PORT34",
	"PORT35",
	"PORT36",
	"PORT37",
	"PORT38",
	"PORT39",
	"R0",
	"R1",
	"R2",
	"R3",
	"R4",
	"R5",
	"R6",
	"R7",
	"R8",
	"R9",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",
	"R16",
	"R17",
	"R18",
	"R19",
	"R20",
	"R21",
	"R22",
	"R23",
	"R24",
	"R25",
	"R26",
	"R27",
	"R28",
	"R29",
	"R30",
	"R31"
};

//regex example:
	
/* 	string input;
	std::regex Cname("([_[:alpha:]][[:alnum:]]*)(?:\\.([[:alnum:]]{3}))?");
	
	dout << "type a valid C identifier" << endl;
	while(true){
		//cin >> input;
		getline(cin, input);
		dout << "[" << input << "]" << endl;
		if(!cin) break;
		if(input == "q") break;
		smatch sm;
		if(regex_match(input,sm,Cname)){
			dout << "yay you did a thing!" << endl;
			float n = 1;
			auto I = sm.begin()+1;
			for(; I != sm.end(); I++){
				dout << n << ": (" << I->str() << ")" << endl; 
				n++;
			}
		}else{
			dout << "nooo it not work" << endl;
		}
		dout << endl;
	} */

// int accept(char **input, string str2){
	// char *str = *input;
	// int I = 0;
	// for(int I = 0; I < str2.length(); I++){
		// if(str[I] != str2[I]){return 0;}
	// }
	// *input = str+I;
	// return 1;
// }