// UTF-8 encoded

#include "adv.h"

int main(void)
{
  InitBoard(); // 初始化棋盘
  InitAI(); // 初始化AI
  char command[10]="";
  while(true)
  {
    scanf("%s", command);
    if(strcmp(command, "START")==0)
    {
      int size=20;
      scanf("%d", &size);
      if(size>MaxSize||size<=5)
      {
        printf("ERROR\n");
        fflush(stdout);
      }
      else
      {
        SetSize(size);
        printf("OK\n");
        fflush(stdout);
      }
    }
    else if(strcmp(command, "RESTART")==0)
    {
      ReStart();
      printf("OK\n");
      fflush(stdout);
    }
    else if(strcmp(command, "TAKEBACK")==0)
    {
      DelMove();
      printf("OK\n");
      fflush(stdout);
    }
    else if(strcmp(command, "BEGIN")==0)
    {
      Pos best=TurnBest();
      TurnMove(best);
      printf("%d,%d\n", best.x, best.y);
      fflush(stdout);
    }
    else if(strcmp(command, "TURN")==0)
    {
      Pos input;
      scanf("%d,%d", &input.x, &input.y);
      if (input.x<0||input.x>=Ribo.size||input.y<0||input.y>=Ribo.size||Ribo.cell[input.x+4][input.y+4].piece!=Empty)
      {
        printf("ERROR\n");
        fflush(stdout);
      }
      else
      {
        TurnMove(input);
        Pos best=TurnBest();
        TurnMove(best);
        printf("%d,%d\n", best.x, best.y);
        fflush(stdout);
      }
    }
    else if(strcmp(command, "BOARD")==0)
    {
      scanf("%s", command);
      while(strcmp(command, "DONE"))
        scanf("%s", command);
      //TODO
      printf("0,0\n");
      fflush(stdout);
    }
    else if(strcmp(command, "INFO")==0)
    {
      char key[16]="";
      int value;
      scanf("%s", key);
      if(strcmp(key, "timeout_turn")==0)
      {
        scanf("%d", &value);
        if(value>0)
          Ribo.timeout_turn=value;
      }
      else if(strcmp(key, "timeout_match")==0)
      {
        scanf("%d", &value);
        if(value>0)
          Ribo.timeout_match=value;
      }
      else if(strcmp(key, "time_left")==0)
      {
        scanf("%d", &value);
        if(value>0)
          Ribo.time_left=value;
      }
      else
      {
        scanf("%s", key);
        //TODO
      }
    }
    else if(strcmp(command, "END")==0)
      break;
  }
  return 0;
}

// ---------- 与棋盘有关的函数 ----------
void InitBoard() // 初始化棋盘
{
  Ribo.step=0;
  Ribo.size=20;
  InitType(); // 初始化半成品棋型
  InitPattern(); // 初始化完整棋型表
  InitPval(); // 初始化估值
  InitZobrist(); // 初始化zobrist hash表
  memset(Ribo.cell, 0, sizeof(Ribo.cell));
  memset(Ribo.IsLose, 0, sizeof(Ribo.IsLose));
  memset(Ribo.remMove, 0, sizeof(Ribo.remMove));
  memset(Ribo.hashTable, 0, sizeof(Ribo.hashTable));
  SetSize(20); // 设置棋盘默认尺寸为20
}

U64 Rand64() // 生成64位随机数
{
  return rand()^((U64)rand()<<15)^((U64)rand()<<30)^((U64)rand()<<45)^((U64)rand()<<60);
}

void InitZobrist() // 初始化zobrist hash表
{
  srand(time(NULL));
  for (int i=0;i<MaxSize+4;i++)
    for (int j=0;j<MaxSize+4;j++)
    {
      Ribo.zobrist[0][i][j]=Rand64();
      Ribo.zobrist[1][i][j]=Rand64();
    }
}

void SetSize(int _size) // 设置棋盘尺寸和边界
{
  Ribo.size=_size;
  Ribo.b_start=4, Ribo.b_end=Ribo.size+4;
  for (int i=0;i<MaxSize+8;i++)
    for (int j=0;j<MaxSize+8;j++)
      if (i<4||i>=Ribo.size+4||j<4||j>=Ribo.size+4)
        Ribo.cell[i][j].piece = Outside;
      else
        Ribo.cell[i][j].piece = Empty;
}

void MakeMove(Pos next) // 落子
{
  int x=next.x;
  int y=next.y;
  ++Ribo.step;
  Ribo.cell[x][y].piece=color(Ribo.step);
  Ribo.zobristKey^=Ribo.zobrist[Ribo.step&1][x][y];
  Ribo.remMove[Ribo.step]=next;
  UpdateType(x, y);
  for(int i=x-2;i<=x+2;i++)
    for(int j=y-2;j<=y+2;j++)
      Ribo.cell[i][j].IsCand++;
}

