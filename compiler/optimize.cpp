#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "sign_table.h"
#include "midcode.h"
#include "optimize.h"

#define isNum(x) (x[0]=='+'||x[0]=='-'||isdigit(x[0]))


void midcode_opt() {
	omc = mc;	//深拷贝 mc将保存优化后的中间代码 omc存储过去的
	//copy_spread();	//做复制传播
	const_spread();	//做常量传播
	//kk_opt();	//窥孔优化
	//modify();	//整理紧凑
}

void modify() {
	mce tmp;
	for(int i=1;i<(int)blocks.size();i++){
		blocks[i].start = blocks[i - 1].end + 1;	//每一块的开始应该是下一块的结束+1
		int tag_end = blocks[i].start;	//标记结束位置
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {//每一块的所有指令
			if (mc[j].op == "NULL")
				continue;
			//遇到非空指令
			tmp = mc[j];
			mc[j] = mc[tag_end];
			mc[tag_end] = tmp;
			tag_end++;
		}
		blocks[i].end = tag_end - 1;
	}
}
//基本块内的复制传播算法
void copy_spread() {
	bool islocal;
	int loc = -1;
	for (int i = 0;i < (int)blocks.size();i++) {	//对每个块
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//对每条指令
			if (mc[j].op != "ASSIGN" || mc[j].res[0] != '#')
				continue;
			loc = search_tab(mc[i].n1, islocal);
			if (!islocal||loc==-1)	continue;	//全局变量也慎重
			//仅处理对中间变量的复制传播
			string src = mc[j].n1;	//复制来源
			string dst = mc[j].res;//复制目标
			bool change = false, canDelete = true;
			for (int k = j+1;k <= blocks[i].end;k++) {	//从下一条指令开始
				if (mc[k].res != dst&&mc[k].res!=src&&!change) {	//来源没有被改变 使用的目标都可以替换
					mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
					mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n1;	//TODO:重新规范检查midcode
				}
				else if (change) {	//在被改变的情况下 检查dst还是否被使用过
					if (mc[k].n1 == dst || mc[k].n2 == dst)
						canDelete = false;	//如果还被用过 那就不能再删除了
				}
				else {//来源被改变的那一条还是可以换的
					mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
					mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n1;
					change = true;	//标记来源被改变了
				}
			}
			if (canDelete)	//能删则删
				mc[j].op = "NULL";	//潜在问题 op域判断逻辑是否完善
		}
	}
}

//基本块内的常量传播算法
void const_spread() {
	bool change = false;
	for (int i = 0;i < (int)blocks.size();i++) {	//对每个块
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//对每条代码
			if (mc[j].op == "ASSIGN"&&mc[j].res[0] == '#'&&isNum(mc[j].n1)) {
				string src = mc[j].n1;
				string dst = mc[j].res;
				mc[j].op = "NULL";
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (mc[k].n1 == dst)
						mc[k].n1 = src;
				}
			}
			else if (mc[j].op == "ADD" || mc[j].op == "SUB" || mc[j].op == "MULT" || mc[j].op == "DIV") {
				if (isNum(mc[j].n1) && isNum(mc[j].n2)) {
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
void kk_opt() {
	for (int i = 0;i < (int)mc.size() - 1;i++) {
		//优化1 OUTS OUTC合并成一个
		if (mc[i].op == "OUTS"&&(mc[i+1].op=="OUTC")) {
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
			i = j-1;
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
		else if (mp[i].op == "jr") {
			if (mp[i - 1].res == "$v0"&&mp[i - 1].op == "move"&&mp[i - 2].res == mp[i - 1].n1
				&&(mp[i-2].op=="add"|| mp[i - 2].op == "addi"
					|| mp[i - 2].op == "sub"|| mp[i - 2].op == "move"||
					mp[i-2].op=="lw"||mp[i-2].op=="li"||mp[i-2].op=="mflo")) {
				mp[i - 2].res = "$v0";
				mp.erase(mp.begin() + i - 1);
				i--;
			}
		}

	}
}