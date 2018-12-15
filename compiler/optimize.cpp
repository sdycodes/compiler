#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "sign_table.h"
#include "midcode.h"

//�������ڵĸ��ƴ����㷨
void copy_spread() {
	bool islocal;
	for (int i = 0;i < (int)blocks.size();i++) {	//��ÿ����
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {	//��ÿ��ָ��
			if (nmc[j].op != "ASSIGN" || nmc[j].res[0] != '#')
				continue;
			search_tab(nmc[i].n1, islocal);
			if (!islocal)	continue;	//ȫ�ֱ���Ҳ����
			//��������м�����ĸ��ƴ���
			string src = nmc[j].n1;	//������Դ
			string dst = nmc[j].res;//����Ŀ��
			bool change = false, canDelete = true;
			for (int k = j+1;k <= blocks[i].end;k++) {	//����һ��ָ�ʼ
				if (nmc[k].res != src&&!change) {	//��Դû�б��ı� ʹ�õ�Ŀ�궼�����滻
					nmc[k].n1 = nmc[k].n1 == dst ? src : nmc[k].n1;
					nmc[k].n2 = nmc[k].n2 == dst ? src : nmc[k].n1;	//TODO:���¹淶���midcode
				}
				else if (change) {	//�ڱ��ı������� ���dst���Ƿ�ʹ�ù�
					if (nmc[k].n1 == dst || nmc[k].n2 == dst)
						canDelete = false;	//��������ù� �ǾͲ�����ɾ����
				}
				else {//��Դ���ı����һ�����ǿ��Ի���
					nmc[k].n1 = nmc[k].n1 == dst ? src : nmc[k].n1;
					nmc[k].n2 = nmc[k].n2 == dst ? src : nmc[k].n1;
					change = true;	//�����Դ���ı���
				}
			}
			if (canDelete)	//��ɾ��ɾ
				nmc[j].op = "NULL";	//Ǳ������ op���ж��߼��Ƿ�����
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
		//�Ż�3 ��LELEM��SELEMһ������ȥ��
		else if (mc[i].op == "LELEM"&&mc[i + 1].op == "SELEM") {
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