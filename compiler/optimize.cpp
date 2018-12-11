#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"

map<string, int> nodetable;
struct node{
	int nodeno;
	vector<int> par;
	int left, right;
	string name;
};
struct node dag[3000];
int nodenum;
void genDAG_expr(int blockno) {
	nodetable.clear();
	nodenum = 0;
	for (int i = 0;i < 3000;i++) {
		dag[i].nodeno = i;
		dag[i].par.clear();
		dag[i].left = -1;
		dag[i].right = -1;
		dag[i].name = "";
	}
	int begin = blocks[blockno].start;
	int end = blocks[blockno].end;
	for (int i = begin;i <= end;i++) {	//��ÿһ��ָ�� 
		if (mc[i].op == "NULL" || mc[i].n1 == "" || mc[i].n1 == "" || mc[i].n2 == "" || mc[i].res == "")
			continue;	//����z=x op y
		map<string, int>::iterator it = nodetable.begin();
		while (it != nodetable.end()) {
			if (it->first == mc[i].n1)
				break;
			it++;
		}
		int rec1;
		if (it == nodetable.end()) {
			dag[nodenum].nodeno = nodenum;
			dag[nodenum].left = -1;
			dag[nodenum].right = -1;
			dag[nodenum].par.clear();
			if (isdigit(mc[i].n1[0]) || mc[i].n1[0] == '-' || mc[i].n1[0] == '+')
				dag[nodenum].name = mc[i].n1;
			else
				dag[nodenum].name = "0"+mc[i].n1;
			rec1 = nodenum++;
		}
		else
			rec1 = it->second;
		//���� rec1��¼��n1�ı��
		it = nodetable.begin();
		while (it != nodetable.end()) {
			if (it->first == mc[i].n2)
				break;
			it++;
		}
		int rec2;
		if (it == nodetable.end()) {
			dag[nodenum].nodeno = nodenum;
			dag[nodenum].left = -1;
			dag[nodenum].right = -1;
			dag[nodenum].par.clear();
			if (isdigit(mc[i].n2[0]) || mc[i].n2[0] == '-' || mc[i].n2[0] == '+')
				dag[nodenum].name = mc[i].n2;
			else
				dag[nodenum].name = "0" + mc[i].n2;
			rec2 = nodenum++;
		}
		else
			rec2 = it->second;
		//���� rec2��¼��n1�ı��
		//��dagͼ��Ѱ���м���
		int rec3;
		for (rec3 = 0;rec3 < nodenum;rec3++) 
			if (dag[rec3].left == rec1 && dag[rec3].right == rec2) 
				break;
		if(rec3==nodenum){
			dag[nodenum].name = mc[i].op;
			dag[nodenum].left = rec1;
			dag[rec1].par.push_back(nodenum);
			dag[nodenum].right = rec2;
			dag[rec2].par.push_back(nodenum);
			dag[nodenum].nodeno = nodenum++;
		}
		//����rec3Ϊ�м�����
		it = nodetable.begin();
		while (it != nodetable.end()) {
			if (it->first == mc[i].res)
				break;
			it++;
		}
		if (it == nodetable.end())
			nodetable[mc[i].res] = rec3;
		else
			it->second = rec3;
	}
}

void common_expr() {
	for (int i = 0;i < (int)blocks.size();i++) {	//��ÿ����
		genDAG_expr(i);	//�������Ӧ��DAG
		
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