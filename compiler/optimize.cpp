#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "reg_distribution.h"
#include "sign_table.h"
#include "midcode.h"
#include "optimize.h"

#define isCon(x) (x[0]=='+'||x[0]=='-'||isdigit(x[0]))

bool modified(mce x, string src) {
	//检查来源是否被改变
	if ((x.op == "ADD" || x.op == "SUB" || x.op == "MULT" || x.op == "DIV" || x.op == "ASSIGN" || x.op == "LELEM") && x.res == src)
		return true;
	if ((x.op == "INC" || x.op == "INV") && x.n1 == src)
		return true;
	if ((x.op == "LT" || x.op == "LE" || x.op == "GT" || x.op == "GE" || x.op == "EQ" || x.op == "NE") && x.res == src)
		return true;
	return false;
}

void midcode_opt() {
	omc = mc;	//深拷贝 mc将保存优化后的中间代码 omc存储过去的
	const_copy_spread();
	local_var_spread();
	kk_opt();	//窥孔优化
}


//基本块内中间变量的常量复制传播算法
void const_copy_spread() {
	bool change = false;
	int loc = -1;
	bool islocal = false;
	int def_loc = -1;
	for (int i = 0;i < (int)blocks.size();i++) {	//对每个块
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//对每条代码
			if (mc[j].op == "LABEL"&&mc[j].n1[0] != '$') {	//更新现在在哪个函数里
				def_loc = search_tab(mc[j].n1 == "main" ? "main" : mc[j].n1.substr(5), islocal);
				continue;
			}
			else if (mc[j].op == "ASSIGN"&&mc[j].res[0]=='#'&&isCon(mc[j].n1)) {	
				//中间变量 不可能跨块 不可能被改值 替换即可
				string src = mc[j].n1;
				string dst = mc[j].res;
				mc[j].op = "NULL";
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (mc[k].n1 == dst)
						mc[k].n1 = src;
					if (mc[k].n2 == dst)
						mc[k].n2 = src;
				}
			}
			else if (mc[j].op=="ASSIGN"&&mc[j].res[0]=='#'&&mc[j].n1!="#RET") {
				//中间变量复制了一个不是常数的变量
				string src = mc[j].n1;	//复制来源
				string dst = mc[j].res;//复制目标
				bool change = false, canDelete = true;
				for (int k = j + 1;k <= blocks[i].end;k++) {	//从下一条指令开始
					if (!change&&!modified(mc[k], src)) {	//来源没有被改变 使用的目标都可以替换
						mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
						mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n2;	
					}
					else if (change) {	//在被改变的情况下 检查dst还是否被使用过
						if ((mc[k].op != "INC"&&mc[k].op != "INV")&&(mc[k].n1 == dst || mc[k].n2 == dst))
							canDelete = false;	//如果还被用过 那就不能再删除了
					}
					else {//来源被改变的那一条还是可以换的
						if (mc[k].op != "INC"&&mc[k].op != "INV") {
							mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
							mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n1;
						}
						change = true;	//标记来源被改变了
					}
				}
				if (canDelete)	//能删则删
					mc[j].op = "NULL";	
			}
			else if (mc[j].op == "ADD" || mc[j].op == "SUB" || mc[j].op == "MULT" || mc[j].op == "DIV") {
				if (isCon(mc[j].n1) && isCon(mc[j].n2)) {
					int res = mc[j].op == "ADD" ? stoi(mc[j].n1) + stoi(mc[j].n2) :
							  (mc[j].op == "SUB" ? stoi(mc[j].n1) - stoi(mc[j].n2) :
							  (mc[j].op == "MULT" ? stoi(mc[j].n1)*stoi(mc[j].n2) :
							  stoi(mc[j].n1) / stoi(mc[j].n2)));
					mc[j].op = "ASSIGN";
					mc[j].n1 = to_string(res);
					j--;
				}	
			}
		}
	}
}

