#pragma once
#include "TConfig.h"
#include "TSymbolTable.h"
#include "zcpu_types.h"
#include "stl.h"
#include <regex>
#include <sstream>
using std::regex;
using std::smatch;
using std::stringstream;

struct TParserDict {
	TParserDict(); //initializes regex
	string r_ref = "\\**";
	string r_type = "(?:(?:void|float|char)" + r_ref + ")";
	string r_ident = "(?:[_[:alpha:]][_[:alnum:]]*)";
	string r_sp = "[[:space:]]*";
	string r_spp = "[[:space:]]+";
	string r_param = "(?:" + r_sp + "(?:" + r_type + " )?" + r_sp + r_ref + r_ident + r_sp + ")";
	string r_param_list = r_param + "?" + "(?:" + "," + r_param + ")*";
	string r_func_signature = r_type + "[[:space:]]+" + r_ref + "(" + r_ident + ")" + "\\(" + r_param_list + "\\)";

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

	vector<string> keywords = {
		"mov", 	"div",	"frnd",	"out",	"int",	"min",	"mcopy",	"sub",	"add",	"ret",	"inc",
		"rstack",	"fint",	"fpwr",	"mod",	"FFRAC",	"FLN",	"FE",	"FPI",	"FLOG10",	"FABS",
		"FCEIL",	"max",	"rand",	"mul",	"FSIN",	"FCOS",	"FTAN",	"FINV",	"FASIN",	"FACOS",
		"FATAN",	"timer",	"NOP",	"push",	"call",	"pop",	"enter",	"cpuget",	"in",
		"jmp",	"CPUSET",	"STM",	"CLM",	"lidtr",	"leave",	"iret",	"clef",	"stef",	"sti",
		"cli", //opcodes end
		"programsize",	"__PROGRAMSIZE__",	"__DATE_YEAR__",	"__DATE_MONTH__",	"__DATE_DAY__",	
		"__DATE_HOUR__",	"__DATE_MINUTE__",	"__DATE_SECOND__",	"regClk",	"regReset",	"regHWClear",
		"regVertexMode",	"regHalt",	"regRAMReset",	"regAsyncReset",	"regAsyncClk",	"regAsyncFreq",
		"regIndex",	"regHScale",	"regVScale",	"regHWScale",	"regRotation",	"regTexSize",
		"regTexDataPtr",	"regTexDataSz",	"regRasterQ",	"regTexBuffer",	"regWidth",	"regHeight",
		"regRatio",	"regParamList",	"regCursorX",	"regCursorY",	"regCursor",	"regCursorButtons",
		"regBrightnessW",	"regBrightnessR",	"regBrightnessG",	"regBrightnessB",	"regContrastW",	
		"regContrastR",	"regContrastG",	"regContrastB",	"regCircleQuality",	"regOffsetX",	"regOffsetY",
		"regRotation",	"regScale",	"regCenterX",	"regCenterY",	"regCircleStart",	"regCircleEnd",
		"regLineWidth",	"regScaleX",	"regScaleY",	"regFontAlign",	"regFontHalign",	"regZOffset",
		"regFontValign",	"regCullDistance",	"regCullMode",	"regLightMode",	"regVertexArray",	
		"regTexRotation",	"regTexScale",	"regTexCenterU",	"regTexCenterV",	"regTexOffsetU",	
		"regTexOffsetV",	"GOTO",	"FOR",	"IF",	"ELSE",	"WHILE",	"DO",	"SWITCH",	"CASE",	
		"CONST",	"RETURN",	"BREAK",	"CONTINUE",	"EXPORT",	"INLINE",	"FORWARD",	"REGISTER",	
		"STRUCT",	"DB",	"ALLOC",	"SCALAR","VECTOR1F","VECTOR2F","UV","VECTOR3F",	"VECTOR4F",
		"COLOR","VEC1F","VEC2F","VEC3F","VEC4F","MATRIX",	"STRING",	"DB",	"DEFINE",	"CODE",	
		"DATA",	"ORG",	"OFFSET",	"VOID","FLOAT","CHAR","INT48","VECTOR",	"PRESERVE",	"ZAP",	"EAX",
		"EBX","ECX","EDX","ESI","EDI","ESP","EBP",	"CS","SS","DS","ES","GS","FS","KS","LS",	"PORT0",
		"PORT1",	"PORT2",	"PORT3",	"PORT4",	"PORT5",	"PORT6",	"PORT7",	"PORT8",	
		"PORT9",	"PORT10",	"PORT11",	"PORT12",	"PORT13",	"PORT14",	"PORT15",	"PORT16",	
		"PORT17",	"PORT18",	"PORT19",	"PORT20",	"PORT21",	"PORT22",	"PORT23",	"PORT24",	
		"PORT25",	"PORT26",	"PORT27",	"PORT28",	"PORT29",	"PORT30",	"PORT31",	"PORT32",	
		"PORT33",	"PORT34",	"PORT35",	"PORT36",	"PORT37",	"PORT38",	"PORT39",	"R0",	
		"R1",	"R2",	"R3",	"R4",	"R5",	"R6",	"R7",	"R8",	"R9",	"R10",	"R11",	"R12",	
		"R13",	"R14",	"R15",	"R16",	"R17",	"R18",	"R19",	"R20",	"R21",	"R22",	"R23",	"R24",	
		"R25",	"R26",	"R27",	"R28",	"R29",	"R30",	"R31"
	};
};

