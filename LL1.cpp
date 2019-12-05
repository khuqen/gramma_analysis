#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <stack>
using namespace std;

/* 产生式结构体 */
struct Production {
    char left; // 左部符号
    vector<char> rigths;  // 右部符号串
};

/* 文法结构体 */
struct Grammar {
    int num;  // 产生式数量
    vector<char> T;   // 终结符
    vector<char> N;   // 非终结符
    vector<Production> prods;  //产生式
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

/* 判断ch是否是终结符 */
int isInT(char ch)
{
    for (int i = 0; i < grammar.T.size(); i++) {
        if (grammar.T[i] == ch) {
            return i + 1;
        }
    }
    return 0;
}

/* 判断ch是否是非终结符 */
int isInN(char ch)
{
    for (int i = 0; i < grammar.N.size(); i++) {
        if (grammar.N[i] == ch) {
            return i + 1;
        }
    }
    return 0;
}
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
                /* 当前符号是非终结符，且当前符号可以推出空，则还需判断下一个符号 */
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
    
    printf("FIRST:\n");
    for (int i = 0; i < grammar.N.size(); i++) {
        char X = grammar.N[i];
        printf("%c: ", X);
        for (auto it = first[X].begin(); it != first[X].end(); it++) {
            printf("%c ", *it);
        }
        printf("\n");
    }
}
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
                        char A = P.left;    // A为左部符号
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

    printf("FOLLOW:\n");
    for (int i = 0; i < grammar.N.size(); i++) {
        char X = grammar.N[i];
        printf("%c: ", X);
        for (auto it = follow[X].begin(); it != follow[X].end(); it++) {
            printf("%c ", *it);
        }
        printf("\n");
    }
}
/* 把生成式插入到预测分析表对应的项中 */
void insertTOForecastAnalysisTable(char A, char a, Production &P)
{
    /* 根据A和a找到对应表项 */
    int i = isInN(A) - 1;
    int j = isInT(a) - 1;
    /* 把P.left->P.rights插入*/
    M[i][j].push_back(P.left);
    M[i][j].push_back('-');
    M[i][j].push_back('>');
    for (auto it = P.rigths.begin(); it != P.rigths.end(); it++) {
        M[i][j].push_back(*it);
    }
}
/* 取出预测分析表对应的项中的产生式 */
void getFromForecastAnalysisTable(char A, char a, vector<char> &s)
{
    /* 根据A和a找到对应表项 */
    int i = isInN(A) - 1;
    int j = isInT(a) - 1;
    /* 取出 */
    s.assign(M[i][j].begin(), M[i][j].end());
}
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
    /* 输出预测分析表 */
    printf("forecast analysis table:\n");
    printf("\t");
    for (int i = 0; i < grammar.T.size(); i++) {
        printf("%c\t\t", grammar.T[i]);
    }
    printf("\n");
    for (int i = 0; i < grammar.N.size(); i++) {
        printf("%c\t", grammar.N[i]);
        for (int j = 0; j < grammar.T.size(); j++) {
            for(auto k = M[i][j].begin(); k != M[i][j].end(); k++) {
                printf("%c", *k);
            }
            printf("\t\t");
        }
        printf("\n");
    }
}
/* 读入并初始化语法 */
void initGrammar()
{
    printf("Please enter the num of production:\n");
    cin >> grammar.num;
    string s;
    printf("Please enter the production:\n");
    for (int i = 0; i < grammar.num; i++) {
        cin >> s;
        Production tmp;
        tmp.left = s[0];
        for (int j = 3; j < s.size(); j++) {
            tmp.rigths.push_back(s[j]);
        }
        grammar.prods.push_back(tmp);
    }
    printf("Please enter the non-terminators(end with #):\n");
    char ch;
    cin >> ch;
    while (ch != '#') {
        grammar.N.push_back(ch);
        cin >> ch;
    }
    printf("Please enter the terminators(end with #):\n");
    cin >> ch;
    while (ch != '#') {
        grammar.T.push_back(ch);
        cin >> ch;
    }
    /* 把$当作终结符 */
    grammar.T.push_back('$');
    /* 求FIRST集和FOLLOW集 */
    getFirstSet();
    getFollowSet();

    /* 生成预测分析表 */
    productForecastAnalysisTable();

    /* 读入待分析串并初始化分析栈 */
    printf("Please enter the String to be analyzed:\n");
    cin >> str;
    str += '$';
    ST.push('$');
    ST.push(grammar.N[0]);
}
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

int main()
{
    initGrammar();
    process();
    return 0;
}