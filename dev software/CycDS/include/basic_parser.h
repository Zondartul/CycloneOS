#ifndef BASIC_PARSER_GUARD
#define BASIC_PARSER_GUARD
#include <string>
using std::string;

struct basic_parser{
	const char *inputInitial = 0;
	const char *input = 0;
	bool ignorecase = true;
	bool alwaysSkipSpaces = true;
	void (*error)(string text);
	basic_parser(const char *newInput);
	bool accept(string text);			void expect(string text);
	bool acceptIdent(string &ident);	void expectIdent(string &ident);
	bool acceptInt(int &number);		void expectInt(int &number);
	bool acceptEOF();					void expectEOF();
	bool isEOF();
	void skipSpaces();
	void getErrPos(int pos, int *linenum, int *col, string *linestr);
	void pointOutError();
};

#endif