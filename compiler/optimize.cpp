#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "sign_table.h"
#include "midcode.h"
#include "optimize.h"

#define isNum(x) (x[0]=='+'||x[0]=='-'||isdigit(x[0]))


void midcode_opt() {
	omc = mc;	//��� mc�������Ż�����м���� omc�洢��ȥ��
	//copy_spread();	//�����ƴ���
	const_spread();	//����������
	//kk_opt();	//�����Ż�
	//modify();	//�������
}

void modify() {
	mce tmp;
	for(int i=1;i<(int)blocks.size();i++){
		blocks[i].start = blocks[i - 1].end + 1;	//ÿһ��Ŀ�ʼӦ������һ��Ľ���+1
		int tag_end = blocks[i].start;	//��ǽ���λ��
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {//ÿһ�������ָ��
			if (mc[j].op == "NULL")
				continue;
			//�����ǿ�ָ��
			tmp = mc[j];
			mc[j] = mc[tag_end];
			mc[tag_end] = tmp;
			tag_end++;
		}
		blocks[i].end = tag_end - 1;
	}
}
//�������ڵĸ��ƴ����㷨
void copy_spread() {
	bool islocal;
	int loc = -1;
	for (int i = 0;i < (int)blocks.size();i++) {	//��ÿ����
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//��ÿ��ָ��
			if (mc[j].op != "ASSIGN" || mc[j].res[0] != '#')
				continue;
			loc = search_tab(mc[i].n1, islocal);
			if (!islocal||loc==-1)	continue;	//ȫ�ֱ���Ҳ����
			//��������м�����ĸ��ƴ���
			string src = mc[j].n1;	//������Դ
			string dst = mc[j].res;//����Ŀ��
			bool change = false, canDelete = true;
			for (int k = j+1;k <= blocks[i].end;k++) {	//����һ��ָ�ʼ
				if (mc[k].res != dst&&mc[k].res!=src&&!change) {	//��Դû�б��ı� ʹ�õ�Ŀ�궼�����滻
					mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
					mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n1;	//TODO:���¹淶���midcode
				}
				else if (change) {	//�ڱ��ı������� ���dst���Ƿ�ʹ�ù�
					if (mc[k].n1 == dst || mc[k].n2 == dst)
						canDelete = false;	//��������ù� �ǾͲ�����ɾ����
				}
				else {//��Դ���ı����һ�����ǿ��Ի���
					mc[k].n1 = mc[k].n1 == dst ? src : mc[k].n1;
					mc[k].n2 = mc[k].n2 == dst ? src : mc[k].n1;
					change = true;	//�����Դ���ı���
				}
			}
			if (canDelete)	//��ɾ��ɾ
				mc[j].op = "NULL";	//Ǳ������ op���ж��߼��Ƿ�����
		}
	}
}

//�������ڵĳ��������㷨
void const_spread() {
	bool change = false;
	for (int i = 0;i < (int)blocks.size();i++) {	//��ÿ����
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//��ÿ������
			if (mc[j].op == "ASSIGN"&&mc[j].res[0] == '#'&&isNum(mc[j].n1)) {
				string src = mc[j].n1;
				string dst = mc[j].res;
				mc[j].op = "NULL";
				for (int k = j + 1;k <= blocks[i].end;k++) {
					if (mc[k].n1 == dst)
						mc[k].n1 = src;
				}
			}
			else if (mc[j].op == "ADD" || mc[j].op == "SUB" || mc[j].op == "MULT" || mc[j].op == "DIV") {
				if (isNum(mc[j].n1) && isNum(mc[j].n2)) {
					int res = mc[j].op == "ADD" ? stoi(mc[j].n1) + stoi(mc[j].n2) :
							  (mc[j].op == "SUB" ? stoi(mc[j].n1) - stoi(mc[j].n2) :
							  (mc[j].op == "MULT" ? stoi(mc[j].n1)*stoi(mc[j].n2) :
							  stoi(mc[j].n1) / stoi(mc[j].n2)));
					mc[j].op = "ASSIGN";
					mc[j].n1 = to_string(res);
					j--;
				}	
			}
		}
	}
}
void kk_opt() {
	for (int i = 0;i < (int)mc.size() - 1;i++) {
		//�Ż�1 OUTS OUTC�ϲ���һ��
		if (mc[i].op == "OUTS"&&(mc[i+1].op=="OUTC")) {
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
		//�Ż�2 GOTO����һ��label֮��Ĵ���ɾ��
		else if (mc[i].op == "GOTO") {
			int j = i + 1;
			while (mc[j].op != "LABEL") {
				mc[j].op = "NULL";
				j++;
			}
			i = j-1;
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
		else if (mp[i].op == "jr") {
			if (mp[i - 1].res == "$v0"&&mp[i - 1].op == "move"&&mp[i - 2].res == mp[i - 1].n1
				&&(mp[i-2].op=="add"|| mp[i - 2].op == "addi"
					|| mp[i - 2].op == "sub"|| mp[i - 2].op == "move"||
					mp[i-2].op=="lw"||mp[i-2].op=="li"||mp[i-2].op=="mflo")) {
				mp[i - 2].res = "$v0";
				mp.erase(mp.begin() + i - 1);
				i--;
			}
		}

	}
}