#include "TParser.h"
#include "builder_cli.h" //for dout
#include "stringUtils.h"
#include <regex>
using namespace std;

TParserDict::TParserDict() {
	// --- regexInit() ---
	reg_define = regex("#define (" + r_ident + ").*");
	reg_ifndef = regex("#ifndef (" + r_ident + ").*");
	reg_else = regex("#else.*");
	reg_ifdef = regex("#ifdef (" + r_ident + ").*");
	reg_endif = regex("#endif.*");
	reg_include = regex("#include (?:<|\")([^>]+)(?:>|\")");//regex("#include <([^>]+)>");
	reg_pragma = regex("#pragma ([^[:space:]]+)(?: ([^[:space:]]+))?.*");
	reg_func_signature = regex(r_func_signature);
	reg_var = regex("(?:register )?" + r_type + r_sp + r_ref + "(" + r_ident + ")");
	reg_var_extra = regex(r_sp + "," + r_sp + r_ref + "(" + r_ident + ")");
	reg_string = regex("\"(?:[^\\\\]|\\\\.)*?\"");
	reg_char = regex("\'(?:[^\\\\']|\\\\[^'])?\'");
	reg_ident = regex(r_ident);
	reg_label = regex("(" + r_ident + "):");
	// --- keywordsLower() ---
	for (auto I = keywords.begin(); I != keywords.end(); I++) {
		*I = toLower(*I);
	}
}

//======== TParser ===================

TParser::TParser(TConfig& cfg, TSymbolTable& symTable) :
	cfg(cfg), symTable(symTable) {}

//#define advancebymatch(S2,sm) (S2 = S2.substr(sm.position(0)+sm.length(0),-1))
void advancebymatch(string& S2, smatch& sm) {
	S2 = S2.substr(sm.position(0) + sm.length(0), -1);
}

void removeStringLiterals(string &S, smatch &sm, 
	TParserDict &dict) {
	//string literal removal
	while (regex_search(S, sm, dict.reg_string)) {
		//dout << "removed string literal \"" << sm[0].str() << "\"\n";
		S = S.substr(0, sm.position(0)) + "0" + S.substr(sm.position(0) + sm.length(0), -1);
	}
}

void removeCharacterLiterals(string &S, smatch &sm, 
	TParserDict &dict) {
	//character literal removal
	while (regex_search(S, sm, dict.reg_char)) {
		//dout << "removed character literal \"" << sm[0].str() << "\"\n";
		S = S.substr(0, sm.position(0)) + "0" + S.substr(sm.position(0) + sm.length(0), -1);
	}
}

void removeLineComment(int &pos, string &S) {
	//relies on string remover for "//" case.
	pos = S.find("//");
	if (pos != -1) {
		string Snew = S.substr(0, pos);
		S = Snew;
	}
}

void removeMultilineComment(int &pos, 
	bool &multilineComment, string &S) {
	//multiline comment removal
	pos = S.find("/*");
	if (pos != -1) {
		string Snew = S.substr(0, pos);
		multilineComment = true;
		pos = S.find("*/");
		if (pos != -1) {
			Snew = Snew + S.substr(pos, S.length());
			multilineComment = false;
		}
		S = Snew;
	}

	if (multilineComment) {
		pos = S.find("*/");
		string Snew;
		if (pos != -1) {
			Snew = S.substr(pos, S.length());
			multilineComment = false;
		}
		S = Snew;
	}
}

void countBrackets(bool& if_skip, int &brk_found, 
	int &bracketLevel, string &S, err_data &erdata) {
	//bracket counting
	//relies on string remover for "//" and "{" cases.
	//just dumbly counts the opening and closing brackets on a line
	if (!if_skip) {
		brk_found = -1;
	brk_loop:
		brk_found = S.find("{", brk_found + 1);
		if (brk_found != -1) {
			bracketLevel++;
			goto brk_loop;
		}
		brk_found = -1;
	brk_loop2:
		brk_found = S.find("}", brk_found + 1);
		if (brk_found != -1) {
			bracketLevel--;
			if (bracketLevel == 0) {
				//we check falling edge, cause opening bracket
				//may be away from func def
				currentFunc = 0;
				//also funcs inside funcs are not allowed
			}
			if (bracketLevel < 0) {
				//dout << "error: mismatched closing bracket" << endl;
				//dout << "line: " << line << ", file: " << file << endl;
				//exit(0);
				stringstream ss;
				ss << "error: mismatched closing bracket" << endl;
				//ss << "line: " << line << ", file: " << file << endl;
				erdata.msg = ss.str();
				parse_error(erdata);
			}
			goto brk_loop2;
		}
	}
}