void DelMove() // 删子
{
  int x=Ribo.remMove[Ribo.step].x;
  int y=Ribo.remMove[Ribo.step].y;
  Ribo.zobristKey^=Ribo.zobrist[Ribo.step&1][x][y];
  --Ribo.step;
  Ribo.cell[x][y].piece=Empty;
  UpdateType(x, y);
  for(int i=x-2;i<=x+2;i++)
    for(int j=y-2;j<=y+2;j++)
      Ribo.cell[i][j].IsCand--;
}

void Undo() // 悔棋
{
  if(Ribo.step>=2)
  {
    DelMove();
    DelMove();
  }
}

void ReStart() // 重新开局
{
  Ribo.zobristKey = 0;
  memset(Ribo.hashTable, 0, sizeof(Ribo.hashTable));
  while(Ribo.step)
    DelMove();
}

void UpdateType(int x, int y) // 更新(x,y)点棋型
{
  int a, b;
  int key;
  for(int i=0; i<4; ++i)
  {
    a=x+dx[i];
    b=y+dy[i];
    for(int j=0;j<4&&CheckXy(a, b);a+=dx[i], b+=dy[i], ++j)
    {
      key=GetKey(a, b, i);
      Ribo.cell[a][b].pattern[0][i]=Ribo.patternTable[key][0];
      Ribo.cell[a][b].pattern[1][i]=Ribo.patternTable[key][1];
    }
    a = x-dx[i];
    b = y-dy[i];
    for(int k=0;k<4&&CheckXy(a, b);a-=dx[i], b-=dy[i], ++k)
    {
      key=GetKey(a, b, i);
      Ribo.cell[a][b].pattern[0][i]=Ribo.patternTable[key][0];
      Ribo.cell[a][b].pattern[1][i]=Ribo.patternTable[key][1];
    }
  }
}

int GetKey(int x, int y, int i) // 获取(x,y)点i方向子串
{
  int key=0;
  int a=x-dx[i]*4;
  int b=y-dy[i]*4;
  for(int k=0;k<9;a+=dx[i], b+=dy[i], k++)
  {
    if(k==4)
      continue;
    key<<=2;
    key^=Ribo.cell[a][b].piece;
  }
  return key;
}

int LineType(int role, int key) // 根据子串判断棋型，用于填充棋型表
{
  int line_left[9];
  int line_right[9];
  for(int i=0;i<9;i++)
    if(i==4)
    {
      line_left[i]=role;
      line_right[i]=role;
    }
    else
    {
      line_left[i]=key&3;
      line_right[8-i]=key&3;
      key>>=2;
    }
  // 双向判断，取最佳棋型
  int p1=ShortLine(line_left);
  int p2=ShortLine(line_right);
  // 同线双四,双三特判
  if(p1==block3&&p2==block3)
    return CheckFlex3(line_left);
  if(p1==block4&&p2==block4)
    return CheckFlex4(line_left);
  return p1>p2?p1:p2;
}

int CheckFlex3(int *line) // 单行双三特判
{
  int role=line[4];
  int type;
  for(int i=0;i<9;i++)
    if(line[i]==Empty)
    {
      line[i]=role;
      type=CheckFlex4(line);
      line[i]=Empty;
      if(type==flex4)
        return flex3;
    }
  return block3;
}

int CheckFlex4(int *line) // 单行双四特判
{
  int count;
  int five=0;
  int role=line[4];
  for(int i=0;i<9;i++)
    if(line[i]==Empty)
    {
      count=0;
      for(int j=i-1;j>=0&&line[j]==role;j--)
        count++;
      for(int j=i+1;j<=8&&line[j]==role;j++)
        count++;
      if(count>=4)
        five++;
    }
  return five>=2?flex4:block4;
}

int ShortLine(int *line) // 判断棋型（单方向）
{
  int kong=0, block=0;
  int len=1, len2=1, count=1;

  int role=line[4];
  for (int k=5;k<=8;k++)
    if(line[k]==role)
    {
      if(kong+count>4)
        break;
      ++count;
      ++len;
      len2=kong+count;
    }
    else if(line[k]==Empty)
    {
      ++len;
      ++kong;
    }
    else
    {
      if(len2==kong+count)
        ++block;
      break;
    }
  // 计算中间空格
  kong=len2-count;
  for(int k=3;k>=0;k--)
    if(line[k]==role)
    {
      if(kong+count>4)
        break;
      ++count;
      ++len;
      len2=kong+count;
    }
    else if(line[k]==Empty)
    {
      ++len;
      ++kong;
    }
    else
    {
      if(len2==kong+count)
        ++block;
      break;
    }
  return Ribo.typeTable[len][len2][count][block];
}

