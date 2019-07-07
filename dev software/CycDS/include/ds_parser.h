#ifndef DS_PARSER_GUARD
#define DS_PARSER_GUARD
#include "basic_parser.h"
#include "ds_typeinfo.h"

struct ds_parser:public basic_parser{
	ds_parser(const char *newInput);
	bool acceptDSdef(vector<typeinfo*> &types);
	bool acceptEntry(vector<type_entry> &entries, /* int &entryNum, */ int *entryID);
	bool acceptTypelist(vector<type_entry> &entries, /* int &entryNum, */ int *entryID);
	bool acceptVariantDef(vector<typeinfo*> &types);
	bool acceptSubvariantDef(vector<typeinfo*> &types, typeinfo &ti);
	bool acceptUnionDef(vector<typeinfo*> &types);
	bool acceptCSdef(vector<typeinfo*> &types);
	bool acceptDef(vector <typeinfo*> &types);
	
	void expectDSdef(vector<typeinfo*> &types);
	void expectEntry(vector<type_entry> &entries, /* int &entryNum, */ int *entryID);
	void expectTypelist(vector<type_entry> &entries, /* int &entryNum, */ int *entryID);
	void expectVariantDef(vector<typeinfo*> &types);
	void expectSubvariantDef(vector<typeinfo*> &types, typeinfo &ti);
	void expectUnionDef(vector<typeinfo*> &types);
	void expectCSdef(vector<typeinfo*> &types);
	void expectDef(vector<typeinfo*> &types);
};

#endif