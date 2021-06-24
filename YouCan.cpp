#include <iostream>
#include <cmath>
#include <vector>
#include <string.h>
#include <string>
#include <algorithm>
#include <stack>
#include <fstream>

//#include"interpreter.cpp"

using namespace std;
//---------------------------------------

void ReturnUnexpect(int Errornum)
{
    exit(Errornum);
    //cout << Errornum << endl;
    exit(1);
}

//------------------------------------------
//语义分析使用到的临时变量
int procedureId = 0; //记录当前的过程层数
int tableId = 0;     //当前名字表的下标
int dx = 3;          //当前层数的偏移地址
int ip = 0;          //全局符号表的指针

//判断是否为数字
const char *input;
//int index = 0;
int codeId = 0;  //代码的下标
string codefile; //调式时输入程序的文件
//产生中间代码时opr操作对应编号的意义
string klabel[29] = {"+", "-", "*", "/", "=", "#", "<", "<=", ">", ">=", ",", ";", ":=", "(", ")", ".", "CONST", "VAR", "PROCEDURE", "BEGIN", "END", "ODD", "IF", "THEN", "CALL", "WHILE", "DO", "READ", "WRITE"};
//---------------------------------------

//---------------------------------------
//smantic anaysis
//符号表，分别为名字、类型、地址、属性值、常量值。不一定都会用到
struct Symble
{
    string name;
    string kind;
    int address;
    int attribute;
    int constvalue; //常量的值
};

vector<Symble> sym_table; //符号表
stack<int> procstack;     //记录过程信息的栈、用于确定过程之间的关系
//为了回填地址记录的单元
struct backpathcall
{
    int callcid;
    int callprocindex;
};

stack<backpathcall> callstack; //记录了需要回填地址的所有单元

//查看是否声明了同层次的变量
void If_Redeclaration(Symble s)
{
    //先不检查同层变量
    //因为发现好像同层变量可以重复声明，找变量表的时候只要找离得近的？
    // int lev = s.attribute;
    // string sname = s.name;
    // for (int i = 0; i < tableId; i++)
    // {
    //     if (sym_table[i].name == sname && sym_table[i].attribute == lev)
    //     {
    //         cerr << "If_Redeclaration :: error same name same lev" << endl;
    //         ReturnUnexpect(10);
    //     }
    // }
}

int old_For_testvarreclaration = 0; //符号表中这个过程作用域中起始下标，为了检查重复声明变量
//int new_For_testvarreclaration = 0;
void testvarreclaration(Symble s)
{
    for (int i = old_For_testvarreclaration; i < tableId; i++)
    {
        if (sym_table[i].name == s.name)
        {
            //redeclaration变量在同一个过程中重复声明
            cerr << "Compiler error :: var redeclaration" << endl;
            ReturnUnexpect(6);
        }
    }
}

//寻找标识符，不包括PROC（访问控制的规则不一样）
int FindSymName(string n, int proId)
{
    int sym_index = -1;
    int lev = 5; //设定最多不能嵌套3层，所以初始值先设为5
    bool fflag = false;
    bool fc = false;
    //bool fcflag=false;
    for (int i = 0; i < tableId; i++)
    {
        if (sym_table[i].name == n)
        {
            fflag = true;

            if (sym_table[i].attribute > proId)
            {
                //不能访问层数高于自己的变量
                //报错信息
                //cerr << "You can't found ID " << n << " may be in other procedure" << endl;
                //ReturnUnexpect(6);
            }
            else
            {
                fc = true;
                int tempp = proId - sym_table[i].attribute;
                if (tempp < lev)
                {
                    lev = tempp;
                    sym_index = i;
                }
                else
                {
                    //变量优先访问层次离自己近的
                    //层次相同怎么办？
                    //应该不允许同层次重复变量声明吧   后面要加上检查这个的语句
                    //允许同层,有限访问离自己近的，就是符号表靠后的
                    //这里没有检查超过三层的变量，应该是语法错误检查
                    if (tempp == lev)
                    {
                        sym_index = i;
                    }
                }
            }
        }
    }
    if (fflag && fc)
    {
    }
    else
    {
        if (fflag == true && fc == false)
        {
            //访问了高层变量
            cerr << "FindSymName :: error :: ID may in higher level" << endl;
            ReturnUnexpect(6);
        }
        if (fflag == false)
        {
            //无声明
            cerr << "FindSymName :: error :: ID not declare" << endl;
            ReturnUnexpect(5);
            return -1;
        }
    }
    return sym_index;
}
//记录指令结构，包括f、l、d
struct Instruction
{
    string funcCode = " ";
    int levelDiff = 0;
    int displacement = 0;
    string content = " ";
};

vector<Instruction> code;

//从关键字表查找关键字的下标
int findKlabel(string str)
{
    for (int i = 0; i < 29; i++)
    {
        if (str == klabel[i])
            return i + 1;
    }
    return -1;
}
//向内存中加一条指令
// void codeAdd(string str, int lev, int place, string con)
// {
//     Instruction temp;
//     temp.funcCode = str;
//     temp.levelDiff = lev;
//     temp.displacement = place;
//     temp.content = con;
//     code.push_back(temp);
//     codeId++;
// }

void codeAdd(string str, int lev, int place)
{
    Instruction temp;
    temp.funcCode = str;
    temp.levelDiff = lev;
    temp.displacement = place;
    code.push_back(temp);
    codeId++;
}
//将str类型转化成int
int Convert_Str_Int(string s)
{
    return atoi(s.c_str());
}

//*********************************
//当遇到间接递归时地址错误
//结束程序时反填call指令地址
//可以做一下类型检查
void BackPath_ForProcCall()
{
    while (!callstack.empty())
    {
        backpathcall tempbackpathcall = callstack.top();
        callstack.pop();
        code[tempbackpathcall.callcid].displacement = sym_table[tempbackpathcall.callprocindex].address;
    }
}

//---------------------------------------
//define syntax tree

