#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "sign_table.h"
#include "midcode.h"

//基本块内的复制传播算法
void copy_spread() {
	bool islocal;
	for (int i = 0;i < (int)blocks.size();i++) {	//对每个块
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//对每条指令
			if (nmc[j].op != "ASSIGN" || nmc[j].res[0] != '#')
				continue;
			search_tab(nmc[i].n1, islocal);
			if (!islocal)	continue;	//全局变量也慎重
			//仅处理对中间变量的复制传播
			string src = nmc[j].n1;	//复制来源
			string dst = nmc[j].res;//复制目标
			bool change = false, canDelete = true;
			for (int k = j+1;k <= blocks[i].end;k++) {	//从下一条指令开始
				if (nmc[k].res != src&&!change) {	//来源没有被改变 使用的目标都可以替换
					nmc[k].n1 = nmc[k].n1 == dst ? src : nmc[k].n1;
					nmc[k].n2 = nmc[k].n2 == dst ? src : nmc[k].n1;	//TODO:重新规范检查midcode
				}
				else if (change) {	//在被改变的情况下 检查dst还是否被使用过
					if (nmc[k].n1 == dst || nmc[k].n2 == dst)
						canDelete = false;	//如果还被用过 那就不能再删除了
				}
				else {//来源被改变的那一条还是可以换的
					nmc[k].n1 = nmc[k].n1 == dst ? src : nmc[k].n1;
					nmc[k].n2 = nmc[k].n2 == dst ? src : nmc[k].n1;
					change = true;	//标记来源被改变了
				}
			}
			if (canDelete)	//能删则删
				nmc[j].op = "NULL";	//潜在问题 op域判断逻辑是否完善
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
		//优化3 先LELEM后SELEM一样可以去除
		else if (mc[i].op == "LELEM"&&mc[i + 1].op == "SELEM") {
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