//局部变量的常量传播算法
void local_var_spread() {
	bool islocal;
	int def_loc;
	int loc;
	for (int i = 0;i < (int)mc.size();i++) {
		if (mc[i].op == "LABEL"&&mc[i].n1[0]!= '$') {	//一个函数的开始
			def_loc = search_tab(mc[i].n1 == "main" ? "main" : mc[i].n1.substr(5), islocal);
			int j = i+1;
			//检查这个函数的所有中间代码
			while (j < (int)mc.size() && !(mc[j].op == "LABEL"&&mc[j].n1[0] != '$')) {
				if (mc[j].op == "ASSIGN"&&isCon(mc[j].n1)) {
					loc = search_tab(mc[j].res, islocal, def_loc);
					if (!islocal) { j++; continue; }	//全局变量不要乱优化
					if (isOB[loc]) { j++; continue; }		//跨块变量不要乱优化
					int k = j + 1;
					while (k<(int)mc.size()&&!(mc[k].op == "LABEL"&&mc[k].n1[0] != '$')){
						if (((mc[k].op == "INV" || mc[k].op == "INC") && mc[k].n1 == mc[j].res) ||
							((mc[k].op == "LELEM" || mc[k].op == "ASSIGN") && mc[k].res == mc[j].res))
							break;
						if (mc[k].n1 == mc[j].res) mc[k].n1 = mc[j].n1;
						if (mc[k].n2 == mc[j].res) mc[k].n2 = mc[j].n1;
						k++;
					}
					mc[j].op = "NULL";
				}
				j++;
			}
			i = j-1;
		}
	}
}
void kk_opt() {
	for (int i = 0;i < (int)mc.size() - 1;i++) {
		//优化1 OUTS OUTC合并成一个
		if (mc[i].op == "OUTS" && (mc[i + 1].op == "OUTC")) {
			if (mc[i + 1].n1[0] >= '0'&&mc[i + 1].n1[0] <= '9') {
				char append = (char)(stoi(mc[i + 1].n1) % 256);
				mc[i + 1].op = "NULL";
				int j = stoi(mc[i].n1);
				while (strtab[j] != '\0')
					j++;
				strtab[j] = append;
			}
			i++;
		}
		//优化2 GOTO到下一个label之间的代码删除
		else if (mc[i].op == "GOTO") {
			int j = i + 1;
			while (mc[j].op != "LABEL") {
				mc[j].op = "NULL";
				j++;
			}
			i = j - 1;
		}
		//优化3 先LELEM后SELEM SELEM可以去除
		else if (mc[i].op == "LELEM"&&mc[i + 1].op == "SELEM") {
			if (mc[i].res == mc[i + 1].n2&&mc[i].n1 == mc[i + 1].res&&mc[i].n2 == mc[i + 1].n1) {
				mc[i + 1].op = "NULL";
				i++;
			}
		}
		//优化4 先SELEM后LELEM LELEM可以去除
		else if (mc[i].op == "SELEM"&&mc[i + 1].op == "LELEM") {
			if (mc[i].res == mc[i + 1].n2&&mc[i].n1 == mc[i + 1].res&&mc[i].n2 == mc[i + 1].n1) {
				mc[i + 1].op = "NULL";
				i++;
			}
		}
		//优化5 两个连续的EXIT或RET 说明是自己添加的 可以删除后面的那个
		else if (mc[i].op == "RET"&&mc[i + 1].op == "RET") {
			mc[i + 1].op = "NULL";
			i++;
		}
		else if (mc[i].op == "EXIT"&&mc[i + 1].op == "EXIT") {
			mc[i + 1].op = "NULL";
			i++;
		}
		//优化6 对于中间变量仅作为四则运算传递媒介 可以优化但是要保证MIPS翻译逻辑能够应对各种情况
		else if ((mc[i].op == "ADD" || mc[i].op == "SUB" || mc[i].op == "MULT" || mc[i].op == "DIV")
			&& mc[i + 1].op == "ASSIGN"&&mc[i].res == mc[i + 1].n1) {
			mc[i].res = mc[i + 1].res;
			mc[i + 1].op = "NULL";
			i++;
		}
		//优化7 完全无效代码的删除
		else if (mc[i].op == "ASSIGN"&&mc[i].n1 == mc[i].res)
			mc[i].op = "NULL";
		
	}
	
}

void mips_opt() {
	for (int i = 0;i < (int)mp.size();i++) {
		//减立即数是伪指令翻译会多一条
		if (mp[i].op == "subi") {
			mp[i].op = "addi";
			mp[i].n2 = to_string(-stoi(mp[i].n2));
		}
		//对返回值的特定优化
		else if (i>=2&&mp[i].op == "jr") {
			if (mp[i - 1].res == "$v0"&&mp[i - 1].op == "move"&&mp[i - 2].res == mp[i - 1].n1
				&&(mp[i-2].op=="add"|| mp[i - 2].op == "addi"
					|| mp[i - 2].op == "sub"|| mp[i - 2].op == "move"||
					mp[i-2].op=="lw"||mp[i-2].op=="li")) {
				mp[i - 2].res = "$v0";
				mp.erase(mp.begin() + i - 1);
				i--;
			}
		}
		//如果一条lw下面lw的被改掉 那么删除这条lw
		else if (i < (int)mp.size() - 1 && mp[i].op == "lw") {
			if (mp[i + 1].res == mp[i].res&&
				(mp[i+1].op=="add"||mp[i+1].op=="sub"||mp[i+1].op=="mul"||mp[i+1].op=="div"||
					mp[i+1].op=="addi"||mp[i+1].op=="subi"||mp[i+1].op=="lw"||
					mp[i+1].op=="move"||mp[i+1].op=="li"||mp[i+1].op=="sll"))
				mp[i].op = "nop";
		}

	}
}