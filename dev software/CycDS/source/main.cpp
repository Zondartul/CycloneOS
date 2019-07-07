#include "stdio.h"
#include <iostream>
#include <fstream>
#include <string>
//#include <regex>
//#include <algorithm>
//#include <ctime>
using namespace std;
using std::string;
#include "ds_parser.h"

string filename_in;

/*
//----------------------
ds boop{
    float adr;
    string name;
};

ds test{
    ptr string name;
    float [3] vec;
    float size;
    float num = 1;
    float where = &word;
    ptr [size] float arr1;
    [0] boop boops;
    cs vector V;
    float word;
    variant what(num) kerbal;
};

variant what{
  case 1:
        float beep;
        float bap;
        float boop;
  case 2:
        string hi;  
};

//-------------------
void dsSerialize(ds object, destination);                //construct a serialized copy of an object in the destination
float dsSerializedSize(ds object);                       //calculate the size an object would have when serialized
ds* dsDeserialize(source);                               //construct a live copy of a previously serialized object  
float dsDeserializedSize(source);                        //calculate the size of an object once deserialized
ds* dsNew(typeInfo);                                     //create a valid ds object
void dsSet(ds object, string param, auto value);         //set a variable in a ds object
ptr dsGet(ds object, string param);                      //get a pointer to a variable in a ds object
typeinfo dsGetType(ds object);                           //get the ds object type
bool dsVerify(ds object);                                //check that a ds object is healthy
bool dsVerifySerialized(source);                         //check that a serialized ds object is healthy
dsPrint(ds object);                                      //do a fancy print of all object properties
----------------------------------------
typeinfo{
    canary/typeof;
    string name;                //structure name
    float size;                 //0 for unknown
    float numExplicitEntries;   //how many entries are actual variables?
    float numTotalEntries;      //how many entries total (incl. type adapters)
    [numTotalEntries] typeentry entries;      //the list of entries.
}

entry{
    enum entryType; // float, string, addr, ptr, array, ds, cs, variant
    float param;    //array size (float), ds or cs typeinfo
    float subentry; //ptr to entry, array of size entry, variant choice
    string name;    //name of this property
}

//----------------------------------
*/



struct stringstore{
	vector<string> strings;
	int add(string &S);
	int get(string &S);
	string &get(int I);
};

int stringstore::add(string &S){
	int I = get(S);
	if(I != -1){return I;}
	I = strings.size();
	strings.push_back(S);
	return I;
}

int stringstore::get(string &S){
	for(int I = 0; I < strings.size(); I++){
		if(strings[I] == S){return I;}
	}
	return -1;
}

string &stringstore::get(int I){return strings[I];}


void readParams(int argc, char **argv);
void finalizeData(vector<typeinfo*> &types, stringstore &SS);
void printTypeInfo(typeinfo &ti);
void outputData(vector<typeinfo*> &types, stringstore &SS);


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

ofstream output;
dual_stream dout(cout,output);

int main(int argc, char **argv){
	readParams(argc,argv);
	ifstream fs;
	fs.open(filename_in, ios::in);
	if(!fs.is_open()){return 0;}
	string filename_out = filename_in.substr(0, filename_in.find_last_of("."));
	filename_out += "_out.txt";
	output.open(filename_out);
	if(!output.is_open()){return 0;}
	cout << "reading file " << filename_in << "\n";
	string file_s;
	//fs >> file_s;
	int C;
	while((C = fs.get()) != EOF){file_s += (char)C;}
	printf("file to parse:\n");
	printf("----------------------------\n");
	printf("%s\n",file_s.c_str());
	printf("----------------------------\n");
	
	//file_s << fs;
	//basic_parser parser(file_s.c_str());
	ds_parser parser(file_s.c_str());
	parser.ignorecase = true;
	parser.alwaysSkipSpaces = true;
	
	vector<typeinfo*> types;
	stringstore SS;
	typeinfo ti;
	parser.expectDef(types);
	while(parser.acceptDef(types));
	parser.expectEOF();
	printf("\n-----------------------------\n");
	cout << "got "<<types.size() << " types.\n";
	for(int I = 0; I < types.size(); I++){
		printTypeInfo(*types[I]);
	}
	cout << "done\n";
	
	printf("finalizing...");
	finalizeData(types,SS);
	printf("done\n");
	
	printf("outputting...\n");
	outputData(types,SS);
	printf("done\n");
	
	return 0;
}

