# 语法分析程序的设计与实现



## 环境

1. win10
2. vscode
3. mingw



## 综述

共实现了三种语法分析程序，即LL1、SLR1和LR1语法分析程序，对满足条件的输入文法能够自动的生成该文法的语法分析程序并执行分析过程，输出所采用的产生式。

对于LL1语法分析程序有以下功能（要求输入文法为LL1文法）

- 自动构建FIRST集
- 自动构建FOLLOW集
- 自动构建预测分析表
- 执行分析程序分析输入串
- 输出所采用的产生式

对于SLR1分析程序有以下功能（要求输入文法为SLR1文法）

- 自动构建FIRST集
- 自动构建FOLLOW集
- 自动构建有效项目集规范族和DFA
- 自动构建SLR1分析表
- 执行分析程序分析输入串
- 输出所采用的产生式

对于LR1分析程序有以下功能（要求输入文法为LR1文法）

- 自动构建FIRST集
- 自动构建有效项目集规范族和DFA
- 自动构建LR1分析表
- 执行分析程序分析输入串
- 输出所采用的产生式



三种语法分析程序均要求输入的文法符号为**单个**字符，请将原文法符号不是单个字符自行更换为单文法符号，如S'更换为A，id更换为n，以此类推。具体输入格式见每个程序测试部分的输入样例。



## LL1语法分析程序

### 数据结构

```cpp
/* 产生式结构体 */
struct Production {
    char left; // 左部符号
    vector<char> rigths;  // 右部符号串
};

/* 文法结构体 */
struct Grammar {
    int num;  // 产生式数量
    vector<char> T;   // 终结符集
    vector<char> N;   // 非终结符集
    vector<Production> prods;  //产生式集
} grammar;

/* FIRST集和FOLLOW集 */
map<char, set<char> > first;
map<char, set<char> > follow;

/* 分析栈 */
stack<char> ST;

/* 待分析串 */
string str;

/* 预测分析表 */
vector<char> M[50][50];
```

### 辅助函数

```cpp
/* 判断ch是否是终结符, 若是返回所在位置 */
int isInT(char ch);

/* 判断ch是否是非终结符, 若是返回所在位置 */
int isInN(char ch);

/* 读入并初始化语法 */
void initGrammar();
    
/* 把生成式插入到预测分析表对应的项中 */
void insertTOForecastAnalysisTable(char A, char a, Production &P);

/* 取出预测分析表对应的项中的产生式 */
void getFromForecastAnalysisTable(char A, char a, vector<char> &s);

```

### 构建FIRST集和FOLLOW集

#### 求单个文法符号的FIRST集

![image-20191126143323246](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126143323246.png)

```cpp
/* 求(T U N)的FIRST集 */
void getFirstSet()
{
    /* 终结符的FIRST集是其本身 */
    for (int i = 0; i < grammar.T.size(); i++) {
        char X = grammar.T[i];
        set<char> tmp;
        tmp.insert(X);
        first[X] = tmp;
    }
    /* 当非终结符的FIRST集发生变化时循环 */
    bool change = true;
    while (change) {
        change = false;
        /* 枚举每个产生式 */
        for (int i = 0; i < grammar.prods.size(); i++) {
            Production &P = grammar.prods[i];
            char X = P.left;
            set<char> &FX = first[X];
            /* 如果右部第一个符号是空或者是终结符，则加入到左部的FIRST集中 */
            if (isInT(P.rigths[0]) || P.rigths[0] == '&') {
                /* 查找是否FIRST集是否已经存在该符号 */
                auto it = FX.find(P.rigths[0]);
                /* 不存在 */
                if (it == FX.end()) {
                    change = true; // 标注FIRST集发生变化，循环继续
                    FX.insert(P.rigths[0]);
                }
            } else {
                /* 当前符号是非终结符，若当前符号可以推出空，则还需判断下一个符号 */
                bool next = true;
                /* 待判断符号的下标 */
                int idx = 0;
                while (next && idx < P.rigths.size()) {
                    next = false;
                    char Y = P.rigths[idx];
                    set<char> &FY = first[Y];
                    for (auto it = FY.begin(); it != FY.end(); it++) {
                        /* 把当前符号的FIRST集中非空元素加入到左部符号的FIRST集中 */
                        if (*it != '&') {
                            auto itt = FX.find(*it);
                            if (itt == FX.end()) {
                                change = true;
                                FX.insert(*it);
                            }
                        }
                    }
                    /* 当前符号的FIRST集中有空, 标记next为真，idx下标+1 */
                    auto it = FY.find('&');
                    if (it != FY.end()) {
                        next = true;
                        idx = idx + 1;
                    }
                }
            }
        }
    }
}
```

