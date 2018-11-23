#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_


string factor(bool &onlyChar);	//not yet
string term(bool &onlyChar);	//done
string expr(bool &onlyChar);	//done
void funcCall(int ident_idx);
void assignstmt(int ident_idx);
void forstmt();
void scanfstmt();	//done
void printfstmt();	//done
void returnstmt();	//done
string conditions();
void dowhilestmt();
void ifstmt();
void state();
void stmtlist();
void statements();
void mainDef(int loc);
int paraList();
void funcDef(int loc);
void varDecl(int var_type, bool islocal);	//done
void constDef(bool islocal);	//done
void program();

#endif