string toString(type_entry_type type){
	switch(type){
		case TE_FLOAT	: return "TE_FLOAT"	    ;break;
		case TE_STRING	: return "TE_STRING"    ;break;
		case TE_CHAR	: return "TE_CHAR"	    ;break;
		case TE_CS		: return "TE_CS"		;break;
		case TE_DS		: return "TE_DS"		;break;
		case TE_ARRAY	: return "TE_ARRAY"	    ;break;
		case TE_PTR		: return "TE_PTR"		;break;		
		case TE_ADDR	: return "TE_ADDR"	    ;break;
		case TE_VARIANT	: return "TE_VARIANT"	;break;
		case TE_UNION	: return "TE_UNION"	    ;break;
		case TE_RECORD	: return "TE_RECORD"	;break;
		case TE_ERROR	: return "TE_ERROR"		;break;
		default: return "???";break;
	}
}

string toString(entry_param_type type){
	switch(type){
		case EP_NONE:		return "EP_NONE"; break;
		case EP_CONST_NUM:	return "EP_CONST_NUM"; break;
		case EP_CONST_STR:	return "EP_CONST_STR"; break;
		case EP_ENTRY_VAL:	return "EP_ENTRY_VAL"; break;
		case EP_ENTRY_ADDR: return "EP_ENTRY_ADDR"; break;
		default: return "ERROR"; break;
	}
}

string toString(type_info_type type){
	switch(type){
		case TI_CS	    : return "TI_CS"      ;break;
		case TI_DS      : return "TI_DS"      ;break;
		case TI_VARIANT : return "TI_VARIANT" ;break;
		case TI_UNION   : return "TI_UNION"   ;break;
		case TI_SUBTYPE : return "TI_SUBTYPE" ;break;
		case TI_ERROR   : return "TI_ERROR"   ;break;
		default: return "???"; break;
	}
}

bool isValtype(type_entry &entry){
	switch(entry.type){
		case TE_FLOAT:	
		case TE_STRING: 
		case TE_CHAR: 	
		case TE_ARRAY:	
		case TE_ADDR:	
			return true;
			break;
		default:
			return false;
			break;		
	}
}

bool isStructtype(type_entry &entry){
	switch(entry.type){
		case TE_CS:		
		case TE_DS:		
		case TE_VARIANT:		
		case TE_UNION:	
			return true;
			break;
		default:
			return false;
			break;
	}
}

bool hasSubentry(type_entry &entry){
	bool a = (entry.subentryNum != -1);
	bool b = (entry.subentryString != "");
	bool c = (a || b);
	//cout << "<"<<a<<","<<b<<","<<c<<">";
	return ((entry.subentryNum != -1) || (entry.subentryString != ""));
	
}

int getEntry(typeinfo &ti, int num, string name){
	if(num != -1){
		return num;
	}
	if(name != ""){
		for(int I = 0; I != ti.entries.size(); I++){
			if(ti.entries[I].name == name){
				return I;
			}
		}
	}
	return -1;
}

