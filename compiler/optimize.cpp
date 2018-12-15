#include "pch.h"
#include "stdHeader.h"
#include "globalvar.h"
#include "base_block.h"
#include "sign_table.h"
#include "midcode.h"
map<string, int> nodetable;
struct node{
	int nodeno;
	vector<int> par;
	int left, right;
	string name;
};
struct node dag[3000];
int nodenum;
int max(int a, int b) {
	return a > b ? a : b;
}
void genDAG_expr(int blockno, int t_tag) {
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
		if (mc[i].op == "NULL" || mc[i].n1 == "" || mc[i].n1 == "" || mc[i].n2 == "" || mc[i].res == "") {
			nmc.push_back(mc[i]);	//ֱ�ӷŵ��µ�����
			continue;	//����z=x op y
		}
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
			else {
				dag[nodenum].name = mc[i].n1 + "#";
				dag[nodenum].name += to_string(t_tag++);
			}
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
			else {
				dag[nodenum].name = mc[i].n1 + "#";
				dag[nodenum].name += to_string(t_tag++); 
			}
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

bool judge(int i, vector<int> queue) {
	if (i == -1)	return false;
	if (dag[i].par.size() == 0)	return true;
	for (int j = 0;j < (int)dag[i].par.size();j++) {
		for (int k = 0;k < (int)queue.size();k++)
			if (queue[k] == j)
				continue;
		return false;
	}
	return true;
}
void common_expr() {
	int def_loc;
	bool islocal;
	for (int i = 0;i < (int)blocks.size();i++) {	//��ÿ����
		if (mc[blocks[i].start].op == "LABEL"&&mc[blocks[i].start].n1 != "$")	//���º���λ��
			def_loc = search_tab(mc[blocks[i].start].n1=="main"?"main": mc[blocks[i].start].n1.substr(5), islocal);
		blocks[i].start = nmc.size();	//�µĿ�λ��
		//���������м����
		int min_idx = INT_MAX;
		int max_idx = -1;
		int t_tag = 0;
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {
			if (mc[j].n1[0] == '#') {
				min_idx = min_idx > stoi(mc[j].n1.substr(1)) ? stoi(mc[j].n1.substr(1)) : min_idx;
				max_idx = max_idx < stoi(mc[j].n1.substr(1)) ? stoi(mc[j].n1.substr(1)) : max_idx;
			}
			if (mc[j].n2[0] == '#') {
				min_idx = min_idx > stoi(mc[j].n2.substr(1)) ? stoi(mc[j].n2.substr(1)) : min_idx;
				max_idx = max_idx < stoi(mc[j].n2.substr(1)) ? stoi(mc[j].n2.substr(1)) : max_idx;
			}
			if (mc[j].res[0] == '#') {
				min_idx = min_idx > stoi(mc[j].res.substr(1)) ? stoi(mc[j].res.substr(1)) : min_idx;
				max_idx = max_idx > stoi(mc[j].res.substr(1)) ? stoi(mc[j].res.substr(1)) : max_idx;
			}
		}
		for (int j = blocks[i].start;j <= blocks[i].end;j++) {
			if (mc[j].n1[0] == '#')
				mc[j].n1 = '#' + to_string(stoi(mc[j].n1.substr(1))-min_idx);
			if (mc[j].n2[0] == '#')
				mc[j].n2 = '#' + to_string(stoi(mc[j].n2.substr(1)) - min_idx);
			if (mc[j].res[0] == '#')
				mc[j].res = '#' + to_string(stoi(mc[j].res.substr(1)) - min_idx);
		}
		t_tag = max_idx - min_idx + 1;
		genDAG_expr(i, t_tag);	//�������Ӧ��DAG  ͬʱ�Ѳ����Ż�����Ԫʽ����
		//���µ�����������ջΪ�м�������ٵĿռ�
		st[def_loc].size = st[def_loc].addr+32*4+max(st[def_loc].size-32*4-st[def_loc].addr, t_tag*4);
		//�ҳ������м���
		vector<int> midnode;
		for (int j = 0;j < nodenum;j++) {
			if(dag[j].left!=-1||dag[j].right!=-1)
				midnode.push_back(j);
		}
		vector<int> queue;
		while (midnode.size() > 0) {
			int j;
			for(j=0;j<(int)midnode.size();j++)
				if (judge(midnode[j], queue))
					break;
			int k = judge(midnode[j], queue);
			while (judge(k, queue)) {
				queue.push_back(k);
				int l;
				for (l = 0;l < (int)midnode.size();l++)
					if (midnode[l] == k)
						break;
				midnode.erase(midnode.begin()+l);
				k = dag[k].left;
			}
		}
		
		//�����м����
		map<string, int>::iterator it = nodetable.begin();
		//�Ա���Ҷ��� ��Ҫ��һ���м��������
		for (int j = 0;j < nodenum;j++) {
			int m = dag[j].name.find("#");
			if (m!=-1&&m!=0) {
				genNewmc("ASSIGN", dag[j].name.substr(0, m), "", dag[j].name.substr(m));
			}
		}
		//���ն��е����������µ��м����
		for (int j = (int)queue.size() - 1;j >= 0;j--) {
			it = nodetable.begin();
			while (it != nodetable.end()) {
				if (it->second == j)
					break;
				it++;
			}
			int m = dag[dag[j].left].name.find("#");
			string n1 = m>0? dag[dag[j].left].name.substr(m):dag[dag[j].left].name;
			m = dag[dag[j].right].name.find("#");
			string n2 = m>0? dag[dag[j].right].name.substr(m):dag[dag[j].right].name;
			genNewmc(dag[j].name, n1, n2, it->first);
		}
		blocks[i].end = nmc.size()-1;
	}
}

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