//#define MAXCHILDNUM 5000
//语法分析，输出语法树时构建的节点
//template<class T>
struct node
{
    int childnum;
    //node *child[MAXCHILDNUM];
    vector<node *> child;
    string str;
    node()
    {
        childnum = 0;
        // for(int i=0;i<MAXCHILDNUM;i++){
        //     child[i]=NULL;
        // }
        child.clear();
    }
    node(string nstr)
    {
        childnum = 0;
        str = nstr;
        // for(int i=0;i<MAXCHILDNUM;i++){
        //     child[i]=NULL;
        // }
        child.clear();
    }
    void mkchild(node *childnode)
    {
        //child[childnum]=childnode;
        child.push_back(childnode);
        childnum++;
    }
    //将语法树打印出来，对应实验二要求的输出
    void print()
    {
        if (str == ",")
        {
            str = "COMMA";
        }
        else if (str == "(")
        {
            str = "LP";
        }
        else if (str == ")")
        {
            str = "RP";
        }
        if (childnum == 0)
        {
            cout << str;
        }
        else
        {
            cout << str << "(";
            for (int i = 0; i < childnum; i++)
            {
                // if(child[i]==NULL){
                //     continue;
                // }
                child[i]->print();
                if (i != childnum - 1)
                {
                    cout << ",";
                }
            }
            cout << ")";
        }
    }
};

//--------------------------------------------------------

//type -1---error,0---EOF,1---key,2----identifer,
//3---number,4---operator,5---speration char
//&str
//enum LEXPROPERTY{KEY=1,IDENTIFER=2,NUMBER=3,OPERATOR=4,SPERATION=5};
//词法分析，单词条目
//根据pl0语言特点设计的DFA，识别的单词放入lextable中
// lexical
struct LexAnaEntry
{
    string LexStr;
    int LexProperty;
    //..... more
    LexAnaEntry(string sstr, int lp)
    {
        LexStr = sstr;
        LexProperty = lp;
    }
    LexAnaEntry(){};
};
//词法分析关键字
string LEX_KEY[] = {"CONST", "VAR", "PROCEDURE", "BEGIN", "END", "ODD", "IF", "THEN", "CALL", "WHILE", "DO", "READ", "WRITE"};

#define ID_MAXLEN 10
#define LEX_KEYSIZE 13
//--------------------------------
//词法分析临时变量
//char Lex_last;
char Lex_ch;
bool Lex_flag = false;
bool Lex_isEof = false;
//判断是否进入调试模式（重定向输入）
bool Lex_ifreopen = false;
//----------------------------------
vector<LexAnaEntry> LexTable; //词法分析单词表

void myoutput(int, string &);
//判断是否是字母
bool isa1(char ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    {
        // cout<<ch<<"   is  a alpha"<<endl;
        return true;
    }
    return false;
}
//判断是否是数字
bool isd1(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        // cout<<ch<<"  is a digital"<<endl;
        return true;
    }
    return false;
}
//从输入获得一个字符
char mygetch()
{
    if (Lex_flag)
    {
        Lex_flag = false;
        return Lex_ch;
    }
    else
    {
        // Lex_last=ch;
        if (scanf("%c", &Lex_ch) != EOF)
        {
        }
        else
        {
            //endout();
            //exit(0);
            Lex_isEof = true;
            Lex_ch = '\0';
        }
        //if(ch==EOF){exit(1);}
    }

    return Lex_ch;
}
//回退一个单位
void backp()
{

    Lex_flag = true;
    return;
}
//词法分析getsym函数
//get a word,return type -1---error,0---EOF,1---key,2----identifer,
//3---number,4---operator,5---speration char
//&str
int getSYM(string &str)
{

    str.clear();
    char ch = mygetch();
    if (Lex_isEof)
    {
        // endout();
        // exit(0);
        return 0;
    }
    //cout<<"we next got"<<ch<<endl;
    // if (ch == EOF) {
    // 	cout<<"we got EOF"<<endl;
    //     //over or error
    //     return 0;
    // }
    while (ch == ' ' || ch == '\n' || ch == '\t')
    {
        ch = mygetch();
    }
    if (isa1(ch))
    {
        //cout<<"We got a start al"<<ch<<endl;
        while (isa1(ch) || isd1(ch))
        {
            str = str + ch;
            if (str.length() > 10)
            { //cout << "id too long!" << endl;
                return -1;
            }
            ch = mygetch();
        }

        //fseek(infile, -1, SEEK_CUR);
        backp();
        transform(str.begin(), str.end(), str.begin(), ::toupper);
        //bool iskey = false;
        //int keynum = -1;
        for (int i = 0; i < LEX_KEYSIZE; i++)
        {
            if (LEX_KEY[i] == str)
            {
                //iskey = true;
                //keynum = i;
                //   cout<<"we got return 1"<<endl;
                myoutput(1, str);
                return 1;
            }
        }
        //  cout<<"we got return 2"<<endl;
        myoutput(2, str);
        return 2;
    }
    else if (isd1(ch))
    {
        while (isd1(ch))
        {
            str = str + ch;
            ch = mygetch();
        }
        if (isa1(ch))
        {
            return -1;
        }
        /*long pos = infile.tellg();
        infile.seekg(-1,ios_base::cur);*/
        //fseek(infile, -1, SEEK_CUR);
        backp();
        myoutput(3, str);
        return 3;
    }
    else
    {
        //ifoperator
        // cout<<"we got else not in alpha"<<endl;
        if (ch == '=')
        {
            str = str + '=';
            myoutput(4, str);
            return 4;
        }
        else if (ch == ':')
        {
            ch = mygetch();
            if (ch == '=')
            {
                str = str + ":=";
                myoutput(4, str);
                return 4;
            }
            else
            {
                //cout << "SYM :: IF_operator error,not :=" << endl;
                return -1;
            }
        }
        else if (ch == '+')
        {
            str = str + '+';
            myoutput(4, str);
            return 4;
        }
        else if (ch == '-')
        {
            str = str + '-';
            myoutput(4, str);
            return 4;
        }
        else if (ch == '*')
        {
            str = str + '*';
            myoutput(4, str);
            return 4;
        }
        else if (ch == '/')
        {
            str = str + '/';
            myoutput(4, str);
            return 4;
        }
        else if (ch == '#')
        {
            str = str + '#';
            myoutput(4, str);
            return 4;
        }
        else if (ch == '<')
        {
            ch = mygetch();
            if (ch == '=')
            {
                str = str + "<=";
                myoutput(4, str);
                return 4;
            }
            else
            {
                /*long pos = infile.tellg();
                infile.seekg(-1,ios_base::cur);*/
                //fseek(infile, -1, SEEK_CUR);
                backp();
                str = str + '<';
                myoutput(4, str);
                return 4;
            }
        }
        else if (ch == '>')
        {
            ch = mygetch();
            if (ch == '=')
            {
                str = str + ">=";
                myoutput(4, str);
                return 4;
            }
            else
            {
                /*long pos = infile.tellg();
                infile.seekg(-1,ios_base::cur);*/
                //fseek(infile, -1, SEEK_CUR);
                backp();
                str = str + '>';
                myoutput(4, str);
                return 4;
            }
        }
        else if (ch == ';')
        { // if speration char
            str = str + ';';
            myoutput(5, str);
            return 5;
        }
        else if (ch == ',')
        {
            str = str + ',';
            myoutput(5, str);
            return 5;
        }
        else if (ch == '.')
        {
            str = str + '.';
            myoutput(5, str);
            return 5;
        }
        else if (ch == '(')
        {
            str = str + '(';
            myoutput(5, str);
            return 5;
        }
        else if (ch == ')')
        {
            str = str + ')';
            myoutput(5, str);
            return 5;
        }
        else
        {
            if (ch == '\0' && Lex_isEof)
            {
                return 0;
            }
            return -1;
        }
    }
}
//输出到单词表
void myoutput(int i, string &str)
{
    if (i == -1)
    {
        //clean up file , error processing
        //cout << "lexical error" << endl;
    }
    else if (i == 1)
    {

        //cout<<str<<endl;
        //fprintf(outfile, "%s\n", str);
        LexAnaEntry tmpLex(str, 1);
        LexTable.push_back(tmpLex);
    }
    else if (i == 2)
    {

        //cout<<"IDENTIFIER "<<str<<endl;
        transform(str.begin(), str.end(), str.begin(), ::toupper);
        //string strtmp="IDENTIFIER "+str;
        LexAnaEntry tmpLex(str, 2);
        LexTable.push_back(tmpLex);
        //fprintf(outfile, "IDENTIFIER %s\n", str);
    }
    else if (i == 3)
    {

        //<<"NUMBER "<<str<<endl;
        long long dig = atoll(str.c_str());
        string strtmp = std::to_string(dig);
        //string strtmp=str;
        LexAnaEntry tmpLex(strtmp, 3);
        LexTable.push_back(tmpLex);
        //fprintf(outfile, "NUMBER %s\n", str);
    }
    else if (i == 4)
    {

        //cout<<str<<endl;
        LexAnaEntry tmpLex(str, 4);
        LexTable.push_back(tmpLex);
        //fprintf(outfile, "%s\n", str);
    }
    else if (i == 5)
    {
        //cout<<str<<endl;
        LexAnaEntry tmpLex(str, 5);
        LexTable.push_back(tmpLex);
        //fprintf(outfile, "%s\n", str);
    }
    else
    {
        //error
    }
}

