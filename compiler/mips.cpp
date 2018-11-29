#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
#include "mips.h"
#define no2name(x) (x)>=16&&(x)<=23?"$s"+to_string(x-16):	\
					(x)<16&&(x)>=8?"$t"+to_string(x-8): \
					(x)==24||(x)==25?"$t"+to_string(x-16):"!!!"

#define isCon(num) (isdigit(num[0])||num[0]=='-'||num[0]=='+')
#define REG_NUM 10
int stk[REG_NUM] = { 8,9,10,11,12,13,14,15,24,25 };	//t-register stack

map<int, string> alloc;	//record each t-register and its variable name
vector<string> midvar;

int call_midvar_loc(string name) {
	int i;
	for (i = midvar.size() - 1;i >= 0;i--)
		if (midvar[i] == name)
			break;
	if (i == -1) {
		gen_mips("subi", "$sp", "$sp", "4");
		midvar.push_back(name);
		return 4;
	}
	return (midvar.size() - i) * 4;
}
string get_reg(string name, int def_loc) {
	if (name == "#RET")
		return "$v0";
	static int n = 0;
	n = (n + 1) % 5;
	bool islocal;
	int loc = search_tab(name, islocal, def_loc);
	if (loc != -1 && islocal)
		gen_mips("lw", "$t" + to_string(n), "$fp", to_string(-st[loc].addr));
	else if (loc != -1 && !islocal)
		gen_mips("lw", "$t" + to_string(n), "$gp", to_string(st[loc].addr));
	else if (loc == -1 && name[0] == '#') {
		gen_mips("lw", "$t"+to_string(n), "$sp", to_string(call_midvar_loc(name)));
	}
	return "$t" + to_string(n);
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
	for (int i = 0;i < (int)mc.size();i++) {
		if (mc[i].op == "LABEL") {	//标签
			gen_mips(mc[i].n1+':');
			if (mc[i].n1[0] != '$') {	//是一个函数标签
				midvar.clear();	//清空中间变量
				alloc.clear();	//清空对应关系
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
					gen_mips("addi", numres, get_reg(mc[i].res, def_loc),num1);
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
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#') 
				gen_mips("sw", numres, "$sp", to_string(call_midvar_loc(mc[i].res)));

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
					gen_mips("li", "$k1", num1);
					gen_mips("div", numres, "$k1", num2);
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
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string(call_midvar_loc(mc[i].res)));
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
					gen_mips("li", "$k1", paras[j]);
					gen_mips("sw", "$k1", "$sp", to_string(-j * 4));
				}
				else
					gen_mips("sw", get_reg(paras[j], def_loc), "$sp", to_string(-j * 4));
			}
			paras.clear();
			for (int j = 8;j <= 15;j++)	//保存现场操作
				gen_mips("sw", "$" + to_string(j), "$sp", to_string(-context_offset - j * 4));
			for(int j=24;j<=31;j++)
				gen_mips("sw", "$" + to_string(j), "$sp", to_string(-context_offset - j * 4));
			//分配函数运行栈
			gen_mips("move", "$fp", "$sp");
			gen_mips("subi", "$sp", "$fp", to_string(st[loc].size));
			//jump to function
			gen_mips("jal", mc[i].n1);
			for (int j = 8;j <= 15;j++)	//恢复现场操作
				gen_mips("lw", "$" + to_string(j), "$fp", to_string(-context_offset - j * 4));
			for(int j=24;j<=29;j++)
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
				gen_mips("li", "$k1", isCon(mc[i-1].n1) ? num1 : num2);
				if (isCon(mc[i-1].n1))	num1 = "$k1";
				else num2 = "$k1";
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
			//这里是对于全局变量的特殊操作 未来需要保留
			/*int loc = search_tab(mc[i].n1, islocal, def_loc);
			if (loc != -1 && !islocal)
				gen_mips("sw", get_reg(mc[i].n1, def_loc), "$gp", to_string(st[loc].addr));*/
			int loc = search_tab(mc[i].n1, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string(call_midvar_loc(mc[i].res)));
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
			/*
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc!=-1&&!islocal)
				gen_mips("sw", get_reg(mc[i].res, def_loc), "$gp", to_string(st[loc].addr));*/
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string(call_midvar_loc(mc[i].res)));
		}
		else if (mc[i].op == "SELEM") { 
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			if (isCon(mc[i].n1))
				gen_mips("li", "$k1", num1);
			else
				gen_mips("move", "$k1", num1);
			gen_mips("sll", "$k1", "$k1", "2");
			gen_mips("addi", "$k1", "$k1", to_string(st[arr_loc].addr));
			if (islocal)
				gen_mips("sub", "$k1", "$fp", "$k1");
			else
				gen_mips("add", "$k1", "$gp", "$k1");
			if (isCon(mc[i].n2))
				gen_mips("li", "$k0", mc[i].n2);
			gen_mips("sw", isCon(mc[i].n2) ? "$k0" : num2, "$k1", "0");
		}
		else if (mc[i].op == "LELEM") {
			int arr_loc = search_tab(mc[i].n1, islocal, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, def_loc);
			gen_mips(isCon(mc[i].n2)?"li":"move", "$k1", num2);
			gen_mips("sll", "$k1", "$k1", "2");
			gen_mips("addi", "$k1", "$k1", to_string(st[arr_loc].addr));
			if (islocal)
				gen_mips("sub", "$k1", "$fp", "$k1");
			else
				gen_mips("add", "$k1", "$gp", "$k1");
			string numres = get_reg(mc[i].res);
			gen_mips("lw", numres, "$k1", "0");
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && islocal)
				gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			else if (loc != -1 && !islocal)
				gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
			else if (loc == -1 && mc[i].res[0] == '#')
				gen_mips("sw", numres, "$sp", to_string(call_midvar_loc(mc[i].res)));
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