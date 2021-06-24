//#include"YouCan.cpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <string.h>
#include <string>
#include <algorithm>
#include <stack>
#include <fstream>

using namespace std;
//-----------------------------------------------------------

int codeId = 0;
ofstream outactionstack; //打印活动栈
//中间代码指令
struct Instruction
{
	string funcCode;  //f
	int levelDiff;	  //l
	int displacement; //a
	string content;
	Instruction()
	{
		string funcCode = " ";
		levelDiff = 0;
		displacement = 0;
		string content = " ";
	}
};

vector<Instruction> code; //中间代码
//加入一条中间代码
void codeAdd(string str, int lev, int place)
{
	Instruction temp;
	temp.funcCode = str;
	temp.levelDiff = lev;
	temp.displacement = place;
	//temp.content = con;
	code.push_back(temp);
	codeId++;
}
//初始化中间代码，即将文件中的中间代码加载进入内存
void InitCode2()
{
	code.clear();
	ifstream incode;
	incode.open("program.code", ios_base::in);
	if (!incode)
	{
		cerr << "Interpreter.cpp::InitCode:: Can't open file code.txt" << endl;
	}
	string str;
	int lev;
	int place;
	while (incode >> str)
	{
		incode >> lev;
		incode >> place;
		codeAdd(str, lev, place);
	}
	return;
}
//将标准输入的中间代码加载入内存
void InitCode()
{
	code.clear();
	string str;
	int lev;
	int place;
	while (cin >> str)
	{
		cin >> lev;
		cin >> place;
		codeAdd(str, lev, place);
	}
	return;
}
//用于调试，查看中间代码输出
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

//------------------------------------------------------------

//解释执行函数
struct var
{
	string name;
	int value;
};

vector<int> action1;	   //活动记录表
int moveId = 0;			   //动态链
int backId = 0;			   //返回地址
int staticId = 0;		   //静态链
int actId = 0;			   //活动表条目数
int cid = 0;			   //当前中间代码编号
bool ifprintstack = false; //是否打印运行栈信息
//stack<int> com;//运算栈
//stack<action> actStack;//活动记录栈
//运行栈
struct mystack
{
	stack<int> s;
	void pop()
	{
		int temp = s.top();
		s.pop();
		if (ifprintstack)
		{
			cout << "stack::pop" << temp << endl;
		}
	}
	void push(int a)
	{
		s.push(a);
		if (ifprintstack)
		{
			cout << "stack::push" << a << endl;
		}
	}
	int top()
	{
		int temp = s.top();
		if (ifprintstack)
		{
			cout << "stack::top" << temp << endl;
		}
		return temp;
	}
	void empty()
	{
		s.empty();
		if (ifprintstack)
		{
			cout << "stack::empty!" << endl;
		}
	}
};

