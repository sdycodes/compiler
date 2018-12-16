#ifndef _MIDCODE_H_
#define _MIDCODE_H_

#include "stdHeader.h"
#include "globalvar.h"
void genmc(string op, string n1, string n2, string res);
string gent();
string genlabel();
void dumpmc(vector<mce> mc);

#endif