void outputfortest();
//词法分析程序入口
int Compiler_Lexcial()
{
    if (Lex_ifreopen)
    {
        freopen(codefile.c_str(), "r", stdin);
    }

    string str;
    int i = 0;
    //int i = getSYM(str);
    //cout<<"i=="<<i<<endl;
    while ((i = getSYM(str)) > 0)
    {
        //cout<<"===="<<i<<endl;
        //cout<<"1"<<endl;
        //myoutput(i,str);
        // i = getSYM(str);
        //cout<<"i==:"<<i<<endl;
    }
    if (i == -1)
    {
        cout << "Lexical Error" << endl;
        //cout<<str<<endl;
        //endout();
        exit(1);
        //exit(1);
    }
    if (i == 0)
    {
        //std::cerr<<"Lexical anasysis successfully done !"<<endl;
        //outputfortest();
        return 0;
    }

    if (Lex_ifreopen)
    {
        fclose(stdin);
    }

    return 0;
};
//实验一要求的输出
void outputfortest()
{
    int n = LexTable.size();
    //cerr<<"size of lexTable is"<<n<<endl;
    for (int i = 0; i < n; i++)
    {
        int lex_op = LexTable[i].LexProperty;
        switch (lex_op)
        {
        case 1:
            cout << LexTable[i].LexStr << endl;
            break;
        case 2:
            cout << "IDENTIFIER " << LexTable[i].LexStr << endl;
            break;
        case 3:
            cout << "NUMBER " << LexTable[i].LexStr << endl;
            break;
        case 4:
            cout << LexTable[i].LexStr << endl;
            break;
        case 5:
            cout << LexTable[i].LexStr << endl;
            break;
            //            default:
            //            cerr<<"error in lexTable, some propety not in 1-5"<<endl;
        }
    }
}
//语法分析、语义分析、中间代码生成部分
//----------------------------------------------------------
void Compiler_Syntax(node *cnode);         //第二部分入口，语法分析入口
void Syntax_Unsint(node *cnode);           //无符号数
void Syntax_ID(node *cnode);               //标识符
void SyntaxAnalysis_Prog(node *cnode);     //程序
void SyntaxAnalysis_subProg(node *cnode);  //分程序
void SyntaxAnalysis_Constdef(node *cnode); //常数定义
void SyntaxAnalysis_Vardecl(node *cnode);  //变量声明
void SyntaxAnalysis_Condecl(node *cnode);  //常数声明
void SyntaxAnalysis_Procdecl(node *cnode); //过程声明

void SyntaxAnalysis_Statement(node *cnode);         //语句
void SyntaxAnalysis_State_Assignment(node *cnode);  //赋值语句
void SyntaxAnalysis_State_Condition(node *cnode);   //条件语句
void SyntaxAnalysis_State_While_Do(node *cnode);    //当型循环语句
void SyntaxAnalysis_State_Call(node *cnode);        //过程调用语句
void SyntaxAnalysis_State_Read(node *cnode);        //读语句
void SyntaxAnalysis_State_Write(node *cnode);       //写语句
void SyntaxAnalysis_State_Compond(node *cnode);     //复合语句
void SyntaxAnalysis_State_Nop(node *cnode);         //空语句
void SyntaxAnalysis_Condition(node *cnode);         //条件
void SyntaxAnalysis_Expression(node *cnode);        //表达式
void SyntaxAnalysis_Term(node *cnode);              //项
void SyntaxAnalysis_Factor(node *cnode);            //因子
void SyntaxAnalysis_Operator_plusor(node *cnode);   //加减运算符
void SyntaxAnalysis_Operator_divor(node *cnode);    //乘除运算符
void SyntaxAnalysis_Operator_Relation(node *cnode); //关系运算符
void SyntaxAnalysis_alpha(node *cnode);
void SyntaxAnalysis_digital(node *cnode);

node *root; //语法树根节点
//语法分析报错
void SyntaxAnalysis_error()
{
    cout << "Syntax Error" << endl;
    //root->child[0]->print();
    exit(1);
}
//----------------------------------------------------------
//Syntax Analysis

int Syntax_Pointer = 0;
bool SynFlag = false;
int Syn_Proc_Stacknum = 0;

