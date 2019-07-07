#include "ds_parser.h"
//simple grammar for ds definitions:
//DSdef: "ds" name "{" entry* "}" ";"
//entry: typelist name?
//typelist: type typelist?
//type: (all the stuff in type_entry_type)
#include <iostream>
using std::cout;

ds_parser::ds_parser(const char *newInput):basic_parser(newInput){}

void ds_parser::expectDSdef(vector<typeinfo*> &types){
	if(!acceptDSdef(types)){
		pointOutError();
		//printf("expected DS definition, got [%c][%d]",*input,*input);
		printf("expected a DS definition.\n");
		exit(0);
	}
}

bool ds_parser::acceptDSdef(vector<typeinfo*> &types){
	typeinfo &ti = *(new typeinfo());
	const char *in = input;
	//int entryNum = 0;
	if(!accept("ds")){goto ads_def_bad;}
	ti.type = TI_DS;
	expectIdent(ti.name);
	expect("{");
	expectEntry(ti.entries,/* entryNum, */0);
	while(acceptEntry(ti.entries,/* entryNum, */0));
	expect("}");
	expect(";");
	types.push_back(&ti);
	return true;
	
	ads_def_bad:
	input = in;
	delete &ti;
	return false;
}

void ds_parser::expectEntry(vector<type_entry> &entries, /* int &entryNum, */ int *entryID){
	if(!acceptEntry(entries,/* entryNum, */ entryID)){
		pointOutError();
		//printf("expected entry, got [%c][%d]",*input,*input);
		printf("Expected an entry.\n");
		exit(0);
	}
}

//waah, mess with entry nums and pointers
//int *entryID passed up so that parent can link to child
//entryNum passed down so child goes after parent in order.
//hacky and doesn't work, lets remove entryNum.

//what we have:
//0: I <--+
//1: E ---+
//2: I <-------+
//3: I <--+ ---+
//4: E ---+
//
//what we want:
//0: E ---+
//1: E ---|-+
//2: I <--+ |
//3: I <----+ --+
//4: I <--------+


bool ds_parser::acceptEntry(vector<type_entry> &entries, /*int &entryNum,*/ int *entryID){
	const char *in = input;
	//int entryNumInit = entryNum;
	
	//1. grab the entry type.
	int entryID2;
	type_entry *entry = 0;
	//int thisEntryNum = entryNum++;
	if(!acceptTypelist(entries,/*entryNum,*/&entryID2)){goto ae_bad;}
	//entryNum--;
	
	ae_good:
	
	entry = &entries[entryID2];
	entry->isExplicit = true;
	//if(entry->ID == -1){entry->ID = 0;}
	//entry->ID = entry->ID*100+thisEntryNum;
	
	//2. see if we need a name too
	switch(entry->type){
		case TE_DS:		 //types with internal variables can optionally be 
		case TE_CS:		 //anonymous, in that case their variables are imported
		case TE_RECORD:	 //to the same scope.
		case TE_UNION:	 
			acceptIdent(entry->name);
			break;
		case TE_VARIANT: //variants are always anonymous
			break;
		default:		 //everything else must be named
			expectIdent(entry->name);
			break;
	}
	//3. see if a default value is supplied
	switch(entries[entryID2].type){
		case TE_FLOAT:
		case TE_ADDR:
			if(accept("=")){
				if(acceptInt(entry->param2ConstNum)){
					entry->paramtype = EP_CONST_NUM;
				}else{
					expect("&");
					expectIdent(entry->param2EntryString);
					entry->paramtype = EP_ENTRY_ADDR;
				}
			}
		default:
			break;
	}
	
	expect(";");
	//4. return the result, the position of the new entry, and an 'ok'.
	if(entryID){*entryID = entryID2;}
	//cout << "accepted a ds def\n";
	return true;
	
	ae_bad:
	input = in;
	//entryNum = entryNumInit;
	return false;
}

