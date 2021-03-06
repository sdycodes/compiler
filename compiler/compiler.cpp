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
#include "reg_distribution.h"

//no global var have reg
int main(int argc, char *argv[]) {
	isError = false;
	CHOOSEA = 4;
	init_gvar();
	readFile(code);
	printf("the MIPS code is in result.txt\n");
	//词法分析、语法分析、语义分析和中间代码生成
	program();
	if (!isError) {
		//计算基本块和流图	
		split_block();
		gen_DAG();
		cal_in_out();
		cal_isOB();
		//中间代码优化
		midcode_opt();
		//dump_blocks();	//输出基本块
		freopen("..\\results\\midcode.txt", "w", stdout);
		dumpmc(omc);	//输出旧中间代码
		fclose(stdout);
		//printf("--------------------------\n");
		freopen("..\\results\\new_midcode.txt", "w", stdout);
		dumpmc(mc);		//输出新中间代码
		fclose(stdout);
		cal_alloc();	//寄存器分配
		//生成目标代码
		mc2mp();
		freopen("..\\results\\old_mips.txt", "w", stdout);
		dump_mips();
		fclose(stdout);
		//目标代码优化
		mips_opt();
		//输出目标代码
		freopen("..\\results\\result.txt", "w", stdout);
		dump_mips();
		//dump_tab();	//输出符号表
		//dump_blocks();	//输出基本块
		fclose(stdout);
	}
	//getchar();
	
	return 0;
}

