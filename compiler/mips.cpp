#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
#include "mips.h"
#include "base_block.h"
#define no2name(x) (x)>=16&&(x)<=23?"$s"+to_string(x-16):	\
					(x)<16&&(x)>=8?"$t"+to_string(x-8): \
					(x)==24||(x)==25?"$t"+to_string(x-16):"!!!"

#define isCon(num) (isdigit(num[0])||num[0]=='-'||num[0]=='+')
#define REG_NUM 8
int tstk[REG_NUM] = { 8,9,10,11,12,13,14,15 };
map<int, string> t_alloc;
string get_reg(string name, int def_loc) {
	//#RET 直接返回v0
	if (name == "#RET")
		return "$v0";
	bool islocal;
	string reg;
	int loc = search_tab(name, islocal, def_loc);
	static int data_buffer = 6;
	//一个全局变量 或者是一个未分配寄存器的跨基本块局部变量
	if (loc != -1 && (!islocal|| name2reg[loc] == -1)) {	
		reg = "$s" + to_string(data_buffer);
		gen_mips("lw", reg, "$gp", to_string(loc));	//从内存中读取
		data_buffer = data_buffer == 6 ? 7 : 6;
		return reg;
	}
	//如果是一个分配了s寄存器的跨基本块局部变量
	if (loc != -1 && name2reg[loc]!=0&&name2reg[loc]!=-1) {
		return "$s" + to_string(name2reg[loc]);
	}
	//如果是不跨基本块的局部变量或者中间变量 采用LRU算法
	if(loc==-1||name2reg[loc]==0){
		map<int, string>::iterator it = t_alloc.begin();
		for (;it != t_alloc.end();it++) {
			if (it->second == name)
				break;
		}
		//如果当前变量已经分配了寄存器
		if (it != t_alloc.end()) {
			int v;
			//寻找它对应的寄存器位置
			for (v = 0;v < REG_NUM;v++) {
				if (tstk[v] == it->first)
					break;
			}
			int r = tstk[v];
			//放到顶部 LRU算法
			for (int w = v;w >= 1;w--)
				tstk[w] = tstk[w - 1];
			tstk[0] = r;
			return no2name(r);
		}
		//如果没有分配寄存器
		else {
			string rec_name = "";
			//如果栈底寄存器不空 说明寄存器全部被占用 需要弹出栈底
			if (t_alloc.find(tstk[REG_NUM - 1]) != t_alloc.end() && t_alloc[tstk[REG_NUM - 1]] != "") {
				rec_name = t_alloc[tstk[REG_NUM - 1]];
			}
			int r = tstk[REG_NUM - 1];
			for (int k = REG_NUM - 1;k >= 1;k--)
				tstk[k] = tstk[k - 1];
			tstk[0] = r;	//栈底寄存器放到栈顶
			t_alloc[r] = name;	//分配寄存器
			//rec_name不空说明被挤掉了，需要回写
			if (rec_name != "") {
				int rec_loc = search_tab(rec_name, islocal, def_loc);
				if (rec_loc == -1) 	//一个中间变量
					gen_mips("sw", no2name(r), "$sp", to_string((stoi(rec_name.substr(1)) << 2) + 4));
				else //一个不跨越基本块的局部变量
					gen_mips("sw", no2name(r), "$fp", to_string(-st[rec_loc].addr));
			}
			//把新分配的load出来
			if (loc == -1)
				gen_mips("lw", no2name(r), "$sp", to_string((stoi(name.substr(1)) << 2) + 4));
			else
				gen_mips("lw", no2name(r), "$fp", to_string(-st[loc].addr));
			return no2name(r);
		}
	}
	else {
		return "!!!";
	}
}
void gen_mips(string op, string res, string n1, string n2) {
	mce tmp;
	tmp.op = op;
	tmp.res = res;
	tmp.n1 = n1;
	tmp.n2 = n2;
	mp.push_back(tmp);
}
void mc2mp() {
	mce tmp;
	bool islocal;
	vector<string> paras;
	int def_loc;
	gen_mips(".data", "", "", "");
	string all_string="";
	for (int s = 0;s < strtabp;s++) {
		if (strtab[s] == '\0')
			all_string += "\\0";
		else if(strtab[s]=='\\')
			all_string += "\\\\";
		else
			all_string += strtab[s];
	}
	all_string += '"';
	all_string = '"' + all_string;
	gen_mips("$string:", ".asciiz", all_string, "");
	gen_mips(".text", "", "", "");
	def_loc = search_tab("main", islocal, -1);
	gen_mips("li", "$fp", "0x7fffeffc");
	gen_mips("subi", "$sp", "$fp", to_string(st[def_loc].size));
	int block_end = 0, block_no = 1;
	for (int i = 0;i < (int)mc.size();i++) {
		//标签
		if (mc[i].op == "LABEL") {	
			gen_mips(mc[i].n1+':');
			if (mc[i].n1[0] != '$') {	//是一个函数标签
				def_loc = search_tab(mc[i].n1=="main"?"main":mc[i].n1.substr(5), islocal);
			}
		}
		else if (mc[i].op == "ADD"||mc[i].op=="SUB") {
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);
			string numres = get_reg(mc[i].res, def_loc);
			//都是常量 li
			if (isCon(mc[i].n1)&&isCon(mc[i].n2)) {
				int res = mc[i].op == "ADD" ? stoi(num1) + stoi(num2) : stoi(num1) - stoi(num2);
				gen_mips("li", numres, to_string(res));
			}
			//一个是常量 一个是变量 addi/subi
			else if (isCon(mc[i].n2)) {
				string op = mc[i].op == "ADD" ? "addi" : "subi";
				gen_mips(op, numres, num1, num2);
			}
			else if (isCon(mc[i].n1)) {
				if (mc[i].op == "SUB") {
					gen_mips("sub", numres, "$0", num2);
					gen_mips("addi", numres, numres, num1);
				}
				else {
					gen_mips("addi", numres, num2, num1);
				}
			}
			//都是变量
			else {
				string op = mc[i].op == "ADD" ? "add" : "sub";
				gen_mips(op, numres, num1, num2);
			}
			/*
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#') 
				gen_mips("sw", numres, "$sp", to_string((stoi(mc[i].res.substr(1)) << 2) + 4));

			*/
		}
		else if(mc[i].op=="MULT"||mc[i].op=="DIV"){
			//都是常量 li
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);
			string numres = get_reg(mc[i].res, def_loc);
			if (isCon(mc[i].n1)&&isCon(mc[i].n2)) {
				int res = mc[i].op == "MULT" ? stoi(num1) * stoi(num2) : stoi(num1) / stoi(num2);
				gen_mips("li", numres, to_string(res));
			}
			//一个是常量 一个是变量
			else if (isCon(mc[i].n2)) {
				string op = mc[i].op == "MULT" ? "mul" : "div";
				gen_mips(op, numres, num1, num2);
			}
			else if (isCon(mc[i].n1)) {
				if (mc[i].op == "DIV") {
					gen_mips("li", "$t8", num1);
					gen_mips("div", numres, "$t8", num2);
				}
				else {
					gen_mips("mul", numres, num2, num1);
				}
			}
			//都是变量
			else {
				string op = mc[i].op == "MULT" ? "mult" : "div";
				gen_mips(op, num1, num2);
				gen_mips("mflo", numres);
			}
			/*
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string((stoi(mc[i].res.substr(1)) << 2) + 4));
			*/
		}
		else if (mc[i].op == "PUSH") {
			while (mc[i].op == "PUSH") {
				paras.push_back(mc[i].n1);
				i++;
			}
			i--;
		}
		else if (mc[i].op == "CALL") {
			int loc = search_tab(mc[i].n1=="main"?"main":mc[i].n1.substr(5), islocal);
			int context_offset = st[loc].addr;	//保存现场的起始位置
			//传参数
			for (int j = 0;j < (int)paras.size();j++) {
				if (isCon(paras[j])) {
					gen_mips("li", "$t8", paras[j]);
					gen_mips("sw", "$t8", "$sp", to_string(-j * 4));
				}
				else
					gen_mips("sw", get_reg(paras[j], def_loc), "$sp", to_string(-j * 4));
			}
			paras.clear();
			for (int j = 8;j <= 31;j++)	//保存现场操作
				gen_mips("sw", "$" + to_string(j), "$sp", to_string(-context_offset - j * 4));
			//分配函数运行栈
			gen_mips("move", "$fp", "$sp");
			gen_mips("subi", "$sp", "$fp", to_string(st[loc].size));
			//jump to function
			gen_mips("jal", mc[i].n1);
			for (int j = 8;j <= 29;j++)	//恢复现场操作
				gen_mips("lw", "$" + to_string(j), "$fp", to_string(-context_offset - j * 4));
			gen_mips("lw", "$" + to_string(31), "$fp", to_string(-context_offset - 31*4));
			gen_mips("lw", "$fp", "$fp", to_string(-context_offset - 120));
		}
		else if (mc[i].op == "RET") {
			//put the result on $v0
			if (isCon(mc[i].n1))
				gen_mips("li", "$v0", mc[i].n1);
			else if (mc[i].n1 != "#")
				gen_mips("move", "$v0", get_reg(mc[i].n1, def_loc));
			gen_mips("jr", "$ra");
		}
		else if (mc[i].op == "OUTS"|| mc[i].op == "OUTV"|| mc[i].op == "OUTC") {
			gen_mips("li", "$v0", mc[i].op == "OUTS" ? "4" : mc[i].op == "OUTV" ? "1" : "11");
			if (mc[i].op == "OUTS") {
				gen_mips("li", "$a0", to_string(0x10010000 + stoi(mc[i].n1)));
				gen_mips("syscall");
			}
			else {
				if (isdigit(mc[i].n1[0]) || mc[i].n1[0] == '-' || mc[i].n1[0] == '+') {
					gen_mips("li", "$a0", mc[i].n1);
					gen_mips("syscall");
				}
				else {
					gen_mips("move", "$a0", get_reg(mc[i].n1, def_loc));
					gen_mips("syscall");
				}
			}
		}
		else if(mc[i].op=="EXIT"){
			gen_mips("li", "$v0", "10");
			gen_mips("syscall");
		}
		else if (mc[i].op == "LT" || mc[i].op == "LE" || mc[i].op == "GT" || mc[i].op == "GE" || mc[i].op == "EQ" || mc[i].op == "NE") {
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);	
			i++;
			if (isCon(mc[i-1].n1)&&isCon(mc[i-1].n2)) {
				if ((mc[i - 1].op == "LT"&&stoi(num1) < stoi(num2))||
					(mc[i - 1].op == "LE"&&stoi(num1) <= stoi(num2))||
					(mc[i - 1].op == "GT"&&stoi(num1)>stoi(num2))||
					(mc[i - 1].op == "GE"&&stoi(num1)>=stoi(num2))||
					(mc[i - 1].op == "EQ"&&stoi(num1)==stoi(num2))||
					(mc[i - 1].op == "NE"&&stoi(num1)!=stoi(num2)))
					if (mc[i].op == "BEZ") {
						gen_mips("j", mc[i].res);
						continue;
					}
				if ((mc[i - 1].op == "LT"&&stoi(num1) >= stoi(num2)) ||
					(mc[i - 1].op == "LE"&&stoi(num1) > stoi(num2)) ||
					(mc[i - 1].op == "GT"&&stoi(num1) <= stoi(num2)) ||
					(mc[i - 1].op == "GE"&&stoi(num1) < stoi(num2)) ||
					(mc[i - 1].op == "EQ"&&stoi(num1) != stoi(num2)) ||
					(mc[i - 1].op == "NE"&&stoi(num1) == stoi(num2)))
					if (mc[i].op == "BNZ") {
						gen_mips("j", mc[i].res);
						continue;
					}
			}
			if (isCon(mc[i-1].n1) || isCon(mc[i-1].n2)) {
				gen_mips("li", "$t8", isCon(mc[i-1].n1) ? num1 : num2);
				if (isCon(mc[i-1].n1))	num1 = "$t8";
				else num2 = "$t8";
			}
			if (mc[i - 1].op == "LT"&&mc[i].op == "BNZ")
				gen_mips("bge", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "LT"&&mc[i].op == "BEZ")
				gen_mips("blt", mc[i].res, num1, num2);
			else if(mc[i-1].op=="LE"&&mc[i].op=="BNZ")
				gen_mips("bgt", mc[i].res, num1, num2);
			else if(mc[i-1].op=="LE"&&mc[i].op=="BEZ")
				gen_mips("ble", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GT"&&mc[i].op == "BNZ")
				gen_mips("ble", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GT"&&mc[i].op == "BEZ")
				gen_mips("bgt", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GE"&&mc[i].op == "BNZ")
				gen_mips("blt", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GE"&&mc[i].op == "BEZ")
				gen_mips("bge", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "EQ"&&mc[i].op == "BNZ")
				gen_mips("bne", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "EQ"&&mc[i].op == "BEZ")
				gen_mips("beq", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "NE"&&mc[i].op == "BNZ")
				gen_mips("beq", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "NE"&&mc[i].op == "BEZ")
				gen_mips("bne", mc[i].res, num1, num2);
		}
		else if (mc[i].op == "INC"||mc[i].op=="INV") {
			gen_mips("li", "$v0", mc[i].op=="INV"?"5":"12");
			gen_mips("syscall");
			string numres = get_reg(mc[i].n1, def_loc);
			gen_mips("move", numres, "$v0");
			//这里是对于全局变量的特殊操作 
			int loc = search_tab(mc[i].n1, islocal, def_loc);
			if (loc != -1 && (!islocal||name2reg[loc]==-1))
				gen_mips("sw", get_reg(mc[i].n1, def_loc), "$gp", to_string(st[loc].addr));
			/*
			int loc = search_tab(mc[i].n1, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string((stoi(mc[i].res.substr(1)) << 2) + 4));
			*/
		}
		else if (mc[i].op == "GOTO") {
			gen_mips("j", mc[i].n1);
		}
		else if (mc[i].op == "ASSIGN") {
			string numres = get_reg(mc[i].res, def_loc);
			if (isCon(mc[i].n1))
				gen_mips("li", numres, mc[i].n1);
			else
				gen_mips("move", numres, get_reg(mc[i].n1, def_loc));
			//对全局变量的特殊操作
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc!=-1&&(!islocal||name2reg[loc]==-1))
				gen_mips("sw", get_reg(mc[i].res, def_loc), "$gp", to_string(st[loc].addr));
			/*int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string((stoi(mc[i].res.substr(1)) << 2) + 4));*/
		}
		else if (mc[i].op == "SELEM") { 
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			if (isCon(mc[i].n1))
				gen_mips("li", "$t8", num1);
			else
				gen_mips("move", "$t8", num1);
			gen_mips("sll", "$t8", "$t8", "2");
			gen_mips("addi", "$t8", "$t8", to_string(st[arr_loc].addr));
			if (islocal)
				gen_mips("sub", "$t8", "$fp", "$t8");
			else
				gen_mips("add", "$t8", "$gp", "$t8");
			if (isCon(mc[i].n2))
				gen_mips("li", "$t9", mc[i].n2);
			gen_mips("sw", isCon(mc[i].n2) ? "$t9" : num2, "$t8", "0");
		}
		else if (mc[i].op == "LELEM") {
			int arr_loc = search_tab(mc[i].n1, islocal, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);
			gen_mips(isCon(mc[i].n2)?"li":"move", "$t8", num2);
			gen_mips("sll", "$t8", "$t8", "2");
			gen_mips("addi", "$t8", "$t8", to_string(st[arr_loc].addr));
			if (islocal)
				gen_mips("sub", "$t8", "$fp", "$t8");
			else
				gen_mips("add", "$t8", "$gp", "$t8");
			//对全局变量的特殊操作
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && (!islocal || name2reg[loc] == -1))
				gen_mips("sw", get_reg(mc[i].res, def_loc), "$gp", to_string(st[loc].addr));

			/*string numres = get_reg(mc[i].res);
			gen_mips("lw", numres, "$t8", "0");
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string((stoi(mc[i].res.substr(1)) << 2) + 4));
				*/
		}
		//一个基本块结束 清除t寄存器的对应关系  更新位置
		if (i == block_end) {
			t_alloc.clear();
			block_no++;
			if (block_no <= (int)blocks.size() - 1)
				block_end = blocks[block_no].end;
		}
	}
}

void dump_mips() {
	string a, b, c, d;
	for (int i = 0;i < (int)mp.size();i++){
		if (mp[i].op == "beq" || mp[i].op == "bne" || mp[i].op == "ble" || mp[i].op == "blt" || mp[i].op == "bgt" || mp[i].op == "bge")
			printf("%s %s %s %s\n", mp[i].op.c_str(), mp[i].n1.c_str(), mp[i].n2.c_str(), mp[i].res.c_str());
		else if (mp[i].op == "sw" || mp[i].op == "lw")
			printf("%s %s %s(%s)\n", mp[i].op.c_str(), mp[i].res.c_str(), mp[i].n2.c_str(), mp[i].n1.c_str());
		else
			printf("%s %s %s %s\n", mp[i].op.c_str(), mp[i].res.c_str(), mp[i].n1.c_str(), mp[i].n2.c_str());
	}
}