void printSubEntry(typeinfo &ti, int I, int sublevel){
	for(int I = 0; I < sublevel; I++){cout<<" ";}
	auto &entry = ti.entries[I];
	//cout << "["<< entry.ID <<"]";
	cout << " "<<I << ": ";
	string S;
	if(entry.isExplicit){
		S = string() +"\'" +entry.name + "\'";
	}
	while(S.length() < 12){ S += ' '; }
	cout << S;
	
	cout << "["<< (entry.isExplicit? "E" : "I") << "]";
	
	cout << "["<< toString(entry.type) << "]";
	if(isValtype(entry)){
		cout << "["<< toString(entry.paramtype) << "]";
		switch(entry.paramtype){
			case EP_CONST_NUM:
				cout << "[" << entry.param2ConstNum << "]";
				break;
			case EP_CONST_STR:
				cout << "[" << entry.param2ConstString << "]";
				break;
			case EP_ENTRY_VAL:
				cout << "[" << entry.param2EntryNum << ": =" << entry.param2EntryString << "]";
				break;
			case EP_ENTRY_ADDR:
				cout << "[" << entry.param2EntryNum << ": &" << entry.param2EntryString << "]";
				break;
			case EP_NONE:
				break;
			default:
				cout << "<ERROR>";
				break;
		}
		//cout << "["<< entry.subentryNum << ": " << entry.subentryString << "]";
	}
	if(isStructtype(entry)){
		cout << "[" << entry.param1TypeID << "]";
		cout << "[" << entry.param2TypeName << "]";
		if(entry.type == TE_VARIANT){cout << "[" << entry.param3EntryString << "]";}
		if(entry.type == TE_SUBVARIANT){cout << "[" << entry.param3ConstNum << "]";}
	}
	if(hasSubentry(entry)){
		cout << "[" << entry.subentryNum << ": "<<entry.subentryString << "]";
		
		cout << "\n";
		printSubEntry(ti, getEntry(ti,entry.subentryNum, entry.subentryString), sublevel+1);
	}
}

void printEntry(typeinfo &ti, int I){
	if(!ti.entries[I].isExplicit){return;}
	printSubEntry(ti, I,0);
	cout << "\n";
	/*
	cout << "["<<toString(entry.type)<<"]";
	cout << (entry.isExplicit ? "(explicit)" : "(implicit)");
	cout << "[const:"<<entry.const_number<<"]";
	cout << "[sub:"<<entry.subentryName<<","<<entry.subentryID<<"]";
	cout << "[strct:"<<entry.structname<<"]";
	cout << "[name:"<<entry.name<<"]";
	*/
	
}

void printTypeInfo(typeinfo &ti){
	cout << "\n";
	cout << "typeinfo ["<<ti.name << "]\n";
	//cout << "casenum: "<<ti.casenum<<"\n";
	cout << "type: "<<toString(ti.type)<<"\n";
	cout << "entries: ("<<ti.entries.size()<<")\n";
	for(int I = 0; I < ti.entries.size(); I++){
		printEntry(ti, I);
	}
	/*
	cout << "supertypes: ("<<ti.supertypes.size()<<")\n";
	for(int I = 0; I < ti.supertypes.size(); I++){
		cout << " "<<I << ": " << ti.supertypes[I]->name << "\n";
	}
	cout << "subtypes: ("<<ti.subtypes.size()<<")\n";
	for(int I = 0; I < ti.subtypes.size(); I++){
		cout << " "<<I << ": " << ti.subtypes[I]->name << "\n";
	}
	*/
	cout << "\n";
	
}

int getTypeID(vector<typeinfo*> &types, type_entry &entry){
	if(entry.param2TypeName == ""){return 0;}
	for(int I = 0; I < types.size(); I++){
		if(types[I]->name == entry.param2TypeName){
			return I;
		}
	}
	return 0;
}


