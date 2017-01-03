// UTF-8 encoded

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------- 与棋盘有关的声明 ----------

typedef unsigned long long U64;
typedef unsigned char bool;

#define true 1
#define false 0

// 棋盘参数
#define Ntype 8 // 棋型个数
#define MaxSize 20 // 允许的棋盘最大尺寸
#define hashSize 1 << 20 // hash表大小

// 棋型常量
#define win 7 // 连五
#define flex4 6 // 活四
#define block4 5 // 冲四
#define flex3 4 // 活三
#define block3 3 // 眠三
#define flex2 2 // 活二
#define block2 1 // 眠二

// hash表常量
#define hash_exact 0 // 精确值hash
#define hash_alpha 1 // alpha值hash
#define hash_beta 2 // beta值hash
#define unknown -20000 // 未知值

// 点状态常量
#define Outside 3 // 界外
#define Empty 2
#define Black 1
#define White 0

// 四个正方向的方向向量
const int dx[4] = { 1, 0, 1, 1 };
const int dy[4] = { 0, 1, 1, -1 };

// 坐标
typedef struct Pos {
  int x;
  int y;
} Pos;

// 包含估值的点
typedef struct Point {
  Pos p; // 坐标
  int val; // 估值
} Point;

// 棋盘单点结构
typedef struct Cell {
  int piece; // 点状态（以点状态常量表示）
  int IsCand; // 附近两格内是否有棋子
  int pattern[2][4]; // 双方四个方向的棋型
} Cell;

// hash表结构
typedef struct Hashe {
  U64 key; // hash键
  int depth; // 当前深度
  int hashf; // hash模式（以hash表常量表示）
  int val; // hash值
} Hashe;

// 棋盘和AI的主结构
typedef struct Board {
  // 棋盘相关
  int step; // 当前步数（从0开始计数）
  int size; // 当前棋盘大小
  int b_start, b_end; // 棋盘坐标范围
  U64 zobristKey; // 表示当前局面的zobristKey
  U64 zobrist[2][MaxSize+4][MaxSize+4]; // zobrist键值表
  Hashe hashTable[hashSize];  // hash表
  int typeTable[10][6][6][3]; // 半成品棋型表
  int patternTable[65536][2]; // 完整棋型表
  int pval[8][8][8][8]; // 棋型估值
  Cell cell[MaxSize+8][MaxSize+8]; // 棋盘
  Pos remMove[MaxSize*MaxSize]; // 落子记录（从1开始计数）
  Point cand[256]; // 保存所有两格内有子的点
  bool IsLose[MaxSize+4][MaxSize+4]; // 根节点上必输的点
  // AI相关
  int total; // 搜索局面数
  int hashCount; // hash表命中计数
  int BestVal; // 最佳点分值
  int MaxDepth; // 实际搜索层数
  int SearchDepth; // 最大搜索层数
  int time_left; // 当前剩余时间
  int timeout_turn; // 步时
  int timeout_match; // 局时
  int ThinkTime; // 当前思考时间
  bool stopThink; // 是否需要停止思考
  clock_t start; // 记录开始思考的时间
  Pos BestMove; // 最佳点
} Board;

Board Ribo; // 主结构实例

// 初始化函数
void InitBoard(); // 初始化棋盘
void InitType(); // 初始化半成品棋型
void InitPval(); // 初始化估值
void InitPattern(); // 初始化完整棋型表
void InitZobrist(); // 初始化zobrist hash表

void SetSize(int _size); // 设置棋盘尺寸和边界
void MakeMove(Pos next); // 落子
void DelMove(); // 删子
void Undo();  // 悔棋
void ReStart(); // 重新开局
void UpdateType(int x, int y); // 更新(x,y)点棋型
U64 Rand64(); // 生成64位随机数
int GetKey(int x, int y, int i); // 获取(x,y)点i方向子串
int LineType(int role, int key); // 根据子串判断棋型
int ShortLine(int* line); // 判断棋型（单方向）
int CheckFlex3(int* line); // 单行双三特判
int CheckFlex4(int* line); // 单行双四特判
int GetType(int len, int len2, int count, int block); // 根据参数为ShortLine返回棋型
int GetPval(int a, int b, int c, int d); // 根据四个方向的棋型打分

// 以下为可内联函数
int color(int step) { // 返回当前步落子方
  return step&1;
}

bool CheckXy(int x, int y) { // 判断(x,y)是否在棋盘内
  return Ribo.cell[x][y].piece!=Outside;
}

Cell* LastMove() { // 返回最后一步
  return &Ribo.cell[Ribo.remMove[Ribo.step].x][Ribo.remMove[Ribo.step].y];
}

void TypeCount(Cell* c, int role, int* type) { // 某点棋型统计
  ++type[c->pattern[role][0]];
  ++type[c->pattern[role][1]];
  ++type[c->pattern[role][2]];
  ++type[c->pattern[role][3]];
}

bool IsType(Cell* c, int role, int type) { // 判断某点是否有某棋型
  return c->pattern[role][0] == type
    || c->pattern[role][1] == type
    || c->pattern[role][2] == type
    || c->pattern[role][3] == type;
}

bool CheckWin() { // 判断是否获胜
  int role = color(Ribo.step);
  Cell* c = LastMove();
  return c->pattern[role][0] == win
    || c->pattern[role][1] == win
    || c->pattern[role][2] == win
    || c->pattern[role][3] == win;
}

// ---------- 与棋盘有关的声明 结束 ----------
// ---------- 与AI有关的声明 ----------

int Cval[8] = { 0, 3, 18, 27, 144, 216, 1200, 1800 }; // 评分表
int Hval[8] = { 0, 2, 12, 18, 96, 144, 800, 1200 }; // 评分表

void InitAI(); // AI初始化
Pos TurnBest(); // 返回最佳点
Pos gobang(); // 搜索最佳点
void sort(Point* a, int n); // 插入排序
void TurnMove(Pos next); // 界面落子
void RecordHash(int depth, int val, int hashf); // 写入置换表
int ProbeHash(int depth, int alpha, int beta); // 查询置换表
int GetTime(); // 获取当前时间
int StopTime(); // 获取停止时间
int ScoreMove(Cell* c, int me, int you); // 着法打分
int minimax(int depth, int alpha, int beta); // 根节点极大极小值搜索
int AlphaBeta(int depth, int alpha, int beta); // 带PVS的Alpha-Beta剪枝搜索
int CutCand(Pos* move, Point* cand, int Csize); // 棋型剪枝
int GetMove(Pos* move, int MaxMoves); // 获取最好的MaxMoves个着法
int evaluate(); // 局面评价
bool Same(Pos a, Pos b); // 判断两点是否为同一点（可内联）

// ---------- 与AI有关的声明 结束 ----------