#### 求一串文法符号的FIRST集

要求FIRST($\alpha$)，先将$\alpha$的第一个文法符号的FIRST集加入到加入到FIRST($\alpha$)，若当前符号的FIRST集含空，则继续将下一个文法符号的FIRST加入到FIRST($\alpha$)中，直至末尾。

```cpp
/* 产找alpha串的FIRST集， 保存到FS集合中 */
void getFirstByAlphaSet(vector<char> &alpha, set<char> &FS)
{
    /* 当前符号是非终结符，若当前符号可以推出空，则还需判断下一个符号 */
    bool next = true;
    int idx = 0;
    while (idx < alpha.size() && next) {
        next = false;
        /* 当前符号是终结符或空，加入到FIRST集中 */
        if (isInT(alpha[idx]) || alpha[idx] == '&') {
            /* 判断是否已经在FIRST集中 */
            auto itt = FS.find(alpha[idx]);
            if (itt == FS.end()) {
                FS.insert(alpha[idx]);
            }
        } else {
            char B = alpha[idx];
            set<char> &FB = first[B];
            for (auto it = first[B].begin(); it != first[B].end(); it++) {
                /* 当前符号FIRST集包含空，标记next为真，并跳过当前循环 */
                if (*it == '&') {
                    next = true;
                    continue;
                }
                /* 把非空元素加入到FIRST集中 */
                auto itt = FS.find(*it);
                if (itt == FS.end()) {
                    FS.insert(*it);
                }
            }
        }
        idx = idx + 1;
    }
    /* 如果到达产生式右部末尾next还为真，说明alpha可以推空，将空加入到FIRST集中 */
    if (next) {
        FS.insert('&');
    }
}
```

#### 求非终结符的FOLLOW集

![image-20191126144856856](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126144856856.png)

```cpp
/* 求非终结符的FOLLOW集 */
void getFollowSet()
{
    /* 初始化终结符的FOLLOW集为空集 */
    for (int i = 0; i < grammar.N.size(); i++) {
        char B = grammar.N[i];
        follow[B] = set<char>();
    }
    /* 将$加入到文法的开始符号的FOLLOW集中 */
    char S = grammar.N[0];
    follow[S].insert('$');

    bool change = true;
    while (change) {
        change = false;
        /* 枚举每个产生式 */
        for (int i = 0; i < grammar.prods.size(); i++) {
            Production &P = grammar.prods[i];
            for (int j = 0; j < P.rigths.size(); j++) {
                char B = P.rigths[j];
                /* 当前符号是非终结符 */
                if (isInN(B)) {
                    set<char> &FB = follow[B];
                    set<char> FS;
                    /* alpha是从当前符号下一个符号开始的符号串 */
                    vector<char> alpha(P.rigths.begin() + j + 1, P.rigths.end());
                    /* 求alpha的FIRST集，即FS */
                    getFirstByAlphaSet(alpha, FS);
                    /* 将alpha的FIRST集中所有非空元素加入到当前符号的FOLLOW集中 */
                    for (auto it = FS.begin(); it != FS.end(); it++) {
                        if (*it == '&') {
                            continue;
                        }
                        auto itt = FB.find(*it);
                        if (itt == FB.end()) {
                            change = true;
                            FB.insert(*it);
                        }
                    }
                    /* 如果alpha能推空，或者当前符号是产生式右部末尾，则将文法左部符号的FOLLOW集加入到当前符号的FOLLOW集中 */
                    auto itt = FS.find('&');
                    if (itt != FS.end() || (j + 1) >= P.rigths.size()) {
                        char A = P.left; //A为左部符号
                        for (auto it = follow[A].begin(); it != follow[A].end(); it++) {
                            auto itt = FB.find(*it);
                            if (itt == FB.end()) {
                                change = true;
                                FB.insert(*it);
                            }
                        }    
                    }
                }
            }
        }
    }
}
```