void finalizeData(vector<typeinfo*> &types, stringstore &SS){
	//post-process all types, entries, etc.
	for(int T = 0; T < types.size(); T++){
		typeinfo &ti = *types[T];
		if(ti.name != ""){
			ti.param_name = SS.add(ti.name);
		}else{
			ti.param_name = -1;
		}
		ti.param_type = ti.type;
		for(int I = 0; I < ti.entries.size(); I++){
			type_entry &entry = ti.entries[I];
			if(entry.name != ""){
				entry.param_name = SS.add(entry.name);
			}else{
				entry.param_name = -1; 
			}
			if(entry.isExplicit){
				entry.param_type = entry.type;
				//1. valtype value
				if(isValtype(entry)){
					entry.param_1 = entry.paramtype;
					switch(entry.paramtype){
						case EP_CONST_NUM:
							entry.param_2 = entry.param2ConstNum;
							break;
						case EP_CONST_STR:
							entry.param_2 = SS.add(entry.param2ConstString);
							break;
						case EP_ENTRY_VAL:
						case EP_ENTRY_ADDR:
							entry.param_2 = getEntry(ti, entry.param2EntryNum, entry.param2EntryString);
							break;
						case EP_NONE:
							entry.param_2 = 0;
							break;
						default:
							entry.param_2 = 0;
							cout << "<ENUM ERROR>";
							break;
					}
				}
				//2. structtype value
				if(isStructtype(entry)){
					entry.param_1 = getTypeID(types, entry); //or maybe leave it 0?
					entry.param_2 = SS.add(entry.param2TypeName);
				}
				//3. structtype param3
				if(entry.type == TE_VARIANT){
					entry.param_3 = getEntry(ti, -1, entry.param3EntryString);
				}
				if(entry.type == TE_SUBVARIANT){
					entry.param_3 = entry.param3ConstNum;
				}
	
				//4. subentry
				entry.param_subentry = getEntry(ti, entry.subentryNum, entry.subentryString);
			}
		}
	}
	
}

void outputData(vector<typeinfo*> &types, stringstore &SS){
	//1. table of all tables
	dout << "zds_table_of_tables:\n";
	dout << "    db zds_table_ti;\n";
	dout << "    db zds_stringstore;\n";
	dout << "zds_table_of_tables_end: db 0;\n";
	
	dout << "\n";
	
	dout << "zds_table_ti:\n";
	for(int I = 0; I < types.size(); I++){
	dout << "    db ti_" << types[I]->name << ";\n";
	}
	dout << "zds_table_ti_end: db 0;\n";
	
	dout << "\n";
	
	for(int I = 0; I < types.size(); I++){
		typeinfo &ti = *types[I];
	dout << "ti_" << ti.name << ":\n";
	dout << "    db " << ti.param_name << "; // name\n";
	dout << "    db " << ti.param_type << "; // type\n";
	dout << "    db " << ti.entries.size() << "; //num entries\n";
	dout << "    //name, type, isexplicit, param1, param2, param3, subentry\n";
		for(int J = 0; J < ti.entries.size(); J++){
			type_entry &entry = ti.entries[J];
	dout << "        db "<<entry.param_name<<",\t"
						 <<entry.param_type<<",\t"
						 <<(int)entry.isExplicit<<",\t"
						 <<entry.param_1<<",\t"
						 <<entry.param_2<<",\t"
						 <<entry.param_3<<",\t"
						 <<entry.param_subentry<<"; "
						 << "// entry "<<J<<"\t\""<<entry.name<<"\"\n";
		}
	dout << "ti_" << ti.name << "_end: db 0;\n";
	dout << "\n";
	}
	
	dout << "zds_stringstore:\n";
	for(int I = 0; I < SS.strings.size(); I++){
		string S = SS.strings[I];
		dout << "    db str_" << S << ";\n";
	}
	dout << "zds_stringstore_end: db 0;\n";
	
	dout << "\n";
	
	dout << "zds_stringstore_strings:\n";
	for(int I = 0; I < SS.strings.size(); I++){
		string S = SS.strings[I];
		dout << "    str_" << S << ": db \"" << S << "\", 0;\n";
	}
	dout << "zds_stringstore_strings_end: db 0;\n";
}

void readParams(int argc, char **argv){
	if(argc <= 1){
		printf("usage: drag-and-drop the file of interest into this executable,\n"
		"all outputs will go into the /generated folder nearby\n");
	}else{
		filename_in = argv[1];
	}
}