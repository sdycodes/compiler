#include "pch.h"
#include "stdHeader.h"
#include "lexical.h"
#include "globalvar.h"
#include "grammar.h"
#include "sign_table.h"
#include "midcode.h"
#include "mips.h"
#include "base_block.h"
#include "optimize.h"
int main(int argc, char *argv[]) {
	isError = false;
	init_gvar();
	readFile(code);
	printf("the MIPS code is in result.txt");
	freopen("result.txt", "w", stdout);
	//词法分析、语法分析、语义分析和中间代码生成
	program();
	if (!isError) {
		//计算基本块和流图	还没有in out
		split_block();
		gen_DAG();

		//中间代码优化
		//midcode_opt();
		//dump_blocks();	//输出基本块
		//dumpmc(omc);	//输出旧中间代码
		//printf("--------------------------\n");
		//dumpmc(mc);		//输出新中间代码
		//计算寄存器分配
		cal_alloc();	//计算了in out和寄存器分配

		//生成目标代码
		mc2mp();
		//目标代码优化
		//mips_opt();
		//输出目标代码
		dump_mips();
		//dump_tab();	//输出符号表
		//dump_blocks();	//输出基本块
	}
	fclose(stdout);
	return 0;
}

