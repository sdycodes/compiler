#include "pch.h"
#include "stdHeader.h"
#include "base_block.h"
#include "globalvar.h"
#include "sign_table.h"
vector<block> blocks;
int name2reg[MAXSIGNNUM] = { 0 };
#define isVar(x) (((x)[0]=='_'||((x)[0]>='a'&&(x)[0]<='z')||((x)[0]>='A'&&(x)[0]<='Z'))&&islocal)

void init_block(block& b) {
	b.no = 0;
	b.start = 0;
	b.end = 0;
	for (int i = 0;i < MAXSIGNNUM;i++) {
		b.use[i] = false;
		b.def[i] = false;
		b.in[i] = false;
		b.out[i] = false;
	}
}
void split_block() {
	int cnt = 0;
	//入口块
	block init;
	init_block(init);
	init.no = cnt++;
	blocks.push_back(init);
	block tmp;
	int i = 1;
	while (i < (int)mc.size()) {
		init_block(tmp);
		tmp.no = cnt++;
		tmp.start = i;
		int j = i;
		//找到本块的结束或下一块的开始
		while (j < (int)mc.size() &&
			mc[j].op != "BNZ"&&mc[j].op != "BEZ"&&mc[j].op != "GOTO" &&
			!(i!=j&&mc[j].op == "LABEL"&&mc[j].n1[0] == '$') &&
			mc[j].op != "EXIT"&&mc[j].op!="RET")
			j++;
		if (j == mc.size()) {
			tmp.end = j - 1;
			blocks.push_back(tmp);
			break;
		}
		else if (mc[j].op == "BNZ" || mc[j].op == "BEZ" || mc[j].op == "GOTO" || mc[j].op == "EXIT" || mc[j].op == "RET") {
			tmp.end = j;
			blocks.push_back(tmp);
			i = j;
		}
		else {
			tmp.end = j - 1;
			blocks.push_back(tmp);
			i = j - 1;
		}
		i++;
	}
	block exit;
	init_block(exit);
	exit.no = cnt++;
	blocks.push_back(exit);
}

void gen_DAG() {
	//计算每个块的后继
	blocks[0].next.push_back(1);
	for (int i = 1;i < (int)blocks.size() - 1;i++) {
		if (mc[blocks[i].end].op == "GOTO") {
			for (int j = 1;j < (int)blocks.size()-1;j++)
				if (mc[blocks[j].start].n1 == mc[blocks[i].end].n1&&mc[blocks[j].start].op=="LABEL")
					blocks[i].next.push_back(j);
		}
		else if (mc[blocks[i].end].op == "BEZ" || mc[blocks[i].end].op == "BNZ") {
			blocks[i].next.push_back(i + 1);
			for (int j = 1;j < (int)blocks.size()-1;j++)
				if (mc[blocks[j].start].n1 == mc[blocks[i].end].res&&mc[blocks[j].start].op == "LABEL")
					blocks[i].next.push_back(j);
		}
		else if (mc[blocks[i].end].op == "EXIT"||mc[blocks[i].end].op=="RET")
			blocks[i].next.push_back(blocks.size() - 1);
		else
			blocks[i].next.push_back(i+1);
	}
	//计算每个块的前驱
	blocks[1].pre.push_back(0);
	for (int i = 1;i < (int)blocks.size()-1;i++) {
		if (i != 1) {
			if (mc[blocks[i - 1].end].op != "GOTO" && mc[blocks[i - 1].end].op != "RET")
				blocks[i].pre.push_back(i-1);
		}
		for (int j = 1;j < (int)blocks.size() - 1;j++) {
			if (mc[blocks[j].end].op == "BNZ" || mc[blocks[j].end].op == "BEZ") {
				if (mc[blocks[j].end].res == mc[blocks[i].start].n1&&mc[blocks[i].start].op == "LABEL")
					blocks[i].pre.push_back(j);
			}
			else if (mc[blocks[j].end].op == "GOTO"&&mc[blocks[j].end].n1 == mc[blocks[i].start].n1&&mc[blocks[i].start].op == "LABEL")
				blocks[i].pre.push_back(j);
		}
	}
	//计算exit块的前驱
	for (int j = 1;j < (int)blocks.size() - 1;j++) {
		if (mc[blocks[j].end].op == "RET" || mc[blocks[j].end].op == "EXIT")
			blocks.back().pre.push_back(j);
	}
}