int GetType(int len, int len2, int count, int block) // 根据参数为ShortLine返回棋型
{
  if (len>=5&&count>1)
  {
    if(count==5)
      return win;
    if(len>5&&len2<5&&block==0)
    {
      switch(count)
      {
      case 2:
        return flex2;
      case 3:
        return flex3;
      case 4:
        return flex4;
      }
    }
    else
    {
      switch(count)
      {
      case 2:
        return block2;
      case 3:
        return block3;
      case 4:
        return block4;
      }
    }
  }
  return 0;
}

int GetPval(int a, int b, int c, int d) // 根据四个方向的棋型打分
{
  int type[8]={0};
  type[a]++; type[b]++; type[c]++; type[d]++;
  if(type[win]>0)
    return 5000;
  if(type[flex4]>0||type[block4]>1)
    return 1200;
  if(type[block4]>0&&type[flex3]>0)
    return 1000;
  if(type[flex3]>1)
    return 200;
  int val[6]={0, 2, 5, 5, 12, 12};
  int score=0;
  for (int i=1; i<=block4; i++)
    score+=val[i]*type[i];
  return score;
}

void InitType() // 初始化半成品棋型
{
  for(int i=0;i<10;++i)
    for(int j=0;j<6;++j)
      for(int k=0;k<6;++k)
        for(int l=0;l<3;++l)
          Ribo.typeTable[i][j][k][l]=GetType(i, j, k, l);
}

void InitPattern() // 初始化完整棋型表
{
  for (int key=0;key<65536;key++)
  {
    Ribo.patternTable[key][0]=LineType(0, key);
    Ribo.patternTable[key][1]=LineType(1, key);
  }
}

void InitPval() // 初始化估值
{
  for(int a=0;a<8;++a)
    for(int b=0;b<8;++b)
      for(int c=0;c<8;++c)
        for(int d=0;d<8;++d)
          Ribo.pval[a][b][c][d]=GetPval(a, b, c, d);
}

// ---------- 与棋盘有关的函数 结束 ----------
// ---------- 与AI有关的函数 ----------

void InitAI() // AI初始化
{
  Ribo.total = 0;
  Ribo.hashCount = 0;
  Ribo.BestVal = 0;
  Ribo.MaxDepth = 0;
  Ribo.SearchDepth = 18;
  Ribo.time_left = 10000000;
  Ribo.timeout_turn = 30000;
  Ribo.timeout_match = 10000000;
  Ribo.ThinkTime = 0;
}

int GetTime() // 获取当前时间
{
  return (double)(clock()-Ribo.start)/CLOCKS_PER_SEC*1000;
}

int StopTime() // 获取停止时间
{
  return (Ribo.timeout_turn<Ribo.time_left/7)?Ribo.timeout_turn:Ribo.time_left/7;
}

int ProbeHash(int depth, int alpha, int beta) // 查询置换表
{
  Hashe *phashe=&Ribo.hashTable[Ribo.zobristKey%hashSize];
  if (phashe->key==Ribo.zobristKey)
    if (phashe->depth>=depth)
    {
      switch (phashe->hashf)
      {
      case hash_exact:
        return phashe->val;
      case hash_alpha:
        if (phashe->val<=alpha)
        return phashe->val;
        break;
      case hash_beta:
        if (phashe->val>=beta)
        return phashe->val;
        break;
      }
    }
  return unknown;
}

void RecordHash(int depth, int val, int hashf) // 写入置换表
{
  Hashe *phashe=&Ribo.hashTable[Ribo.zobristKey%hashSize];
  phashe->key=Ribo.zobristKey;
  phashe->val=val;
  phashe->hashf=hashf;
  phashe->depth=depth;
}

void TurnMove(Pos next) // 界面落子
{
  next.x+=4, next.y+=4;
  MakeMove(next);
}

Pos TurnBest() // 返回最佳点
{
  Pos best=gobang();
  best.x-=4, best.y-=4;
  return best;
}

