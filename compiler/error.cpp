#include "pch.h"
#include "stdHeader.h"
#include "error.h"
#include "globalvar.h"
#include "lexical.h"

//策略编号 0-不跳读
//1-一个单词
//2-跳到分号 停在分号
//3-跳到右大括号
//31-跳到右大括号的下一个
//4-跳到右方括号
//5-跳到右小括号
//51-跳到右小括号的下一个

void errmsg(string err, int strategy) {
	printf("ERROR: line:%d, col:%d\t", lc, cc);
	cout << err << '\n';
	switch (strategy) {
	case 0:
		return;
	case 1:
		nextsym(type, val, name);
		return;
	case 2:
		while (type != END && type != SEMICOLON) {
			nextsym(type, val, name);
		}
		break;
	case 3:
		while (type != END && type != RBRACE) {
			nextsym(type, val, name);
		}
		break;
	case 31:
		while (type != END && type != RBRACE) {
			nextsym(type, val, name);
		}
		if (type != END)
			nextsym(type, val, name);
		break;
	case 4:
		while (type != END && type != RBRACKET) {
			nextsym(type, val, name);
		}
		break;
	case 5:
		while (type != END && type != RPAR) {
			nextsym(type, val, name);
		}
	case 51:
		while (type != END && type != RPAR) {
			nextsym(type, val, name);
		}
		if (type != END)
			nextsym(type, val, name);
		break;
	default:
		break;
	}
	return;
}