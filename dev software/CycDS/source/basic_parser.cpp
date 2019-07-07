#include "basic_parser.h"
#include <iostream>
using std::cout;

basic_parser::basic_parser(const char *newInput):
	input(newInput),inputInitial(newInput),error(0){}

bool basic_parser::accept(string text){
	if(alwaysSkipSpaces){skipSpaces();}
	const char *in = input;	
	char Cin1, Cin2, Cout1, Cout2;
	for(int I = 0; I < text.length(); I++){
		if(!*input){goto accept_bad;}
		Cin1 = *input++; Cout1 = text[I];
		if(ignorecase){
			Cin2 = tolower(Cin1);
			Cout2 = tolower(Cout1);
		}else{
			Cin2 = Cin1;
			Cout2 = Cout1;
		}
		//pointOutError();
		//printf("Cin1: %c, Cin2: %c, Cout1: %c, Cout2: %c\n",Cin1,Cin2,Cout1,Cout2);
		if(Cin2 != Cout2){
			input = in;
			goto accept_bad;
		}
	}
	
	accept_good:
	//cout << "\naccepted ["<<text<<"]\n";
	return true;
	
	accept_bad:
	input = in;
	//cout << "na(\'"<<text<<"\'),";
	return false;
}

bool basic_parser::acceptIdent(string &ident){
	if(alwaysSkipSpaces){skipSpaces();}
	const char *in = input;
	char C;
	string res;
	if(!*input){return false;}
	C = *input++;
	if(!isalpha(C)){goto ai_bad;}
	while(isalnum(C)){
		res += C;
		C = *input++;
		if(!*input){goto ai_good;}
	}
	input--;
	ai_good:
	ident = res;
	//cout << "accepted ident ["<<ident<<"]\n";
	return true;
	
	ai_bad:
	input = in;
	//cout << "na(Ident),";
	return false;
}

bool basic_parser::acceptInt(int &number){
	if(alwaysSkipSpaces){skipSpaces();}
	const char *in = input;
	string res;
	char C = *input++;
	if(!*input){return false;}
	if(!isdigit(C)){goto bp_ai2_bad;}
	while(isdigit(C)){
		res += C;
		C = *input++;
		if(!*input){goto bp_ai2_good;}
	}
	input--;
	bp_ai2_good:
	number = atoi(res.c_str());
	//cout << "accepted number ["<<number<<"]\n";
	return true;
	
	bp_ai2_bad:
	input = in;
	//cout << "na(Int),";
	return false;
}

bool basic_parser::acceptEOF(){
	if(alwaysSkipSpaces){skipSpaces();}
	return isEOF();
}
void basic_parser::expectEOF(){
	if(!acceptEOF()){
		pointOutError();
		//printf("expected EOF, got [%c][%d]",*input,*input);
		printf("Expected EOF.\n");
		exit(0);	
	}
}

void basic_parser::skipSpaces(){
	const char *in = input;
	while(isspace(*input++)){
		if(!*input){return;}
	}
	input--;
}

void basic_parser::expect(string text){
	if(!accept(text)){
		pointOutError();
		//printf("expected [%s], got [%c][%d]",text.c_str(),*input,*input);
		printf("Expected \"%s\".\n",text.c_str());
		exit(0);	
	}
}

void basic_parser::expectIdent(string &ident){
	if(!acceptIdent(ident)){
		pointOutError();
		//printf("expected ident, got [%c][%d]\n", *input,*input);
		printf("Expected an identifier.\n");
		exit(0);
	}
}

void basic_parser::expectInt(int &number){
	if(!acceptInt(number)){
		pointOutError();
		//printf("expected number, got[%c][%d]\n", *input,*input);
		printf("Expected an integer.\n");
		exit(0);
	}
}

bool basic_parser::isEOF(){return (*input == 0);}

void basic_parser::getErrPos(int pos, int *linenum, int *col, string *linestr){
	int res_linenum = 0;
	int res_col = 0;
	string res_linestr;
	const char *ptr = inputInitial;
	char C = *ptr;
	while(pos--){
		if(!C){break;}
		C = *ptr++;
		if(C == '\n'){
			res_linestr = "";
			res_col = 0;
			res_linenum++;
		}else{
			res_linestr += C;
			res_col++;
		}
	}
	while(C){
		C = *ptr++;
		if(C == '\n'){break;}
		res_linestr += C;
	}
	if(linenum){*linenum = res_linenum;}
	if(col){*col = res_col;}
	if(linestr){*linestr = res_linestr;}
}

void basic_parser::pointOutError(){
	if(alwaysSkipSpaces){skipSpaces();}
	int pos = input-inputInitial;
	int linenum,col;
	string linestr;
	getErrPos(pos,&linenum,&col,&linestr);
	
	printf("line %d:%d:\n",linenum,col);
	printf("%s\n",linestr.c_str());
	for(int I = 0; I < col; I++){
		printf(" ");
	}
	printf("^\n");
}









