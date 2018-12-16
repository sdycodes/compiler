#include "pch.h"
#include "stdHeader.h"
#include "base_block.h"
#include "globalvar.h"
#include "sign_table.h"
vector<block> blocks;
bool conflictG[MAXSIGNNUM][MAXSIGNNUM];
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

void cal_ConfG() {
	memset(conflictG, 0, sizeof(conflictG));
	for (int i = 1;i < (int)blocks.size();i++) {
		for (int j = 0;j < stp;j++) {
			for (int k = j + 1;k < stp;k++) {	//in集合两两冲突	in与def冲突
				if (blocks[i].in[j] && (blocks[i].in[k]||blocks[i].def[k])) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
			}
		}
		
	}
}

//计算基本块
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

//计算流图
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

//计算def use
void cal_def_use() {
	int def_loc, loc;
	bool islocal;
	for (int i = 1;i < (int)blocks.size() - 1;i++) {
		map<int, bool> rec;
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {
			if (mc[j].op == "LABEL"&&mc[j].n1[0]!='$') {
				def_loc = search_tab(mc[j].n1=="main"?"main":mc[j].n1.substr(5), islocal);
				//把参数的def置为true
				int k = def_loc + 1;
				while (k < stp&&st[k].kind == ST_PARA) {
					blocks[i].def[k] = true;
					k++;
				}
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

//一些集合计算的辅助函数
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

//计算in out
void cal_in_out() {
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

//冲突图着色分配
void colorAlloc() {
	//计算冲突图
	cal_ConfG();
	bool islocal;
	int sregtot = 6;	//总数	
	for (int i = 1;i < (int)blocks.size();i++) {	//对每个块
		//是一个函数
		if (mc[blocks[i].start].op == "LABEL"&&mc[blocks[i].start].n1[0] != '$') {
			//找到这个函数的所有块
			int n = i + 1;
			while (n<(int)blocks.size()&&(mc[blocks[n].start].op != "LABEL" || mc[blocks[n].start].n1[0] == '$'))
				n++;
			//从i到n左闭右开区间是这个函数的块
			//通过符号表找到这个函数的所有局部变量
			string funcname = mc[blocks[i].start].n1 == "main" ? "main" : mc[blocks[i].start].n1.substr(5);
			int loc = search_tab(funcname, islocal);
			int end = loc+1;
			while (end < stp&&st[end].kind != ST_FUNC)
				end++;
			//通过冲突图构造关于度的map
			//找到这个函数的跨块变量
			bool tmp[MAXSIGNNUM];
			memset(tmp, 0, sizeof(tmp));
			//所有的跨块变量都在集合tmp中
			for (int j = i;j < n;j++) {
				unionset(blocks[j].in, tmp, tmp);
			}
			vector<int> vars;	//变量的编号
			for (int j = loc + 1;j < end;j++)
				if (tmp[j])
					vars.push_back(j);
			//这里的倒置操作只是为了方便看
			//vector<int> vars2;
			//for (int j = vars.size() - 1;j >= 0;j--)
			//	vars2.push_back(vars[j]);
			//vars = vars2;
			//至此所有的需要考察的变量都在vars中
			map<int, int> loc2deg;	//记录这些变量的度
			for (int j = 0;j <(int)vars.size();j++) {
				loc2deg[vars[j]] = 0;	//这个键插入
				for (int k = 0;k <(int)vars.size();k++)	//统计度
					if (conflictG[vars[j]][vars[k]])
						loc2deg[vars[j]]++;
			}
			//开始分配
			stack<int> rec;
			int lasttime = 0;
			while (vars.size() > 1) {
				do {
					lasttime = rec.size();	//一直更新到无法再去除新的点
					for (int j = 0;j < (int)vars.size()&&vars.size()>1;j++) {
						if (loc2deg[vars[j]] < sregtot) {
							//更新度
							for (int x = 0;x < (int)vars.size();x++)
								if (conflictG[vars[j]][vars[x]])
									loc2deg[vars[x]]--;
							rec.push(vars[j]);	//这个点压栈
							vars.erase(vars.begin() + j);	//从图中删除
							j--;
						}
					}
				}while (rec.size() != lasttime && vars.size() > 1);
				if (vars.size() >= 2) {	//如果此时图中还有超过两个点
					//找度最大的点
					int max = -1;
					int not_alloc = -1;
					for (int x = 0;x < (int)vars.size();x++)
						if (max < loc2deg[vars[x]]) {
							max = loc2deg[vars[x]];
							not_alloc = x;
						}
					//标记为不分配
					name2reg[vars[not_alloc]] = -1;
					//更新度
					for (int x = 0;x < (int)vars.size();x++) {
						if (conflictG[vars[not_alloc]][vars[x]])
							loc2deg[vars[x]]--;
					}
					vars.erase(vars.begin()+not_alloc);	//从图中删除
				}
			}
			//着色
			if (vars.size() == 0)
				continue;
			name2reg[vars[0]] = 16;	//最后剩的那个给个寄存器
			int noUse[6];
			while (!rec.empty()) {	//逆序输出 每输出一个 就分配一个寄存器
				memset(noUse, 0, sizeof(noUse));
				int t = rec.top();	//弹栈
				rec.pop();
				for (int x = 0;x < (int)vars.size();x++)	//检查所有的变量
					if (conflictG[t][vars[x]]&&name2reg[vars[x]]>0)	//如果被冲突了并且人家分配了
						noUse[name2reg[vars[x]]-16] = true;	//这个寄存器不能用
				for (int x = 0;x < 6;x++)
					if (!noUse[x]) {
						name2reg[t] = x + 16;	//如果有一个能用 那就分配
						vars.push_back(t);
						break;
					}
			}
		}
	}
}
//朴素的顺序分配
void inOrderAlloc() {
	int sregcnt = 16;	//编号 从16-21
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
void cal_alloc() {
	cal_in_out();
	//inOrderAlloc();
	colorAlloc();
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

