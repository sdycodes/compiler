#ifndef _BASE_BLOCK_H_
#define _BASE_BLOCK_H_
#include "stdHeader.h"
#include "globalvar.h"
typedef struct block {
	int no;
	int start;
	int end;
	vector<int> pre;
	vector<int> next;
	bool def[MAXSIGNNUM], use[MAXSIGNNUM], in[MAXSIGNNUM], out[MAXSIGNNUM];
}block;
extern vector<block> blocks;
extern int name2reg[MAXSIGNNUM];
void dump_blocks();
void init_block(block& b);
void split_block();
void gen_DAG();
void cal_def_use();
void unionset(bool a[], bool b[], bool c[], int size = MAXSIGNNUM);
void substract(bool a[], bool b[], bool c[], int size = MAXSIGNNUM);
bool assign_and_check_change(bool a[], bool b[], int size = MAXSIGNNUM);
void cal_in_out();
#endif