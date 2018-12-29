#include "pch.h"
#include "stdHeader.h"
#include "base_block.h"
#include "globalvar.h"
#include "sign_table.h"
#include "reg_distribution.h"
#include "mips.h"
bool conflictG[MAXSIGNNUM][MAXSIGNNUM];
int name2reg[MAXSIGNNUM] = { 0 };

void cal_ConfG() {
	memset(conflictG, 0, sizeof(conflictG));
	for (int i = 1;i < (int)blocks.size();i++) {
		for (int j = 0;j < stp;j++) {
			for (int k = j + 1;k < stp;k++) {	//in����������ͻ	in��def��ͻ
				if (blocks[i].in[j] && blocks[i].in[k]) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
				if (blocks[i].in[j] && blocks[i].def[k]) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
				if (blocks[i].in[k] && blocks[i].def[j]) {
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
				if (blocks[i].def[j] && blocks[i].def[k]) { //def����������ͻ
					conflictG[j][k] = true;
					conflictG[k][j] = true;
				}
			}
		}
	}
}

//��ͻͼ��ɫ����
void colorAlloc() {
	//�����ͻͼ
	cal_ConfG();
	bool islocal;
	int sregtot = 6;	//����	
	for (int i = 1;i < (int)blocks.size();i++) {	//��ÿ����
		//��һ������
		if (mc[blocks[i].start].op == "LABEL"&&mc[blocks[i].start].n1[0] != '$') {
			//�ҵ�������������п�
			int n = i + 1;
			while (n < (int)blocks.size() && (mc[blocks[n].start].op != "LABEL" || mc[blocks[n].start].n1[0] == '$'))
				n++;
			//��i��n����ҿ���������������Ŀ�
			//ͨ�����ű��ҵ�������������оֲ�����
			string funcname = mc[blocks[i].start].n1 == "main" ? "main" : mc[blocks[i].start].n1.substr(5);
			int loc = search_tab(funcname, islocal, -2);
			int end = loc + 1;
			while (end < stp&&st[end].kind != ST_FUNC)
				end++;
			//ͨ����ͻͼ������ڶȵ�map
			//�ҵ���������Ŀ�����
			bool tmp[MAXSIGNNUM];
			memset(tmp, 0, sizeof(tmp));
			//���еĿ��������ڼ���tmp��
			for (int j = i;j < n;j++) {
				unionset(blocks[j].in, tmp, tmp, stp);
			}
			vector<int> vars;	//�����ı��
			for (int j = loc + 1;j < end;j++)
				if (tmp[j]) 
					vars.push_back(j);
			/*
			cout << mc[blocks[i].start].n1 << "\n";
			for (int j = 0;j < (int)vars.size();j++)
				cout << st[vars[j]].name << " ";
			cout << "\n";
			for (int j = 0;j < vars.size();j++) {
				for (int k =0;k < vars.size();k++)
					cout << conflictG[vars[j]][vars[k]] << " ";
				cout << "\n";
			}*/
			//�������е���Ҫ����ı�������vars��
			map<int, int> loc2deg;	//��¼��Щ�����Ķ�
			for (int j = 0;j < (int)vars.size();j++) {
				loc2deg[vars[j]] = 0;	//���������
				for (int k = 0;k < (int)vars.size();k++)	//ͳ�ƶ�
					if (conflictG[vars[j]][vars[k]])
						loc2deg[vars[j]]++;
			}
			//��ʼ����
			stack<int> rec;
			int lasttime = 0;
			while (vars.size() > 1) {
				do {
					lasttime = rec.size();	//һֱ���µ��޷���ȥ���µĵ�
					for (int j = 0;j < (int)vars.size() && vars.size()>1;j++) {
						if (loc2deg[vars[j]] < sregtot) {
							//���¶�
							for (int x = 0;x < (int)vars.size();x++)
								if (conflictG[vars[j]][vars[x]])
									loc2deg[vars[x]]--;
							rec.push(vars[j]);	//�����ѹջ
							vars.erase(vars.begin() + j);	//��ͼ��ɾ��
							j--;
						}
					}
				} while (rec.size() != lasttime && vars.size() > 1);
				if (vars.size() >= 2) {	//�����ʱͼ�л��г���������
					//�Ҷ����ĵ�
					int max = -1;
					int not_alloc = -1;
					for (int x = 0;x < (int)vars.size();x++)
						if (max < loc2deg[vars[x]]) {
							max = loc2deg[vars[x]];
							not_alloc = x;
						}
					//���Ϊ������
					name2reg[vars[not_alloc]] = -1;
					//���¶�
					for (int x = 0;x < (int)vars.size();x++) {
						if (conflictG[vars[not_alloc]][vars[x]])
							loc2deg[vars[x]]--;
					}
					vars.erase(vars.begin() + not_alloc);	//��ͼ��ɾ��
				}
			}
			//��ɫ
			if (vars.size() == 0)
				continue;
			name2reg[vars[0]] = 16;	//���ʣ���Ǹ������Ĵ���
			int noUse[6];
			while (!rec.empty()) {	//������� ÿ���һ�� �ͷ���һ���Ĵ���
				memset(noUse, 0, sizeof(noUse));
				int t = rec.top();	//��ջ
				rec.pop();
				for (int x = 0;x < (int)vars.size();x++)	//������еı���
					if (conflictG[t][vars[x]] && name2reg[vars[x]] > 0)	//�������ͻ�˲����˼ҷ�����
						noUse[name2reg[vars[x]] - 16] = true;	//����Ĵ���������
				for (int x = 0;x < 6;x++)
					if (!noUse[x]) {
						name2reg[t] = x + 16;	//�����һ������ �Ǿͷ���
						vars.push_back(t);
						break;
					}
			}
		}
	}
}
//���ص�˳�����
void inOrderAlloc() {
	int sregcnt = 16;	//��� ��16-21
	int func_begin, func_end;
	bool islocal;
	for (int i = 1;i < (int)blocks.size() - 1;i++) {
		//���������һ�������Ŀ�ʼ ��ô���Դ�ͷ����Ĵ���
		if (mc[blocks[i].start].op == "LABEL"&&mc[blocks[i].start].n1[0] != '$') {
			sregcnt = 16;
			func_begin = search_tab(mc[blocks[i].start].n1 == "main" ? "main" : mc[blocks[i].start].n1.substr(5), islocal, -2);
			func_end = func_begin + 1;
			while (func_end < stp&&st[func_end].kind != ST_FUNC)
				func_end++;	//һ�������ڷ��ű��еķ�Χ ����ҿ�����
		}
		for (int j = func_begin;j < func_end;j++) {
			//���ĳ������������������in����
			if (blocks[i].in[j]) {
				//�мĴ������Ҵ˱���û����� �����
				if (sregcnt <= 21 && name2reg[j] == 0)
					name2reg[j] = sregcnt++;
				//���û�мĴ��������������Ҫ���䵫��ȴ�޷����䣬���Ϊ-1
				else if (sregcnt > 21 && name2reg[j] == 0)
					name2reg[j] = -1;
			}
		}
	}
}
void cal_alloc() {
	//ȫ�ֱ�������
	/*
	int i = 0, cnt = CHOOSEA;
	int regNo = 4 + CHOOSEA;
	while (i < stp&&cnt < 4 && st[i].kind != ST_FUNC) {
		if (st[i].kind == ST_VAR) {
			name2reg[i] = regNo++;
			cnt++;
		}
		i++;
	}
	int i = 0, cnt = 0;
	int regNo = 22;
	while (i < stp&&cnt < 1 && st[i].kind != ST_FUNC) {
		if (st[i].kind == ST_VAR) {
			name2reg[i] = regNo++;
			cnt++;
		}
		i++;
	}*/
	colorAlloc();
	//inOrderAlloc();
}