### 构建预测分析表

![image-20191126145227507](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126145227507.png)

```cpp
/* 构建预测分析表 */
void productForecastAnalysisTable()
{
    /* 枚举所有产生式 */
    for (int i = 0; i < grammar.prods.size(); i++) {
        /* 假设P为 A->alpha */
        Production &P = grammar.prods[i];
        set<char> FS;
        /* 对每个 a in FIRST(alpha) 把 A->alpha放入M[A, a]中 */
        getFirstByAlphaSet(P.rigths, FS);
        for (auto it = FS.begin(); it != FS.end(); it++) {
            insertTOForecastAnalysisTable(P.left, *it, P);
        }
        /* 如果alpha能推空，则把每个b in FOLLOW(A) 把 A->alpha放入M[A, b]中*/
        auto itt = FS.find('&');
        if (itt != FS.end()) {
            for (auto it = follow[P.left].begin(); it != follow[P.left].end(); it++) {
                insertTOForecastAnalysisTable(P.left, *it, P);
            }
        }
    }
}
```



### 分析程序

![image-20191126145547747](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126145547747.png)

```cpp
/* 分析程序 */
void process()
{
    /* 指向当前字符 */
    int ip = 0;
    /* 栈顶符号X， 和当前输入符号a */
    char X, a;
    printf("The answer:\n");
    do{
        X = ST.top();
        a = str[ip];
        /* 如果是终结符或者$ */
        if (isInT(X)) {
            /* 如果栈顶符号和当前符号匹配，出栈，指针前移 */
            if (X == a) {
                ST.pop();
                ip = ip + 1;
            } else { /* 不匹配报错 */
                printf("error1\n");
            }
        } else {    //非终结符
            vector<char> s;
            /* 取出对应预测分析表的项 */
            getFromForecastAnalysisTable(X, a, s);
            /* 预测分析表项中有元素 */
            if (!s.empty()) {
                /* 弹栈并将右部符号串逆序入栈 */
                ST.pop();
                for (int i = s.size() - 1; i >= 3; i--) {
                    if (s[i] != '&') { // 为空时不入栈
                        ST.push(s[i]);
                    }
                }
                /* 输出产生式 */
                for (int i = 0; i < s.size(); i++) {
                        printf("%c", s[i]);
                }
                printf("\n");
            } else { // 空，报错
                printf("error2\n");
            }
        }
    } while (X != '$');
}
```



### 测试

#### 编译源程序

```shell
g++ -o LL1 LL1.cpp
```

#### 执行LL1分析程序

```shell
.\LL1.exe
```

输入一下内容

```
10
E->TA
A->+TA
A->-TA
A->&
T->FB
B->*FB
B->/FB
B->&
F->(E)
F->n
E T A F B #
n + - * / ( ) #
(n+n)*n-n/n
```

#### 查看输出结果

##### FIRST集和FOLLOW集

```
FIRST:
E: ( n
T: ( n
A: & + -
F: ( n
B: & * /
FOLLOW:
E: $ )
T: $ ) + -
A: $ )
F: $ ) * + - /
B: $ ) + -
```

##### 预测分析表

![image-20191126151740466](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126151740466.png)

##### 输出产生式

```
The answer:
E->TA
T->FB
F->(E)
E->TA
T->FB
F->n
B->&
A->+TA
T->FB
F->n
B->&
A->&
B->*FB
F->n
B->&
A->-TA
T->FB
F->n
B->/FB
F->n
B->&
A->&
```

LL1输出**最左推导**，下面验证其正确性。

E=>TA=>FBA=>(E)BA=>(TA)BA=>(FBA)BA=>(nBA)BA=>(nA)BA=>(n+TA)BA=>(n+FBA)BA=>(n+nBA)BA=>(n+nA)BA=>(n+n)BA=>(n+n)*FBA=>(n+n)\*nBA=>(n+n)\*nA=>(n+n)\*n-TA=>(n+n)\*n-FBA=>(n+n)\*n-nBA=>(n+n)\*n-n/FBA=>(n+n)\*n-n/nBA=>(n+n)\*n-n/nA=>(n+n)\*n-n/n

