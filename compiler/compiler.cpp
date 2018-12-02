#include "pch.h"
#include "stdHeader.h"
#include "lexical.h"
#include "globalvar.h"
#include "grammar.h"
#include "sign_table.h"
#include "midcode.h"
#include "mips.h"
#include "base_block.h"
int main(int argc, char *argv[]) {
	init_gvar();
	readFile(code);
	printf("the MIPS code is in result.txt");
	freopen("result.txt", "w", stdout);
	program();
	dump_def_use();
	//dump_tab();
	//dumpmc();
	mc2mp();
	//dump_mips();
	fclose(stdout);
	return 0;
}

