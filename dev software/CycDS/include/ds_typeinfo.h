#ifndef DS_TYPEINFO_GUARD
#define DS_TYPEINFO_GUARD
#include <vector>
using std::vector;

enum type_entry_type{
	TE_ERROR	= 0,//unset
	TE_FLOAT	= 1,//a single byte float 
	TE_STRING	= 2,//same as "ptr [0] char"
	TE_CHAR 	= 3,//a printable character (<=255 and isPrint())
	TE_CS		= 4,//canary-struct - less overhead than DS and no typeinfo.
	TE_DS		= 5,//dynamic-struct
	TE_ARRAY	= 6,//an array[]
	TE_PTR		= 7,//a pointer
	TE_ADDR		= 8,//an address - same as float but is modified when moving
	TE_VARIANT	= 9,//variant subtype - only the members of the selected variant
					//					are visible/constructed in top scope.
	TE_SUBVARIANT=10,//a subtype corresponding to a case number.
	TE_UNION	=11,//union type - if union is anonymous: 
					//					all members visible in top scope
					//			   if union has a name: 
					//					all members visible in the union's scope
					//			   members are not constructed either way.
	TE_RECORD	=12	//records are always-serialized size-data pairs.
};

/*
		  v- is_explicit
type		param1			param2				param3	subentry	name
TE_FLOAT,	[def.val.type]  [def.val.value]  	[]		[]			[name]	
TE_STRING, 	[def.val.type]	[def.val.value]		[]		[]			[name]
TE_CHAR, 	[def.val.type]	[def.val.value]		[]		[]			[name]
TE_CS, 		[typeID]		[typename]			[]		[]			[name]
TE_DS, 		[typeID]		[typename]			[]		[]			[name]
TE_ARRAY,	[def.val.type]	[def.val.value]		[]		[subentry]	[name]
TE_PTR,		[]				[]					[]		[subentry]	[name]
TE_ADDR,	[def.val.type]	[def.val.value]		[]		[]			[name] (in the future, use param3 for origin selection (transform tree node?))
TE_VARIANT, [typeID]		[typename]			[entry]	[subentry]	[name] (must be conditioned on another entries value)
TE_SUBVARIANT,[typeID]		[typename]			[case]	[subentry]	[name] (use param3 for case numbering)
TE_UNION,	[typeID]		[typename]			[]		[]			[name]
TE_RECORD,	[]				[]					[]		[subentry]	[name]
TE_ERROR	[]				[]					[]		[]			[]

is_explicit = implicit entries can only be generated as part of an explicit one.

def.val.type = type of default value:
	constant number
	constant string ptr
	entry (value of another entry)
	selfaddress (address of another entry)
	
def.val.value = actual numeric value of the default

typeID = registered type mark (looked up from name)
typename = string name of the type

subentry = number of another entry.
name = string name.
*/
// note: typeinfo should be linkable with other types to form an "ABI unit".
//
//

enum entry_param_type{
	EP_NONE = 0, 
	EP_CONST_NUM = 1, 
	EP_CONST_STR = 2, 
	EP_ENTRY_VAL = 3, 
	EP_ENTRY_ADDR =4
};

enum type_info_type{
	TI_ERROR = 0,	//unset
	TI_CS = 1,		//canary-struct
	TI_DS = 2,		//dynamic-struct
	TI_VARIANT = 3,	//variant type
	TI_UNION = 4,	//union type
	TI_SUBTYPE = 5	//one of subtypes inside a variant or union type.
};


struct type_entry{
	//intermediary variables
	string name;
	//param_type
	enum type_entry_type type = TE_ERROR;
	bool isExplicit = false;
	//int ID = -1; //number of the entry in a struct.
	
	//valtypes									//structtype
	//----------------------------- param 1 -------------------------------------
	entry_param_type paramtype = EP_NONE;		int param1TypeID = 0;
				
	//----------------------------- param 2 -------------------------------------
	//EP_CONST_NUM, STR
	int param2ConstNum = 0;
	string param2ConstString = "";				string param2TypeName = "";
	
	//EP_ENTRY_ADDR, VAL
	int param2EntryNum = -1;
	string param2EntryString = "";
	
	//----------------------------- param 3 --------------------------------------
												int param3ConstNum = 0;
	
												string param3EntryString = "";
	//----------------------------- subentry -------------------------------------
								int subentryNum = -1;
								string subentryString = "";
	/*
	//param1
	int param1_number = 0
	string param1_string;
	//param2
	int param2_number = 0;
	string param2_string;
	//subentry
	int subentry_number = 0;
	string subentry_string;
	*/
	//final params
	int param_name = 0;
	int param_type = 0;
	int param_1 = 0;
	int param_2 = 0;
	int param_3 = 0;
	int param_subentry = 0;
};

struct typeinfo{
	string name;
	//int casenum = 0;
	enum type_info_type type = TI_ERROR;
	vector<type_entry> entries;
	
	int param_name;
	int param_type;
	//vector<typeinfo*> supertypes;	//types that have this type as a subtype
	//vector<typeinfo*> subtypes;		//children types
};

#endif