可以看到，从输出的产生式可以**最左推导**出我们输入的分析串。

经验证，上述所求的FIRST集、FOLLOW集、预测分析表和输出的产生式均正确。LL1语法分析程序工作正常。



## SLR1语法分析程序



### 数据结构

```cpp
/* 产生式结构体，左部符号和右部符号串 */
struct Production {
    char left;
    vector<char> rigths;
};

/* LR0项目 */
struct LR0Item {
    Production p;
    /* 点的位置 */
    int location;
};
/* LR0项目集 */
struct LR0Items {
    vector<LR0Item> items;
};
/* LR0项目集规范族 */
struct CanonicalCollection {
    /* 项目集集合 */
    vector<LR0Items> items;
    /* 保存DFA的图，first为转移到的状态序号，second是经什么转移 */
    vector< pair<int, char> > g[100];
}CC;

/* 文法结构体 */
struct Grammar {
    int num;  // 产生式数量
    vector<char> T;   // 终结符
    vector<char> N;   // 非终结符
    vector<Production> prods;  //产生式
}grammar;

/* FIRST集和FOLLOW集 */
map<char, set<char> > first;
map<char, set<char> > follow;

/* DFA队列， 用于存储待转移的有效项目集 */
queue< pair<LR0Items, int> > Q;

/* action表和goto表 */
pair<int, int> action[100][100]; // first表示分析动作，0->ACC 1->S 2->R second表示转移状态或者产生式序号
int goton[100][100];

/* 待分析串 */
string str;
/* 分析栈 */
stack< pair<int, char> > ST; // first是state，second 是symble
```



### 辅助函数

```cpp
/* 判断ch是否是非终结符, 若是返回其序号 */
int isInN(char ch);

/* 判断ch是否是终结符, 若是返回其序号 */
int isInT(char ch);

/* 求(T U N)的FIRST集 */
void getFirstSet();

/* 产找alpha串的FIRST集， 保存到FS集合中 */
void getFirstByAlphaSet(vector<char> &alpha, set<char> &FS);

/* 求非终结符的FOLLOW集 */
void getFollowSet();

/* 判断是LR0项目t否在有效项目集I中 */
bool isInLR0Items(LR0Items &I, LR0Item &t);

/* 打印某个项目集 */
void printLR0Items(LR0Items &I);

/* 判断是否在项目集规范族中，若在返回序号 */
int isInCanonicalCollection(LR0Items &I);

/* 读入并初始化语法 */
void initGrammar();
```



### 计算闭包

![image-20191126183442960](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126183442960.png)

```cpp
/* 求I的闭包 */
void closure(LR0Items &I)
{
    bool change =  true;
    while (change) {
        change = false;
        LR0Items J;
        /* 枚举每个项目 */
        J.items.assign(I.items.begin(), I.items.end());
        for (auto it = J.items.begin(); it != J.items.end(); it++) {
            LR0Item &L = *it;
            /* 非规约项目 */
            if (L.location < L.p.rigths.size()) {
                char B = L.p.rigths[L.location];
                if (isInN(B)) {
                    /* 把符合条件的LR0项目加入闭包中 */
                    for (int i = 0; i < grammar.prods.size(); i++) {
                        Production &P = grammar.prods[i];
                        if (P.left == B) {
                            LR0Item t;
                            t.location = 0;
                            t.p.left = P.left;
                            t.p.rigths.assign(P.rigths.begin(), P.rigths.end());
                            if (!isInLR0Items(I, t)) {
                                /* 标记改变 */
                                change = true;
                                I.items.push_back(t);
                            } 
                        } 
                    }
                }
            }
        }
    }
}
```



### 计算转移

![image-20191126183810269](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126183810269.png)

```cpp
/* 转移函数，I为当前的项目集，J为转移后的项目集, 经X转移 */
void go(LR0Items &I, char X, LR0Items &J)
{
    for (auto it = I.items.begin(); it != I.items.end(); it++) {
        LR0Item &L = *it;
        /* 非规约项目 */
        if (L.location < L.p.rigths.size()) {
            char B = L.p.rigths[L.location];
            /* 如果点后面是非终结符，且非终结符为X，点位置加1, 加入到转移项目集中*/
            if (B == X) {
                LR0Item t;
                t.location = L.location + 1;
                t.p.left = L.p.left;
                t.p.rigths.assign(L.p.rigths.begin(), L.p.rigths.end());
                J.items.push_back(t);
            }
        }
    }
    /* 若J中有项目，则求其闭包 */
    if (J.items.size() > 0) {
        closure(J);
    }
}
```



