#ifndef _SIGN_TABLE_H_
#define _SIGN_TABLE_H_
#include "stdHeader.h"

int insert_tab(bool islocal, string name, int kind = -1, int type = -1, int val = -1, int size = -1, int addr = -1);
int search_tab(string name, bool &islocal);
void cal_func_size(int loc);
void dump_tab();
#endif