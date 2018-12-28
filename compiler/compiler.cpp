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

//what I have done
//change s6 s7 to v1 a3
//alloc s6 s7 to 2 global var
//pass para via a0~a2
//caution that write back global var has also been modified
int main(int argc, char *argv[]) {
	isError = false;
	CHOOSEA = 4;
	init_gvar();
	readFile(code);
	printf("the MIPS code is in result.txt");
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
		freopen("result.txt", "w", stdout);
		//dump_blocks();	//输出基本块
		//dumpmc(omc);	//输出旧中间代码
		//printf("--------------------------\n");
		//dumpmc(mc);		//输出新中间代码
		
		cal_alloc();	//寄存器分配
		//生成目标代码
		mc2mp();
		//目标代码优化
		mips_opt();
		//输出目标代码
		dump_mips();
		//dump_tab();	//输出符号表
		//dump_blocks();	//输出基本块
		fclose(stdout);
	}
	
	return 0;
}