### 构建有效项目集规范族和DFA

![image-20191126184218556](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126184218556.png)

```cpp
/* 构建DFA和项目集规范族 */
void DFA()
{
    /* 构建初始项目集 */
    LR0Item t;
    t.location = 0;
    t.p.left = grammar.prods[0].left;
    t.p.rigths.assign(grammar.prods[0].rigths.begin(), grammar.prods[0].rigths.end());
    LR0Items I;
    I.items.push_back(t);
    closure(I);
    /* 加入初始有效项目集 */
    CC.items.push_back(I);
    /* 把新加入的有效项目集加入待扩展队列中 */
    Q.push(pair<LR0Items, int>(I, 0));
    while (!Q.empty()) {
        LR0Items &S = Q.front().first;
        int sidx = Q.front().second;
        /* 遍历每个终结符 */
        for (int i = 0; i  < grammar.T.size(); i++) {
            LR0Items D;
            go(S, grammar.T[i], D);
            int idx;
            /* 若不为空 */
            if (D.items.size() > 0) {
                /* 查找是否已经在有效项目集族里 */
                idx = isInCanonicalCollection(D); 
                if (idx > 0) {
                    idx = idx - 1;
                } else {
                    idx = CC.items.size();
                    CC.items.push_back(D);
                    /* 把新加入的有效项目集加入待扩展队列中 */
                    Q.push(pair<LR0Items, int>(D, idx));
                }
                /* 从原状态到转移状态加一条边，边上的值为转移符号 */
                CC.g[sidx].push_back(pair<char, int>(grammar.T[i], idx));
            }
        }
        /* 遍历每个非终结符 */
        for (int i = 0; i  < grammar.N.size(); i++) {
            LR0Items D;
            go(S, grammar.N[i], D);
            int idx;
            if (D.items.size() > 0) {
                /* 查找是否已经在有效项目集族里 */
                idx = isInCanonicalCollection(D); 
                if (idx != 0) {
                    idx = idx - 1;
                } else {
                    idx = CC.items.size();
                    CC.items.push_back(D);
                    /* 把新加入的有效项目集加入待扩展队列中 */
                    Q.push(pair<LR0Items, int>(D, idx));
                }
                /* 从原状态到转移状态加一条边，边上的值为转移符号 */
                CC.g[sidx].push_back(pair<char, int>(grammar.N[i], idx));
            }
        }
    /* 当前状态扩展完毕，移除队列*/
    Q.pop();
    }
}
```



### 生成分析表(SLR(1))

![image-20191126184647328](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126184647328.png)

```cpp
/* 生成SLR1分析表 */
void productSLR1AnalysisTabel()
{
    for (int i = 0; i < CC.items.size(); i++) {
        LR0Items &LIt= CC.items[i];
        /* 构建action表 */
        for (auto it = LIt.items.begin(); it != LIt.items.end(); it++) {
            LR0Item &L = *it;
            /* 非规约项目 */
            if (L.location < L.p.rigths.size()) {
                char a = L.p.rigths[L.location];
                int j = isInT(a);
                /* a是终结符 */
                if (j > 0) {
                    j = j - 1;
                    /* 找到对应a的出边，得到其转移到的状态 */
                    for (int k = 0; k < CC.g[i].size(); k++) {
                        pair<char, int> p = CC.g[i][k];
                        if (p.first == a) {
                            action[i][j].first = 1; // 1->S
                            action[i][j].second = p.second;  //转移状态
                            break;
                        }
                    }
                }
            } else { // 规约项目
                /* 接受项目 */
                if (L.p.left == grammar.prods[0].left) {
                    action[i][grammar.T.size() - 1].first = 3;
                } else {
                    char A = L.p.left;
                    for (auto a = follow[A].begin(); a != follow[A].end(); a++) {
                        int j = isInT(*a);
                        /* 终结符 */
                        if (j > 0) {
                            j = j - 1;
                            /* 找到产生式对应的序号 */
                            for (int k = 0; k < grammar.prods.size(); k++) {
                                if (L.p == grammar.prods[k]) {
                                    action[i][j].first = 2;
                                    action[i][j].second = k;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        /* 构建goto表 */
        for (int k = 0; k < CC.g[i].size(); k++) {
            pair<char, int> p = CC.g[i][k];
            char A = p.first;
            int j = isInN(A);
            /* 终结符 */
            if (j > 0) {
                j = j - 1;
                goton[i][j] = p.second; //转移状态
            }
        }
    }
}
```



