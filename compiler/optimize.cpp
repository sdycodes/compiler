#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "reg_distribution.h"
#include "sign_table.h"
#include "midcode.h"
#include "optimize.h"

#define isCon(x) (x[0]=='+'||x[0]=='-'||isdigit(x[0]))

bool modified(mce x, string src) {
	//�����Դ�Ƿ񱻸ı�
	if ((x.op == "ADD" || x.op == "SUB" || x.op == "MULT" || x.op == "DIV" || x.op == "ASSIGN" || x.op == "LELEM") && x.res == src)
		return true;
	if ((x.op == "INC" || x.op == "INV") && x.n1 == src)
		return true;
	if ((x.op == "LT" || x.op == "LE" || x.op == "GT" || x.op == "GE" || x.op == "EQ" || x.op == "NE") && x.res == src)
		return true;
	return false;
}
bool used(mce x, string src) {
	if (x.op != "INC"&&x.op != "INV"&&(x.n1 == src || x.n2 == src))
		return true;
	return false;
}

void common_expr() {
	int loc = -1;
	bool islocal = false;
	bool islocal1 = false, islocal2 = false;
	int def_loc = -1;
	for (int i = 0;i < (int)blocks.size();i++) {
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//�Կ���ÿ��ָ��
			//labelָ�� ����def_loc �����Ժ�����
			if (mc[j].op == "LABEL"&&mc[j].n1[0] != '$')
				def_loc = search_tab(mc[j].n1, islocal);
			//���ڼӼ��˳�ָ�� �������������ӱ��ʽ
			else if (mc[j].op == "ADD"||mc[j].op == "SUB"||mc[j].op == "MULT" || mc[j].op == "DIV"||mc[j].op=="LELEM") {
				//��ȫ�ֱ�����Ҫ��
				int loc1 = search_tab(mc[j].n1, islocal1, def_loc);
				int loc2 = search_tab(mc[j].n2, islocal2, def_loc);
				if ((loc1!=-1&&!islocal1)||(loc2!=-1&&!islocal2))	continue;
				bool n1change = false, n2change = false;	//�����������Ƿ񱻸ı�
				//������ı��ʽ
				int k;
				for (k = j + 1;k <= blocks[i].end;k++) {
					n1change = modified(mc[k], mc[j].n1);
					n2change = modified(mc[k], mc[j].n2);
					if (n1change || n2change)	break;
					//��Դû�б��ı�	������һ�� �Ǿ��ǹ����ӱ��ʽ��
					if (mc[k].op == mc[j].op&&mc[k].n1==mc[j].n1&&mc[k].n2==mc[j].n2) {
						mc[k].op = "ASSIGN";
						mc[k].n1 = mc[j].res;	
						//����������
						if (mc[k].res[0] == '#'&&mc[k].n1 != "#RET") {
							//�м����������һ�����ǳ����ı���
							string src = mc[k].n1;	//������Դ
							string dst = mc[k].res;//����Ŀ��
							//������ȫ�ֱ�����������
							search_tab(src, islocal, def_loc);
							if (src[0] != '#' && !islocal)	continue;
							bool change = false, canDelete = true;
							for (int w = k + 1;w <= blocks[i].end;w++) {	//����һ��ָ�ʼ
								if (!change && !modified(mc[w], src)) {	//��Դû�б��ı� ʹ�õ�Ŀ�궼�����滻
									mc[w].n1 = mc[w].n1 == dst ? src : mc[w].n1;
									mc[w].n2 = mc[w].n2 == dst ? src : mc[w].n2;
								}
								else if (change) {	//�ڱ��ı������� ���dst���Ƿ�ʹ�ù�
									if ((mc[w].op != "INC"&&mc[w].op != "INV") && (mc[w].n1 == dst || mc[k].n2 == dst))
										canDelete = false;	//��������ù� �ǾͲ�����ɾ����
								}
								else {//��Դ���ı����һ�����ǿ��Ի���
									if (mc[w].op != "INC"&&mc[w].op != "INV") {
										mc[w].n1 = mc[w].n1 == dst ? src : mc[w].n1;
										mc[w].n2 = mc[w].n2 == dst ? src : mc[w].n1;
									}
									change = true;	//�����Դ���ı���
								}
							}
							if (canDelete)	//��ɾ��ɾ
								mc[k].op = "NULL";
						}
					}
				}
			}
		}
	}
}