Pos gobang() // 搜索最佳点
{
  Ribo.start=clock();
  Ribo.total=0;
  Ribo.BestVal=0;
  Ribo.hashCount=0;
  Ribo.stopThink=false;
  // 第一步下中央
  if(Ribo.step==0)
  {
    Ribo.BestMove.x=Ribo.size/2+4;
    Ribo.BestMove.y=Ribo.size/2+4;
    return Ribo.BestMove;
  }
  /*
  if(Ribo.step==1||Ribo.step==2)
  {
    int rx, ry;
    int d=Ribo.step*2+1;
    srand(time(NULL));
    do
    {
      rx=rand()%d+Ribo.remMove[1].x-Ribo.step;
      ry=rand()%d+Ribo.remMove[1].y-Ribo.step;
    } while(!CheckXy(rx, ry)||Ribo.cell[rx][ry].piece!=Empty);
    Ribo.BestMove.x=rx;
    Ribo.BestMove.y=ry;
    return Ribo.BestMove;
  }
  */
  //第二步随机下对角
  if(Ribo.step==1)
  {
    int rx, ry;
    srand(time(NULL));
    do
    {
      rx=(2*(rand()%2)-1)*dx[2+rand()%2]+Ribo.remMove[1].x;
      ry=(2*(rand()%2)-1)*dy[2+rand()%2]+Ribo.remMove[1].y;
    } while(!CheckXy(rx, ry)||Ribo.cell[rx][ry].piece!=Empty);
    Ribo.BestMove.x=rx;
    Ribo.BestMove.y=ry;
    return Ribo.BestMove;
  }
  //第三步随机下第二步周围
  if(Ribo.step==2)
  {
    int rx, ry;
    srand(time(NULL));
    do
    {
      rx=rand()%3+Ribo.remMove[2].x-1;
      ry=rand()%3+Ribo.remMove[2].y-1;
    } while(!CheckXy(rx, ry)||Ribo.cell[rx][ry].piece!=Empty);
    Ribo.BestMove.x=rx;
    Ribo.BestMove.y=ry;
    return Ribo.BestMove;
  }
  // 迭代加深搜索
  memset(Ribo.IsLose, false, sizeof(Ribo.IsLose));
  for(int i=2;i<=Ribo.SearchDepth&&Ribo.BestVal!=10000;i+=2)
  {
    if(i>8&&GetTime()*12>=StopTime())
      break;
    Ribo.MaxDepth=i;
    Ribo.BestVal=minimax(i, -10001, 10000);
  }
  Ribo.ThinkTime = GetTime();
  return Ribo.BestMove;
}

bool Same(Pos a, Pos b) // 判断两点是否为同一点（可内联）
{
  return a.x==b.x&&a.y==b.y;
}

int minimax(int depth, int alpha, int beta) // 根节点极大极小值搜索
{
  Pos move[64];
  int move_count = GetMove(move, 40);
  if(move_count==1)
  {
    Ribo.BestMove=move[1];
    return Ribo.BestVal;
  }
  move[0]=(depth>2)?Ribo.BestMove:move[1];
  // 遍历可选点
  int val;
  for(int i=0;i<=move_count;i++)
  {
    if(i>0&&Same(move[0], move[i]))
      continue;
    // 跳过必败点
    if(Ribo.IsLose[move[i].x][move[i].y])
      continue;
    MakeMove(move[i]);
    do
    {
      if(i>0&&alpha+1<beta)
      {
        val=-AlphaBeta(depth-1, -alpha-1, -alpha);
        if(val<=alpha||val>=beta)
          break;
      }
      val=-AlphaBeta(depth-1, -beta, -alpha);
    } while(0);
    DelMove();
    if(Ribo.stopThink) break;
    if(val==-10000)
      Ribo.IsLose[move[i].x][move[i].y] = true;
    if(val>=beta)
    {
      Ribo.BestMove=move[i];
      return val;
    }
    if(val>alpha)
    {
      alpha=val;
      Ribo.BestMove=move[i];
    }
  }
  return alpha==-10001?Ribo.BestVal:alpha;
}

int AlphaBeta(int depth, int alpha, int beta) // 带PVS的Alpha-Beta剪枝搜索
{
  Ribo.total++;
  static int cnt = 1000;
  if(--cnt<=0)
  {
    cnt=1000;
    if(GetTime()+200>=StopTime())
      Ribo.stopThink=true;
  }
  // 对方已成五
  if(CheckWin())
    return -10000;
  // 叶子节点
  if(depth==0)
    return evaluate();
  int val=ProbeHash(depth, alpha, beta);
  if(val!=unknown)
  {
    Ribo.hashCount++;
    return val;
  }
  Pos move[64];
  int move_count=GetMove(move, 40);
  int hashf=hash_alpha;
  int val_best=-10000;
  for (int i=1;i<=move_count;i++)
  {
    MakeMove(move[i]);
    do
    {
      if (i>1&&alpha+1<beta)
      {
        val=-AlphaBeta(depth-1, -alpha-1, -alpha);
        if(val<=alpha||val>=beta)
          break;
      }
      val=-AlphaBeta(depth-1, -beta, -alpha);
    } while(0);
    DelMove();
    if(Ribo.stopThink)
      break;
    if(val>=beta)
    {
      RecordHash(depth, val, hash_beta);
      return val;
    }
    if(val>val_best)
    {
      val_best=val;
      if(val>alpha)
      {
        hashf=hash_exact;
        alpha=val;
      }
    }
  }
  if(!Ribo.stopThink)
    RecordHash(depth, val_best, hashf);
  return val_best;
}

