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
extern bool isOB[MAXSIGNNUM];
void dump_blocks();
void init_block(block& b);
void split_block(void);
void gen_DAG(void);
void cal_def_use(void);
void unionset(bool a[], bool b[], bool c[], int size);
void substract(bool a[], bool b[], bool c[], int size);
bool assign_and_check_change(bool a[], bool b[], int size);
void cal_in_out(void);
void cal_isOB(void);
#endif