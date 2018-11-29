#include "pch.h"
#include "stdHeader.h"
#include "base_block.h"
#include "globalvar.h"
#include "sign_table.h"
vector<block> blocks;

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
	int i = 0;
	while (i < mc.size()) {
		init_block(tmp);
		tmp.no = cnt++;
		tmp.start = i;
		int j = i + 1;
		//找到本块的结束或下一块的开始
		while (j < mc.size() &&
			mc[j].op != "BNZ"&&mc[j].op != "BEZ"&&mc[j].op != "GOTO" &&
			!(mc[j].op == "LABEL"&&mc[j].n1[0] == '$') &&
			mc[j].op != "EXIT"&&mc[j].op!="RET")
			j++;
		if (j == mc.size()) {
			tmp.end = j - 1;
			blocks.push_back(tmp);
			break;
		}
		else if (mc[j].op == "BNZ" || mc[j].op == "BEZ" || mc[j].op == "GOTO"||mc[j].op=="EXIT"||mc[j].op=="RET") {
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
	for (int i = 1;i < blocks.size() - 1;i++) {
		if (mc[blocks[i].end].op == "GOTO") {
			for (int j = 1;j < blocks.size()-1;j++)
				if (mc[blocks[j].start].n1 == mc[blocks[i].end].n1)
					blocks[i].next.push_back(j);
		}
		else if (mc[blocks[i].end].op == "BEZ" || mc[blocks[i].end].op == "BNZ") {
			blocks[i].next.push_back(i + 1);
			for (int j = 1;j < blocks.size()-1;j++)
				if (mc[blocks[j].start].n1 == mc[blocks[i].end].res)
					blocks[i].next.push_back(j);
		}
		else if (mc[blocks[i].end].op == "EXIT"||mc[blocks[i].end].op=="RET")
			blocks[i].next.push_back(blocks.size() - 1);
		else
			blocks[i].next.push_back(i+1);
	}
	//计算每个块的前驱
	blocks[1].pre.push_back(0);
	for (int i = 1;i < blocks.size()-1;i++) {
		if (i != 1) {
			if (mc[blocks[i - 1].end].op != "GOTO" && mc[blocks[i - 1].end].op != "RET")
				blocks[i].pre.push_back(i-1);
		}
		for (int j = 1;j < blocks.size() - 1;j++) {
			if (mc[blocks[j].end].op == "BNZ" || mc[blocks[j].end].op == "BEZ") {
				if (mc[blocks[j].end].res == mc[blocks[i].start].n1)
					blocks[i].pre.push_back(j);
			}
			else if (mc[blocks[j].end].op == "GOTO"&&mc[blocks[j].end].n1 == mc[blocks[i].start].n1)
				blocks[i].pre.push_back(j);
			else if (mc[blocks[j].end].op == "RET"||mc[blocks[j].end].op=="EXIT")
				blocks.back().pre.push_back(blocks.size()-1);
		}
	}
}

void cal_def_use() {
	int def_loc, loc;
	bool islocal;
	for (int i = 1;i < blocks.size() - 1;i++) {
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
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (rec.find(loc) == rec.end()&&isVar(mc[j].res)) {
					rec.insert(pair<int, bool>(loc, true));
				}
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec.insert(pair<int, bool>(loc, false));
				}
				islocal = false;
				loc = search_tab(mc[j].n2, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n2)) {
					rec.insert(pair<int, bool>(loc, false));
				}
			}
			else if (mc[j].op == "ASSIGN") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec.insert(pair<int, bool>(loc, false));
				}
				islocal = false;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].res)) {
					rec.insert(pair<int, bool>(loc, true));
				}
			}
			else if (mc[j].op == "LELEM") {
				islocal = false;
				loc = search_tab(mc[j].n2, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n2)) {
					rec.insert(pair<int, bool>(loc, false));
				}
				islocal = false;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].res)) {
					rec.insert(pair<int, bool>(loc, true));
				}
			}
			else if (mc[j].op == "PUSH"||mc[j].op=="BEZ"||mc[j].op=="BNZ"||(mc[j].op == "RET"&&mc[j].n1 != "#")) {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec.insert(pair<int, bool>(loc, false));
				}
			}
			else if (mc[j].op == "SELEM") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec.insert(pair<int, bool>(loc, false));
				}
				islocal = false;
				loc = search_tab(mc[j].n2, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n2)) {
					rec.insert(pair<int, bool>(loc, false));
				}
			}
			else if (mc[j].op == "INC" || mc[j].op == "INV") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec.insert(pair<int, bool>(loc, true));
				}
			}
			else if (mc[j].op == "OUTV" || mc[j].op == "OUTC") {
				islocal = false;
				loc = search_tab(mc[j].n1, islocal, def_loc);
				if (rec.find(loc) == rec.end() && isVar(mc[j].n1)) {
					rec.insert(pair<int, bool>(loc, false));
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

void dump_def_use() {
	split_block();
	gen_DAG();
	cal_def_use();
	for (int i = 1;i < blocks.size()-1;i++) {
		cout << "\n--------------------"<<blocks[i].no<<"-----------------\n";
		cout << "def:";
		for (int j = 0;j<MAXSIGNNUM;j++)
			if(blocks[i].def[j]) cout << st[j].name<<" ";
		cout << "\nuse:";
		for (int j = 0;j < MAXSIGNNUM;j++)
			if (blocks[i].use[j]) cout << st[j].name << " ";
		cout << "\npre:";
		for (int j = 0;j < blocks[i].pre.size();j++)
			cout << blocks[i].pre[j] << " ";
		cout << "\nnext:";
		for (int j = 0;j < blocks[i].next.size();j++)
			cout << blocks[i].next[j] << " ";
		cout << "\n";
		for (int j = blocks[i].start;j <= blocks[i].end;j++)
			cout << j<<":"<<mc[j].op << " " << mc[j].res << " " << mc[j].n1 << " " << mc[j].n2<<"\n";
		cout << "-----------------------------------------\n";
	}
}

void cal_in_out(){
	bool change = false;
	do {
		for (int i = 0;i < blocks.size();i++) {
			for (int j = 0;j < blocks[i].next.size();j++) {
				
			}
		}
	} while (change);
}