int CutCand(Pos* move, Point* cand, int Csize) // 棋型剪枝
{
  int you=color(Ribo.step);
  int me=you^1;
  int Msize=0;
  if(cand[1].val>=2400)
  {
    move[1]=cand[1].p;
    Msize=1;
  }
  else if(cand[1].val==1200)
  {
    move[1]=cand[1].p;
    Msize=1;
    if(cand[2].val==1200)
    {
      move[2]=cand[2].p;
      Msize=2;
    }
    Cell* p;
    for(int i=Msize+1;i<=Csize;++i)
    {
      p=&Ribo.cell[cand[i].p.x][cand[i].p.y];
      if(IsType(p, me, block4)||IsType(p, you, block4))
      {
        ++Msize;
        move[Msize]=cand[i].p;
      }
    }
  }
  return Msize;
}

int GetMove(Pos* move, int MaxMoves) // 获取最好的MaxMoves个着法
{
  int Csize=0, Msize=0;
  int you=color(Ribo.step);
  int me=you^1;
  int val;
  for(int i=Ribo.b_start;i<Ribo.b_end;i++)
    for(int j=Ribo.b_start;j<Ribo.b_end;j++)
      if(Ribo.cell[i][j].IsCand&&Ribo.cell[i][j].piece==Empty)
      {
        val=ScoreMove(&Ribo.cell[i][j], me, you);
        if(val>0)
        {
          ++Csize;
          Ribo.cand[Csize].p.x=i;
          Ribo.cand[Csize].p.y=j;
          Ribo.cand[Csize].val=val;
        }
      }
  sort(Ribo.cand, Csize);
  Csize=(Csize<MaxMoves)?Csize:MaxMoves; // 剪枝
  Msize=CutCand(move, Ribo.cand, Csize);
  // 如果没发生剪枝
  if(Msize==0)
  {
    Msize=Csize;
    for(int k=1;k<=Msize;++k)
      move[k]=Ribo.cand[k].p;
  }
  return Msize;
}

void sort(Point* a, int n) // 插入排序
{
  int i, j;
  Point key;
  for(i=2;i<=n;i++)
  {
    key=a[i];
    for(j=i;j>1&&a[j-1].val<key.val;j--)
      a[j] = a[j-1];
    a[j]=key;
  }
}

int evaluate() // 局面评价
{
  int Ctype[8]={0}, Htype[8]={0};
  int you=color(Ribo.step), me=you^1;
  int p_block4;
  for(int i=Ribo.b_start;i<Ribo.b_end;++i)
    for(int j=Ribo.b_start; j<Ribo.b_end;++j)
      if(Ribo.cell[i][j].IsCand&&Ribo.cell[i][j].piece==Empty)
      {
        p_block4=Ctype[block4];
        TypeCount(&Ribo.cell[i][j], me, Ctype);
        TypeCount(&Ribo.cell[i][j], you, Htype);
        if (Ctype[block4]-p_block4>1)
          Ctype[flex4]++;
      }
  if(Ctype[win]>0)
    return 10000;
  if(Htype[win]>1)
    return -10000;
  if(Ctype[flex4]>0&&Htype[win]==0)
    return 10000;
  int Cscore=0, Hscore=0;
  for (int i=1;i<8;++i)
  {
    Cscore+=Ctype[i]*Cval[i];
    Hscore+=Htype[i]*Hval[i];
  }
  return Cscore-Hscore;
}

int ScoreMove(Cell* c, int me, int you) // 着法打分
{
  int score[2];
  score[me]=Ribo.pval[c->pattern[me][0]][c->pattern[me][1]][c->pattern[me][2]][c->pattern[me][3]];
  score[you]=Ribo.pval[c->pattern[you][0]][c->pattern[you][1]][c->pattern[you][2]][c->pattern[you][3]];
  if(score[me]>=200||score[you]>=200)
    return score[me]>=score[you]?score[me]*2:score[you];
  else
    return score[me]*2+score[you];
}
