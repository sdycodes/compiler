#ifndef _LEXICAL_H_
#define _LEXICAL_H_

#include "stdHeader.h"
#include "globalvar.h"
void readFile(char code[]);
bool isalu(char c);
bool isalundernum(char c);
char nextch(void);
void nextsym_inner(type_set &type, int &val, string &name);
void nextsym(type_set &type, int &val, string &name);

void outputLexicalAnalyzeRes(void);
#endif