struct err_data {
	string msg;
	string file;
	int line = 0;
	int col = 0;
	string S;
	vector<string> file_stack;
};


class TParser {
public:
	TParser(TConfig& cfg, TSymbolTable& symTable);
	TParserDict dict;
	TConfig& cfg;
	TSymbolTable& symTable;
	cpu_file* read_cpu_file(string file, err_data erdata);

	void parseFunctionSignature(cpu_func* f);
	bool validRef(string ident);
	void addRef(string name);
	void retypeRefs();
};

//#define advancebymatch(S2,sm) (S2 = S2.substr(sm.position(0)+sm.length(0),-1))
void advancebymatch(string& S2, smatch& sm);
void removeStringLiterals(string& S, smatch& sm, TParserDict& dict);
void removeCharacterLiterals(string& S, smatch& sm, TParserDict& dict);
void removeLineComment(int& pos, string& S);
void removeMultilineComment(int& pos, bool& multilineComment, string& S);
void countBrackets(bool& if_skip, int& brk_found, int& bracketLevel, string& S, err_data &erdata);
bool checkPragmaNoExport(string& S, smatch& sm, TParserDict& dict);
void checkEndIf(string& S, smatch& sm, TParserDict& dict, bool& if_skip);
void checkDefine(string& S, smatch& sm, TParserDict& dict, TSymbolTable& symTable);
bool checkIfndef(string& S, smatch& sm, TParserDict& dict, TSymbolTable& symTable, bool& if_skip);
bool checkElse(string& S, smatch& sm, TParserDict& dict, bool& if_skip);
bool checkIfdef(string& S, smatch& sm, TParserDict& dict, TSymbolTable& symTable, bool& if_skip);
bool checkEndif(string S, smatch& sm, TParserDict& dict);
bool checkInclude(string& S, smatch& sm, TParserDict& dict, bool& if_skip, TParser& parser, cpu_file*& cpf, err_data &erdata);
void checkLabel(string& S, smatch& sm, TParserDict& dict, bool& if_skip, TSymbolTable& symTable);
bool checkFuncSignature(string& S, smatch& sm, TParserDict& dict, bool& if_skip, TSymbolTable& symTable, cpu_file*& cpf, TParser& parser);
void checkVarDef(string& S, smatch& sm, TParserDict& dict, bool& if_skip, int& bracketLevel, cpu_file*& cpf, TSymbolTable& symTable, err_data &erdata);
void checkReferences(string& S, smatch& sm, TParserDict& dict, bool& if_skip, TParser& parser);

ifstream find_cpu_file(string name, TConfig& cfg);


void drawArrowAtCol(int n);
void listIncludes(vector<string> file_stack);
void point_out_error(err_data data);
void parse_error(err_data data);