### 分析程序

![image-20191126185427849](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126185427849.png)

```cpp
/* 分析程序 */
void process()
{
    int ip = 0;
    printf("The ans:\n");
    do {
        int s = ST.top().first;
        char a = str[ip];
        int j = isInT(a) - 1;
        /* 移进 */
        if (action[s][j].first == 1) {
            ST.push(pair<int, char>(action[s][j].second, a));
            ip = ip + 1;
        } else if (action[s][j].first == 2) { // 规约
            Production &P = grammar.prods[action[s][j].second];
            /* 弹出并输出产生式 */
            printf("%c->", P.left);
            for (int i = 0; i < P.rigths.size(); i++) {
                ST.pop();
                printf("%c", P.rigths[i]);
            }
            printf("\n");
            s = ST.top().first;
            char A = P.left;
            j = isInN(A) - 1;
            ST.push(pair<int, char>(goton[s][j], A));
        } else if (action[s][j].first == 3) {   //接受
            printf("ACC\n");
            return;
        } else {
            printf("error\n");
        }
    } while(1);
}
```



### 测试

#### 编译源程序

```shell
g++ -o SLR1 SLR1.cpp
```

#### 执行SLR1分析程序

```
.\SLR1.exe
```

输入以下内容

```
9
A->E
E->E+T
E->E-T
E->T
T->T*F
T->T/F
T->F
F->(E)
F->n
A E T F #
n + - * / ( ) #
(n+n)*n-n/n
```

#### 查看输出结果

##### FIRST和FOLLOW集

```
FIRST:
A: ( n 
E: ( n 
T: ( n 
F: ( n 
FOLLOW:
A: $ 
E: $ ) + - 
T: $ ) * + - / 
F: $ ) * + - / 
```

##### 查看项目集规范族和DFA

```
CC size: 16
LR0Items 0:
A->.E E->.E+T E->.E-T E->.T T->.T*F T->.T/F T->.F F->.(E) F->.n
to 1 using n
to 2 using (
to 3 using E
to 4 using T
to 5 using F
LR0Items 1:
F->n.
LR0Items 2:
F->(.E) E->.E+T E->.E-T E->.T T->.T*F T->.T/F T->.F F->.(E) F->.n
to 1 using n
to 2 using (
to 6 using E
to 4 using T
to 5 using F
LR0Items 3:
A->E. E->E.+T E->E.-T
to 7 using +
to 8 using -
LR0Items 4:
E->T. T->T.*F T->T./F
to 9 using *
to 10 using /
LR0Items 5:
T->F.
LR0Items 6:
F->(E.) E->E.+T E->E.-T
to 7 using +
to 8 using -
to 11 using )
LR0Items 7:
E->E+.T T->.T*F T->.T/F T->.F F->.(E) F->.n
to 1 using n
to 2 using (
to 12 using T
to 5 using F
LR0Items 8:
E->E-.T T->.T*F T->.T/F T->.F F->.(E) F->.n 
to 1 using n
to 2 using (
to 13 using T
to 5 using F
LR0Items 9:
T->T*.F F->.(E) F->.n
to 1 using n
to 2 using (
to 14 using F
LR0Items 10:
T->T/.F F->.(E) F->.n
to 1 using n
to 2 using (
to 15 using F
LR0Items 11:
F->(E).
LR0Items 12:
E->E+T. T->T.*F T->T./F 
to 9 using *
to 10 using /
LR0Items 13:
E->E-T. T->T.*F T->T./F
to 9 using *
to 10 using /
LR0Items 14:
T->T*F.
LR0Items 15:
T->T/F.
```

