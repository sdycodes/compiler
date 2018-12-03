#include "pch.h"
#include "globalvar.h"
#include "stdHeader.h"
#include "lexical.h"
#include "error.h"
#include "sign_table.h"
#include "grammar.h"
#include "midcode.h"
#define else_ERR(x,y) else{errmsg(x);}
#define else_ERR_break(x,y) else{errmsg(x);break;}
#define DUMP_GREAMMAR false
string factor(bool &onlyChar) {
	int co = 1;
	//char tc;
	string t, rec;
	switch (type) {
	case PLUS:
	case MINUS:
		co = type == PLUS ? 1 : -1;
		onlyChar = false;
		nextsym(type, val, name);
		if (type == NUM) {
			t = to_string(co*val);
			nextsym(type, val, name);
		}
		else_ERR("expect a integer", 1)
		break;
	case NUM:
		onlyChar = false;
		t = to_string(val);
		nextsym(type, val, name);
		break;
	case CHAR:
		t = to_string(val % 256);
		nextsym(type, val, name);
		break;
	case LPAR:
		onlyChar = false;
		nextsym(type, val, name);
		rec = expr(onlyChar);
		if (type == RPAR) {
			t = rec;
			nextsym(type, val, name);
		}
		else_ERR("expect a right parent", 1)
		break;
	case IDEN:
		int ident_idx;
		bool islocal, onlyChar2;
		ident_idx = search_tab(name, islocal);
		if (ident_idx != -1) {
			onlyChar = st[ident_idx].type == ST_CHAR ? onlyChar : false;
			if (st[ident_idx].kind == ST_FUNC) {
				nextsym(type, val, name);
				funcCall(ident_idx);
				t = gent();
				genmc("ASSIGN", "#RET", "0", t);
			}
			else if (st[ident_idx].kind == ST_ARR) {
				nextsym(type, val, name);
				if (type == LBRACKET) {
					nextsym(type, val, name);
					if (type == NUM && val>=st[ident_idx].val) {
						errmsg("index out of length", 1);
					}
					else {
						rec = expr(onlyChar2);
					}
					if (type == RBRACKET) {
						if (st[ident_idx].type != ST_VOID) {
							nextsym(type, val, name);
							t = gent();
							genmc("LELEM", st[ident_idx].name, rec, t);
						}
						else_ERR("type not match", 1)
					}
					else_ERR("expect a right bracket", 1)
				}
				else_ERR("expect a left bracket", 1)
			}
			else if (st[ident_idx].kind == ST_VAR||st[ident_idx].kind==ST_PARA) {
				t = st[ident_idx].name;
				nextsym(type, val, name);
			}
			else if (st[ident_idx].kind == ST_CONST) {
				t = to_string(st[ident_idx].val);
				nextsym(type, val, name);
			}
			else_ERR("unexpected beginning of factor", 1)
		}
		else_ERR("undefined identifier", 1)
		break;
	default:
		errmsg("unexpected begining of factor", 1);
	}
	return t;
}
string term(bool &onlyChar) {
	string rec1, rec2, t;
	string op;
	rec1 = factor(onlyChar);
	t = rec1;
	while (type==TIMES||type==DIV) {
		onlyChar = false;
		op = type == TIMES ? "MULT" : "DIV";
		nextsym(type, val, name);
		t = gent();
		rec2 = factor(onlyChar);
		genmc(op, rec1, rec2, t);
		rec1 = t;
	}
	return t;
}
string expr(bool &onlyChar) {
	string t, rec1 = "0", rec2, op;
	if (type == PLUS || type == MINUS) {
		onlyChar = false;
		do {
			op = type == PLUS ? "ADD" : "SUB";
			nextsym(type, val, name);
			rec2 = term(onlyChar);
			t = gent();
			genmc(op, rec1, rec2, t);
			rec1 = t;
		} while (type == PLUS || type == MINUS);
	}
	else {
		rec1 = term(onlyChar);
		t = rec1;
		while (type == PLUS || type == MINUS) {
			op = type == PLUS ? "ADD" : "SUB";
			onlyChar = false;
			nextsym(type, val, name);
			rec2 = term(onlyChar);
			t = gent();
			genmc(op, rec1, rec2, t);
			rec1 = t;
		}
	}
	return t;
}
void funcCall(int ident_idx) {
	string t, rec;
	vector<string> paras;
	int para = ident_idx + 1;
	if (type == LPAR) {
		nextsym(type, val, name);
		if (type == RPAR && st[ident_idx].val==0) {
			genmc("CALL", "FUNC_"+st[ident_idx].name, "0", "0");
			nextsym(type, val, name);
		}
		else if (type == RPAR && st[ident_idx].val != 0) {
			errmsg("too few arguments", 1);
		}
		else {
			bool onlyChar;
			while (para < stp && st[para].kind == ST_PARA) {
				onlyChar = true;
				rec = expr(onlyChar);
				paras.push_back(rec);
				if (true||(onlyChar&&st[para].type == ST_CHAR) || (!onlyChar&&st[para].type == ST_INT)) {
					if (type == COMMA || type == RPAR) {
						if (type == RPAR) {
							nextsym(type, val, name);
							break;
						}
						nextsym(type, val, name);
						para++;
					}
					else_ERR_break("unexpected arguments", 1)
				}
				else_ERR_break("type not match", 1)
			}
			//judge number can match
			if ((int)paras.size() > st[ident_idx].val) {
				errmsg("too many paras", 0);
			}
			else if ((int)paras.size() < st[ident_idx].val) {
				errmsg("too few paras", 0);
			}
			else {
				for (int i = 0;i < (int)paras.size();i++) {
					genmc("PUSH", paras[i], "0", "0");
				}
				genmc("CALL", "FUNC_"+st[ident_idx].name, "0", "0");
			}
		}
	}
	else_ERR("expect a left parent", 51);
}
void assignstmt(int ident_idx) {
	string rec_idx, rec_val;
	bool onlyChar = true;
	if (type == BECOMES) {
		if (st[ident_idx].kind == ST_VAR||st[ident_idx].kind==ST_PARA) {
			nextsym(type, val, name);
			rec_val = expr(onlyChar);
			if (true||onlyChar&&st[ident_idx].type == ST_CHAR || !onlyChar&&st[ident_idx].type != ST_CHAR) {
				genmc("ASSIGN", rec_val, "0", st[ident_idx].name);
			}
			else_ERR("type not match", 2)
		}
		else_ERR("type not match", 2)
	}
	else if (type == LBRACKET) {
		if (st[ident_idx].kind == ST_ARR) {
			nextsym(type, val, name);
			rec_idx = expr(onlyChar);
			if (onlyChar)	errmsg("illegal index of array", 0);
			if (type == RBRACKET) {
				nextsym(type, val, name);
				if (type == BECOMES) {
					nextsym(type, val, name);
					onlyChar = true;
					rec_val = expr(onlyChar);
					if (true||(onlyChar&&st[ident_idx].type == ST_CHAR) || (!onlyChar&&st[ident_idx].type != ST_CHAR)) {
						genmc("SELEM", rec_idx, rec_val, st[ident_idx].name);
					}
					else_ERR("type not match", 2);
				}
				else_ERR("expect a becomes", 2)
			}
			else_ERR("expect a right bracket", 2)
		}
		else_ERR("type not match", 2)
	}
	else_ERR("expect a becomes or a left bracket", 2)
}
void forstmt() {
	string ident_name;
	string label_judge, label_end, rec_init, rec_cond, op, rec_step;
	string name2;
	bool islocal, onlyChar = true;
	int loc;

	if (type == LPAR) {
		nextsym(type, val, name);
		if (type == IDEN) {
			ident_name = name;
			loc = search_tab(ident_name, islocal);
			if (loc != -1) {
				nextsym(type, val, name);
				if (type == BECOMES) {
					nextsym(type, val, name);
					rec_init = expr(onlyChar);
					genmc("ASSIGN", rec_init, "0", ident_name);
					label_judge = genlabel();
					genmc("LABEL", label_judge, "0", "0");
					if (type == SEMICOLON) {
						nextsym(type, val, name);
						rec_cond = conditions();
						label_end = genlabel();
						genmc("BNZ", rec_cond, "0", label_end);
						if (type == SEMICOLON) {
							nextsym(type, val, name);
							if (type == IDEN) {
								name2 = name;
								nextsym(type, val, name);
								if (type == BECOMES) {
									nextsym(type, val, name);
									if (type == IDEN) {
										nextsym(type, val, name);
										if (type == PLUS || type == MINUS) {
											op = type == PLUS ? "ADD" : "SUB";
											nextsym(type, val, name);
											if (type == NUM) {
												rec_step = to_string(val);
												nextsym(type, val, name);
												if (type == RPAR) {
													nextsym(type, val, name);
													state();
													genmc(op, name2, rec_step, name2);
													genmc("GOTO", label_judge, "0", "0");
													genmc("LABEL", label_end, "0", "0");
													if (DUMP_GREAMMAR) printf("%d %d this is a for statement\n", lc, cc);
												}
												else_ERR("expect a right parent", 31)
											}
											else_ERR("expect a unsigned integer", 31)
										}
										else_ERR("expect a plus or minus", 31)
									}
									else_ERR("expect an identifier", 31)
								}
								else_ERR("expect a becomes", 31)
							}
							else_ERR("expect a identifier", 31)
						}
						else_ERR("expect a semicolon", 31)
					}
					else_ERR("expect a semicolon", 31)
				}
				else_ERR("expect a becomes", 31)
			}
			else_ERR("undefined identifier", 31)
		}
		else_ERR("expect an identifier", 31)
	}
	else_ERR("expect a left parent", 31)
}
void scanfstmt() {
	bool islocal;
	int loc;
	if (type == LPAR) {
		nextsym(type, val, name);
		if (type == IDEN) {
			while(type==IDEN){
				loc = search_tab(name, islocal);
				if (loc != -1) {
					if (st[loc].kind != ST_FUNC && st[loc].kind != ST_ARR) {
						genmc(st[loc].type==ST_INT?"INV":"INC", st[loc].name, "0", "0");
						nextsym(type, val, name);
						if (type == COMMA) {
							nextsym(type, val, name);
						}
						else if (type == RPAR) {
							nextsym(type, val, name);
							break;
						}
						else_ERR_break("expect a comma or right parent", 2)
					}
					else_ERR_break("illegal variable to get input", 2)
				}
				else_ERR_break("undefined identifier", 2)
			}
		}
		else_ERR("expect an identifier", 2)
	}
	else_ERR("expect left parent", 2)
}
void printfstmt() {
	bool onlyChar = true;
	string rec_val;
	if (type == LPAR) {
		nextsym(type, val, name);
		if (type == STR) {
			genmc("OUTS", to_string(val), "0", "0");
			nextsym(type, val, name);
			if (type == COMMA) {
				nextsym(type, val, name);
				rec_val = expr(onlyChar);
				genmc(onlyChar?"OUTC":"OUTV",rec_val, "0", "0");
				if(type==RPAR){
					//genmc("OUTC", "32", "0", "0");
					nextsym(type, val, name);
				}
				else_ERR("expect a right parent", 2)
			}
			else if (type == RPAR) {
				//genmc("OUTC", "32", "0", "0");
				nextsym(type, val, name);
			}
			else_ERR("expect a comma or a right parent", 2)
		}
		else {
			rec_val = expr(onlyChar);
			genmc(onlyChar ? "OUTC" : "OUTV", rec_val, "0", "0");
			if (type == RPAR) {
				//genmc("OUTC", "32", "0", "0");
				nextsym(type, val, name);
			}
			else_ERR("expect a right parent", 2)
		}
	}
	else_ERR("expect a left parent", 2)
}
void returnstmt() {
	bool onlyChar = true;
	string rec_val;
	if (type == SEMICOLON) {
		genmc("RET", "#", "0", "0");
		return;
	}
	else if (type == LPAR) {
		nextsym(type, val, name);
		rec_val = expr(onlyChar);
		if (type == RPAR) {
			genmc("RET", rec_val, "0", "0");
			nextsym(type, val, name);
		}
		else_ERR("expect a right parent", 2)
	}
	else_ERR("unexpected sign", 1)
}
string conditions() {
	string t, rec_val1, rec_val2, op;
	bool onlyChar = true;
	type_set rec_type;
	rec_val1 = expr(onlyChar);
	if (type == LESS || type == LESSEQ || type == EQ || type == NEQ || type == GRTER || type == GRTEREQ) {
		rec_type = type;
		nextsym(type, val, name);
		rec_val2 = expr(onlyChar);
		switch (rec_type) {
		case LESS:op = "LT";break;
		case LESSEQ:op = "LE";break;
		case EQ:op = "EQ";break;
		case NEQ:op = "NE";break;
		case GRTER:op = "GT";break;
		case GRTEREQ:op = "GE";break;
		}
		t = gent();
		genmc(op, rec_val1, rec_val2, t);
	}
	else {
		t = gent();
		genmc("NE", rec_val1, "0", t);
	}
	return t;
}
void dowhilestmt() {
	string label = genlabel();
	string rec;
	genmc("LABEL", label, "0", "0");
	state();
	if (type == WHILESY) {
		nextsym(type, val, name);
		if (type == LPAR) {
			nextsym(type, val, name);
			rec = conditions();
			genmc("BEZ", rec, "0", label);
			if (type == RPAR) {
				if (DUMP_GREAMMAR) printf("%d %d this is a do-while statement\n", lc, cc);
				nextsym(type, val, name);
			}
			else_ERR("expect a right parent", 1)
		}
		else_ERR("expect left parent", 51)
	}
	else_ERR("expect a while", 51)
}
void ifstmt() {
	string label_else = genlabel();
	string label_end = genlabel();
	string rec;
	if (type == LPAR) {
		nextsym(type, val, name);
		rec = conditions();
		genmc("BNZ", rec, "0", label_else);
		if (type == RPAR) {
			nextsym(type, val, name);
			state();
			genmc("GOTO", label_end, "0", "0");
			genmc("LABEL", label_else, "0", "0");
			if (type == ELSESY) {
				nextsym(type, val, name);
				state();
			}
			genmc("LABEL", label_end, "0", "0");
		}
		else_ERR("expect a right parent", 21)
	}
	else_ERR("expect a left brace", 21)
		if (DUMP_GREAMMAR) printf("%d %d this is an if statement\n", lc, cc);
}
void state() {
	string stmttype;
	bool need_semicolon = false;
	switch (type) {
	case IFSY:
		nextsym(type, val, name);
		ifstmt();
		break;
	case DOSY:
		nextsym(type, val, name);
		dowhilestmt();
		break;
	case FORSY:
		nextsym(type, val, name);
		forstmt();
		break;
	case LBRACE:
		nextsym(type, val, name);
		stmtlist();
		break;
	case SCANFSY:
		nextsym(type, val, name);
		scanfstmt();
		stmttype = "scanf";
		need_semicolon = true;
		break;
	case PRINTFSY:
		nextsym(type, val, name);
		printfstmt();
		stmttype = "printf";
		need_semicolon = true;
		break;
	case RETURNSY:
		nextsym(type, val, name);
		returnstmt();
		stmttype = "return";
		need_semicolon = true;
		break;
	case SEMICOLON:
		stmttype = "null";
		need_semicolon = true;
		break;
	case IDEN:
		int ident_idx;
		bool ident_lev;
		ident_idx = search_tab(name, ident_lev);
		if (ident_idx != -1) {
			nextsym(type, val, name);
			if (type == LPAR) {
				funcCall(ident_idx);
				stmttype = "function call";
				need_semicolon = true;
			}
			else {
				assignstmt(ident_idx);
				stmttype = "assign";
				need_semicolon = true;
			}
		}
		else_ERR("undefined identifier", 2)
		break;
	default:
		errmsg("illegal beginning of a statement", 2);
		return;
	}
	if (type == SEMICOLON) {
		if (DUMP_GREAMMAR) printf("%d %d this is a %s statement\n", lc, cc, stmttype.c_str());
		nextsym(type, val, name);
	}
	else if(need_semicolon){
		errmsg("expect a semicolon", 1);
	}
}
void stmtlist() {
	while (type != RBRACE&&type!=END) {
		state();
	}
	if(type!=END)
		nextsym(type, val, name);
}
void statements() {
	int ident_kind;
	if (type == CONSTSY) {
		nextsym(type, val, name);
		constDef(true);
	}
	while (type == INTSY || type == CHARSY) {
		ident_kind = type == INTSY ? ST_INT : ST_CHAR;
		nextsym(type, val, name);
		varDecl(ident_kind, true);
	}
	stmtlist();
}
void mainDef(int loc) {
	genmc("LABEL", "main", "0", "0");
	int main_loc = mc.size()-1;
	if (type == LPAR) {
		nextsym(type, val, name);
		if (type == RPAR) {
			nextsym(type, val, name);
			if(type==LBRACE){
				nextsym(type, val, name);
				statements();
				cal_func_size(loc);
				if (DUMP_GREAMMAR) printf("%d %d this is a main function definition\n", lc, cc);
				nextsym(type, val, name);
				int i;
				for (i = main_loc;i < (int)mc.size();i++) {
					if (mc[i].op == "RET")
						mc[i].op = "EXIT";
				}
				genmc("EXIT", "0", "0", "0");
			}
			else_ERR("expect left brace", 1)
		}
		else_ERR("expect right parent", 1)
	}
	else_ERR("expect left parent", 1)
}
int paraList() {
	int para_type;
	string para_name;
	int cnt = 0;
	if (type == INTSY || type == CHARSY) {
		while (type==INTSY||type==CHARSY) {
			cnt++;
			para_type = type == INTSY ? ST_INT : ST_CHAR;
			nextsym(type, val, name);
			if (type == IDEN) {
				para_name = name;
				insert_tab(false, para_name, ST_PARA, para_type, -1, 4, -1);
				nextsym(type, val, name);
				if (type == COMMA) {
					nextsym(type, val, name);
				}
				else if (type == RPAR) {
					nextsym(type, val, name);
					break;
				}
				else_ERR("expect a comma", 1)
			}
			else_ERR("expect an identifier", 1)
		}
	}
	else if (type==RPAR) {
		nextsym(type, val, name);
	}
	else_ERR("expect a type identifier", 1)
	return cnt;
}
void funcDef(int loc) {
	tno = 0;
	int num = paraList();
	st[loc].val = num;
	genmc("LABEL", "FUNC_"+st[loc].name, "0", "0");
	int func_loc = mc.size() - 1;
	if (type == LBRACE) {
		nextsym(type, val, name);
		statements();
		cal_func_size(loc);
	}
	else_ERR("expect a left brace", 1);
	if (st[loc].type == ST_VOID)
		genmc("RET", "#", "0", "0");
	else
		genmc("RET", "0", "0", "0");
	if (DUMP_GREAMMAR) printf("%d %d this is a function definition\n",lc, cc);
}
void varDecl(int var_type, bool islocal) {
	int ident_type, ident_kind;
	string ident_name;
	ident_type = var_type;
	ident_kind = ST_VAR;
	int ident_val;
	if (type == IDEN) {
		while (type == IDEN) {
			ident_name = name;
			nextsym(type, val, name);
			if (type == LBRACKET) {
				ident_kind = ST_ARR;
				nextsym(type, val, name);
				if (type == NUM && val != 0) {
					ident_val = val;
					nextsym(type, val, name);
					if (type == RBRACKET) {
						insert_tab(islocal, ident_name, ident_kind, ident_type, ident_val, 4 * ident_val);
						nextsym(type, val, name);
						if (type == COMMA) {
							nextsym(type, val, name);
						}
						else if (type == SEMICOLON) {
							if (DUMP_GREAMMAR) printf("%d %d this is a variable declaration\n", lc, cc);
							nextsym(type, val, name);
							break;
						}
						else_ERR("expect a semicolon or comma", 1);
					}
					else_ERR("expect a right bracket", 1)
				}
				else if (type != NUM) {
					errmsg("expect a integer");
				}
				else_ERR("array length should not be 0", 1)
			}
			else if (type == COMMA) {
				insert_tab(islocal, ident_name, ident_kind, ident_type, 0, 4, 0);
				nextsym(type, val, name);
			}
			else if (type == SEMICOLON) {
				insert_tab(islocal, ident_name, ident_kind, ident_type, 0, 4, 0);
				if (DUMP_GREAMMAR) printf("%d %d this is a variable declaration\n", lc, cc);
				nextsym(type, val, name);
				return;
			}
			else_ERR("expect comma or semicolon", 1)
		}
	}
	else_ERR("expect identifier", 1)
}
void constDef(bool islocal) {
	int ident_type, ident_val;
	string ident_name;
	do {
		if (type == CONSTSY)
			nextsym(type, val, name);
		if (type == INTSY || type == CHARSY) {
			ident_type = type == INTSY ? ST_INT : ST_CHAR;
			do {
				nextsym(type, val, name);
				if (type == IDEN) {
					ident_name = name;
					nextsym(type, val, name);
					if (type == BECOMES) {
						nextsym(type, val, name);
						if ((ident_type == ST_INT && type == NUM)||(ident_type == ST_CHAR && type == CHAR)) {
							ident_val = (int)val;
							insert_tab(islocal, ident_name, ST_CONST, ident_type, ident_val, 4, 0);
							nextsym(type, val, name);
						}
						else if (type == MINUS || type == PLUS) {
							int co = type == MINUS ? -1 : 1;
							nextsym(type, val, name);
							if (ident_type == ST_INT && type == NUM) {
								ident_val = co*(int)val;
								insert_tab(islocal, ident_name, ST_CONST, ident_type, ident_val, 4);
								nextsym(type, val, name);
							}
							else_ERR("expect a integer", 1)
						}
						else if (type == NUM || type == CHAR)
							errmsg("type does not match");
						else_ERR("expect a number or character", 1)
					}
					else_ERR("expect becomes", 1)
				}
				else_ERR("expect an identifier", 1)
			} while (type == COMMA);
			if (type == SEMICOLON) {
				if (DUMP_GREAMMAR) printf("%d %d this is a const declaration\n", lc, cc);
				nextsym(type, val, name);
			}
			else_ERR("expect a semicolon", 1)
		}
		else_ERR("expect a type identifier", 1)
	} while (type == CONSTSY);	
}
void program() {
	nextsym(type, val, name);
	int loc;
	bool canVar = true, have_main = false;
	string ident_name;
	int ident_kind, ident_val, ident_type;
	genmc("GOTO", "main", "0", "0");
	if (type == CONSTSY) {
		nextsym(type, val, name);
		constDef(false);
	}
	while (type == INTSY||type==CHARSY||type==VOIDSY) {
		if (type == INTSY || type == CHARSY) {
			ident_type = (type == INTSY) ? ST_INT : ST_CHAR;
			nextsym(type, val, name);
			if (type == IDEN) {
				ident_name = name;
				nextsym(type, val, name);
				if (canVar) {
					if (type == LBRACKET) {
						ident_kind = ST_ARR;
						nextsym(type, val, name);
						if (type == NUM && val != 0) {
							ident_val = val;
							nextsym(type, val, name);
							if (type == RBRACKET) {
								insert_tab(false, ident_name, ident_kind, ident_type, ident_val, 4 * ident_val);
								nextsym(type, val, name);
								if (type == COMMA) {
									nextsym(type, val, name);
									varDecl(ident_type, false);
								}
								else if (type == SEMICOLON) {
									if (DUMP_GREAMMAR) printf("%d %d this is a variable declaration\n",lc, cc);
									nextsym(type, val, name);

								}
								else_ERR("expect a semicolon or comma", 1)
							}
							else_ERR("expect a right bracket", 1)
						}
						else if (type != NUM) {
							errmsg("expect a integer");
						}
						else_ERR("array length should not be 0", 1)
					}
					else if (type == COMMA) {
						insert_tab(false, ident_name, ST_VAR, ident_type, 0, 4, 0);
						nextsym(type, val, name);
						varDecl(ident_type, false);
					}
					else if (type == SEMICOLON) {
						insert_tab(false, ident_name, ST_VAR, ident_type, 0, 4, 0);
						if (DUMP_GREAMMAR) printf("%d %d this is a variable declaration\n", lc, cc);
						nextsym(type, val, name);
					}
					else if (type == LPAR) {
						canVar = false;
						loc = insert_tab(false, ident_name, ST_FUNC, ident_type);
						nextsym(type, val, name);
						funcDef(loc);
					}
					else_ERR("illegal sign after an identifier", 1)
				}
				else {
					if (type == LPAR) {
						int loc = insert_tab(false, ident_name, ST_FUNC, ident_type);
						canVar = false;
						nextsym(type, val, name);
						funcDef(loc);
					}
					else if (type == COMMA || type == SEMICOLON || type == LBRACKET) {
						errmsg("displaced variable declaration");
					}
					else_ERR("illegal sign after an identifier", 1)
				}
			}
			else_ERR("expect identifier", 1)
		}
		else if (type == VOIDSY) {
			ident_type = ST_VOID;
			ident_kind = ST_FUNC;
			canVar = false;
			nextsym(type, val, name);
			ident_name = name;
			if (type == MAINSY) {
				nextsym(type, val, name);
				have_main = true;
				loc = insert_tab(false, "main", ST_FUNC, ST_VOID, 0, 0, 0);
				mainDef(loc);
				break;
			}
			else if (type == IDEN) {
				nextsym(type, val, name);
				if (type == LPAR) {
					nextsym(type, val, name);
					loc = insert_tab(false, ident_name, ident_kind, ident_type);
					funcDef(loc);
				}
				else_ERR("expect left parent", 1)
			}
			else_ERR("expect a identifier or main", 1)
		}
		else_ERR("illegal type identifier", 1)
	}
	if (!have_main) {
		errmsg("should have a main function");
	}
	if (type != END) {
		errmsg("expect a end of file");
	}
	if (DUMP_GREAMMAR) printf("%d %d this is a program\n", lc, cc);
}