void ds_parser::expectTypelist(vector<type_entry> &entries, /*int &entryNum,*/ int *entryID){
	if(!acceptTypelist(entries,/*entryNum,*/entryID)){
		pointOutError();
		//printf("expected typelist, got [%c][%d]",*input,*input);
		printf("Expected a typelist.\n");
		exit(0);
	}
}

bool ds_parser::acceptTypelist(vector<type_entry> &entries, /*int &entryNum,*/ int *entryID){
	const char *in = input;
	//int entryNumInit = entryNum;
	//int thisEntryNum = entryNum++;
	type_entry entry;
	//if(entry.ID == -1){entry.ID = 0;}
	//entry.ID = entry.ID*100+thisEntryNum;
	if(accept("float")){
		entry.type = TE_FLOAT;
		goto atl_good;
	}
	if(accept("char")){
		entry.type = TE_CHAR;
		goto atl_good;
	}
	if(accept("string")){
		entry.type = TE_STRING;
		goto atl_good;
	}
	if(accept("cs")){
		entry.type = TE_CS;
		expectIdent(entry.param2TypeName);
		goto atl_good;
	}
	if(accept("ds")){
		entry.type = TE_DS;
		expectIdent(entry.param2TypeName);
		goto atl_good;
	}
	if(accept("[")){
		entry.type = TE_ARRAY;
		if(acceptInt(entry.param2ConstNum)){
			entry.paramtype = EP_CONST_NUM;
		}else{
			expectIdent(entry.param2EntryString);
			entry.paramtype = EP_ENTRY_VAL;
		}
		expect("]");
		expectTypelist(entries, /* entryNum, */ &entry.subentryNum);	//array type must be stated explicitly.
		//entryNum--;
		goto atl_good;
	}
	if(accept("ptr")){
		entry.type = TE_PTR;
		expectTypelist(entries, /* entryNum, */ &entry.subentryNum);
		//entryNum--;
		goto atl_good;
	}
	if(accept("addr")){
		entry.type = TE_ADDR;
		goto atl_good;
	}
	if(accept("variant")){
		entry.type = TE_VARIANT;
		expectIdent(entry.param2TypeName);
		expect("(");
		//if(acceptInt(entry.param2ConstNum)){
		//	entry.paramtype = EP_CONST_NUM;
		//}else{
		//	expectIdent(entry.param2EntryString);
		//	entry.paramtype = EP_ENTRY_VAL;
		//}
		expectIdent(entry.param3EntryString);
		expect(")");
		goto atl_good;
	}
	if(accept("union")){
		entry.type = TE_UNION;
		expectIdent(entry.param2TypeName);
		goto atl_good;
	}
	if(accept("record")){
		entry.type = TE_RECORD;
		expectTypelist(entries, /* entryNum, */ &entry.subentryNum);
		//entryNum--;
		goto atl_good;
	}
	
	atl_bad:
	input = in;
	//entryNum = entryNumInit;
	return false;
	
	atl_good:
	//cout << "accepted a typelist\n";
	entries.push_back(entry);
	if(entryID){*entryID = entries.size()-1;}
	return true;
}


bool ds_parser::acceptVariantDef(vector<typeinfo*> &types){
	typeinfo &ti = *(new typeinfo());
	const char *in = input;
	if(!accept("variant")){goto ads_def_bad;}
	ti.type = TI_VARIANT;
	expectIdent(ti.name);
	expect("{");
	expectSubvariantDef(types,ti);
	while(acceptSubvariantDef(types,ti));
	expect("}");
	expect(";");
	types.push_back(&ti);
	//cout << "accepted a variant def\n";
	return true;
	
	ads_def_bad:
	input = in;
	delete &ti;
	return false;
}

class consecutiveNamer{
	public:
	string basename;
	consecutiveNamer(string newName, int newCount):basename(newName),count(newCount){}
	int count = 0;
	string next(){
		char buff[20];
		sprintf(buff,"%d",count++);
		return basename+string(buff);
	}
};