//��ǩ���� ���ص��ı�ǩɾ��
void label_modify() {
	for (int i = 0;i < (int)mc.size()-1;i++) {
		if (mc[i].op == mc[i + 1].op&&mc[i].op == "LABEL"&&mc[i].n1[0]=='$'&&mc[i+1].n1[0]=='$') {
			mc[i].op = "NULL";
			string dst = mc[i+1].n1;
			string src = mc[i].n1;
			for (int j = 0;j < (int)mc.size();j++) {
				if (mc[j].n1 == src)	mc[j].n1 = dst;
				if (mc[j].res == src)	mc[j].res = dst;
			}
		}
	}
}
void midcode_opt() {
	omc = mc;	//��� mc�������Ż�����м���� omc�洢��ȥ��
	const_copy_spread();	//�м�����ĳ����͸��ƴ��� �ֲ������ĳ�������
	common_expr();			//�ѹ����ӱ��ʽת��Ϊassign���	���Ҵ���
	label_modify();			//�Ѷ��ر�ǩ����
	kk_opt();				//�����Ż�
}


//���������м�����ĳ������ƴ����㷨
void const_copy_spread() {
	bool change = false;
	int loc = -1;
	bool islocal = false;
	int def_loc = -1;
	for (int i = 0;i < (int)blocks.size();i++) {	//��ÿ����
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//��ÿ������
			if (mc[j].op == "LABEL"&&mc[j].n1[0] != '$') {	//�����������ĸ�������
				def_loc = search_tab(mc[j].n1 == "main" ? "main" : mc[j].n1.substr(5), islocal);
				continue;
			}
			else if (mc[j].op == "ASSIGN"&&mc[j].res[0]=='#'&&isCon(mc[j].n1)) {	
				//�м���� �����ܿ�� �����ܱ���ֵ �滻����
				string src = mc[j].n1;
				string dst = mc[j].res;
				mc[j].op = "NULL";
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (mc[k].n1 == dst)
						mc[k].n1 = src;
					if (mc[k].n2 == dst)
						mc[k].n2 = src;
				}
			}
			else if (mc[j].op=="ASSIGN"&&mc[j].res[0]=='#'&&mc[j].n1!="#RET") {
				//�м����������һ�����ǳ����ı���
				string src = mc[j].n1;	//������Դ
				string dst = mc[j].res;//����Ŀ��
				//������ȫ�ֱ�����������
				search_tab(mc[j].n1, islocal, def_loc);
				if (src[0]!='#'&&!islocal)	continue;
				bool change = false, canDelete = true;
				for (int k = j + 1;k <= blocks[i].end;k++) {	//����һ��ָ�ʼ
					if (!change&&!modified(mc[k], src)) {	//��Դû�б��ı� ʹ�õ�Ŀ�궼�����滻
						mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
						mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n2;	
					}
					else if (change) {	//�ڱ��ı������� ���dst���Ƿ�ʹ�ù�
						if ((mc[k].op != "INC"&&mc[k].op != "INV")&&(mc[k].n1 == dst || mc[k].n2 == dst))
							canDelete = false;	//��������ù� �ǾͲ�����ɾ����
					}
					else {//��Դ���ı����һ�����ǿ��Ի���
						if (mc[k].op != "INC"&&mc[k].op != "INV") {
							mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
							mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n1;
						}
						change = true;	//�����Դ���ı���
					}
				}
				if (canDelete)	//��ɾ��ɾ
					mc[j].op = "NULL";	
			}
			else if (mc[j].op == "ADD" || mc[j].op == "SUB" || mc[j].op == "MULT" || mc[j].op == "DIV") {
				if (isCon(mc[j].n1) && isCon(mc[j].n2)) {
					int res = mc[j].op == "ADD" ? stoi(mc[j].n1) + stoi(mc[j].n2) :
							  (mc[j].op == "SUB" ? stoi(mc[j].n1) - stoi(mc[j].n2) :
							  (mc[j].op == "MULT" ? stoi(mc[j].n1)*stoi(mc[j].n2) :
							  stoi(mc[j].n1) / stoi(mc[j].n2)));
					mc[j].op = "ASSIGN";
					mc[j].n1 = to_string(res);
					j--;
				}	
			}
			else if (mc[j].op == "ASSIGN"&&mc[j].res[0] != '#'&&isCon(mc[j].n1)) {
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (isOB[loc]||!islocal)	continue;	//�˲���ֻ��Բ����ľֲ�����
				//�ֲ��������ܱ���ֵ 
				string src = mc[j].n1;
				string dst = mc[j].res;
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (!modified(mc[k], dst)) {
						if (mc[k].n1 == dst)
							mc[k].n1 = src;
						if (mc[k].n2 == dst)
							mc[k].n2 = src;
					}
					else
						break;
				}
			}
		}
		
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//����ɨ�������
			//��Щ�����ľֲ��������ܿ���ɾ��
			if (mc[j].op == "ASSIGN") {
				if (mc[j].res[0] == '#')	continue;
				loc = search_tab(mc[j].res, islocal, def_loc);
				if (isOB[loc]||!islocal)	continue;	//�˲���ֻ��Բ����ľֲ�����
				bool canDelete = true;
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (used(mc[k], mc[j].res)) {
						canDelete = false;
						break;
					}
				}
				if(canDelete) mc[j].op = "NULL";
			}
		}
	}
}