bool checkPragmaNoExport(string &S, smatch &sm, 
	TParserDict &dict) {
	if (regex_match(S, sm, dict.reg_pragma)) {
		if (sm[1].str() == "no_export") {
			//dout << "#pragma no_export, ";
			string flavor = sm[2].str();
			if (flavor != "") {
				string flavor = sm[2].str();
				//dout << "[" << flavor << "] flavor;" << endl;
			}
			else {
				//dout << "default flavour" << endl;
			}
		}//was I going to do something here?
		return true;//goto nextline;
	}
	return false;
}

void checkEndIf(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip) {
	//if we are in an "#ifdef / #ifndef" block,
	//we ignore everything but "#endif".
	if (regex_match(S, sm, dict.reg_endif)) {
		if_skip = false;
	}
}

void checkDefine(string &S, smatch &sm, 
	TParserDict &dict, TSymbolTable &symTable) {
	if (regex_match(S, sm, dict.reg_define)) {
		string def = sm[1].str();
		//dout << "#defined \"" << def << "\"" << endl;
		symTable.defined.push_back(def);
		symTable.lowdefined.push_back(toLower(def));
		//ref catching allowed here
	}
}

bool checkIfndef(string &S, smatch &sm, 
	TParserDict &dict, TSymbolTable &symTable, 
	bool &if_skip) {
	if (regex_match(S, sm, dict.reg_ifndef)) {
		string def = sm[1].str();
		//dout << "#ifndef \"" << def << "\" ";
		if (find(symTable.defined.begin(), symTable.defined.end(), def) != symTable.defined.end()) {
			//dout << "(defined)" << endl;
			if_skip = true;
		}
		else {
			//dout << "(undefined)" << endl;
		}
		return true;//goto nextline;
	}
	return false;
}

bool checkElse(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip) {
	if (regex_match(S, sm, dict.reg_else)) {
		//dout << "#else" << endl;
		if_skip = !if_skip;
		return true;//goto nextline;
	}
	return false;
}

bool checkIfdef(string &S, smatch &sm, 
	TParserDict &dict, TSymbolTable &symTable, 
	bool &if_skip) {
	if (regex_match(S, sm, dict.reg_ifdef)) {
		string def = sm[1].str();
		//dout << "ifdef \"" << def << "\" ";
		if (find(symTable.defined.begin(), symTable.defined.end(), def) != symTable.defined.end()) {
			//dout << "(defined)" << endl;
		}
		else {
			//dout << "(undefined)" << endl;
			if_skip = true;
		}
		return true;//goto nextline;
	}
	return false;
}

bool checkEndif(string S, smatch &sm, 
	TParserDict &dict) {
	if (regex_match(S, sm, dict.reg_endif)) {
		//we're not in an if-skip, so ignore
		return true;//goto nextline;
	}
	return false;
}

bool checkInclude(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip, 
	TParser &parser, cpu_file* &cpf, 
	err_data &erdata) {
	//cout << "is include? ";
	if (regex_match(S, sm, dict.reg_include)) {
		//cout << "yes;";
		if (if_skip) {
			//cout << "if-skipping;";
			//dout << "found include <" << sm[1].str() << ">, if-skipping." << endl;
		}
		else {
			//cout << "parsing include;\n";
			//dout << "found include <" << sm[1].str() << ">" << endl;
			string file_inc = sm[1].str();
			//cout << "file_inc = [" << file_inc << "];\n";
			cpu_file* cpf2 = parser.read_cpu_file(file_inc, erdata);
			if (cpf2) {
				cpf->included.push_back(cpf2);
			}
			else {
				//dout << "can't read included file " << file_inc << endl;
				//exit(0);
				stringstream ss;
				ss << "can't read included file " << file_inc << endl;
				erdata.msg = ss.str();
				parse_error(erdata);
			}
			//cout << "end." << endl;
			return true;//goto nextline;
		}
	}
	//cout << "no.\n";
	return false;
}

