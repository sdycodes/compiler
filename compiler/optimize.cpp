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
	for (int i = begin;i <= end;i++) {	//对每一条指令 
		if (mc[i].op == "NULL" || mc[i].n1 == "" || mc[i].n1 == "" || mc[i].n2 == "" || mc[i].res == "")
			continue;	//形如z=x op y
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
		//至此 rec1记录了n1的编号
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
		//至此 rec2记录了n1的编号
		//在dag图中寻找中间结点
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
		//至此rec3为中间结点编号
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
	for (int i = 0;i < (int)blocks.size();i++) {	//对每个块
		genDAG_expr(i);	//生成其对应的DAG
		
	}
}

void kk_opt() {
	for (int i = 0;i < (int)mc.size() - 1;i++) {
		//优化1 OUTS OUTC合并成一个
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
		//优化2 GOTO到下一个label之间的代码删除
		else if (mc[i].op == "GOTO") {
			int j = i + 1;
			while (mc[j].op != "LABEL") {
				mc[j].op = "NULL";
				j++;
			}
			i = j-1;
		}
		//优化3 先LELEM后SELEM一样可以去除
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
		//减立即数是伪指令翻译会多一条
		if (mp[i].op == "subi") {
			mp[i].op = "addi";
			mp[i].n2 = to_string(-stoi(mp[i].n2));
		}
		//对返回值的特定优化
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