//----------------------------------------------------------
void Compiler_Syntax(node *cnode)
{
    Syntax_Pointer = 0;
    //cout<<"begin prog"<<endl;
    SyntaxAnalysis_Prog(cnode);
}
void Syntax_Unsint(node *cnode)
{
    if (LexTable[Syntax_Pointer].LexProperty == 3)
    {
        node *tnode = new node(LexTable[Syntax_Pointer].LexStr);
        cnode->mkchild(tnode);
        Syntax_Pointer++;
    }
    else
    {
        //cout<<"syn unsigned int error"<<endl;
        SyntaxAnalysis_error();
    }
}
//判断是不是标识符
void Syntax_ID(node *cnode)
{
    //cout<<"enter syn ID"<<endl;
    if (LexTable[Syntax_Pointer].LexProperty == 2)
    {
        node *tnode = new node(LexTable[Syntax_Pointer].LexStr);
        cnode->mkchild(tnode);
        Syntax_Pointer++;
    }
    else
    {
        //cout<<"Syntax_ID error"<<endl;
        SyntaxAnalysis_error();
    }
    //cout<<"out syn ID"<<endl;
}
//程序
void SyntaxAnalysis_Prog(node *cnode)
{ //cnode是当前节点，用于创建自己点tnode名为子程序名，然后用二级节点tnode用作子节点传入参数
    node *tnode = new node("PROGRAM");
    cnode->mkchild(tnode);
    //***********************************************
    codeAdd("jmp", 0, -1); //暂时填一个
                           //***********************************************
    SyntaxAnalysis_subProg(tnode);
    if (Syntax_Pointer == LexTable.size())
    {
        SyntaxAnalysis_error();
    }
    if (LexTable[Syntax_Pointer].LexStr == ".")
    {
        node *ttnode = new node(LexTable[Syntax_Pointer].LexStr);
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        //***********************************************
        codeAdd("opr", 0, 0, "return");
        //***********************************************
    }
    else
    {
        //cout<<"syntax prog error"<<endl;
        SyntaxAnalysis_error();
    }
    //
    //程序结束，反填地址
    BackPath_ForProcCall();
    //cout<<"out syn prog"<<endl;
}
//分程序
void SyntaxAnalysis_subProg(node *cnode)
{
    //变量的作用域从分程序这里算起？
    old_For_testvarreclaration = tableId;
    cerr << "enter subprog" << endl;
    node *tnode = new node("SUBPROG");
    cnode->mkchild(tnode);
    //常量说明
    if (LexTable[Syntax_Pointer].LexStr == "CONST")
    {
        SyntaxAnalysis_Condecl(tnode); //常量说明部分；
    }
    if (LexTable[Syntax_Pointer].LexStr == "VAR")
    {
        //cout<<"enter var decl"<<endl;
        SyntaxAnalysis_Vardecl(tnode); //变量说明
    }
    if (LexTable[Syntax_Pointer].LexStr == "PROCEDURE")
    {
        Syn_Proc_Stacknum++;
        if (Syn_Proc_Stacknum > 3)
        {
            SyntaxAnalysis_error();
        }
        //cout<<"we enter prodecl"<<endl;
        SyntaxAnalysis_Procdecl(tnode); //过程说明部分
        Syn_Proc_Stacknum--;
    }
    //**************目标代码的生成，开辟地址空间
    int varNumber = 0;
    for (int i = 0; i < tableId; i++)
        if (sym_table[i].attribute == procedureId && sym_table[i].kind == "VAR")
        {
            varNumber++;
        }

    codeAdd("int", 0, varNumber + 3);
    //***************目标代码生成,直接跳转到主程序
    //换言之，0层只有主程序
    if (procedureId == 0)
    {
        code[0].displacement = codeId - 1; //主程序开始的位置，即上一句  int 指令
    }
    else
    {
        //为不是主程序的过程调用反填地址
        //设置一个过程调用栈，栈顶元素就是当前要反填的过程
        int procindex = procstack.top();
        procstack.pop();
        sym_table[procindex].address = codeId - 1;
        //根据index修改符号表中的调用栈地址
    }

    //**********************
    SyntaxAnalysis_Statement(tnode); //语句
    cerr << "out subprog" << endl;
}