void cal_def_use() {
	int def_loc, loc;
	bool islocal;
	for (int i = 1;i < (int)blocks.size() - 1;i++) {
		map<int, bool> rec;
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {
			if (mc[j].op == "LABEL"&&mc[j].n1[0]!='$') {
				def_loc = search_tab(mc[j].n1=="main"?"main":mc[j].n1.substr(5), islocal);
			}
			else if (mc[j].op == "ADD" || mc[j].op == "SUB" ||
				mc[j].op == "MULT" || mc[j].op == "DIV" || mc[j].op == "EQ" || mc[j].op == "NE" ||
				mc[j].op == "LE" || mc[j].op == "LT" || 
				mc[j].op == "GE" || mc[j].op == "GT" || mc[j].op == "LELEM") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec[loc] = false;
				}
				islocal = false;
				loc = search_tab(mc[j].n2, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n2)) {
					rec[loc] = false;
				}
				islocal = false;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].res)) {
					rec[loc] = true;
				}
			}
			else if (mc[j].op == "ASSIGN") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec[loc] = false;
				}
				islocal = false;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].res)) {
					rec[loc] = true;
				}
			}
			else if (mc[j].op == "LELEM") {
				islocal = false;
				loc = search_tab(mc[j].n2, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n2)) {
					rec[loc] = false;
				}
				islocal = false;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].res)) {
					rec[loc] = true;
				}
			}
			else if (mc[j].op == "PUSH"||mc[j].op=="BEZ"||mc[j].op=="BNZ"||(mc[j].op == "RET"&&mc[j].n1 != "#")) {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec[loc] = false;
				}
			}
			else if (mc[j].op == "SELEM") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec[loc] = false;
				}
				islocal = false;
				loc = search_tab(mc[j].n2, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n2)) {
					rec[loc] = false;
				}
			}
			else if (mc[j].op == "INC" || mc[j].op == "INV") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec[loc] = true;
				}
			}
			else if (mc[j].op == "OUTV" || mc[j].op == "OUTC") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec[loc] = false;
				}
			}
		}
		for (map<int, bool>::iterator it = rec.begin();it!=rec.end();it++) {
			if (it->second) {
				blocks[i].def[it->first] = true;
			}
			else
				blocks[i].use[it->first] = true;
		}
	}
}

void unionset(bool a[], bool b[], bool c[], int size) {
	for (int i = 0;i < size;i++)
		c[i] = a[i] || b[i];
}

void substract(bool a[], bool b[], bool c[], int size) {
	for (int i = 0;i < size;i++)
		c[i] = b[i] ? false : a[i];
}

bool assign_and_check_change(bool a[], bool b[], int size) {
	bool change = false;
	for (int i = 0;i < size;i++)
		if (a[i] != b[i]) {
			change = true;
			b[i] = a[i];
		}
	return change;
}


void cal_in_out() {
	split_block();
	gen_DAG();
	cal_def_use();
	bool change = false;
	bool tmp[MAXSIGNNUM];
	do {
		change = false;
		for (int i = 1;i < (int)blocks.size()-1;i++) {
			memset(tmp, 0, sizeof(tmp));
			for (int j = 0;j < (int)blocks[i].next.size();j++) {
				unionset(blocks[blocks[i].next[j]].in, tmp, tmp);
			}
			change = !change ? assign_and_check_change(tmp, blocks[i].out) : true;
			substract(blocks[i].out, blocks[i].def, tmp);
			unionset(tmp, blocks[i].use, tmp);
			change = !change ? assign_and_check_change(tmp, blocks[i].in) : true;
		}
	} while (change);
}


void cal_alloc() {
	cal_in_out();
	int sregcnt = 16;
	int func_begin, func_end;
	bool islocal;
	for (int i = 1;i < (int)blocks.size() - 1;i++) {
		//如果本块是一个函数的开始 那么可以从头分配寄存器
		if (mc[blocks[i].start].op == "LABEL"&&mc[blocks[i].start].n1[0] != '$') {
			sregcnt = 16;
			func_begin = search_tab(mc[blocks[i].start].n1 == "main" ? "main" : mc[blocks[i].start].n1.substr(5), islocal);
			func_end = func_begin + 1;
			while (func_end < stp&&st[func_end].kind != ST_FUNC)
				func_end++;	//一个函数在符号表中的范围 左闭右开区间
		}
		for (int j = func_begin;j < func_end;j++) {
			//如果某个变量是这个基本块的in变量
			if (blocks[i].in[j]) {
				//有寄存器并且此变量没分配过 则分配
				if (sregcnt <= 21 && name2reg[j] == 0)
					name2reg[j] = sregcnt++;
				//如果没有寄存器且这个变量需要分配但是却无法分配，标记为-1
				else if (sregcnt > 21 && name2reg[j] == 0)
					name2reg[j] = -1;
			}
		}
	}
}
void dump_blocks() {
	for (int i = 1;i < (int)blocks.size()-1;i++) {
		cout << "\n--------------------"<<blocks[i].no<<"-----------------\n";
		cout << "def:";
		for (int j = 0;j<MAXSIGNNUM;j++)
			if(blocks[i].def[j]) cout << st[j].name<<" ";
		cout << "\nuse:";
		for (int j = 0;j < MAXSIGNNUM;j++)
			if (blocks[i].use[j]) cout << st[j].name << " ";
		cout << "\nin:";
		for (int j = 0;j < MAXSIGNNUM;j++)
			if (blocks[i].in[j]) cout << st[j].name << " ";
		cout << "\nout:";
		for (int j = 0;j < MAXSIGNNUM;j++)
			if (blocks[i].out[j]) cout << st[j].name << " ";
		cout << "\npre:";
		for (int j = 0;j < (int)blocks[i].pre.size();j++)
			cout << blocks[i].pre[j] << " ";
		cout << "\nnext:";
		for (int j = 0;j < (int)blocks[i].next.size();j++)
			cout << blocks[i].next[j] << " ";
		cout << "\n";
		for (int j = blocks[i].start;j <= blocks[i].end;j++)
			cout << j<<":"<<mc[j].op << " " << mc[j].res << " " << mc[j].n1 << " " << mc[j].n2<<"\n";
		cout << "-----------------------------------------\n";
	}
}