void checkLabel(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip, 
	TSymbolTable &symTable) {
	//labels
	if (regex_search(S, sm, dict.reg_label)) {
		if (if_skip) {
			//dout << "found label " << sm[1].str() << ", if-skipping." << endl;
		}
		else {
			//dout << "found label " << sm[1].str() << endl;
			string name = sm[1].str();
			symTable.labels.push_back(name);
			symTable.lowlabels.push_back(toLower(name));
			advancebymatch(S, sm);
		}
	}
}

bool checkFuncSignature(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip, 
	TSymbolTable &symTable, cpu_file* &cpf, 
	TParser &parser) {
	if (regex_search(S, sm, dict.reg_func_signature)) {
		if (if_skip) {
			//dout << "found func " << sm[1].str() << ", if-skipping." << endl;
		}
		else {
			cpu_func* f = new cpu_func();
			f->signature = sm[0].str();
			f->name = sm[1].str();
			f->lowname = toLower(f->name);
			cpf->funcs.push_back(f);
			symTable.all_funcs.push_back(f);
			//dout << "found func " << sm[1].str() << endl;

			parser.parseFunctionSignature(f);
			currentFunc = f;
			advancebymatch(S, sm);
		}
		return true;
	}
	return false;
}

void checkVarDef(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip, 
	int& bracketLevel, cpu_file* &cpf, 
	TSymbolTable &symTable, 
	err_data &erdata) {
	//can't have var def on same line as func def, else regex borks.
	if (regex_search(S, sm, dict.reg_var)) {
		if (if_skip) {
			//dout << "found var " << sm[1].str() << ", if-skipping." << endl;
		}
		else {
			if (bracketLevel) {
				//dout << "found local var " << sm[1].str() << " (BL = " << bracketLevel << ")" << endl;
				cpu_var* v = new cpu_var();
				v->name = sm[1].str();
				v->lowname = toLower(v->name);
				if (!currentFunc) { 
					//dout << "ERROR: local var but no local func" << endl; 
					//dout << "(this usually means you have a variable definition and a function definition on the same line, which CycloneBuilder can't parse right now)" << endl;
					//dout << "S: [" << S << "]" << endl;
					//exit(1);
					stringstream ss;
					ss << "ERROR: local var but no local func" << endl;
					ss << "(this usually means you have a variable definition and a function definition on the same line, which CycloneBuilder can't parse right now)" << endl;
					ss << "(or a multiline function signature)" << endl;
					//ss << "S: [" << S << "]" << endl;
					erdata.msg = ss.str();
					parse_error(erdata);
				}
				currentFunc->vars.push_back(v);
			}
			else {
				//dout << "found global var " << sm[1].str() << endl;
				cpu_var* v = new cpu_var();
				v->name = sm[1].str();
				v->lowname = toLower(v->name);
				cpf->vars.push_back(v);
				symTable.all_vars.push_back(v);
			}
		}
		advancebymatch(S, sm);
		while (regex_search(S, sm, dict.reg_var_extra)) {
			if (if_skip) {
				//dout << "found var " << sm[1].str() << ", if-skipping." << endl;
			}
			else {
				if (bracketLevel) {
					//dout << "found local var " << sm[1].str() << " (BL = " << bracketLevel << ")" << endl;
					cpu_var* v = new cpu_var();
					v->name = sm[1].str();
					v->lowname = toLower(v->name);
					if (!currentFunc) { 
						//dout << "ERROR: local var but no local func" << endl; 
						//exit(1);
						stringstream ss;
						ss << "ERROR: local var but no local func" << endl;
						erdata.msg = ss.str();
						parse_error(erdata);
					}
					currentFunc->vars.push_back(v);
				}
				else {
					//dout << "found global var " << sm[1].str() << endl;
					cpu_var* v = new cpu_var();
					v->name = sm[1].str();
					v->lowname = toLower(v->name);
					cpf->vars.push_back(v);
					symTable.all_vars.push_back(v);
				}
			}
			advancebymatch(S, sm);
		}
	}		
}

