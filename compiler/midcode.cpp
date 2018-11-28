#include "pch.h"
#include "stdHeader.h"
#include "midcode.h"
#include "globalvar.h"
#include "sign_table.h"

void genmc(string op, string n1, string n2, string res) {
	mce tmp;
	tmp.op = op;
	tmp.n1 = n1;
	tmp.n2 = n2;
	tmp.res = res;
	mc.push_back(tmp);
	return;
}

string genlabel() {
	lno++;
	return "$LABEL"+to_string(lno-1);
}

string gent() {
	tno++;
	return "#"+to_string(tno-1);
}

void dumpmc() {
	int i=0, loc;
	bool islocal;
	//output global variables
	while (i < stp&&st[i].kind != ST_FUNC) {
		if (st[i].kind == ST_CONST) {
			if(st[i].type==ST_CHAR)
				printf("CONST CHAR %s '%c'\n", st[i].name.c_str(), (st[i].val%256));
			else
				printf("CONST INT %s %d\n", st[i].name.c_str(), st[i].val);
		}
		else if (st[i].kind == ST_VAR) {
			if (st[i].type == ST_CHAR)
				printf("CHAR %s\n", st[i].name.c_str());
			else
				printf("INT %s\n", st[i].name.c_str());
		}
		else if (st[i].kind == ST_ARR) {
			if (st[i].type == ST_CHAR)
				printf("CHAR %s[%d]\n", st[i].name.c_str(), st[i].val);
			else
				printf("INT %s[%d]\n", st[i].name.c_str(), st[i].val);
		}
		i++;
	}
	//output each midcode
	string mop;
	for (int j = 0;j < (int)mc.size();j++) {
		if (mc[j].op == "LABEL") {
			loc = search_tab(mc[j].n1, islocal);
			// if its a function output its parameters and const and variables
			if (loc != -1) {
				printf("FUNC %s\n", st[loc].name.c_str());
				int k = loc + 1;
				while (k < stp && st[k].kind != ST_FUNC) {
					if (st[k].kind == ST_PARA)
						printf("PARA %s\n", st[k].name.c_str());
					else if (st[k].kind == ST_CONST) {
						if (st[k].type == ST_INT)
							printf("CONST INT %s %d\n", st[k].name.c_str(), st[k].val);
						else
							printf("CONST CHAR %s '%c'\n", st[k].name.c_str(), st[k].val%256);
					}
					else if (st[k].kind == ST_VAR) {
						if (st[k].type == ST_INT)
							printf("INT %s\n", st[k].name.c_str());
						else
							printf("CHAR %s\n", st[k].name.c_str());
					}
					else if (st[i].kind == ST_ARR) {
						if (st[i].type == ST_CHAR)
							printf("CHAR %s[%d]\n", st[i].name.c_str(), st[i].val);
						else
							printf("INT %s[%d]\n", st[i].name.c_str(), st[i].val);
					}
					k++;
				}
			}
			else {
				printf("SET %s\n", mc[j].n1.c_str());
			}
		}
		else if (mc[j].op == "ADD" || mc[j].op == "SUB" || mc[j].op == "MULT" || mc[j].op == "DIV" || \
			mc[j].op == "LT" || mc[j].op == "LE" || mc[j].op == "EQ" || mc[j].op == "NE" || mc[j].op == "GT" || mc[j].op == "GE") {
			mop = mc[j].op == "ADD" ? "+" :
				mc[j].op == "SUB" ? "-" :
				mc[j].op == "MULT" ? "*" :
				mc[j].op == "DIV" ? "/" :
				mc[j].op == "LT" ? "<" :
				mc[j].op == "LE" ? "<=" :
				mc[j].op == "EQ" ? "==" :
				mc[j].op == "NE" ? "!=" :
				mc[j].op == "GT" ? ">" : ">=";
			printf("%s = %s %s %s\n", mc[j].res.c_str(), mc[j].n1.c_str(), mop.c_str(), mc[j].n2.c_str());
		}
		else if (mc[j].op == "CALL" || mc[j].op == "PUSH" || mc[j].op == "GOTO" || mc[j].op == "RET" || \
			mc[j].op == "INC" ||mc[j].op =="INV"|| mc[j].op == "OUTC" || mc[j].op == "OUTV")
			printf("%s %s\n", mc[j].op.c_str(), mc[j].n1.c_str());
		else if (mc[j].op == "ASSIGN")
			printf("%s = %s\n", mc[j].res.c_str(), mc[j].n1.c_str());
		else if (mc[j].op == "SELEM")
			printf("%s[%s] = %s\n", mc[j].res.c_str(), mc[j].n1.c_str(), mc[j].n2.c_str());
		else if (mc[j].op == "LELEM")
			printf("%s = %s[%s]\n", mc[j].res.c_str(), mc[j].n1.c_str(), mc[j].n2.c_str());
		else if (mc[j].op == "BNZ"||mc[j].op=="BEZ")
			printf("%s %s %s\n", mc[j].op.c_str(), mc[j].n1.c_str(), mc[j].res.c_str());
		else if (mc[j].op == "OUTS")
			printf("OUTS %s\n", &strtab[atoi(mc[j].n1.c_str())]);
		else if (mc[j].op=="EXIT")
			printf("EXIT");
		else {
			printf("!!!!!!!!!%s\n", mc[j].op.c_str());
		}
	}
}