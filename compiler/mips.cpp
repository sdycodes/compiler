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
	int def_loc, def_context_offset;
	for (int i = 0;i < (int)mc.size();i++) {
		if (mc[i].op == "LABEL") {
			if (mc[i].n1[0] == '$') {
				gen_mips(mc[i].n1);
			}
			else {
				def_loc = search_tab(mc[i].n1, islocal);
				def_context_offset = st[def_loc].size - 31*4;
				for (int j = 2;j <=28;j++)
					gen_mips("sw", "$"+to_string(j), "$sp", to_string(def_context_offset+(j-1)*4));
				gen_mips("sw", "$" + to_string(30), "$sp", to_string(def_context_offset + (30 - 1) * 4));
				gen_mips("sw", "$" + to_string(31), "$sp", to_string(def_context_offset + (31 - 1) * 4));
			}
		}
		else if (mc[i].op == "ADD"||mc[i].op=="SUB") {
			int loc1 = search_tab(mc[i].n1, islocal, def_loc);
			int loc2 = search_tab(mc[i].n2, islocal, def_loc);
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
			int loc1 = search_tab(mc[i].n1, islocal, def_loc);
			int loc2 = search_tab(mc[i].n2, islocal, def_loc);
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
			int loc = search_tab(mc[i].n1, islocal);
			//alloc a stack
			gen_mips("subi", "$sp", "$sp", to_string(st[loc].size));
			//pass parameters
			for (int j = 0;j < paras.size();j++) {
				gen_mips("sw", get_reg(paras[j]), "$sp", to_string(j*4));
			}
			paras.clear();
			//jump to function
			gen_mips("jal", mc[i].n1);
			//release the stack
			gen_mips("addi", "$sp", "$sp", to_string(st[loc].size));
		}
		else if (mc[i].op == "RET") {
			//put the result on $v0
			if (mc[i].n1 != "#")
				gen_mips("move", "$v0", get_reg(mc[i].n1));
			//recover context def_loc and def_context still can be used
			for (int j = 2;j <= 28;j++)
				gen_mips("lw", "$" + to_string(j), "$sp", to_string(def_context_offset + (j - 1) * 4));
			gen_mips("lw", "$" + to_string(30), "$sp", to_string(def_context_offset + (30 - 1) * 4));
			gen_mips("lw", "$" + to_string(31), "$sp", to_string(def_context_offset + (31 - 1) * 4));
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
		else if (mc[i].op == "INC"||mc[i].op=="INV") {
			gen_mips("li", "$v0", mc[i].op=="INV"?"5":"12");
			gen_mips("move", get_reg(mc[i].n1), "$v0");
		}
		else if (mc[i].op == "GOTO") {
			gen_mips("j", mc[i].n1);
		}
		else if (mc[i].op == "ASSIGN") {
			gen_mips("move", get_reg(mc[i].res), get_reg(mc[i].n1));
		}
		else if (mc[i].op == "SELEM") {
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			gen_mips("addi", "$k0", "$sp", to_string(st[arr_loc].addr));
			gen_mips("add", "$k0", "$k0", get_reg(mc[i].n1));
			gen_mips("sw", get_reg(mc[i].n2), "$k0", "0");
		}
		else if (mc[i].op == "LELEM") {
			int arr_loc = search_tab(mc[i].res, islocal, def_loc);
			gen_mips("addi", "$k0", "$sp", to_string(st[arr_loc].addr));
			gen_mips("add", "$k0", "$k0", get_reg(mc[i].n1));
			gen_mips("lw", get_reg(mc[i].res), "$k0", get_reg(mc[i].n2));
		}
	}
}

void dump_mips() {
	for (int i = 0;i < (int)mp.size();i++) {
	}
}