//常量定义
void SyntaxAnalysis_Constdef(node *cnode)
{
    cerr << "enter const define" << endl;
    node *tnode = new node("CONSTANTDEFINE");
    cnode->mkchild(tnode);
    //*************************************
    Symble stemp;
    stemp.name = LexTable[Syntax_Pointer].LexStr;
    stemp.kind = "CONST";

    //*************************************
    Syntax_ID(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "=")
    {
        node *ttnode = new node("=");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        //*************************************
        stemp.constvalue = Convert_Str_Int(LexTable[Syntax_Pointer].LexStr); //将char型的转化成数字
        stemp.attribute = procedureId;
        //检查常量是否重复声明（同层）
        If_Redeclaration(stemp);
        sym_table.push_back(stemp);
        tableId++;
        //*************************************
        Syntax_Unsint(tnode);
    }
    else
    {
        SyntaxAnalysis_error();
    }
    cerr << "out const define" << endl;
}
//变量说明
void SyntaxAnalysis_Vardecl(node *cnode)
{
    node *tnode = new node("VARIABLEDECLARE");
    cnode->mkchild(tnode);
    //考察变量重复申明还要包括常量、过程名
    //old_For_testvarreclaration = tableId;
    if (LexTable[Syntax_Pointer].LexStr == "VAR")
    {
        node *ttnode = new node("VAR");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        //cout<<"in var decl into syn ID"<<endl;
        //***************************************************
        Symble stemp;
        stemp.name = LexTable[Syntax_Pointer].LexStr;
        stemp.attribute = procedureId;
        stemp.address = dx;
        stemp.kind = "VAR";
        //检查变量是否重复声明（同层）
        If_Redeclaration(stemp);
        testvarreclaration(stemp);
        sym_table.push_back(stemp);
        tableId++;
        dx++;
        //****************************************************
        Syntax_ID(tnode);
        while (LexTable[Syntax_Pointer].LexStr == ",")
        {
            node *ttnode = new node(",");
            tnode->mkchild(ttnode);
            Syntax_Pointer++;
            //************************************************
            Symble sstemp;
            sstemp.name = LexTable[Syntax_Pointer].LexStr;
            sstemp.attribute = procedureId;
            sstemp.address = dx;
            sstemp.kind = "VAR";
            //检查变量是否重复声明（同层）
            If_Redeclaration(sstemp);
            testvarreclaration(sstemp);
            sym_table.push_back(sstemp);
            tableId++;
            dx++;
            //*****************************************************

            Syntax_ID(tnode);
        }
        if (LexTable[Syntax_Pointer].LexStr == ";")
        {
            node *ttnode = new node(";");
            tnode->mkchild(ttnode);
            Syntax_Pointer++;
        }
        else
        {
            //cout<<"Syn var decl error not end with ;"<<endl;
            SyntaxAnalysis_error();
        }
    }
    else
    {
        //cout<<"var decl error"<<endl;
        SyntaxAnalysis_error();
    }
    //new_For_testvarreclaration = tableId;
    //testvarreclaration();
}
//常量说明
void SyntaxAnalysis_Condecl(node *cnode)
{
    cerr << "syn const declare" << endl;
    node *tnode = new node("CONSTANTDECLARE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "CONST")
    {
        node *ttnode = new node("CONST");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Constdef(tnode);
        while (LexTable[Syntax_Pointer].LexStr == ",")
        {
            node *tmpnode = new node(",");
            tnode->mkchild(tmpnode);
            Syntax_Pointer++;
            SyntaxAnalysis_Constdef(tnode);
        }
        if (LexTable[Syntax_Pointer].LexStr == ";")
        {
            node *tmpnode = new node(";");
            tnode->mkchild(tmpnode);
            Syntax_Pointer++;
        }
        else
        {
            cerr << "Syntax const decl error end is not ;" << endl;
            SyntaxAnalysis_error();
        }
    }
    else
    {
        cerr << "Syntax const decl error head is not const\n"
             << endl;
        SyntaxAnalysis_error();
    }
    cerr << "out const declare" << endl;
}
//过程首部
void SyntaxAnalysis_ProcHead(node *cnode)
{
    if (LexTable[Syntax_Pointer].LexStr == "PROCEDURE")
    {
        node *tnode = new node("PROCEDUREHEAD");
        cnode->mkchild(tnode);
        node *ttnode = new node("PROCEDURE");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        //*************************************************
        Symble stemp;
        stemp.kind = "PROC";
        stemp.name = LexTable[Syntax_Pointer].LexStr;
        stemp.address = codeId; //将当前过程下一跳指令作为call指令的跳转地址
        //*****************
        //stemp.attribute = procedureId - 1;
        stemp.attribute = procedureId;

        //*****************
        sym_table.push_back(stemp);
        procstack.push(tableId);
        tableId++;
        //*************************************************
        Syntax_ID(tnode);
        if (LexTable[Syntax_Pointer].LexStr == ";")
        {
            node *tmpnode = new node(";");
            tnode->mkchild(tmpnode);
            Syntax_Pointer++;
        }
        else
        {
            //cout<<"syn proc head error not end with ;"<<endl;
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
}
//过程说明
void SyntaxAnalysis_Procdecl(node *cnode)
{
    //*********************************************
    procedureId++; //层数加1
    dx = 3;        //当前层数内地址变为1
                   //*********************************************
    if (procedureId > 3)
    {
    }
    node *tnode = new node("PROCEDUREDECLARE");
    cnode->mkchild(tnode);
    SyntaxAnalysis_ProcHead(tnode);
    SyntaxAnalysis_subProg(tnode);
    if (LexTable[Syntax_Pointer].LexStr == ";")
    {
        //**************目标代码的生成，过程说明结束
        codeAdd("opr", 0, 0, "return");
        procedureId--;
        //*************
        node *tmpnode = new node(";");
        tnode->mkchild(tmpnode);
        Syntax_Pointer++;
        while (LexTable[Syntax_Pointer].LexStr == "PROCEDURE")
        {
            SyntaxAnalysis_Procdecl(tnode);
        }
    }
    else
    {
        //cout<<"syn proc decl error not end with ;"<<endl;
        SyntaxAnalysis_error();
    }
}

//-------------------

//语句
void SyntaxAnalysis_Statement(node *cnode)
{
    //cout<<"we enter statement,the point is "<<Syntax_Pointer<<"  and the size of table is "<<LexTable.size()<<endl;
    node *tnode = new node("SENTENCE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexProperty == 2)
    {
        SyntaxAnalysis_State_Assignment(tnode);
    }
    else if (LexTable[Syntax_Pointer].LexStr == "IF")
    {
        SyntaxAnalysis_State_Condition(tnode); //条件语句
    }
    else if (LexTable[Syntax_Pointer].LexStr == "WHILE")
    {
        SyntaxAnalysis_State_While_Do(tnode); //当型语句
    }
    else if (LexTable[Syntax_Pointer].LexStr == "CALL")
    {
        SyntaxAnalysis_State_Call(tnode);
    }
    else if (LexTable[Syntax_Pointer].LexStr == "READ")
    {
        SyntaxAnalysis_State_Read(tnode);
    }
    else if (LexTable[Syntax_Pointer].LexStr == "WRITE")
    {
        SyntaxAnalysis_State_Write(tnode);
    }
    else if (LexTable[Syntax_Pointer].LexStr == "BEGIN")
    {
        SyntaxAnalysis_State_Compond(tnode); //复合语句
    }
    else
    {
        SyntaxAnalysis_State_Nop(tnode);
    }
}
//赋值语句
void SyntaxAnalysis_State_Assignment(node *cnode)
{
    //cout<<"we enter state assignment";
    //cout<<"the point is "<<Syntax_Pointer<<"  and the size of table is "<<LexTable.size()<<endl;

    node *tnode = new node("ASSIGNMENT");
    cnode->mkchild(tnode);
    //******************************************
    string tempId = LexTable[Syntax_Pointer].LexStr;
    //*******************************************
    Syntax_ID(tnode);
    if (LexTable[Syntax_Pointer].LexStr == ":=")
    {
        node *tmpnode = new node(":=");
        tnode->mkchild(tmpnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Expression(tnode);
        //**************目标代码的生成，赋值语句
        int tableIndex = FindSymName(tempId, procedureId);
        if (tableIndex == -1)
        {
            cerr << "Syntax_State_Assignment :: Undefined Identifer" << endl;
            ReturnUnexpect(1);
        }
        if (sym_table[tableIndex].kind == "CONST")
        {
            cerr << "Syntax_State_Assignment :: Can't assign CONST" << endl;
            ReturnUnexpect(1);
        }
        codeAdd("sto", sym_table[tableIndex].attribute - procedureId, sym_table[tableIndex].address);
        //*****************************************
    }
    else
    {
        //cout<<"syn state assignment error"<<endl;
        SyntaxAnalysis_error();
    }
}
//条件语句
void SyntaxAnalysis_State_Condition(node *cnode)
{
    node *tnode = new node("IFSENTENCE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "IF")
    {
        node *tmpnode = new node("IF");
        tnode->mkchild(tmpnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Condition(tnode);
        //**************目标代码的生成，非真则跳转
        codeAdd("jpc", 0, 0);    //暂时先填着地址0
        int tempId = codeId - 1; //记录下来这条代码的位置
        //*************
        if (LexTable[Syntax_Pointer].LexStr == "THEN")
        {
            node *ttmpnode = new node("THEN");
            tnode->mkchild(ttmpnode);
            Syntax_Pointer++;
            SyntaxAnalysis_Statement(tnode);
            //**************************回填之前的jpc，如果条件不成立，直接跳转到codeId位置，即THEN后面的语句
            code[tempId].displacement = codeId;
            //*****************************
        }
        else
        {
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
}
//当型循环语句
void SyntaxAnalysis_State_While_Do(node *cnode)
{
    node *tnode = new node("WHILESENTENCE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "WHILE")
    {
        node *tmpnode = new node("WHILE");
        tnode->mkchild(tmpnode);
        Syntax_Pointer++;
        //****************************************
        //int tempAddr = codeId + 1; //while循环的入口地址
        int tempAddr = codeId;
        //*********************************
        SyntaxAnalysis_Condition(tnode);
        //**************目标代码的生成，非真则跳转
        codeAdd("jpc", 0, 0);    //暂时先填着地址0
        int tempId = codeId - 1; //吧上面这条指令的地址记录下来
        //*************
        if (LexTable[Syntax_Pointer].LexStr == "DO")
        {
            node *ttmpnode = new node("DO");
            tnode->mkchild(ttmpnode);
            Syntax_Pointer++;
            SyntaxAnalysis_Statement(tnode);
            //**************目标代码的生成，无条件跳转
            codeAdd("jmp", 0, tempAddr);
            //回填之前的jpc
            code[tempId].displacement = codeId;
            //************************************************
        }
        else
        {
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
}

//过程调用语句
void SyntaxAnalysis_State_Call(node *cnode)
{
    node *tnode = new node("CALLSENTENCE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "CALL")
    {
        node *tmpnode = new node("CALL");
        tnode->mkchild(tmpnode);
        Syntax_Pointer++;
        //****************目标代码生成，过程调用
        int tempAddr = 0;
        int tempId = 0;
        bool tempflag = false;
        for (int i = 0; i < tableId; i++)
        {
            if (sym_table[i].name == LexTable[Syntax_Pointer].LexStr && sym_table[i].kind == "PROC")
            {
                tempflag = true;
                tempAddr = sym_table[i].address;
                tempId = i;
                break;
            }
        }
        if (tempflag == false)
        {
            cerr << "Syntax_Sate_Call:: the Proc" << LexTable[Syntax_Pointer].LexStr << "is not declared" << endl;
            ReturnUnexpect(4);
        }
        if (sym_table[tempId].kind != "PROC")
        {
            cerr << "Syn_State_Call error :: " << sym_table[tempId].name << "is not PROC ID" << endl;
            ReturnUnexpect(8);
        }
        //******************************
        //改
        //codeAdd("cal",abs(procedureId-sym_table[tempId].attribute),tempAddr);
        codeAdd("cal", sym_table[tempId].attribute - procedureId, tempAddr);

        //
        backpathcall tempbackpathcall;
        tempbackpathcall.callcid = codeId - 1;
        tempbackpathcall.callprocindex = tempId;
        callstack.push(tempbackpathcall);
        //*******************************
        //**********************************
        Syntax_ID(tnode);
    }
    else
    {
        SyntaxAnalysis_error();
    }
}
//读语句
void SyntaxAnalysis_State_Read(node *cnode)
{
    node *tnode = new node("READSENTENCE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "READ")
    {
        //**************目标代码的生成，读语句
        codeAdd("opr", 0, findKlabel(LexTable[Syntax_Pointer].LexStr), LexTable[Syntax_Pointer].LexStr);
        //*************
        node *tmpnode = new node("READ");
        tnode->mkchild(tmpnode);
        Syntax_Pointer++;
        if (LexTable[Syntax_Pointer].LexStr == "(")
        {
            node *ttmpnode = new node("(");
            tnode->mkchild(ttmpnode);
            Syntax_Pointer++;
            //*******************目标代码的生成，结果的传递
            int tableIndex = FindSymName(LexTable[Syntax_Pointer].LexStr, procedureId);
            if (tableIndex == -1)
            {
                cerr << "Syntax_State_Read :: Undefined Identifer" << endl;
                ReturnUnexpect(1);
            }
            if (sym_table[tableIndex].kind == "CONST")
            {
                cerr << "Syntax_State_Assignment :: Can't assign CONST" << endl;
                ReturnUnexpect(1);
            }
            if (sym_table[tableIndex].kind == "PROC")
            {
                cerr << "Syntax_State_Assignment :: Can't assign PROC" << endl;
                ReturnUnexpect(1);
            }
            codeAdd("sto", sym_table[tableIndex].attribute - procedureId, sym_table[tableIndex].address);
            //*****************************************
            Syntax_ID(tnode);
            while (LexTable[Syntax_Pointer].LexStr == ",")
            {
                node *tttmpnode = new node(",");
                tnode->mkchild(tttmpnode);
                Syntax_Pointer++;
                //**************目标代码的生成，读语句
                codeAdd("opr", 0, findKlabel("READ"), "READ");
                int tableIndex = FindSymName(LexTable[Syntax_Pointer].LexStr, procedureId);
                if (tableIndex == -1)
                {
                    cerr << "Syntax_State_Read :: Undefined Identifer" << endl;
                    ReturnUnexpect(1);
                }
                if (sym_table[tableIndex].kind == "CONST")
                {
                    cerr << "Syntax_State_Assignment :: Can't assign CONST" << endl;
                    ReturnUnexpect(1);
                }
                if (sym_table[tableIndex].kind == "PROC")
                {
                    cerr << "Syntax_State_Assignment :: Can't assign PROC" << endl;
                    ReturnUnexpect(1);
                }
                codeAdd("sto", sym_table[tableIndex].attribute - procedureId, sym_table[tableIndex].address);
                //*****************************************
                Syntax_ID(tnode);
            }
            if (LexTable[Syntax_Pointer].LexStr == ")")
            {
                node *ttttmpnode = new node(")");
                tnode->mkchild(ttttmpnode);
                Syntax_Pointer++;
            }
            else
            {
                SyntaxAnalysis_error();
            }
        }
        else
        {
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
}
//写语句
void SyntaxAnalysis_State_Write(node *cnode)
{
    node *tnode = new node("WRITESENTENCE");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "WRITE")
    {
        node *ttnode = new node("WRITE");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        if (LexTable[Syntax_Pointer].LexStr == "(")
        {
            node *ttmpnode = new node("(");
            tnode->mkchild(ttmpnode);
            Syntax_Pointer++;
            //**************目标代码的生成，写语句

            int tableIndex = FindSymName(LexTable[Syntax_Pointer].LexStr, procedureId); //在符号表中找到该元素，加载到栈顶
            if (tableIndex != -1)
            {
                if (sym_table[tableIndex].kind == "CONST")
                {
                    codeAdd("lit", 0, sym_table[tableIndex].constvalue);
                }
                else if (sym_table[tableIndex].kind == "VAR")
                {
                    codeAdd("lod", sym_table[tableIndex].attribute - procedureId, sym_table[tableIndex].address);
                }

                if (sym_table[tableIndex].kind == "PROC")
                {
                    cerr << "Syntax_State_write error :: Can't write PROC" << endl;
                    ReturnUnexpect(1);
                }
            }
            else
            {
                cerr << "Syntax_State_Write:: SYM " << LexTable[Syntax_Pointer].LexStr << " not declared" << endl;
                ReturnUnexpect(3);
            }

            codeAdd("opr", 0, findKlabel("WRITE"), "WRITE");
            //*************
            Syntax_ID(tnode);
            while (LexTable[Syntax_Pointer].LexStr == ",")
            {
                node *tttmpnode = new node(",");
                tnode->mkchild(tttmpnode);
                Syntax_Pointer++;
                //**************目标代码的生成，写语句
                int tableIndex = FindSymName(LexTable[Syntax_Pointer].LexStr, procedureId); //在符号表中找到该元素，加载到栈顶
                if (tableIndex != -1)
                {
                    if (sym_table[tableIndex].kind == "CONST")
                    {
                        codeAdd("lit", 0, sym_table[tableIndex].constvalue); //因为常量的值声明后不能改变,所以不用去根据地址去找了，直接把常量10编码到中间代码，直接加载到操作栈
                    }
                    else if (sym_table[tableIndex].kind == "VAR")
                    {
                        codeAdd("lod", sym_table[tableIndex].attribute - procedureId, sym_table[tableIndex].address);
                    }

                    if (sym_table[tableIndex].kind == "PROC")
                    {
                        cerr << "Syntax_State_write error :: Can't write PROC" << endl;
                        ReturnUnexpect(1);
                    }
                }
                else
                {
                    cerr << "Syntax_State_Write:: SYM " << LexTable[Syntax_Pointer].LexStr << " not declared" << endl;
                    ReturnUnexpect(3);
                }

                codeAdd("opr", 0, findKlabel("WRITE"), "WRITE");

                //*************
                Syntax_ID(tnode);
            }
            if (LexTable[Syntax_Pointer].LexStr == ")")
            {
                node *ttttmpnode = new node(")");
                tnode->mkchild(ttttmpnode);
                Syntax_Pointer++;
            }
            else
            {
                SyntaxAnalysis_error();
            }
        }
        else
        {
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
}
//复合语句
void SyntaxAnalysis_State_Compond(node *cnode)
{
    node *tnode = new node("COMBINED");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "BEGIN")
    {
        node *ttnode = new node("BEGIN");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Statement(tnode);
        while (LexTable[Syntax_Pointer].LexStr == ";")
        {
            node *tttmpnode = new node(";");
            tnode->mkchild(tttmpnode);
            Syntax_Pointer++;
            SyntaxAnalysis_Statement(tnode);
        }
        if (LexTable[Syntax_Pointer].LexStr == "END")
        {
            node *ttttmpnode = new node("END");
            tnode->mkchild(ttttmpnode);
            Syntax_Pointer++;
        }
        else
        {
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
}
//空语句
void SyntaxAnalysis_State_Nop(node *cnode)
{
    node *tnode = new node("EMPTY");
    cnode->mkchild(tnode);
}
//条件
void SyntaxAnalysis_Condition(node *cnode)
{
    node *tnode = new node("CONDITION");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexStr == "ODD")
    {
        node *ttnode = new node("ODD");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Expression(tnode);
        //***********目标代码生成，odd运算
        codeAdd("opr", 0, findKlabel("ODD"), "ODD");
        //***************************************************
    }
    else
    {
        SyntaxAnalysis_Expression(tnode);
        //*******************************************************
        //记录关系运算符
        string tempOperatorR = LexTable[Syntax_Pointer].LexStr;
        //*******************************************************
        SyntaxAnalysis_Operator_Relation(tnode);
        SyntaxAnalysis_Expression(tnode);
        //**************目标代码的生成，关系运算符
        codeAdd("opr", 0, findKlabel(tempOperatorR), tempOperatorR);
        //***********************************
    }
}
//表达式
void SyntaxAnalysis_Expression(node *cnode)
{
    node *tnode = new node("EXPRESSION");
    cnode->mkchild(tnode);
    string tempop;
    if (LexTable[Syntax_Pointer].LexStr == "+" || LexTable[Syntax_Pointer].LexStr == "-")
    {
        tempop = LexTable[Syntax_Pointer].LexStr;
        node *ttnode = new node(LexTable[Syntax_Pointer].LexStr);
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
    }
    SyntaxAnalysis_Term(tnode);
    if (tempop == "-")
    {
        codeAdd("opr", 0, 30, "minus");
    }

    while (LexTable[Syntax_Pointer].LexStr == "+" || LexTable[Syntax_Pointer].LexStr == "-")
    {
        node *ttnode = new node(LexTable[Syntax_Pointer].LexStr);
        tnode->mkchild(ttnode);
        //*******************************************************
        string tempAddOrSub = LexTable[Syntax_Pointer].LexStr;
        //*******************************************************
        Syntax_Pointer++;
        SyntaxAnalysis_Term(tnode);
        //**************目标代码的生成，加减运算
        codeAdd("opr", 0, findKlabel(tempAddOrSub), tempAddOrSub);
        //*************************************************
    }
}

//项
void SyntaxAnalysis_Term(node *cnode)
{
    node *tnode = new node("ITEM");
    cnode->mkchild(tnode);
    SyntaxAnalysis_Factor(tnode);
    while (LexTable[Syntax_Pointer].LexStr == "*" || LexTable[Syntax_Pointer].LexStr == "/")
    {
        //*******************************************************
        string tempMutiOrDev = LexTable[Syntax_Pointer].LexStr;
        //*******************************************************
        node *ttnode = new node(LexTable[Syntax_Pointer].LexStr);
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Factor(tnode);
        //**************目标代码的生成，乘除运算
        codeAdd("opr", 0, findKlabel(tempMutiOrDev), tempMutiOrDev);
        //***********************************************
    }
}
//因子
void SyntaxAnalysis_Factor(node *cnode)
{
    //cout<<"enter syn factor"<<endl;
    node *tnode = new node("FACTOR");
    cnode->mkchild(tnode);
    if (LexTable[Syntax_Pointer].LexProperty == 2)
    {
        //标识符
        //*********判断标识符为变量还是常量

        //优先返回与自己层数相近的变量
        int tableIndex = FindSymName(LexTable[Syntax_Pointer].LexStr, procedureId);
        //找到该标识符在符号表中的位置
        if (tableIndex != -1)
        {
            if (sym_table[tableIndex].kind == "CONST")
            {
                codeAdd("lit", 0, sym_table[tableIndex].constvalue); //加载常数，直接把常数的值传递给中间代码
            }
            else if (sym_table[tableIndex].kind == "VAR")
            { //加载变量，需要沿着静态链查找变量，检查是否引用了内层变量

                codeAdd("lod", sym_table[tableIndex].attribute - procedureId, sym_table[tableIndex].address);
            }
        }
        else
        {
            cerr << "Syntax_Factor:: SYM " << LexTable[Syntax_Pointer].LexStr << " not declared" << endl;
            ReturnUnexpect(3);
        }
        //****************/
        Syntax_ID(tnode);
    }
    else if (LexTable[Syntax_Pointer].LexProperty == 3)
    {
        //NUMBER 无符号整数
        //****************目标代码生成，常数
        codeAdd("lit", 0, Convert_Str_Int(LexTable[Syntax_Pointer].LexStr));
        //*******************************
        Syntax_Unsint(tnode);
    }
    else if (LexTable[Syntax_Pointer].LexStr == "(")
    {
        node *ttnode = new node("(");
        tnode->mkchild(ttnode);
        Syntax_Pointer++;
        SyntaxAnalysis_Expression(tnode);
        if (LexTable[Syntax_Pointer].LexStr == ")")
        {
            node *ttmpnode = new node(")");
            tnode->mkchild(ttmpnode);
            Syntax_Pointer++;
        }
        else
        {
            SyntaxAnalysis_error();
        }
    }
    else
    {
        SyntaxAnalysis_error();
    }
    //cout<<"out syn factor"<<endl;
}
//关系运算符
void SyntaxAnalysis_Operator_Relation(node *cnode)
{
    if (LexTable[Syntax_Pointer].LexStr == "=" ||
        LexTable[Syntax_Pointer].LexStr == "#" ||
        LexTable[Syntax_Pointer].LexStr == "<" ||
        LexTable[Syntax_Pointer].LexStr == "<=" ||
        LexTable[Syntax_Pointer].LexStr == ">" ||
        LexTable[Syntax_Pointer].LexStr == ">=")
    {
        node *tnode = new node(LexTable[Syntax_Pointer].LexStr);
        cnode->mkchild(tnode);
        Syntax_Pointer++;
    }
    else
    {
        SyntaxAnalysis_error();
    }
}

//--------------

void codeoutput()
{
    cout << "code:" << endl;
    cout << "codeIndex   "
         << " "
         << "f      "
         << " "
         << "l     "
         << " "
         << "a      "
         << " "
         << "      " << endl;
    for (int i = 0; i < code.size(); i++)
    {
        cout << i << ":        " << code[i].funcCode << "       " << code[i].levelDiff << "       " << code[i].displacement << "         " << code[i].content << endl;
    }
    cout << endl;
}

void codeoutputpure()
{
    for (int i = 0; i < code.size(); i++)
    {
        cout << code[i].funcCode << " " << code[i].levelDiff << "   " << code[i].displacement << "     " << endl;
    }
}

void symtableoutput()
{
    cout << "Symble Table:" << endl;
    cout << "codeIndex   "
         << " "
         << "name      "
         << " "
         << "kind     "
         << " "
         << "attribute      "
         << " "
         << "address      " << endl;
    for (int i = 0; i < sym_table.size(); i++)
    {
        cout << i << ":        " << sym_table[i].name << "       " << sym_table[i].kind << "       " << sym_table[i].attribute << "         " << sym_table[i].address << endl;
    }
    cout << endl;
}

void codeoutputastxt()
{
    ofstream outcode;
    outcode.open("program.code");
    if (!outcode)
    {
        cerr << "codeoutputastxt :: Can't open file code.txt" << endl;
    }
    for (int i = 0; i < code.size(); i++)
    {
        outcode << code[i].funcCode << "   " << code[i].levelDiff << "   " << code[i].displacement << "      " << endl;
    }
}

int main()
{
    //Lex_ifreopen = true;
    Lex_ifreopen = false;     //是否进入调试模式（重定向输入）
    codefile = "program.txt"; //重定向输入文件
    //codefile = "jiecheng.txt";
    root = new node("root"); //创建语法树根节点
    Compiler_Lexcial();      //执行词法分析
    //outputfortest();
    //cout<<"lextable size == "<<LexTable.size()<<endl;
    if (LexTable.size() == 0) //报错
    {
        SyntaxAnalysis_error();
        return 0;
    }
    // if(Lex_ifreopen){
    //     freopen("CON", "r", stdin);
    // }

    Compiler_Syntax(root);                 //语法分析和目标代码生成
    if (LexTable.size() != Syntax_Pointer) //未知错误，未分析完成报错
    {
        SyntaxAnalysis_error();
    }
    // if(root->child[0]!=NULL){
    //     root->child[0]->print();
    // }else{
    //     SyntaxAnalysis_error();
    // }

    //symtableoutput();
    //codeoutput();
    codeoutputastxt(); //把中间代码输出到文件
    //codeoutputpure();
    //Interpreter();
    // cout<<endl;
    // if(root->child[0]==NULL){
    //     cout<<"hhh error"<<endl;
    // }else{
    //     cout<<root->child[0]->str<<endl;
    //     cout<<"Asdfasd"<<endl;
    // }

    return 0;
}