mystack com; //运行栈
//是否打印运行栈信息
void Print_Stack(bool b)
{
	ifprintstack = b;
}
//活动记录表回退时清空的操作
void popactionstack(int oldsp)
{

	//int i = action1.size();
	int cnt = oldsp - actId;
	int temp = cnt;
	while (cnt != 0)
	{
		action1.pop_back();
		cnt--;
	}
	outactionstack << "pop out " << temp << " num of actionstack" << endl;
	return;
}
//打印运行栈信息
void printactionstack()
{
	//	vector<int>::iterator it = action1.begin();
	//	outactionstack << "actionstack update" << endl;
	//	outactionstack << "top ptr: actId " << actId << endl;
	//	outactionstack << "cid : " << cid << endl;
	//	outactionstack << "moveId: " << moveId << endl;
	//	outactionstack << "backId " << backId << endl;
	//	outactionstack << "staticId" << staticId << endl;
	//	int i = 0;
	//	for (; it != action1.end(); it++, i++)
	//	{
	//		outactionstack << i << ": [ " << *it << " ]" << endl;
	//	}
	//	return;
}
//初始化
void init()
{

	outactionstack.open("actionstack.txt", ios_base::out);
	if (!outactionstack)
	{
		cout << "printactionstack ::can't open file" << endl;
	}
	outactionstack << "new we begin" << endl;
}
//解释器入口
void Interpreter()
{
	action1.clear();
	actId = 0;
	com.empty();
	//actStack.empty();
	int temp = 0;	   //用户输入
	cid = 0;		   //代码数组下标
	int tempCount = 0; //暂时保存

	int tempMoveId = 0;
	int tempStaticId = 0;
	int lev = 0; //当前层数
	//add op 30 for minus
	//int codesize = code.size();
	//cout << codesize << " " << codeId << endl;
	while (cid != codeId - 1)
	{
		if (code[cid].funcCode == "jmp") //跳转语句
		{
			if (cid == 0) //初始的入栈
			{
				//action1[actId] =0;
				action1.push_back(0);
				moveId = 0;	  //当前动态链index
				backId = 1;	  //返回地址index
				staticId = 2; //静态连index
				actId++;
				//action1[actId] = 0;
				action1.push_back(0); //初始动态链地址为0，初始返回地址为0，初始静态链地址为0
				actId++;
				//action1[actId] = 0;
				action1.push_back(0);
				actId++;
			}
			cid = code[cid].displacement; //跳转到 a条指令
			outactionstack << "call from jmp" << endl;
			printactionstack();
		}
		else if (code[cid].funcCode == "opr") //运算语句
		{
			switch (code[cid].displacement)
			{
			case 0: //退出过程
			{
				outactionstack << "call from opr 0 : befor exit" << endl;
				printactionstack();
				int tempoldsp = actId;
				cid = action1[backId];	  //返回地址部分放着返回地址的代码
				actId = moveId;			  //活动记录表栈顶指针退回到动态链index（其实是上一个过程结束index+1）
				moveId = action1[moveId]; //更新老SP（动态链地址）
				backId = moveId + 1;	  //返回地址是动态链地址+1
				staticId = moveId + 2;	  //静态链
				cid--;					  //因为在switch最后cid++了
				popactionstack(tempoldsp);
				outactionstack << "call from opr 0 : exit" << endl;
				printactionstack();
				break;
			}
			case 1: //加法运算
				tempCount = com.top();
				com.pop();
				tempCount = tempCount + com.top();
				com.pop();
				com.push(tempCount);
				break;
			case 2: //减法运算
				tempCount = com.top();
				com.pop();
				tempCount = com.top() - tempCount;
				com.pop();
				com.push(tempCount);
				break;
			case 3: //乘法运算
				tempCount = com.top();
				com.pop();
				tempCount = com.top() * tempCount;
				com.pop();
				com.push(tempCount);
				break;
			case 4: //除法运算
				tempCount = com.top();
				com.pop();
				tempCount = com.top() / tempCount;
				com.pop();
				com.push(tempCount);
				break;
			case 5: //等于判断
				tempCount = com.top();
				com.pop();
				if (tempCount == com.top())
					tempCount = 1;
				else
					tempCount = 0;
				com.pop();
				com.push(tempCount);
				break;
			case 6: //不等号运算
				tempCount = com.top();
				com.pop();
				if (tempCount == com.top())
					tempCount = 0;
				else
					tempCount = 1;
				com.pop();
				com.push(tempCount);
				break;
			case 7: //小于
				tempCount = com.top();
				com.pop();
				if (com.top() < tempCount)
					tempCount = 1;
				else
					tempCount = 0;
				com.pop();
				com.push(tempCount);
				break;
			case 8: //小于等于
				int atemp1, atemp2;
				atemp1 = com.top();
				com.pop();
				atemp2 = com.top();
				com.pop();
				if (atemp2 <= atemp1)
				{
					com.push(1);
				}
				else
				{
					com.push(0);
				}
				break;
			case 9:
			{ //大于
				//int atemp1,atemp2;
				atemp1 = com.top();
				com.pop();
				atemp2 = com.top();
				com.pop();
				if (atemp2 > atemp1)
				{
					com.push(1);
				}
				else
				{
					com.push(0);
				}
				break;
			}
			case 10:
			{ //大于等于
				int atemp1, atemp2;
				atemp1 = com.top();
				com.pop();
				atemp2 = com.top();
				com.pop();
				if (atemp2 >= atemp1)
				{
					//cout << ">= right" << endl;
					com.push(1);
				}
				else
				{
					//cout << atemp2 << ">= no" << atemp1 << endl;

					com.push(0);
				}
				break;
			}
			case 22: //判断是否为奇数

				if (com.top() % 2 == 1)
				{
					tempCount = 1;
				}
				else
				{
					tempCount = 0;
				}

				com.pop();
				com.push(tempCount);
				break;
			case 28: //输入
				//cout << "please input";
				cin >> temp;
				//vartable[vartableid].
				com.push(temp); //读入一个数放入栈顶
				break;
			case 29: //输出
				//cout << "we achive write" << endl;
				cout << com.top() << endl; //将栈顶元素输出
				break;
			case 30:
				int aaatmp = com.top();
				com.pop();
				com.push(-aaatmp);
				break;
			}
			cid++;
		}
		else if (code[cid].funcCode == "lit") //常量声明
		{
			com.push(code[cid].displacement);
			cid++;
		}
		else if (code[cid].funcCode == "lod") //加载变量
		{
			// int tempId = 0;
			// int tempoldstatic;
			// int tempoldsp;
			// if (code[cid].levelDiff == 0)
			// 	tempId = moveId;
			// else
			// {
			// 	tempId = staticId; //沿着静态链找n次，n为code中的层数差

			// 	for (int i = 0; i < code[cid].levelDiff; i++)
			// 	{
			// 		tempoldsp=action1[tempId];
			// 		tempId=tempoldsp+2;
			// 		//tempId = action1[tempId];
			// 	}
			// }
			int templev_lod = code[cid].levelDiff;
			//在编译器阶段就应该报错
			if (templev_lod > 0)
			{
				cerr << "Interpreter :: sto :: can't use higher >0 var error witch reported when compiling" << endl;
			}
			templev_lod = -templev_lod;

			int tempoldstatic;
			int tempoldsp;
			tempoldstatic = staticId;
			tempoldsp = moveId;

			for (int i = 0; i < templev_lod; i++)
			{
				tempoldsp = action1[tempoldstatic];
				tempoldstatic = tempoldsp + 2;
			}
			//将变量取出来
			//变量在找到的静态链的基地址+偏移量
			com.push(action1[tempoldsp + code[cid].displacement]);
			cid++;
		}
		else if (code[cid].funcCode == "sto") //将栈顶结果送给变量
		{

			// int tempId = 0; //变量动态链地址（基址）
			// if (code[cid].levelDiff == 0)
			// 	tempId = moveId;
			// else
			// {
			// 	tempId = staticId;
			// 	for (int i = 0; i < code[cid].levelDiff; i++)
			// 	{
			// 		tempId = action1[tempId];
			// 	}
			// }
			// int tempindex = tempId + code[cid].displacement;
			// action1[tempindex] = com.top();
			// cid++;
			// //加载之后是否需要pop掉呢？
			// com.pop();
			int templev_sto = code[cid].levelDiff;
			//在编译器阶段就应该报错
			if (templev_sto > 0)
			{
				cerr << "Interpreter :: sto :: can't use higher >0 var error witch reported when compiling" << endl;
			}
			templev_sto = -templev_sto;

			int tempoldstatic;
			int tempoldsp;
			tempoldstatic = staticId;
			tempoldsp = moveId;

			for (int i = 0; i < templev_sto; i++)
			{
				tempoldsp = action1[tempoldstatic];
				tempoldstatic = tempoldsp + 2;
			}
			//将变量取出来
			//变量在找到的静态链的基地址+偏移量
			int tempindex = tempoldsp + code[cid].displacement;
			action1[tempoldsp + code[cid].displacement] = com.top();
			com.pop();
			cid++;

			outactionstack << "call from sto var " << tempindex << " changed " << endl;
			printactionstack();
		}
		else if (code[cid].funcCode == "cal") //过程调用语句
		{
			int tempMoveId = moveId; //tempMoveId是老sp
			int tempStaticId = staticId;
			int tempBackId = backId;
			//action1[actId] = moveId;
			action1.push_back(moveId); //新过程的动态链指向老sp

			moveId = actId; //新过程的动态链地址
			actId++;
			//action1[actId] = cid + 1;//修改返回地址,返回地址是call语句下一句代码
			action1.push_back(cid + 1);
			backId = actId;
			actId++;
			//静态链形成，bug1，没考虑p层调用q层，当p>q层的情况，也就是说，可能高层调用低层
			//修改
			//将call指令的l改成过程的相对层次，分为0，+1，-1三种情况，应该不会出现调用层差为2的情况吧
			// if (code[cid].levelDiff == 0) //层差为0，意味着直接外层与调用者相同
			// {
			// 	//tempStaticId = staticId;
			// 	//action1[actId] = action1[tempMoveId];//新的静态链
			// 	//*****************************************************
			// 	//action1.push_back(action1[tempMoveId]);
			// 	//修改
			// 	action1.push_back(action1[staticId]);
			// 	//*****************************************************
			// 	staticId = actId;
			// 	actId++;
			// }
			// else if (code[cid].levelDiff == 1) //层差为+1
			// {
			// 	// tempStaticId = staticId;
			// 	// tempCount = staticId;
			// 	//action1[actId] = action1[tempCount];//新的静态链
			// 	//*****************************************************************
			// 	//action1.push_back(action1[tempCount]);
			// 	//修改
			// 	action1.push_back(tempMoveId); //静态链地址指向外层动态链地址index（老SP）
			// 	//********************************************************
			// 	staticId = actId;
			// 	actId++;
			// }
			int templevdiff = code[cid].levelDiff;
			if (templevdiff == 0)
			{
				//层差为0，意味着直接外层与调用者相同
				action1.push_back(action1[staticId]);
				staticId = actId;
				actId++;
			}
			else if (templevdiff > 0)
			{
				//层差大于0，调用高层
				//事实上temlevdiff若为正，只能为1
				//因为只能调用被自己直接包含的高层嵌套
				//检查错误，事实上在编译阶段就应该报错。
				if (templevdiff != 1)
				{
					cerr << "Interpreter :: Call :: static link generator error :: can't call higher > 1 which should be detected when compiling" << endl;
				}
				action1.push_back(tempMoveId); //指向老sp
				staticId = actId;
				actId++;
			}
			else if (templevdiff < 0)
			{
				//调用包含自己的外层，可能是直接外层，也可能是多个嵌套的直接外层
				//静态链地址调整为该外层的值。
				//因为活动记录表要入栈（可能是递归调用，动态链在变化）
				//与变量寻址时查找静态链过程一样
				int ablev = -templevdiff;
				int tempold_Spinhere;
				int tempold_Statichere = tempStaticId;

				for (int i = 0; i < ablev; i++)
				{
					tempold_Spinhere = action1[tempold_Statichere];
					tempold_Statichere = tempold_Spinhere + 2;
				}
				action1.push_back(action1[tempold_Statichere]); //指向当前这层的静态层
				staticId = actId;
				actId++;
			}

			cid = code[cid].displacement;
			outactionstack << "call from cal,add a new motion level" << endl;
			printactionstack();
		}
		else if (code[cid].funcCode == "jpc") //非真跳转语句
		{
			int stemptop = com.top();
			com.pop();
			if (stemptop == 0)
			{
				//cout << "tiao" << endl;
				cid = code[cid].displacement;
			}
			else
			{
				//cout << "no tiaozhuan" << endl;
				cid++;
			}
		}
		else if (code[cid].funcCode == "int")
		{ //变量表初始化//开辟空间
			int codesizetemp = code[cid].displacement - 3;
			for (int i = 0; i < codesizetemp; i++)
			{ //前三个分别是动态链（老sp）、返回地址、静态链
				//action1[actId] = 0;
				action1.push_back(0);
				actId++;
			}
			cid++;
			outactionstack << "call from int，add var place , size= " << codesizetemp << endl;
			printactionstack();
		}
	}
}

//------------------------------------------------------------
//主程序入口
int main()
{

	InitCode2();
	init();
	//codeoutput();
	//freopen("Interpreterout.txt", "w", stdout);//调试用的重定向信息
	//freopen("test1.in", "r", stdin);//调试用的重定向信息
	//Print_Stack(false);
	Interpreter();
	return 0;
}
