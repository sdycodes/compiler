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

void dump_def_use();
#endif