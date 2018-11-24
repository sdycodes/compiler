#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "sign_table.h"
string get_reg(string name) {
	return "$1";
}

void gen_mips(string op, string res="", string n1="", string n2="") {
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
	for (int i = 0;i < mc.size();i++) {
		if (mc[i].op == "LABEL") {
			if (mc[i].n1[0] == '#') {
				gen_mips(mc[i].n1);
			}
			else {
				//what should a function do
			}
		}
		else if (mc[i].op == "ADD"||mc[i].op=="SUB") {
			int loc1 = search_tab(mc[i].n1, islocal);
			int loc2 = search_tab(mc[i].n2, islocal);
			if (loc1 != -1 && st[loc1].type == ST_CONST && loc2 != -1 && st[loc2].type == ST_CONST) {
				int res = mc[i].op == "ADD" ? st[loc1].val + st[loc2].val : st[loc1].val - st[loc2].val;
				gen_mips("li", get_reg(mc[i].res), to_string(res));
			}
			else if (loc1 != -1 && st[loc1].type == ST_CONST){
				if (mc[i].op == "SUB") {
					gen_mips("sub", get_reg(mc[i].res), "$0", get_reg(mc[i].n2));
					gen_mips("addi", get_reg(mc[i].res), get_reg(mc[i].res), to_string(st[loc1].val));
				}
				else {
					gen_mips("add", get_reg(mc[i].res), get_reg(mc[i].n1), to_string(st[loc1].val));
				}
			}
			else if (loc2 != -1 && st[loc2].type == ST_CONST) {
				gen_mips(mc[i].op == "ADD" ? "addi" : "subi", get_reg(mc[i].res), get_reg(mc[i].n1), to_string(st[loc2].val));
			}
			else {
				gen_mips(mc[i].op == "ADD" ? "add" : "sub", get_reg(mc[i].res), get_reg(mc[i].n1), get_reg(mc[i].n2));
			}
		}
		else if(mc[i].op=="MULT"||mc[i].op=="DIV"){
			int loc1 = search_tab(mc[i].n1, islocal);
			int loc2 = search_tab(mc[i].n2, islocal);
			if (loc1 != -1 && st[loc1].type == ST_CONST && loc2 != -1 && st[loc2].type == ST_CONST) {
				int res = mc[i].op == "MULT"?st[loc1].val*st[loc2].val: st[loc1].val/st[loc2].val;
				gen_mips("li", get_reg(mc[i].op), to_string(res));
			}
			else if (loc1 != -1 && st[loc1].type == ST_CONST) {
				gen_mips("li", "$v1", mc[i].n1);
				gen_mips(mc[i].op=="MULT"?"mult":"div", "$v1", get_reg(mc[i].n2));
				gen_mips("mflo", get_reg(mc[i].res));
			}
			else if (loc2 != -1 && st[loc2].type == ST_CONST) {
				gen_mips("li", "$v1", mc[i].n2);
				gen_mips(mc[i].op == "MULT" ? "mult" : "div", get_reg(mc[i].n1), "$v1");
				gen_mips("mflo", get_reg(mc[i].res));
			}
			else {
				gen_mips(mc[i].op == "MULT" ? "mult" : "div", get_reg(mc[i].n1), get_reg(mc[i].n2));
				gen_mips("mflo", get_reg(mc[i].res));
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
			
		}
		else if (mc[i].op == "RET") {}
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
			if (mc[i - 1].op == "LT"&&mc[i].op == "BNZ")
				gen_mips("bge", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "LT"&&mc[i].op == "BEZ")
				gen_mips("blt", get_reg(mc[i-1].n1), get_reg(mc[i-1].n2), mc[i].res);
			else if(mc[i-1].op=="LE"&&mc[i].op=="BNZ")
				gen_mips("bgt", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if(mc[i-1].op=="LE"&&mc[i].op=="BEZ")
				gen_mips("ble", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "GT"&&mc[i].op == "BNZ")
				gen_mips("ble", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "GT"&&mc[i].op == "BEZ")
				gen_mips("bgt", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "GE"&&mc[i].op == "BNZ")
				gen_mips("blt", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "GE"&&mc[i].op == "BEZ")
				gen_mips("bge", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "EQ"&&mc[i].op == "BNZ")
				gen_mips("bne", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "EQ"&&mc[i].op == "BEZ")
				gen_mips("beq", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "NE"&&mc[i].op == "BNZ")
				gen_mips("beq", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
			else if (mc[i - 1].op == "NE"&&mc[i].op == "BEZ")
				gen_mips("bne", get_reg(mc[i - 1].n1), get_reg(mc[i - 1].n2), mc[i].res);
		}
	}
}

void dump_mips() {
	for (int i = 0;i < mp.size();i++) {
	}
}