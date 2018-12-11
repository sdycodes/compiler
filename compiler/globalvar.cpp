#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"

char code[MAXSRCLEN];       //save source code
char strtab[MAXSTRLEN];     //save strings
int strtabp = 0;            //string table pointer

map<string, type_set> reserve_tab;  //map string to its type
map<type_set, string> type2str;

type_set type;  //word's type
int val;        //word's value, valid if the type is CHAR, NUM, STR
string name;    //the word in string format not valid if the type is CHAR, NUM, STR


int cursor = 0;         //global location of source code
int lc = 1, cc = 0;     //the line number and the colum count
char ch = ' ';

//sign table data structure
sigtabe st[MAXSIGNNUM];
int stp = 0;
map<string, int> stidx;

//midcode table data structure
vector<mce> mc;
int tno = 0;
int lno = 0;
//after opt
vector<mce> nmc;


//target code data structure
vector<mce> mp;

void init_gvar() {
	reserve_tab["+"] = PLUS;
	reserve_tab["-"] = MINUS;
	reserve_tab["*"] = TIMES;
	reserve_tab["/"] = DIV;
	reserve_tab["{"] = LBRACE;
	reserve_tab["}"] = RBRACE;
	reserve_tab["["] = LBRACKET;
	reserve_tab["]"] = RBRACKET;
	reserve_tab["("] = LPAR;
	reserve_tab[")"] = RPAR;
	reserve_tab[";"] = SEMICOLON;
	reserve_tab[","] = COMMA;
	reserve_tab["'"] = SQUOTE;
	reserve_tab["\""] = DQUOTE;
	reserve_tab["void"] = VOIDSY;
	reserve_tab["main"] = MAINSY;
	reserve_tab["if"] = IFSY;
	reserve_tab["else"] = ELSESY;
	reserve_tab["do"] = DOSY;
	reserve_tab["while"] = WHILESY;
	reserve_tab["for"] = FORSY;
	reserve_tab["scanf"] = SCANFSY;
	reserve_tab["printf"] = PRINTFSY;
	reserve_tab["return"] = RETURNSY;
	reserve_tab["char"] = CHARSY;
	reserve_tab["int"] = INTSY;
	reserve_tab["const"] = CONSTSY;

	type2str[PLUS] = "PLUS";
	type2str[MINUS] = "MINUS";
	type2str[TIMES] = "TIMES";
	type2str[DIV] = "DIV";
	type2str[BECOMES] = "BECOMES";

	type2str[LESS] = "LESS";
	type2str[LESSEQ] = "LESSEQ";
	type2str[GRTER] = "GRTER";
	type2str[GRTEREQ] = "GRTEREQ";
	type2str[NEQ] = "NEQ";
	type2str[EQ] = "EQ";

	type2str[NUM] = "NUM";
	type2str[STR] = "STR";
	type2str[CHAR] = "CHAR";
	type2str[IDEN] = "IDEN";

	type2str[SQUOTE] = "SQUOTE";
	type2str[DQUOTE] = "DQUOTE";
	type2str[SEMICOLON] = "SEMICOLON";
	type2str[COMMA] = "COMMA";

	type2str[LBRACE] = "LBRACE";
	type2str[RBRACE] = "RBRACE";
	type2str[LBRACKET] = "LBRACKET";
	type2str[RBRACKET] = "RBRACKET";
	type2str[LPAR] = "LPAR";
	type2str[RPAR] = "RPAR";

	type2str[VOIDSY] = "VOIDSY";
	type2str[MAINSY] = "MAINSY";
	type2str[IFSY] = "IFSY";
	type2str[ELSESY] = "ELSESY";
	type2str[DOSY] = "DOSY";
	type2str[WHILESY] = "WHILESY";
	type2str[FORSY] = "FORSY";
	type2str[SCANFSY] = "SCANFSY";
	type2str[PRINTFSY] = "PRINTFSY";
	type2str[RETURNSY] = "RETURNSY";
	type2str[CHARSY] = "CHARSY";
	type2str[INTSY] = "INTSY";
	type2str[CONSTSY] = "CONSTSY";
}

