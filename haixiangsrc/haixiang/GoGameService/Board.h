#pragma once
#include "boost/smart_ptr.hpp"
#include "Chess.h"
#include <map>
#include <list>
#include <vector>

//走棋记录
class BoardRecorder : public boost::enable_shared_from_this<BoardRecorder>
{
public:
	enum RecorderType
	{
		ADD = 0,				//走棋
		EAT = 1,				//吃子
		PASS = 2,				//pass
	};
	RecorderType type;				//走棋类型
	Chess::ChessType cType;		//操作方
	int PosX;
	int PosY;
	int step;									//步数，只有add有步数
};

typedef boost::shared_ptr<BoardRecorder> recorder_ptr;


class Board : public boost::enable_shared_from_this<Board>
{
public:
	int cur_turn;															//回合（1黑2白）
	int cur_timer;														//当前计时
	int history;															//总回合次
	int cur_step;															//当前步数

	float fBlack;															//黑子得分
	float fWhite;															//白子得分


	Chess* last_BChess;												//黑子最后一步棋
	Chess* last_WChess;												//白子最后一步棋
	std::vector<Chess*>	last_eat_list;				//最后一步被吃的子
	std::list<recorder_ptr> recorder_list;		//走棋的记录

	//棋盘
	std::map<std::string, Chess*> chess_list;
	//模拟棋盘
	std::map<std::string, Chess*> simulation_list;



	Board();
	~Board();
	bool init();
	void reset();
	//当前位置是否是空
	bool ChessIsEmpty(int x, int y);
	//添加棋子
	int addChess(int type, int x, int y, bool is_undo = false);
	//移除棋子
	int removeChess(int x, int y);
	//是否有气
	bool	isLive(int type, int x, int y);
	//搜索块
	void	findBlock(int type, int x, int y, std::list<Chess*>& clist);
	//是否能吃子
	bool isEat(int type, int x, int y, std::vector<Chess*>& clist);
	//尝试吃子
	bool tryEat(int type, int x, int y, std::vector<Chess*>& clist);
	//吃子
	void submitEat(int type, int x, int y);
	//执行一次悔棋
	bool carry_out_undo(int type, recorder_ptr& cur_recorder);
	//清除某一个棋子
	void cleanChess(int x, int y);
	//计算得分
	bool caclulateResult();
	//获得某一行棋盘信息
	int getDataByLine(int y, int type);
	//获得某一个棋子
	Chess* getChessByPoint(int x, int y, std::map<std::string, Chess*>& map);
	//点目
	std::list<Chess*> goPoint(int type);
	
	void setPoint(int x, int y, int type, bool isHide);
	void clearPoint();
private:
	
	//获得同类的子list
	std::list<Chess*> getChessFormType(int type, std::map<std::string, Chess*>& map);
	//获得对手类型
	Chess::ChessType getopponent(Chess::ChessType type);

	//点目-点目操作
	void setPoint(int type, int x, int y, std::list<Chess*>& clist);
};