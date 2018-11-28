#include "pch.h"
#include "stdHeader.h"
#include "lexical.h"
#include "globalvar.h"
#include "grammar.h"
#include "sign_table.h"
#include "midcode.h"
#include "mips.h"
int main(int argc, char *argv[]) {
	init_gvar();
	readFile(code);
	freopen("result.txt", "w", stdout);
	//nextsym(type, val, name);
	//nextsym(type, val, name);
	//expr();
	//varDecl(ST_INT, false);
	//nextsym(type, val, name);
	//program();
	//constDef(false);
	//returnstmt();
	program();
	//dump_tab();
	//dumpmc();
	mc2mp();
	dump_mips();
	fclose(stdout);
	return 0;
}

