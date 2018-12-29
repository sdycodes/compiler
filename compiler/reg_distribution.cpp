#include "pch.h"
#include "stdHeader.h"
#include "base_block.h"
#include "globalvar.h"
#include "sign_table.h"
#include "reg_distribution.h"
#include "mips.h"
bool conflictG[MAXSIGNNUM][MAXSIGNNUM];
int name2reg[MAXSIGNNUM] = { 0 };

void cal_ConfG() {
	memset(conflictG, 0, sizeof(conflictG));
	for (int i = 1;i < (int)blocks.size();i++) {
		for (int j = 0;j < stp;j++) {
			for (int k = j + 1;k < stp;k++) {	//in集合两两冲突	in与def冲突
				if (blocks[i].in[j] && blocks[i].in[k]) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
				if (blocks[i].in[j] && blocks[i].def[k]) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
				if (blocks[i].in[k] && blocks[i].def[j]) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
				if (blocks[i].def[j] && blocks[i].def[k]) { //def集合两两冲突
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
			}
		}
	}
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
			while (n < (int)blocks.size() && (mc[blocks[n].start].op != "LABEL" || mc[blocks[n].start].n1[0] == '$'))
				n++;
			//从i到n左闭右开区间是这个函数的块
			//通过符号表找到这个函数的所有局部变量
			string funcname = mc[blocks[i].start].n1 == "main" ? "main" : mc[blocks[i].start].n1.substr(5);
			int loc = search_tab(funcname, islocal, -2);
			int end = loc + 1;
			while (end < stp&&st[end].kind != ST_FUNC)
				end++;
			//通过冲突图构造关于度的map
			//找到这个函数的跨块变量
			bool tmp[MAXSIGNNUM];
			memset(tmp, 0, sizeof(tmp));
			//所有的跨块变量都在集合tmp中
			for (int j = i;j < n;j++) {
				unionset(blocks[j].in, tmp, tmp, stp);
			}
			vector<int> vars;	//变量的编号
			for (int j = loc + 1;j < end;j++)
				if (tmp[j]) 
					vars.push_back(j);
			/*
			cout << mc[blocks[i].start].n1 << "\n";
			for (int j = 0;j < (int)vars.size();j++)
				cout << st[vars[j]].name << " ";
			cout << "\n";
			for (int j = 0;j < vars.size();j++) {
				for (int k =0;k < vars.size();k++)
					cout << conflictG[vars[j]][vars[k]] << " ";
				cout << "\n";
			}*/
			//至此所有的需要考察的变量都在vars中
			map<int, int> loc2deg;	//记录这些变量的度
			for (int j = 0;j < (int)vars.size();j++) {
				loc2deg[vars[j]] = 0;	//这个键插入
				for (int k = 0;k < (int)vars.size();k++)	//统计度
					if (conflictG[vars[j]][vars[k]])
						loc2deg[vars[j]]++;
			}
			//开始分配
			stack<int> rec;
			int lasttime = 0;
			while (vars.size() > 1) {
				do {
					lasttime = rec.size();	//一直更新到无法再去除新的点
					for (int j = 0;j < (int)vars.size() && vars.size()>1;j++) {
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
				} while (rec.size() != lasttime && vars.size() > 1);
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
					vars.erase(vars.begin() + not_alloc);	//从图中删除
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
					if (conflictG[t][vars[x]] && name2reg[vars[x]] > 0)	//如果被冲突了并且人家分配了
						noUse[name2reg[vars[x]] - 16] = true;	//这个寄存器不能用
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
			func_begin = search_tab(mc[blocks[i].start].n1 == "main" ? "main" : mc[blocks[i].start].n1.substr(5), islocal, -2);
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
	//全局变量处理
	/*
	int i = 0, cnt = CHOOSEA;
	int regNo = 4 + CHOOSEA;
	while (i < stp&&cnt < 4 && st[i].kind != ST_FUNC) {
		if (st[i].kind == ST_VAR) {
			name2reg[i] = regNo++;
			cnt++;
		}
		i++;
	}
	int i = 0, cnt = 0;
	int regNo = 22;
	while (i < stp&&cnt < 1 && st[i].kind != ST_FUNC) {
		if (st[i].kind == ST_VAR) {
			name2reg[i] = regNo++;
			cnt++;
		}
		i++;
	}*/
	colorAlloc();
	//inOrderAlloc();
}