void checkReferences(string &S, smatch &sm, 
	TParserDict &dict, bool &if_skip, TParser &parser) {
	//reference collection
	//after everything else cause we don't want to
	//accidentally grab definitions
	string S2 = S;
	while (regex_search(S2, sm, dict.reg_ident)) {
		string id = sm[0].str();
		if (parser.validRef(id)) {
			if (if_skip) {
			}
			else {
				//dout << "found reference to " << sm[0].str() << endl;
				parser.addRef(id);
			}
		}
		advancebymatch(S2, sm);
	}
}

ifstream find_cpu_file(string name, TConfig &cfg) {
	ifstream fs;
	//1. check in the current folder
	string filepath1 = cfg.dirs.dir_folder + name;
	//cout << "opening file [" << filepath << "]" << endl;
	fs.open(filepath1);
	if (fs.is_open()) {
		cout << "reading local file \"" << name << "\"" << endl;
		return fs;
	}
	else {
		//2. check in the overal folder
		string filepath2 = cfg.dirs.dir_cpuchip + name;
		fs.open(filepath2);
		if (fs.is_open()) {
			cout << "reading global file \"" << name << "\"" << endl;
		}
		else {
			cout << "can't open file \"" << name << "\"\n";
			cout << "looked in [" << filepath1 << "]\n";
			cout << "looked in [" << filepath2 << "]\n";
		}
	}
	return fs;
}

//read a HL-ZASM source code file 
//<file> - relative address of the file
//returns a structure with everything gathered from the file
cpu_file* TParser::read_cpu_file(string file, err_data erdata) {
	//ifstream fs;
	//string filepath = cfg.dirs.dir_folder + file;
	//cout << "opening file [" << filepath << "]" << endl;
	//fs.open(filepath, ios::in);
	//if (!fs.is_open()) { return 0; }
	//dout << "reading file " << file << endl;
	ifstream fs = find_cpu_file(file, cfg);
	if (!fs.is_open()) {
		return 0;
	}
	cpu_file* cpf = new cpu_file();
	cpf->name = file;
	erdata.file = file;
	erdata.file_stack.push_back(file);

	string S;
	int line = 1;					//file line counter, for reporting
	bool if_skip = false;			//are we currently skipping an if-else block?
	int bracketLevel = 0;			//how deep are we in the nested brackets?
	bool multilineComment = false;	//are we currently skipping a multiline comment?
	do {
		getline(fs, S);
		

		//dout << "line " << line << ":\n";
		//dout << "source:  [" << S << "]" << endl;
		int brk_found = 0;
		int pos = -1;
		smatch sm;

		erdata.line = line;
		erdata.col = pos;
		erdata.S = S;

		//proprocessor stuff!

		//removeStringLiterals(S, sm, dict);
		removeCharacterLiterals(S, sm, dict);
		removeLineComment(pos, S);
		removeMultilineComment(pos, multilineComment, S);
		countBrackets(if_skip, brk_found, bracketLevel, S, erdata);
		//dout << "preproc: [" << S << "]" << endl;

		//as per zCPU compiler limitation, ifdefs do not stack.
		if (checkPragmaNoExport(S, sm, dict)) {goto nextline;}

		if (if_skip) {
			//cout << "if-skipping" << endl;
			checkEndIf(S, sm, dict, if_skip);
		}
		else {
			//cout << "parsing" << endl;
			checkDefine(S, sm, dict, symTable);
			if (checkIfndef(S, sm, dict, symTable, if_skip))	{goto nextline;}
			if (checkElse(S, sm, dict, if_skip))				{goto nextline;}
			if (checkIfdef(S, sm, dict, symTable, if_skip))		{goto nextline;}
			if (checkEndif(S, sm, dict))						{goto nextline;}
			if (checkInclude(S, sm, dict, if_skip, 
				*this, cpf, erdata)) {goto nextline;}
			checkLabel(S, sm, dict, if_skip, symTable);
			if (!checkFuncSignature(S, sm, dict, if_skip, symTable, cpf, *this)) {
				checkVarDef(S, sm, dict, if_skip, bracketLevel, 
					cpf, symTable, erdata);
			}
			checkReferences(S, sm, dict, if_skip, *this);
		}
	nextline:
		line++;
	} while (!fs.eof());
	fs.close();
	//cout << "EOF " << file << endl;
	symTable.all_files.push_back(cpf);
	return cpf;
}



