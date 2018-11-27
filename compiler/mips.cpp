#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
#include "mips.h"
#define no2name(x) (x)>=16&&(x)<=23?"s"+to_string(x):(x)<16?"t"+to_string(x-8):"t"+to_string(x-16)
int stk[18] = { 8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25 };	//register stack
map<int, string> alloc;	//record each register and its variable name
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
	int loc;
	bool islocal;
	int max_num = 0;
	map<int, string>::iterator it = alloc.begin();
	for (;it != alloc.end();it++) {
		if (it->second == name)
			break;
	}
	//如果当前变量已经分配了寄存器
	if (it != alloc.end()) {
		int i;
		//寻找它对应的寄存器位置
		for (i = 0;i < 18;i++) {
			if (stk[i] == it->first)
				break;
		}
		int r = i;
		//放到顶部 LRU算法
		for (int j = i;j >= 1;j--)
			stk[j] = stk[j - 1];
		stk[0] = r;
		return no2name(r);
	}
	//如果没有分配寄存器
	else {
		string rec_name = "";
		//如果栈底寄存器不空 说明寄存器全部被占用 需要弹出栈底
		if (alloc.find(stk[17]) != alloc.end()&&alloc[stk[17]]!="") {
			rec_name = alloc[stk[17]];
		}
		int r = stk[17];
		for (int k = 17;k >= 1;k--)
			stk[k] = stk[k - 1];
		stk[0] = r;	//栈底寄存器放到栈顶
		alloc[r] = name;	//分配寄存器
		//rec_name不空说明被挤掉了，需要回写
		if (rec_name != "") {
			loc = search_tab(rec_name, islocal, def_loc);
			if (loc==-1) 	//中间变量回写可能还没分配内存，需要压栈，在call_midvar_loc中
				gen_mips("sw", no2name(r), "$sp", to_string(call_midvar_loc(name)));
			else if(islocal)
				gen_mips("sw", no2name(r), "$fp", to_string(-st[loc].addr));
			else
				gen_mips("sw", no2name(r), "$gp", to_string(st[loc].addr));
		}
		//把新分配的load出来
		loc = search_tab(name, islocal, def_loc);
		if (loc==-1)
			gen_mips("lw", no2name(r), "$sp", to_string(call_midvar_loc(name)));
		else if(islocal)
			gen_mips("lw", no2name(r), "$fp", to_string(-st[loc].addr));
		else
			gen_mips("lw", no2name(r), "$gp", to_string(st[loc].addr));
		return no2name(r);
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
	int def_loc, def_context_offset;
	for (int i = 0;i < (int)mc.size();i++) {
		if (mc[i].op == "LABEL") {	//标签
			gen_mips(mc[i].n1);
			if (mc[i].n1[0] != '$') {	//是一个函数标签
				midvar.clear();	//清空中间变量
				def_loc = search_tab(mc[i].n1, islocal);
			}
		}
		else if (mc[i].op == "ADD"||mc[i].op=="SUB") {
			int loc1 = search_tab(mc[i].n1, islocal, def_loc);
			int loc2 = search_tab(mc[i].n2, islocal, def_loc);
			string num1 = loc1 != -1 && st[loc1].kind == ST_CONST ? to_string(st[loc1].val) :
				loc1 != -1 && st[loc1].kind != ST_CONST ? get_reg(mc[i].n1, def_loc) :
				loc1 == -1 && mc[i].n1[0] == '#' ? get_reg(mc[i].n1, def_loc) :
				mc[i].n1;
			string num2 = loc2 != -1 && st[loc2].kind == ST_CONST ? to_string(st[loc2].val) :
				loc2 != -1 && st[loc2].kind != ST_CONST ? get_reg(mc[i].n2, def_loc) :
				loc2 == -1 && mc[i].n2[0] == '#' ? get_reg(mc[i].n2, def_loc) :
				mc[i].n2;
			bool isn1Con = loc1 != -1 && st[loc1].kind == ST_CONST ?true:
				loc1 != -1 && st[loc1].kind != ST_CONST ? false :
				loc1 == -1 && mc[i].n1[0] == '#' ? false :true;
			bool isn2Con = loc2 != -1 && st[loc2].kind == ST_CONST ? true :
				loc2 != -1 && st[loc2].kind != ST_CONST ? false :
				loc2 == -1 && mc[i].n2[0] == '#' ? false : true;
			//都是常量 li
			if (isn1Con&&isn2Con) {
				int res = mc[i].op == "ADD" ? stoi(num1) + stoi(num2) : stoi(num1) - stoi(num2);
				gen_mips("li", get_reg(mc[i].res, def_loc), to_string(res));
			}
			//一个是常量 一个是变量 addi/subi
			else if (isn2Con) {
				string op = mc[i].op == "ADD" ? "addi" : "subi";
				gen_mips(op, get_reg(mc[i].res, def_loc), num1, num2);
			}
			else if (isn1Con) {
				if (mc[i].op == "SUB") {
					gen_mips("sub", get_reg(mc[i].res, def_loc), "$0", num2);
					gen_mips("addi", get_reg(mc[i].res, def_loc), get_reg(mc[i].res, def_loc), num1);
				}
				else {
					gen_mips("addi", get_reg(mc[i].res, def_loc), num1, num2);
				}
			}
			//都是变量
			else {
				string op = mc[i].op == "ADD" ? "add" : "sub";
				gen_mips(op, get_reg(mc[i].res, def_loc), num1, num2);
			}
		}
		else if(mc[i].op=="MULT"||mc[i].op=="DIV"){
			int loc1 = search_tab(mc[i].n1, islocal, def_loc);
			int loc2 = search_tab(mc[i].n2, islocal, def_loc);
			string num1 = loc1 != -1 && st[loc1].kind == ST_CONST ? to_string(st[loc1].val) :
				loc1 != -1 && st[loc1].kind != ST_CONST ? get_reg(mc[i].n1, def_loc) :
				loc1 == -1 && mc[i].n1[0] == '#' ? get_reg(mc[i].n1, def_loc) :
				mc[i].n1;
			string num2 = loc2 != -1 && st[loc2].kind == ST_CONST ? to_string(st[loc2].val) :
				loc2 != -1 && st[loc2].kind != ST_CONST ? get_reg(mc[i].n2, def_loc) :
				loc2 == -1 && mc[i].n2[0] == '#' ? get_reg(mc[i].n2, def_loc) :
				mc[i].n2;
			bool isn1Con = loc1 != -1 && st[loc1].kind == ST_CONST ? true :
				loc1 != -1 && st[loc1].kind != ST_CONST ? false :
				loc1 == -1 && mc[i].n1[0] == '#' ? false : true;
			bool isn2Con = loc2 != -1 && st[loc2].kind == ST_CONST ? true :
				loc2 != -1 && st[loc2].kind != ST_CONST ? false :
				loc2 == -1 && mc[i].n2[0] == '#' ? false : true;
			//都是常量 li
			if (isn1Con&&isn2Con) {
				int res = mc[i].op == "MULT" ? stoi(num1) * stoi(num2) : stoi(num1) / stoi(num2);
				gen_mips("li", get_reg(mc[i].res, def_loc), to_string(res));
			}
			//一个是常量 一个是变量
			else if (isn1Con||isn2Con) {
				string op = mc[i].op == "MULT" ? "mult" : "div";
				gen_mips("li", "$v1", isn1Con?num1:num2);
				gen_mips(op, get_reg(mc[i].res, def_loc), isn1Con?"$v1":num1, isn2Con?"$v1":num2);
				gen_mips("mflo", get_reg(mc[i].res, def_loc));
			}
			//都是变量
			else {
				string op = mc[i].op == "MULT" ? "mult" : "div";
				gen_mips(op, num1, num2);
				gen_mips("mflo", get_reg(mc[i].res, def_loc));
			}
		}
		else if (mc[i].op == "PUSH") {
			while (mc[i].op == "PUSH") {
				paras.push_back(mc[i].n1);
				i++;
			}
			i--;
		}
		else if (mc[i].op == "CALL") {
			int loc = search_tab(mc[i].n1, islocal);
			int context_offset = st[loc].addr;	//保存现场的起始位置
			//传参数
			for (int j = 0;j < (int)paras.size();j++) {
				gen_mips("sw", get_reg(paras[j], def_loc), "$sp", to_string(-j * 4));
			}
			paras.clear();
			for (int j = 2;j <= 31;j++)	//保存现场操作
				gen_mips("sw", "$" + to_string(j), "$sp", to_string(-context_offset - (j - 1) * 4));
			//分配函数运行栈
			gen_mips("move", "$fp", "$sp");
			gen_mips("subi", "$sp", "$fp", to_string(st[loc].size));
			//jump to function
			gen_mips("jal", mc[i].n1);
			for (int j = 2;j <= 28;j++)	//恢复现场操作
				gen_mips("lw", "$" + to_string(j), "$fp", to_string(-context_offset - (j - 1) * 4));
			gen_mips("lw", "$" + to_string(30), "$fp", to_string(-context_offset - (30 - 1) * 4));
			gen_mips("lw", "$" + to_string(31), "$fp", to_string(-context_offset - (31 - 1) * 4));
			//release the stack
			gen_mips("addi", "$fp", "$sp", to_string(st[loc].size));
		}
		else if (mc[i].op == "RET") {
			//put the result on $v0
			if (mc[i].n1 != "#")
				gen_mips("move", "$v0", get_reg(mc[i].n1, def_loc));
			gen_mips("jr", "$ra");
		}
		else if (mc[i].op == "OUTS"|| mc[i].op == "OUTV"|| mc[i].op == "OUTC") {
			gen_mips("li", "$v0", mc[i].op == "OUTS" ? "4" : mc[i].op == "OUTV" ? "5" : "11");
			gen_mips("la", "$a0", mc[i].n1);
			gen_mips("syscall");
		}
		else if(mc[i].op=="EXIT"){
			gen_mips("li", "$v0", "10");
			gen_mips("syscall");
		}
		else if (mc[i].op == "LT" || mc[i].op == "LE" || mc[i].op == "GT" || mc[i].op == "GE" || mc[i].op == "EQ" || mc[i].op == "NE") {
			i++;
			int loc1 = search_tab(mc[i].n1, islocal, def_loc);
			int loc2 = search_tab(mc[i].n2, islocal, def_loc);
			string num1 = loc1 != -1 && st[loc1].kind == ST_CONST ? to_string(st[loc1].val) :
				loc1 != -1 && st[loc1].kind != ST_CONST ? get_reg(mc[i].n1, def_loc) :
				loc1 == -1 && mc[i].n1[0] == '#' ? get_reg(mc[i].n1, def_loc) :
				mc[i].n1;
			string num2 = loc2 != -1 && st[loc2].kind == ST_CONST ? to_string(st[loc2].val) :
				loc2 != -1 && st[loc2].kind != ST_CONST ? get_reg(mc[i].n2, def_loc) :
				loc2 == -1 && mc[i].n2[0] == '#' ? get_reg(mc[i].n2, def_loc) :
				mc[i].n2;
			bool isn1Con = loc1 != -1 && st[loc1].kind == ST_CONST ? true :
				loc1 != -1 && st[loc1].kind != ST_CONST ? false :
				loc1 == -1 && mc[i].n1[0] == '#' ? false : true;
			bool isn2Con = loc2 != -1 && st[loc2].kind == ST_CONST ? true :
				loc2 != -1 && st[loc2].kind != ST_CONST ? false :
				loc2 == -1 && mc[i].n2[0] == '#' ? false : true;
			if (isn1Con&&isn2Con) {
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
			if (isn1Con || isn2Con) {
				gen_mips("li", "$v1", isn1Con ? num1 : num2);
				if (isn1Con)	num1 = "$v1";
				else num2 = "$v1";
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
			gen_mips("move", get_reg(mc[i].n1, def_loc), "$v0");
		}
		else if (mc[i].op == "GOTO") {
			gen_mips("j", mc[i].n1);
		}
		else if (mc[i].op == "ASSIGN") {
			gen_mips("move", get_reg(mc[i].res, def_loc), get_reg(mc[i].n1, def_loc));
		}
		else if (mc[i].op == "SELEM") {
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			gen_mips("subi", "$v1", islocal?"$fp":"$gp", to_string((islocal?-1:1)*st[arr_loc].addr));
			gen_mips("subi", "$v1", "$v1", get_reg(mc[i].n1, def_loc));
			gen_mips("sw", get_reg(mc[i].n2, def_loc), "$v1", "0");
		}
		else if (mc[i].op == "LELEM") {
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			gen_mips("addi", "$v1", islocal?"$fp":"$gp", to_string((islocal ? -1 : 1)*st[arr_loc].addr));
			gen_mips("add", "$v1", "$v1", get_reg(mc[i].n1, def_loc));
			gen_mips("lw", get_reg(mc[i].res, def_loc), "$v1", get_reg(mc[i].n2, def_loc));
		}
	}
}

void dump_mips() {
	string a, b, c, d;
	for (int i = 0;i < (int)mp.size();i++){
		cout << mp[i].op << " " << mp[i].res << " " << mp[i].n1 << " " << mp[i].n2<<"\n";
	}
}