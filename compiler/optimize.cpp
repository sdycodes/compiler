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
bool used(mce x, string src) {
	if (x.op != "INC"&&x.op != "INV"&&(x.n1 == src || x.n2 == src))
		return true;
	return false;
}

void common_expr() {
	int loc = -1;
	bool islocal = false;
	bool islocal1 = false, islocal2 = false;
	int def_loc = -1;
	for (int i = 0;i < (int)blocks.size();i++) {
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//对块内每条指令
			//label指令 更新def_loc 用于以后查表用
			if (mc[j].op == "LABEL"&&mc[j].n1[0] != '$')
				def_loc = search_tab(mc[j].n1, islocal);
			//对于加减乘除指令 可作消除公共子表达式
			else if (mc[j].op == "ADD"||mc[j].op == "SUB"||mc[j].op == "MULT" || mc[j].op == "DIV"||mc[j].op=="LELEM") {
				//有全局变量不要做
				int loc1 = search_tab(mc[j].n1, islocal1, def_loc);
				int loc2 = search_tab(mc[j].n2, islocal2, def_loc);
				if ((loc1!=-1&&!islocal1)||(loc2!=-1&&!islocal2))	continue;
				bool n1change = false, n2change = false;	//两个操作数是否被改变
				//检查后面的表达式
				int k;
				for (k = j + 1;k <= blocks[i].end;k++) {
					n1change = modified(mc[k], mc[j].n1);
					n2change = modified(mc[k], mc[j].n2);
					if (n1change || n2change)	break;
					//来源没有被改变	且运算一样 那就是公共子表达式了
					if (mc[k].op == mc[j].op&&mc[k].n1==mc[j].n1&&mc[k].n2==mc[j].n2) {
						mc[k].op = "ASSIGN";
						mc[k].n1 = mc[j].res;	
						//作常量传播
						if (mc[k].res[0] == '#'&&mc[k].n1 != "#RET") {
							//中间变量复制了一个不是常数的变量
							string src = mc[k].n1;	//复制来源
							string dst = mc[k].res;//复制目标
							//复制了全局变量不做传播
							search_tab(src, islocal, def_loc);
							if (src[0] != '#' && !islocal)	continue;
							bool change = false, canDelete = true;
							for (int w = k + 1;w <= blocks[i].end;w++) {	//从下一条指令开始
								if (!change && !modified(mc[w], src)) {	//来源没有被改变 使用的目标都可以替换
									mc[w].n1 = mc[w].n1 == dst ? src : mc[w].n1;
									mc[w].n2 = mc[w].n2 == dst ? src : mc[w].n2;
								}
								else if (change) {	//在被改变的情况下 检查dst还是否被使用过
									if ((mc[w].op != "INC"&&mc[w].op != "INV") && (mc[w].n1 == dst || mc[k].n2 == dst))
										canDelete = false;	//如果还被用过 那就不能再删除了
								}
								else {//来源被改变的那一条还是可以换的
									if (mc[w].op != "INC"&&mc[w].op != "INV") {
										mc[w].n1 = mc[w].n1 == dst ? src : mc[w].n1;
										mc[w].n2 = mc[w].n2 == dst ? src : mc[w].n1;
									}
									change = true;	//标记来源被改变了
								}
							}
							if (canDelete)	//能删则删
								mc[k].op = "NULL";
						}
					}
				}
			}
		}
	}
}

//标签收缩 把重叠的标签删除
void label_modify() {
	for (int i = 0;i < (int)mc.size()-1;i++) {
		if (mc[i].op == mc[i + 1].op&&mc[i].op == "LABEL"&&mc[i].n1[0]=='$'&&mc[i+1].n1[0]=='$') {
			mc[i].op = "NULL";
			string dst = mc[i+1].n1;
			string src = mc[i].n1;
			for (int j = 0;j < (int)mc.size();j++) {
				if (mc[j].n1 == src)	mc[j].n1 = dst;
				if (mc[j].res == src)	mc[j].res = dst;
			}
		}
	}
}
void midcode_opt() {
	omc = mc;	//深拷贝 mc将保存优化后的中间代码 omc存储过去的
	const_copy_spread();	//中间变量的常量和复制传播 局部变量的常量传播
	common_expr();			//把公共子表达式转化为assign语句	并且传播
	label_modify();			//把多重标签收缩
	kk_opt();				//窥孔优化
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
				//复制了全局变量不做传播
				search_tab(mc[j].n1, islocal, def_loc);
				if (src[0]!='#'&&!islocal)	continue;
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
			else if (mc[j].op == "ASSIGN"&&mc[j].res[0] != '#'&&isCon(mc[j].n1)) {
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (isOB[loc]||!islocal)	continue;	//此操作只针对不跨块的局部变量
				//局部变量可能被改值 
				string src = mc[j].n1;
				string dst = mc[j].res;
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (!modified(mc[k], dst)) {
						if (mc[k].n1 == dst)
							mc[k].n1 = src;
						if (mc[k].n2 == dst)
							mc[k].n2 = src;
					}
					else
						break;
				}
			}
		}
		
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//重新扫描这个块
			//有些不跨块的局部变量可能可以删除
			if (mc[j].op == "ASSIGN") {
				if (mc[j].res[0] == '#')	continue;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (isOB[loc]||!islocal)	continue;	//此操作只针对不跨块的局部变量
				bool canDelete = true;
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (used(mc[k], mc[j].res)) {
						canDelete = false;
						break;
					}
				}
				if(canDelete) mc[j].op = "NULL";
			}
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
		//优化2 GOTO到下一个label之间的代码删除 相邻则可以删除GOTO
		else if (mc[i].op == "GOTO") {
			int j = i + 1;
			while (mc[j].op != "LABEL") {
				mc[j].op = "NULL";
				j++;
			}
			if (mc[j].op == "LABEL"&&mc[i].n1 == mc[j].n1)
				mc[i].op = "NULL";
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
		//如果一条lw下面lw的被改掉 那么删除这条lw 而且必须保证这个寄存器没有被使用
		/*
		else if (i < (int)mp.size() - 1 && mp[i].op == "lw") {
			if (mp[i + 1].res == mp[i].res&&
				(mp[i+1].op=="add"||mp[i+1].op=="sub"||mp[i+1].op=="mul"||mp[i+1].op=="div"||
					mp[i+1].op=="addi"||mp[i+1].op=="subi"||mp[i+1].op=="lw"||
					mp[i+1].op=="move"||mp[i+1].op=="li"||mp[i+1].op=="sll")&&
				(mp[i+1].n1!=mp[i].res&&mp[i+1].n2!=mp[i].res))
				mp[i].op = "nop";
		}*/

	}
}