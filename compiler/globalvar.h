#ifndef _GLOBALVAR_H_
#define _GLOBALVAR_H_
#include "stdHeader.h"
#define MAXSRCLEN 1000000     //max length of source code
#define MAXSTRLEN 256*256*256      //max string length of all string in source code
#define MAXVALLEN 20        //max word length
enum type_set {
	NOTYPE, PLUS, MINUS, TIMES, DIV, BECOMES, \
	LESS, LESSEQ, GRTER, GRTEREQ, NEQ, EQ, \
	NUM, STR, CHAR, IDEN, \
	SQUOTE, DQUOTE, SEMICOLON, COMMA, \
	LBRACE, RBRACE, LBRACKET, RBRACKET, LPAR, RPAR, \
	IFSY, ELSESY, DOSY, WHILESY, FORSY, SCANFSY, PRINTFSY, RETURNSY, \
	MAINSY, VOIDSY, CONSTSY, INTSY, CHARSY, END
};
extern char code[MAXSRCLEN];       //save source code
extern char strtab[MAXSTRLEN];     //save strings
extern int strtabp;
extern map<string, type_set> reserve_tab;  //map string and its type
extern map<type_set, string> type2str;
extern type_set type;  //word's type
extern int val;        //word's value, valid if the type is CHAR, NUM, STR
extern string name;    //the word in string format not valid if the type is CHAR, NUM, STR

extern int cursor, lc, cc;     //the line number and the colum count
extern char ch;
void init_gvar(void);

//sign table entry struct
#define ST_CONST 0
#define ST_VAR 1
#define ST_FUNC 2
#define ST_ARR 3
#define ST_STR 4
#define ST_PARA 5
#define ST_VOID 0
#define ST_INT 1
#define ST_CHAR 2
typedef struct sigtab_ent {
	string name;
	int kind;	//0-const 1-var 2-func 3-array 4-string 5-para
	int type;	//0-void 1-int 2-char
	int size;	
	int addr;
	int val;	//const char-ascii const int-val func-para num array-len string-first loc
	bool islocal;
}sigtabe;

#define MAXSIGNNUM 4000
extern sigtabe st[MAXSIGNNUM];
extern int stp;
extern map<string, int> stidx;


typedef struct midcode {
	string op;
	string n1;
	string n2;
	string res;
}mce;


extern vector<mce> mc;
extern int tno;
extern int lno;

extern vector<mce> omc;
extern vector<mce> mp;
#endif
