#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
#include "error.h"
int insert_tab(bool islocal, string name, int kind, int type, int val, int size, int addr) {
	//插入符号表之前 需要先检查先前是否定义过
	bool prevlocal;
	int prevloc = search_tab(name, prevlocal);
	if (prevloc != -1 && ((prevlocal&&islocal) || (!prevlocal && !islocal))) {
		errmsg("redefined variable or const", 0);
		return prevloc;
	}
	st[stp].name = name;
	st[stp].kind = kind;
	st[stp].type = type;
	st[stp].val = val;
	st[stp].size = size;
	st[stp].islocal = islocal;
	if (st[stp].kind == ST_FUNC)
		st[stp].addr = 0;
	else if (stp != 0 && st[stp - 1].kind == ST_FUNC)
		st[stp].addr = 0;
	else if(stp==0){
		st[stp].addr = 0;
	}
	else {
		st[stp].addr = st[stp - 1].addr + st[stp-1].size;
	}
	if(!islocal && kind!=ST_PARA)
		stidx[name] = stp;
	stp++;
	return stp - 1;
}

int search_tab(string name, bool &islocal, int def_loc) {
	int loc = def_loc==-1?stp - 1:def_loc+1;
	if (def_loc != -2) {
		while (st[loc].kind != ST_FUNC && loc >= 0 && loc < stp) {
			if (st[loc].name == name) {
				islocal = st[loc].islocal;
				return loc;
			}
			if (def_loc == -1)
				loc--;
			else
				loc++;
		}
	}
	map<string, int>::iterator it = stidx.find(name);
	// global variable
	if (it != stidx.end()) {
		islocal = false;
		return it->second;
	}
	return -1;
}

void cal_func_size(int loc) {
	int i = loc + 1;
	int size = 0;
	while (i < stp) {
		size += st[i].size;
		i++;
	}
	st[loc].addr = size;
	size += 4 * 32;	//保存现场用
	size += 4 * tno;	//中间变量存储空间
	st[loc].size = size;
}
void dump_tab() {
	printf("id\tname\t\tkind\ttype\tval\tsize\tislocal\taddr\n");
	for (int i = 0;i < stp;i++) {
		cout << i << '\t' << 
			st[i].name << "\t\t" << st[i].kind << '\t' << st[i].type << '\t' 
			<< st[i].val<<'\t'<<st[i].size << '\t'<<st[i].islocal<<'\t'<<st[i].addr<<'\n';
	}
}