/*       () (ident) (float ident) (float ident, x)             */
void TParser::parseFunctionSignature(cpu_func* f) {
	//dout << endl << endl << endl;
	string sig = f->signature;
	//dout << "sig1 = " << sig << endl;
	sig = sig.substr(sig.find("("), -1);
	//dout << "sig2 = " << sig << endl;

	smatch sm;
	regex reg_param = regex("(?:" + dict.r_sp + "(?:" + dict.r_type + " )?" + dict.r_sp + dict.r_ref + "(" + dict.r_ident + ")" + dict.r_sp + ")");
	while (regex_search(sig, sm, reg_param)) {
		string name = sm[1];
		//dout << "found param " << name << endl;
		cpu_var* v = new cpu_var();
		v->name = name;
		v->lowname = toLower(name);
		f->vars.push_back(v);
		advancebymatch(sig, sm);
	}
}


bool TParser::validRef(string ident) {
	string ident_lower = toLower(ident);
	//note we are not searching for positions of references
	//- that would require full preprocessing with macro-expansion

	//not a reference if defined away;
	for (auto I = symTable.lowdefined.begin(); I != symTable.lowdefined.end(); I++) {
		if (*I == ident_lower) { goto vr_isinvalid; }
	}
	for (auto I = symTable.keywords.begin(); I != symTable.keywords.end(); I++) {
		if (*I == ident_lower) { goto vr_isinvalid; }
	}
	for (auto I = symTable.references.begin(); I != symTable.references.end(); I++) {
		if ((I->type == REF_STACK) && (I->func != currentFunc)) { continue; } //ignore refs to other locals
		if (I->lowname == ident_lower) { goto vr_isinvalid; }
	}

vr_isvalid:
	return true;

vr_isinvalid:
	return false;
}

void TParser::addRef(string name) {
	//ident is okay to add to references
	int type = REF_EXT;
	struct ref R;
	string lowname = toLower(name);
	R.name = name;
	R.lowname = lowname;
	//if a stack variable with that name is already defined,
	//immediately make this a stack reference
	if (currentFunc) {
		//dout << "currentFunc = " << currentFunc->name << endl;
		auto& vars = currentFunc->vars;
		for (auto I = vars.begin(); I != vars.end(); I++) {
			if ((*I)->lowname == lowname) { type = REF_STACK; }
		}
	}
	else {
		//dout << "no currentFunc" << endl;
	}
	R.type = type;
	//dout << "ref " << name << " is " << (type == REF_STACK ? "REF_STACK" : "REF_EXT") << endl;
	R.func = currentFunc;
	symTable.references.push_back(R);
}

void TParser::retypeRefs() {
	for (auto J = symTable.references.begin(); J != symTable.references.end(); J++) {
		string lowname = J->lowname;
		int type = J->type;
		if (type != REF_STACK) { //stack refs shadow other refs
			for (auto I = symTable.lowlabels.begin(); I != symTable.lowlabels.end(); I++) {
				if (*I == lowname) { type = REF_INT_LABEL; }
			}
			for (auto I = symTable.all_funcs.begin(); I != symTable.all_funcs.end(); I++) {
				if ((*I)->lowname == lowname) { type = REF_INT_FUNC; }
			}
			for (auto I = symTable.all_vars.begin(); I != symTable.all_vars.end(); I++) {
				if ((*I)->lowname == lowname) { type = REF_INT_VAR; }
			}
		}
		J->type = type;
	}
}

void drawArrowAtCol(int n) {
	for (int i = 0; i < (n - 1); i++) {
		dout << " ";
	}
	dout << "^";
	dout << endl;
}

void listIncludes(vector<string> file_stack) {
	for (int i = file_stack.size()-2; i >= 0; i--) {
		dout << "included from file [" << file_stack[i] << "]" << endl;
	}
}

void point_out_error(err_data data) {
	dout << "in file [" << data.file << "] @ " << data.line << ":" << data.col << endl;
	cout << data.S << endl;
	drawArrowAtCol(data.col);
	listIncludes(data.file_stack);
}

void parse_error(err_data data) {
	dout << endl;
	dout << "----------- ERROR ---------------" << endl;
	point_out_error(data);
	dout << "---------------------------------" << endl;
	dout << data.msg << endl;
	dout << "---------------------------------" << endl;
	exit(1);
}
