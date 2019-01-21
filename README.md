# Compiler
BUAA C0 compiler source code
## Enviroments and Language
Visual Studio 2017 c++
## How to Use
Input the full path of the source code.
The result in mips code will be in the directory compiler/results.
## C0 Grammar
1) ＜加法运算符＞ ::= +｜-
2) ＜乘法运算符＞  ::= *｜/
3) ＜关系运算符＞  ::=  <｜<=｜>｜>=｜!=｜==
4) ＜字母＞   ::= ＿｜a｜．．．｜z｜A｜．．．｜Z
5) ＜数字＞   ::= ０｜＜非零数字＞
6) ＜非零数字＞  ::= １｜．．．｜９
7) ＜字符＞    ::=  '＜加法运算符＞'｜'＜乘法运算符＞'｜'＜字母＞'｜'＜数字＞'
8) ＜字符串＞   ::=  "｛十进制编码为32,33,35-126的ASCII字符｝"                              
9) ＜程序＞    ::= ［＜常量说明＞］［＜变量说明＞］{＜有返回值函数定义＞|＜无返回值函数定义＞}＜主函数＞
10) ＜常量说明＞ ::=  const＜常量定义＞;{ const＜常量定义＞;}
11) ＜常量定义＞   ::=   int＜标识符＞＝＜整数＞{,＜标识符＞＝＜整数＞}
                            | char＜标识符＞＝＜字符＞{,＜标识符＞＝＜字符＞}
12) ＜无符号整数＞  ::= ＜非零数字＞｛＜数字＞｝｜０
13) ＜整数＞        ::= ［＋｜－］＜无符号整数＞
14) ＜标识符＞    ::=  ＜字母＞｛＜字母＞｜＜数字＞｝
15) ＜声明头部＞   ::=  int＜标识符＞|char＜标识符＞
16) ＜变量说明＞  ::= ＜变量定义＞;{＜变量定义＞;}
17) ＜变量定义＞  ::= ＜类型标识符＞(＜标识符＞|＜标识符＞'['＜无符号整数＞']'){,(＜标识符＞|＜标识符＞'['＜无符号整数＞']') } //＜无符号整数＞表示数组元素的个数，其值需大于0
18) ＜类型标识符＞      ::=  int | char
19) ＜有返回值函数定义＞  ::=  ＜声明头部＞'('＜参数表＞')' '{'＜复合语句＞'}'
20) ＜无返回值函数定义＞  ::= void＜标识符＞'('＜参数表＞')''{'＜复合语句＞'}'
21) ＜复合语句＞   ::=  ［＜常量说明＞］［＜变量说明＞］＜语句列＞
22) ＜参数表＞    ::=  ＜类型标识符＞＜标识符＞{,＜类型标识符＞＜标识符＞}| ＜空＞
23) ＜主函数＞    ::= void main'('')' '{'＜复合语句＞'}'
24) ＜表达式＞    ::= ［＋｜－］＜项＞{＜加法运算符＞＜项＞}   //[+|-]只作用于第一个<项>
25) ＜项＞     ::= ＜因子＞{＜乘法运算符＞＜因子＞}
26) ＜因子＞    ::= ＜标识符＞｜＜标识符＞'['＜表达式＞']'｜＜整数＞|＜字符＞｜＜有返回值函数调用语句＞|'('＜表达式＞')'
27) ＜语句＞    ::= ＜条件语句＞｜＜循环语句＞| '{'＜语句列＞'}'｜＜有返回值函数调用语句＞; |＜无返回值函数调用语句＞;｜＜赋值语句＞;｜＜读语句＞;｜＜写语句＞;｜＜空＞;｜＜返回语句＞;
28) ＜赋值语句＞   ::=  ＜标识符＞＝＜表达式＞|＜标识符＞'['＜表达式＞']'=＜表达式＞
29) ＜条件语句＞  ::=  if '('＜条件＞')'＜语句＞［else＜语句＞］
30) ＜条件＞    ::=  ＜表达式＞＜关系运算符＞＜表达式＞｜＜表达式＞ //表达式为0条件为假，否则为真
31) ＜循环语句＞   ::=  do＜语句＞while '('＜条件＞')' |for'('＜标识符＞＝＜表达式＞;＜条件＞;＜标识符＞＝＜标识符＞(+|-)＜步长＞')'＜语句＞
32) ＜步长＞::= ＜无符号整数＞  
33) ＜有返回值函数调用语句＞ ::= ＜标识符＞'('＜值参数表＞')'
34) ＜无返回值函数调用语句＞ ::= ＜标识符＞'('＜值参数表＞')'
35) ＜值参数表＞   ::= ＜表达式＞{,＜表达式＞}｜＜空＞
36) ＜语句列＞   ::=｛＜语句＞｝
37) ＜读语句＞    ::=  scanf '('＜标识符＞{,＜标识符＞}')'
38) ＜写语句＞::= printf'('＜字符串＞,＜表达式＞')'|printf '('＜字符串＞')'|printf '('＜表达式＞')'
39) ＜返回语句＞   ::=  return['('＜表达式＞')']
40) 有关说明如下：  
（1）char类型的变量或常量，用字符的ASCII码对应的整数参加运算。  
（2）标识符区分大小写字母。  
（3）写语句中，字符串原样输出，单个字符类型的变量或常量输出字符，其他表达式按整型输出。  
（4）数组的下标从0开始。  