consecutiveNamer variantNames("ti_variant_",0);

void ds_parser::expectVariantDef(vector<typeinfo*> &types){
	if(!acceptVariantDef(types)){
		pointOutError();
		//printf("expected typelist, got [%c][%d]",*input,*input);
		printf("Expected a variant definition.\n");
		exit(0);
	}
}

bool ds_parser::acceptSubvariantDef(vector<typeinfo*> &types, typeinfo &ti_super){
	typeinfo &ti = *(new typeinfo);
	const char *in = input;
	type_entry entry;
	//int entryNum = 0;
	if(!accept("case")){goto asv_bad;}
	ti.type = TI_SUBTYPE;
	ti.name = variantNames.next();
	
	entry.type = TE_SUBVARIANT;
	entry.name = ti.name;
	entry.param2TypeName = ti.name;
	
	expectInt(entry.param3ConstNum);
	expect(":");
	while(acceptEntry(ti.entries,/* entryNum, */0));

	asv_good:
	//cout << "accepted a subvariant def\n";
	types.push_back(&ti);
	//ti.supertypes.push_back(&ti_super);
	//ti_super.subtypes.push_back(&ti);
	ti_super.entries.push_back(entry);
	return true;

	asv_bad:
	input = in;
	delete &ti;
	return false;
}

void ds_parser::expectSubvariantDef(vector<typeinfo*> &types, typeinfo &ti_super){
	if(!acceptSubvariantDef(types, ti_super)){
		pointOutError();
		//printf("expected typelist, got [%c][%d]",*input,*input);
		printf("Expected a subvariant definition.\n");
		exit(0);
	}
}

bool ds_parser::acceptUnionDef(vector<typeinfo*> &types){
	typeinfo &ti = *(new typeinfo());
	const char *in = input;
	//int entryNum = 0;
	if(!accept("union")){goto aun_def_bad;}
	ti.type = TI_UNION;
	expectIdent(ti.name);
	expect("{");
	expectEntry(ti.entries, /* entryNum, */0);
	while(acceptEntry(ti.entries, /* entryNum, */0));
	expect("}");
	expect(";");
	types.push_back(&ti);
	//cout << "accepted a union def\n";
	return true;
	
	aun_def_bad:
	input = in;
	return false;
}

void ds_parser::expectUnionDef(vector<typeinfo*> &types){
	if(!acceptUnionDef(types)){
		pointOutError();
		printf("Expected a union definition.\n");
		exit(0);
	}
}

void ds_parser::expectCSdef(vector<typeinfo*> &types){
	if(!acceptCSdef(types)){
		pointOutError();
		//printf("expected DS definition, got [%c][%d]",*input,*input);
		printf("expected a CS definition.\n");
		exit(0);
	}
}

bool ds_parser::acceptCSdef(vector<typeinfo*> &types){
	typeinfo &ti = *(new typeinfo());
	const char *in = input;
	//int entryNum = 0;
	if(!accept("cs")){goto ads_def_bad;}
	ti.type = TI_CS;
	expectIdent(ti.name);
	expect("{");
	expectEntry(ti.entries, /* entryNum, */0);
	while(acceptEntry(ti.entries, /* entryNum, */0));
	expect("}");
	expect(";");
	types.push_back(&ti);
	return true;
	
	ads_def_bad:
	input = in;
	delete &ti;
	return false;
}

bool ds_parser::acceptDef(vector<typeinfo*> &types){
	if(acceptDSdef(types)){return true;}
	if(acceptCSdef(types)){return true;}
	if(acceptVariantDef(types)){return true;}
	if(acceptUnionDef(types)){return true;}
	return false;
}

void ds_parser::expectDef(vector<typeinfo*> &types){
	if(!acceptDef(types)){
		pointOutError();
		printf("Expected a definition.\n");
		exit(0);
	}	
}