##### 查看分析表

![image-20191126190440849](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191126190440849.png)

##### 查看输出产生式

```
The ans:
F->n
T->F
E->T
F->n
T->F
E->E+T
F->(E)
T->F
F->n
T->T*F
E->T
F->n
T->F
F->n
T->T/F
E->E-T
ACC
```

LR输出的产生式是最右推导的逆序列，下面验证其正确性。

E=>E-T=>E-T/F=>E-T/n=>E-F/n=>E-n/n=>T-n/n=>T*F-n/n=>T\*n-n/n=>F\*n-n/n=>(E)\*n-n/n=>(E+T)\*n-n/n=>(E+F)\*n-n/n=>(E+n)\*n-n/n=>(T+n)\*n-n/n=>(F+n)\*n-n/n=>(n+n)\*n-n/n

可以看出，上述产生式的确是**最右推导**的逆序列，所以其是正确的。

经验证，程序自动构建的有效项目集规范族和DFA均正确，其分析表亦正确，对给定的输出串分析输出的产生式验证也正确。



## LR1语法分析程序

仅仅对上面的SLR1语法分析程序做出少量的修改即可实现LR1语法分析程序。

### 添加向前看符号

```cpp
/* LR1项目 */
struct LR1Item {
    Production p;
    /* 点的位置 */
    int location;
    /* 向前看符号 */
    char next;
};
```



### 修改相关函数

#### 闭包

![image-20191223184130454](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191223184130454.png)

#### 转移函数

![image-20191223184234241](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191223184234241.png)

#### 构造分析表

主要改动如下

![image-20191223184521125](%E8%AF%AD%E6%B3%95%E5%88%86%E6%9E%90.assets/image-20191223184521125.png)

上述函数的实现见源代码，相对于SLR1的改动不多。



### 测试

#### 编译源文件

```shell
g++ -o LR1 LR1.cpp
```

#### 执行LR1分析 程序

```
.\LR1.exe
```

输入一下内容(LR1有效项目集族状态 较多，此处给出一个状态相对较少的例子)

```
4
A->S
S->CC
C->cC
C->d
A S C #
c d #
cccdcd
```

#### 查看输出结果

##### FIRST集

```
FIRST:
A: c d
S: c d
C: c d
```

##### 查看项目集规范族和DFA

```
CC size: 10
LR1Items 0:
A->.S,$   S->.CC,$   C->.cC,c   C->.cC,d   C->.d,c   C->.d,d
to 1 using c
to 2 using d
to 3 using S
to 4 using C
LR1Items 1:
C->c.C,c   C->c.C,d   C->.cC,c   C->.d,c   C->.cC,d   C->.d,d
to 1 using c
to 2 using d
to 5 using C
LR1Items 2:
C->d.,c   C->d.,d
LR1Items 3:
A->S.,$
LR1Items 4:
S->C.C,$   C->.cC,$   C->.d,$
to 6 using c
to 7 using d
to 8 using C
LR1Items 5:
C->cC.,c   C->cC.,d
LR1Items 6:
C->c.C,$   C->.cC,$   C->.d,$
to 6 using c
to 7 using d
to 9 using C
LR1Items 7:
C->d.,$
LR1Items 8:
S->CC.,$
LR1Items 9:
C->cC.,$
```

##### 查看分析表

```
        action                  goto
        c       d       $       |       S       C
0       S1      S2              |       3       4
1       S1      S2              |               5
2       R3      R3              |
3                       ACC     |
4       S6      S7              |               8
5       R2      R2              |
6       S6      S7              |               9
7                       R3      |
8                       R1      |
9                       R2      |
```

##### 查看输出产生式

```
The ans:
C->d
C->cC
C->cC
C->cC
C->d
C->cC
S->CC
ACC
```

LR输出的产生式是最右推导的逆序列，下面验证其正确性。

S=>CC=>CcC=>Ccd=>cCcd=>ccCcd=>cccCcd=>cccdcd

可以看出，上述产生式的确是**最右推导**的逆序列，所以其是正确的。

经验证，程序自动构建的有效项目集规范族和DFA均正确，其分析表亦正确，对给定的输出串分析输出的产生式验证也正确。