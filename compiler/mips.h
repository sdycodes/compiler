#ifndef _MIPS_H_
#define _MIPS_H_
#include "stdHeader.h"
int call_midvar_loc(string name);
string get_reg(string name, int def_loc = -1);
void gen_mips(string op, string res = "", string n1 = "", string n2 = "");
void mc2mp(void);
void dump_mips(void);
#endif
