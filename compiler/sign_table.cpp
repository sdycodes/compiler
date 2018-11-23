#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
int insert_tab(bool islocal, string name, int kind, int type, int val, int size, int addr) {
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

int search_tab(string name, bool &islocal) {
	int loc = stp - 1;
	while (st[loc].kind != ST_FUNC && loc>=0) {
		if (st[loc].name == name) {
			islocal = true;
			return loc;
		}
		loc--;
	}
	map<string, int>::iterator it = stidx.find(name);
	// local variable
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