void kk_opt() {
	for (int i = 0;i < (int)mc.size() - 1;i++) {
		//�Ż�1 OUTS OUTC�ϲ���һ��
		if (mc[i].op == "OUTS" && (mc[i + 1].op == "OUTC")) {
			if (mc[i + 1].n1[0] >= '0'&&mc[i + 1].n1[0] <= '9') {
				char append = (char)(stoi(mc[i + 1].n1) % 256);
				mc[i + 1].op = "NULL";
				int j = stoi(mc[i].n1);
				while (strtab[j] != '\0')
					j++;
				strtab[j] = append;
			}
			i++;
		}
		//�Ż�2 GOTO����һ��label֮��Ĵ���ɾ�� ���������ɾ��GOTO
		else if (mc[i].op == "GOTO") {
			int j = i + 1;
			while (mc[j].op != "LABEL") {
				mc[j].op = "NULL";
				j++;
			}
			if (mc[j].op == "LABEL"&&mc[i].n1 == mc[j].n1)
				mc[i].op = "NULL";
			i = j - 1;
		}
		//�Ż�3 ��LELEM��SELEM SELEM����ȥ��
		else if (mc[i].op == "LELEM"&&mc[i + 1].op == "SELEM") {
			if (mc[i].res == mc[i + 1].n2&&mc[i].n1 == mc[i + 1].res&&mc[i].n2 == mc[i + 1].n1) {
				mc[i + 1].op = "NULL";
				i++;
			}
		}
		//�Ż�4 ��SELEM��LELEM LELEM����ȥ��
		else if (mc[i].op == "SELEM"&&mc[i + 1].op == "LELEM") {
			if (mc[i].res == mc[i + 1].n2&&mc[i].n1 == mc[i + 1].res&&mc[i].n2 == mc[i + 1].n1) {
				mc[i + 1].op = "NULL";
				i++;
			}
		}
		//�Ż�5 ����������EXIT��RET ˵�����Լ���ӵ� ����ɾ��������Ǹ�
		else if (mc[i].op == "RET"&&mc[i + 1].op == "RET") {
			mc[i + 1].op = "NULL";
			i++;
		}
		else if (mc[i].op == "EXIT"&&mc[i + 1].op == "EXIT") {
			mc[i + 1].op = "NULL";
			i++;
		}
		//�Ż�6 �����м��������Ϊ�������㴫��ý�� �����Ż�����Ҫ��֤MIPS�����߼��ܹ�Ӧ�Ը������
		else if ((mc[i].op == "ADD" || mc[i].op == "SUB" || mc[i].op == "MULT" || mc[i].op == "DIV")
			&& mc[i + 1].op == "ASSIGN"&&mc[i].res == mc[i + 1].n1) {
			mc[i].res = mc[i + 1].res;
			mc[i + 1].op = "NULL";
			i++;
		}
		//�Ż�7 ��ȫ��Ч�����ɾ��
		else if (mc[i].op == "ASSIGN"&&mc[i].n1 == mc[i].res)
			mc[i].op = "NULL";
	}
	
}

void mips_opt() {
	for (int i = 0;i < (int)mp.size();i++) {
		//����������αָ�����һ��
		if (mp[i].op == "subi") {
			mp[i].op = "addi";
			mp[i].n2 = to_string(-stoi(mp[i].n2));
		}
		//�Է���ֵ���ض��Ż�
		else if (i>=2&&mp[i].op == "jr") {
			if (mp[i - 1].res == "$v0"&&mp[i - 1].op == "move"&&mp[i - 2].res == mp[i - 1].n1
				&&(mp[i-2].op=="add"|| mp[i - 2].op == "addi"
					|| mp[i - 2].op == "sub"|| mp[i - 2].op == "move"||
					mp[i-2].op=="lw"||mp[i-2].op=="li")) {
				mp[i - 2].res = "$v0";
				mp.erase(mp.begin() + i - 1);
				i--;
			}
		}
		//���һ��lw����lw�ı��ĵ� ��ôɾ������lw ���ұ��뱣֤����Ĵ���û�б�ʹ��
		/*
		else if (i < (int)mp.size() - 1 && mp[i].op == "lw") {
			if (mp[i + 1].res == mp[i].res&&
				(mp[i+1].op=="add"||mp[i+1].op=="sub"||mp[i+1].op=="mul"||mp[i+1].op=="div"||
					mp[i+1].op=="addi"||mp[i+1].op=="subi"||mp[i+1].op=="lw"||
					mp[i+1].op=="move"||mp[i+1].op=="li"||mp[i+1].op=="sll")&&
				(mp[i+1].n1!=mp[i].res&&mp[i+1].n2!=mp[i].res))
				mp[i].op = "nop";
		}*/

	}
}