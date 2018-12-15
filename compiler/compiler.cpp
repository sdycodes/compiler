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
	init_gvar();
	readFile(code);
	printf("the MIPS code is in result.txt");
	freopen("result.txt", "w", stdout);
	//词法分析、语法分析、语义分析和中间代码生成
	program();
	//中间代码优化
	//kk_opt();
	//计算流图和寄存器分配
	cal_alloc();
	//生成目标代码
	mc2mp();
	//目标代码优化
	mips_opt();
	//输出目标代码
	dump_mips();
	//dump_tab();	//输出符号表
	//dumpmc();		//输出中间代码
	//dump_blocks();	//输出基本块
	fclose(stdout);
	return 0;
}

