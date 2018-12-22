#include "pch.h"
#include "stdHeader.h"
#include "lexical.h"
#include "globalvar.h"
#include "error.h"
//load the file into char array code
void readFile(char code[]) {
	printf("input the directory of source code(only the first 128 chars will be recognnized):\n");
	char dir[129];
	cin.getline(dir, 128);
	//char dir[] = "/Users/sdy/projects/compilerAssign/test_sharing/16721145_test.txt";
	//char dir[] = "G:\\compilerAssign\\test_sharing\\16721145_test.txt";
	//char dir[] = "grammar_test.txt";
	printf("opening %s...\n", dir);
	FILE *fp;
	fp = fopen(dir, "r");
	if (fp) {
		int i = 0;
		char ch = 0;
		while (!feof(fp)) {
			ch = fgetc(fp);
			if (ch == EOF)
				break;
			if (i >= MAXSRCLEN - 1) {
				cout << "TOO LONG!\n";    //the source code is too long
				break;
			}
			if (ch >= 0 && ch <= 255) {
				code[i] = ch;
				i++;
			}
			else {
				cout << "unexpected char!"<<ch<<"\n";
				i++;
			}
		}
		code[i] = '\0';
	}
	else
		cout << "\nno file found!\n";     //cannot find the source code file
}

bool isalu(char c) {
	return isalpha(c) || c == '_';
}
bool isalundernum(char c) {
	return isalnum(c) || c == '_';
}
char nextch() {
	static bool tag = false;
	char ch = code[cursor];
	cursor++;
	cc++;
	if (tag) {
		tag = false;
		lc++;
		cc = 0;
	}
	if (ch == '\n') {
		tag = true;
	}
	return ch;
}


void nextsym(type_set &type, int &val, string &name) {
	do
		nextsym_inner(type, val, name);
	while (type == NOTYPE);
	return;
}
void nextsym_inner(type_set &type, int &val, string &name) {
	while (ch == ' ' || ch == '\t' || ch == '\n') {
		ch = nextch();
	}
	if (ch == '\0') {
		type = END;
		return;
	}
	switch (ch) {
	case '+':
	case '-':
	case '*':
	case '/':
	case '{':
	case '}':
	case '[':
	case ']':
	case '(':
	case ')':
	case ';':
	case ',':
		name = ch;
		type = reserve_tab[name];
		ch = nextch();
		break;
	case '<':
		ch = nextch();
		if (ch == '=') {
			type = LESSEQ;
			name = "<=";
			ch = nextch();
		}
		else {
			type = LESS;
			name = "<";
		}
		break;
	case '>':
		ch = nextch();
		if (ch == '=') {
			type = GRTEREQ;
			name = ">=";
			ch = nextch();
		}
		else {
			type = GRTER;
			name = ">";
		}
		break;
	case '!':
		ch = nextch();
		if (ch == '=') {
			type = NEQ;
			name = "!=";
			ch = nextch();
		}
		else {
			errmsg("expect '=' sign", 0);
			type = NOTYPE;
			return;
		}
		break;
	case '=':
		ch = nextch();
		if (ch == '=') {
			type = EQ;
			name = "==";
			ch = nextch();
		}
		else {
			type = BECOMES;
			name = "=";
		}
		break;
	case '\'':
		ch = nextch();
		if (isalundernum(ch) || ch == '+' ||ch == '-' || ch == '*' || ch == '/') {
			char r = ch;
			ch = nextch();
			if (ch == '\'') {
				type = CHAR;
				val = r;
				ch = nextch();
				return;
			}
			else {
				//TODO: error;
				errmsg("expect a single quote mark", 0);
				type = NOTYPE;
				return;
			}
		}
		else {
			errmsg("unrecognized character", 0);
			type = NOTYPE;
			ch = nextch();
			return;
		}
		break;
	case '"':
		ch = nextch();
		int rp;
		rp = strtabp;
		while (ch != '"') {
			if (strtabp >= MAXSTRLEN - 1) {
				//TODO: fatal
				errmsg("fatal too many string", 0);
				type = NOTYPE;
				strtabp = rp;
				return;
			}
			if (ch!= 32 && ch != 33 && !(ch >= 35 && ch <= 126)) {
				//TODO:error
				errmsg("unrecognized character", 0);
				type = NOTYPE;
				ch = nextch();
				strtabp = rp;
				return;
			}
			strtab[strtabp++] = ch;
			ch = nextch();
		}
		strtab[strtabp++] = '\0';
		strtab[strtabp++] = '\0';
		type = STR;
		val = rp;
		ch = nextch();
		break;
	default:
		if (isdigit(ch)) {
			type = NUM;
			if (ch == '0') {
				ch = nextch();
				if (isdigit(ch)) {
					//TODO: error
					errmsg("leading zero", 0);
					type = NOTYPE;
					return;
				}
				else {
					val = 0;
					return;
				}
			}
			else {
				val = ch - '0';
				int wid = 1;
				ch = nextch();
				while (isdigit(ch)) {
					wid++;
					if (wid >= 20) {
						//TODO: error
						errmsg("number too big", 0);
						type = NOTYPE;
						ch = nextch();
						return;
					}
					val = val * 10 + ch - '0';
					ch = nextch();
				}
				return;
			}
		}
		else if (isalu(ch)) {
			name = ch;
			int len = 1;
			ch = nextch();
			while (isalundernum(ch)) {
				name += ch;
				len++;
				if (len >= 50) {
					//TODO: error;
					errmsg("identifier too long", 0);
					type = NOTYPE;
					ch = nextch();
					return;
				}
				ch = nextch();
			}
			type = IDEN;
			map<string, type_set>::iterator it = reserve_tab.begin();
			while (it != reserve_tab.end()) {
				if (it->first == name) {
					type = it->second;
					break;
				}
				it++;
			}
		}
		else {
			//TODO: error!!
			errmsg("unrecognized character", 0);
			type = NOTYPE;
			ch = nextch();
			return;
		}
		break;
	}
	return;
}

void outputLexicalAnalyzeRes() {
	freopen("result.txt", "w", stdout);
	//    int i = 0;
	//    while (code[i] != '\0') {
	//        printf("%c", code[i]);
	//        i++;
	//    }
	int i = 0;
	printf("result\n");
	do {
		nextsym(type, val, name);
		if (type != END) {
			//printf("%4d\t", i++);
			printf("line:%d, col:%d\t", lc, cc);
		}
		switch (type) {
		case NUM:
			printf("NUM\t\t%d\n", val);
			break;
		case CHAR:
			printf("CHAR\t\t%c\n", val);
			break;
		case STR:
			printf("STR\t\t%s\n", strtab + val);
			break;
		case END:
			break;
		default:
			cout << type2str[type] << "\t\t" << name << '\n';
			break;
		}
	} while (type != END);
	fclose(stdout);
	return;
}