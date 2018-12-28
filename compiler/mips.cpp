#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
#include "mips.h"
#include "base_block.h"
#include "reg_distribution.h"

#define no2name(x) (x)>=16&&(x)<=23?"$s"+to_string(x-16):	\
					(x)<16&&(x)>=8?"$t"+to_string(x-8): \
					(x)==24||(x)==25?"$t"+to_string(x-16): \
					(x)>=4&&(x)<=7?"$a"+to_string(x-4):"!!!"

#define isCon(num) (isdigit(num[0])||num[0]=='-'||num[0]=='+')
#define REG_NUM 8
int tstk[REG_NUM] = { 8,9,10,11,12,13,14,15 };
map<int, string> t_alloc;

void resetMidvar(string midvarname) {
	map<int, string>::iterator it = t_alloc.begin();
	int cnt = 0;
	while (it != t_alloc.end()) {
		if (it->second == midvarname) {
			it->second = "";
		}
		int k;
		for (k = 0;k < REG_NUM;k++)
			if (tstk[k] == it->first)	break;
		//移动
		for (;k < REG_NUM - 1;k++)
			tstk[k] = tstk[k + 1];
		tstk[k] = it->first;
		it++;
	}
}
bool isOBNL(string funcname) {
	int start = 0;
	while (start < (int)mc.size()) {
		if (mc[start].op == "LABEL"&&mc[start].n1 == funcname)
			break;
		start++;
	}
	int i = start+1;
	while (i < (int)mc.size() && mc[i].op != "LABEL") {
		if (mc[i].op == "CALL")
			return false;
		i++;
	}
	if (i < (int)mc.size() && mc[i].op == "LABEL"&&mc[i].n1[0] == '$')
		return false;
	return true;
}
string get_reg(string name, bool assign, int def_loc) {
	//#RET 直接返回v0
	if (name == "#RET")
		return "$v0";
	if (isOBNL(st[def_loc].name=="main"?"main":"FUNC_"+st[def_loc].name)) {
		if (def_loc+1<stp&&st[def_loc + 1].name == name && st[def_loc + 1].kind == ST_PARA)
			return "$a1";
		if (def_loc+2<stp&&st[def_loc + 2].name == name && st[def_loc + 2].kind == ST_PARA)
			return "$a2";
		if (def_loc + 3 < stp&&st[def_loc + 3].name == name && st[def_loc + 3].kind == ST_PARA)
			return "$a3";
	}
	bool islocal;
	string reg;
	int loc = search_tab(name, islocal, def_loc);
	static int data_buffer = 6;
	//static int data_buffer = 0;
	//一个全局变量 或者是一个未分配寄存器的跨基本块局部变量
	if (loc != -1 && (!islocal|| name2reg[loc] == -1)) {
		if (!islocal&&name2reg[loc] > 0)	return no2name(name2reg[loc]);
		reg = "$s" + to_string(data_buffer);
		//reg = data_buffer == 0 ? "$a3" : "$v1";
		if (!islocal)
			gen_mips("lw", reg, "$gp", to_string(st[loc].addr));	//从内存中读取
		else
			gen_mips("lw", reg, "$fp", to_string(-st[loc].addr));
		data_buffer = data_buffer == 6 ? 6 : 7;
		//data_buffer = data_buffer == 0 ? 1 : 0;
		return reg;
	}
	//如果是一个分配了s寄存器的跨基本块局部变量
	if (loc != -1 && name2reg[loc]!=0&&name2reg[loc]!=-1) {
		return no2name(name2reg[loc]);
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
			//把新分配的load出来 如果不是被赋值的话
			if (!assign) {
				if (loc == -1)
					gen_mips("lw", no2name(r), "$sp", to_string((stoi(name.substr(1)) << 2) + 4));
				else
					gen_mips("lw", no2name(r), "$fp", to_string(-st[loc].addr));
			}
			return no2name(r);
		}
	}
	else {
		return "!!!!!!!"+name;
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
	def_loc = search_tab("main", islocal, -2);
	gen_mips("li", "$fp", "0x7fffeffc");
	gen_mips("subiu", "$sp", "$fp", to_string(st[def_loc].size));
	int block_no = 1;
	int block_end = blocks[block_no].end;
	for (int i = 0;i < (int)mc.size();i++) {
		//标签
		if (mc[i].op == "LABEL") {	
			gen_mips(mc[i].n1+':');
			if (mc[i].n1[0] != '$') {	//是一个函数标签
				def_loc = search_tab(mc[i].n1 == "main" ? "main" : mc[i].n1.substr(5), islocal, -2);
				int k = def_loc + 1;
				int get_cnt = 0;
				while (k < stp&&st[k].kind == ST_PARA) {
					if (name2reg[k] > 0 && get_cnt < CHOOSEA) {
						gen_mips("move", no2name(name2reg[k]), "$a"+to_string(get_cnt));
						get_cnt++;
					}
					else if (name2reg[k] > 0)
						gen_mips("lw", no2name(name2reg[k]), "$fp", to_string(-st[k].addr));
					k++;
				}
			}
		}
		else if (mc[i].op == "ADD"||mc[i].op=="SUB") {
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, false, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, false, def_loc);
			string numres = mc[i].n1==mc[i].res?num1:
							mc[i].n2==mc[i].res?num2:get_reg(mc[i].res, true, def_loc);
			//都是常量 li
			if (isCon(mc[i].n1)&&isCon(mc[i].n2)) {
				int res = mc[i].op == "ADD" ? stoi(num1) + stoi(num2) : stoi(num1) - stoi(num2);
				gen_mips("li", numres, to_string(res));
			}
			//一个是常量 一个是变量 addi/subi
			else if (isCon(mc[i].n2)) {
				string op = mc[i].op == "ADD" ? "addiu" : "subiu";
				gen_mips(op, numres, num1, num2);
			}
			else if (isCon(mc[i].n1)) {
				if (mc[i].op == "SUB") {
					gen_mips("subu", numres, "$0", num2);
					gen_mips("addiu", numres, numres, num1);
				}
				else {
					gen_mips("addiu", numres, num2, num1);
				}
			}
			//都是变量
			else {
				string op = mc[i].op == "ADD" ? "addu" : "subu";
				gen_mips(op, numres, num1, num2);
			}
			//这里是对于全局变量的特殊操作 
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && (!islocal || name2reg[loc] == -1)) {
				if (!islocal) {
					if(!(name2reg[loc]>0))
						gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
				}
				else
					gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			}
		}
		else if(mc[i].op=="MULT"||mc[i].op=="DIV"){
			//都是常量 li
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, false, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, false, def_loc);
			string numres = mc[i].n1 == mc[i].res ? num1 :
							mc[i].n2 == mc[i].res ? num2 : get_reg(mc[i].res, true, def_loc);
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
				string op = mc[i].op == "MULT" ? "mul" : "div";
				gen_mips(op, numres, num1, num2);
			}
			//这里是对于全局变量的特殊操作 
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && (!islocal || name2reg[loc] == -1)) {
				if (!islocal) {
					if (!(name2reg[loc] > 0))
						gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
				}
				else
					gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
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
			int loc = search_tab(mc[i].n1=="main"?"main":mc[i].n1.substr(5), islocal, -2);
			int context_offset = st[loc].addr;	//保存现场的起始位置
			//传参数
			int para_cnt = 0;
			if (isOBNL(mc[i].n1)) {
				for (int j = 0;j < (int)paras.size();j++) {
					if (isCon(paras[j])) {
						if (para_cnt < CHOOSEA-1) {
							gen_mips("li", "$a"+to_string(para_cnt+1), paras[j]);
							para_cnt++;
						}
						else {
							if(paras[j]!="0") gen_mips("li", "$t8", paras[j]);
							gen_mips("sw", paras[j]=="0"?"$0":"$t8", "$sp", to_string(-j * 4));
						}
					}
					else {
						if (para_cnt < CHOOSEA-1) {
							gen_mips("move", "$a"+to_string(para_cnt+1), get_reg(paras[j], false, def_loc));
							para_cnt++;
						}
						else
							gen_mips("sw", get_reg(paras[j], false, def_loc), "$sp", to_string(-j * 4));
					}
				}
			}
			else {
				for (int j = 0;j < (int)paras.size();j++) {
					if (isCon(paras[j])) {
						if (name2reg[loc + j + 1] > 0 && para_cnt < CHOOSEA) {
							gen_mips("li", "$a" + to_string(para_cnt), paras[j]);
							para_cnt++;
						}
						else {
							if(paras[j]!="0") gen_mips("li", "$t8", paras[j]);
							gen_mips("sw", paras[j]=="0"?"$0":"$t8", "$sp", to_string(-j * 4));
						}
					}
					else if (name2reg[loc + j + 1] > 0 && para_cnt < CHOOSEA) {
						gen_mips("move", "$a" + to_string(para_cnt), get_reg(paras[j], false, def_loc));
						para_cnt++;
					}
					else
						gen_mips("sw", get_reg(paras[j], false, def_loc), "$sp", to_string(-j * 4));
				}
			}
			//函数传参后 所有的参数中间变量都应清除 这也意味着公共子表达式的中间变量不能跨函数
			for (int j = 0;j < (int)paras.size();j++) {
				if (paras[j][0] == '#') 
					resetMidvar(paras[j]);
			}
			paras.clear();
			//个性化保存现场
			for (int j = 8;j < 16;j++) 
				if (t_alloc.find(j)!=t_alloc.end()&&t_alloc[j]!="")
					gen_mips("sw", "$" + to_string(j), "$sp", to_string(-context_offset - j * 4));
			int j = def_loc + 1;
			bool should[8];
			memset(should, 0, sizeof(should));
			while (j < stp&&st[j].kind !=ST_FUNC) {
				if (name2reg[j] > 0)
					should[name2reg[j]-16] = true;
				j++;
			}
			for(int l=0;l<6;l++)
				if(should[l])
					gen_mips("sw", no2name(l+16), "$sp", to_string(-context_offset - (l + 16) * 4));
			gen_mips("sw", "$31", "$sp", to_string(-context_offset - 124));
			//分配函数运行栈
			gen_mips("move", "$fp", "$sp");
			gen_mips("subiu", "$sp", "$fp", to_string(st[loc].size));
			//jump to function
			gen_mips("jal", mc[i].n1);
			//个性化恢复现场
			for (int j = 8;j < 16;j++)
				if (t_alloc.find(j) != t_alloc.end() && t_alloc[j] != "")
					gen_mips("lw", "$" + to_string(j), "$fp", to_string(-context_offset - j * 4));
			/*
			j = def_loc + 1;
			while (j < stp&&st[j].kind != ST_FUNC) {
				if (name2reg[j]>0)
					gen_mips("lw", no2name(name2reg[j]), "$fp", to_string(-context_offset - name2reg[j] * 4));
				j++;
			}*/
			for (int l = 0;l < 6;l++)
				if (should[l])
					gen_mips("lw", no2name(l + 16), "$fp", to_string(-context_offset - (l + 16) * 4));
			gen_mips("lw", "$" + to_string(31), "$fp", to_string(-context_offset - 31 * 4));
			gen_mips("move", "$sp", "$fp");
			gen_mips("addiu", "$fp", "$sp", to_string(st[def_loc].size));
			
		}
		else if (mc[i].op == "RET") {
			//put the result on $v0
			if (isCon(mc[i].n1))
				gen_mips("li", "$v0", mc[i].n1);
			else if (mc[i].n1 != "#")
				gen_mips("move", "$v0", get_reg(mc[i].n1, false, def_loc));
			gen_mips("jr", "$ra");
		}
		else if (mc[i].op == "OUTS"|| mc[i].op == "OUTV"|| mc[i].op == "OUTC") {
			gen_mips("li", "$v0", mc[i].op == "OUTS" ? "4" : mc[i].op == "OUTV" ? "1" : "11");
			if (mc[i].op == "OUTS") {
				gen_mips("li", "$a0", to_string(0x10010000 + stoi(mc[i].n1)));
			}
			else {
				if (isdigit(mc[i].n1[0]) || mc[i].n1[0] == '-' || mc[i].n1[0] == '+') {
					gen_mips("li", "$a0", mc[i].n1);
				}
				else {
					gen_mips("move", "$a0", get_reg(mc[i].n1, false, def_loc));
				}
			}
			gen_mips("syscall");
		}
		else if(mc[i].op=="EXIT"){
			gen_mips("li", "$v0", "10");
			gen_mips("syscall");
		}
		else if (mc[i].op == "LT" || mc[i].op == "LE" || mc[i].op == "GT" || mc[i].op == "GE" || mc[i].op == "EQ" || mc[i].op == "NE") {
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, false, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, false, def_loc);	
			i++;
			bool reverse = false;
			string tmp;
			if (isCon(mc[i-1].n1)&&isCon(mc[i-1].n2)) {
				if ((mc[i - 1].op == "LT"&&stoi(num1) < stoi(num2))||
					(mc[i - 1].op == "LE"&&stoi(num1) <= stoi(num2))||
					(mc[i - 1].op == "GT"&&stoi(num1)>stoi(num2))||
					(mc[i - 1].op == "GE"&&stoi(num1)>=stoi(num2))||
					(mc[i - 1].op == "EQ"&&stoi(num1)==stoi(num2))||
					(mc[i - 1].op == "NE"&&stoi(num1)!=stoi(num2)))
					if (mc[i].op == "BEZ") {
						gen_mips("j", mc[i].res);
					}
				else if ((mc[i - 1].op == "LT"&&stoi(num1) >= stoi(num2)) ||
					(mc[i - 1].op == "LE"&&stoi(num1) > stoi(num2)) ||
					(mc[i - 1].op == "GT"&&stoi(num1) <= stoi(num2)) ||
					(mc[i - 1].op == "GE"&&stoi(num1) < stoi(num2)) ||
					(mc[i - 1].op == "EQ"&&stoi(num1) != stoi(num2)) ||
					(mc[i - 1].op == "NE"&&stoi(num1) == stoi(num2)))
					if (mc[i].op == "BNZ") {
						gen_mips("j", mc[i].res);
					}
			}
			else if (isCon(mc[i-1].n1)) {
				tmp = num2;
				num2 = num1;
				num1 = tmp;
				reverse = true;
			}
			if (num2 == "0") num2 = "$0";
			if (mc[i - 1].op == "LT"&&mc[i].op == "BNZ") 
				gen_mips(reverse?"blt":"bge", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "LT"&&mc[i].op == "BEZ")
				gen_mips(reverse?"bge":"blt", mc[i].res, num1, num2);
			else if(mc[i-1].op=="LE"&&mc[i].op=="BNZ")
				gen_mips(reverse?"ble":"bgt", mc[i].res, num1, num2);
			else if(mc[i-1].op=="LE"&&mc[i].op=="BEZ")
				gen_mips(reverse?"bgt":"ble", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GT"&&mc[i].op == "BNZ")
				gen_mips(reverse?"bgt":"ble", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GT"&&mc[i].op == "BEZ")
				gen_mips(reverse?"ble":"bgt", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GE"&&mc[i].op == "BNZ")
				gen_mips(reverse?"bge":"blt", mc[i].res, num1, num2);
			else if (mc[i - 1].op == "GE"&&mc[i].op == "BEZ")
				gen_mips(reverse?"blt":"bge", mc[i].res, num1, num2);
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
			string numres = get_reg(mc[i].n1, true, def_loc);
			gen_mips("move", numres, "$v0");
			//这里是对于全局变量的特殊操作 
			int loc = search_tab(mc[i].n1, islocal, def_loc);
			if (loc != -1 && (!islocal || name2reg[loc] == -1)) {
				if (!islocal) {
					if (!(name2reg[loc] > 0))
						gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
				}
				else
					gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			}
		}
		else if (mc[i].op == "GOTO") {
			gen_mips("j", mc[i].n1);
		}
		else if (mc[i].op == "ASSIGN") {
			string numres = get_reg(mc[i].res, true, def_loc);
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, false, def_loc);
			if (isCon(mc[i].n1))
				gen_mips("li", numres, mc[i].n1);
			else
				gen_mips("move", numres, num1);
			//对全局变量的特殊操作
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && (!islocal || name2reg[loc] == -1)) {
				if (!islocal) {
					if (!(name2reg[loc] > 0))
						gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
				}
				else
					gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			}
			//如果赋值的是一个非中间变量 清除掉所有中间变量
			if (mc[i].res[0] != '#') {
				map<int, string>::iterator it = t_alloc.begin();
				while (it != t_alloc.end()) {
					if (it->second[0] == '#') 
						resetMidvar(it->second);
					it++;
				}
			}
		}
		else if (mc[i].op == "SELEM") { 
			string num1 = isCon(mc[i].n1) ? mc[i].n1 : get_reg(mc[i].n1, false, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, false, def_loc);
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			if (isCon(mc[i].n1)) {
				int offset = stoi(num1) * 4 + st[arr_loc].addr;
				if (isCon(mc[i].n2)) 
					gen_mips("li", "$t9", mc[i].n2);
				if (islocal)
					gen_mips("sw", isCon(mc[i].n2) ? "$t9" : num2, "$fp", to_string(-offset));
				else
					gen_mips("sw", isCon(mc[i].n2) ? "$t9" : num2, "$gp", to_string(offset));
			}
			else {
				gen_mips("sll", "$t8", num1, "2");
				gen_mips("addiu", "$t8", "$t8", to_string(st[arr_loc].addr));
				if (islocal)
					gen_mips("subu", "$t8", "$fp", "$t8");
				else
					gen_mips("addu", "$t8", "$gp", "$t8");
				if (isCon(mc[i].n2)&&mc[i].n2!="0")
					gen_mips("li", "$t9", mc[i].n2);
				gen_mips("sw", isCon(mc[i].n2)?(mc[i].n2=="0"?"$0":"$t9"): num2, "$t8", "0");
			}
		}
		else if (mc[i].op == "LELEM") {
			int arr_loc = search_tab(mc[i].n1, islocal, def_loc);
			string num2 = isCon(mc[i].n2) ? mc[i].n2 : get_reg(mc[i].n2, false, def_loc);
			string numres = get_reg(mc[i].res, true, def_loc);
			if (isCon(mc[i].n2)) {
				int offset = stoi(mc[i].n2) * 4 + st[arr_loc].addr;
				if (islocal)
					gen_mips("lw", numres, "$fp", to_string(-offset));
				else
					gen_mips("lw", numres, "$gp", to_string(offset));
			}
			else {
				gen_mips("sll", "$t8", num2, "2");
				gen_mips("addiu", "$t8", "$t8", to_string(st[arr_loc].addr));
				if (islocal)
					gen_mips("subu", "$t8", "$fp", "$t8");
				else
					gen_mips("addu", "$t8", "$gp", "$t8");
				gen_mips("lw", numres, "$t8", "0");
			}
			//对全局变量的特殊操作
			int loc = search_tab(mc[i].res, islocal, def_loc);
			if (loc != -1 && (!islocal || name2reg[loc] == -1)) {
				if (!islocal) {
					if (!(name2reg[loc] > 0))
						gen_mips("sw", numres, "$gp", to_string(st[loc].addr));
				}
				else
					gen_mips("sw", numres, "$fp", to_string(-st[loc].addr));
			}
		}
		/*
		else if (mc[i].op == "INLINE") {
			for (int j = 0;j < (int)paras.size();j++) {
				if (isCon(paras[j]))	gen_mips("li", "$a"+to_string(j+1), paras[j]);
				else gen_mips("move", "$a" + to_string(j + 1), get_reg(paras[j], false, def_loc));
			}
			paras.clear();
		}
		*/
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
		else if(mp[i].op!="nop")
			printf("%s %s %s %s\n", mp[i].op.c_str(), mp[i].res.c_str(), mp[i].n1.c_str(), mp[i].n2.c_str());
	}
}