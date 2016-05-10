#include "Board.h"
#include <string>
#include "boost/lexical_cast.hpp"


int getX(Chess* var)
{
	return var->getPosX();
}

int getY(Chess* var)
{
	return var->getPosY();
}

//查寻当前坐标是否已经在队列中
template<typename T> 
bool existFromList(int x, int y, std::list<T>& list)
{
	std::list<T>::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		T _cs = *it;
		if(getX(_cs) == x && getY(_cs) == y)
		{
			return true;
		}
	}
	return false;
}


template<typename T>
bool existFromVector(int x, int y, std::vector<T>& list)
{
	std::vector<T>::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		T _cs = *it;
		if(getX(_cs) == x && getY(_cs) == y)
		{
			return true;
		}
	}
	return false;
}






Board::Board()
	:last_BChess(nullptr)
	,last_WChess(nullptr)
	,cur_turn(0)
	,cur_timer(0)
	,history(0)
	,cur_step(0)
{
	last_eat_list.clear();
	recorder_list.clear();
	init();
}

Board::~Board(){}

bool Board::init()
{
	chess_list.clear();
	for(int i = 0; i < 19; ++i)
	{
		for(int j = 0; j < 19; ++j)
		{
			Chess *chess = new Chess(i, j);
			std::string c_name = "c" + std::to_string(i) + "_" + std::to_string(j);
			chess_list[c_name] = chess;
		}
	}
	return true;
}

void Board::reset()
{
	last_BChess = nullptr;
	last_WChess = nullptr;
	last_eat_list.clear();
	recorder_list.clear();

	for(int i = 0; i < 19; i++)
	{
		for(int j = 0; j < 19; j++)
		{
			std::string c_name = "c" + std::to_string(i) + "_" + std::to_string(j);
			Chess *chess = chess_list[c_name];
			chess->reset();
		}
	}
}


bool Board::ChessIsEmpty(int x, int y)
{
	std::string c_name = "c" + std::to_string(x) + "_" + std::to_string(y);
	Chess* chess = chess_list[c_name];
	if(chess)
	{
		if(chess->isEmpty())return true;
	}
	return false;
}


int Board::addChess(int type, int x, int y, bool is_undo /*= false*/)
{
	Chess* chess = getChessByPoint(x, y, chess_list);
	if(chess != nullptr && chess->isEmpty())
	{
		//添加一个棋子的信息
		cur_step++;
		chess->addChess((enum Chess::ChessType)type, cur_step);

		if(!is_undo)
		{
			//加入记录
			recorder_ptr the_recorder;
			the_recorder.reset(new BoardRecorder);
			the_recorder->type = BoardRecorder::ADD;
			the_recorder->cType = chess->getType();
			the_recorder->PosX = chess->getPosX();
			the_recorder->PosY = chess->getPosY();
			the_recorder->step = cur_step;
			recorder_list.push_back(the_recorder);

			if(chess->getType() == Chess::Black)
			{
				last_BChess = chess;
			}
			else if(chess->getType() == Chess::White)
			{
				last_WChess = chess;
			}

			history++;
		}
		return 0;
	}else
	{
		return -1;
	}
}

int Board::removeChess(int x, int y)
{
	return 0;
}


bool Board::isLive(int type, int x, int y)
{
	std::string c_name = "c" + std::to_string(x) + "_" + std::to_string(y);
	Chess* chess = chess_list[c_name];

	//判断当前位置是否为空
	if(!chess->isEmpty())
		return false;

	//获得类型
	Chess::ChessType chess_type = (enum Chess::ChessType)type;
	std::list<Chess*> clist;
	findBlock(chess_type, x, y, clist);

	std::list<Chess*>::iterator it;
	for(it = clist.begin(); it != clist.end(); it++)
	{
		Chess *_cs = *it;
		std::string c_name;
		Chess* chess;

		if(_cs->hasLeft() && !existFromList<Chess*>(_cs->getPosX(), _cs->getPosY() - 1, clist))
		{

			c_name = "c" + std::to_string(_cs->getPosX()) + "_" + std::to_string(_cs->getPosY() - 1);
			chess = chess_list[c_name];
			if(chess && chess->getType() == Chess::Empty){
				return true;
			}
		}

		if(_cs->hasRight() && !existFromList<Chess*>(_cs->getPosX(), _cs->getPosY() + 1, clist))
		{
			c_name = "c" + std::to_string(_cs->getPosX()) + "_" + std::to_string(_cs->getPosY() + 1);
			chess = chess_list[c_name];
			if(chess && chess->getType() == Chess::Empty){
				return true;
			}
		}

		if(_cs->hasUp() && !existFromList<Chess*>(_cs->getPosX() - 1, _cs->getPosY(), clist))
		{
			c_name = "c" + std::to_string(_cs->getPosX() - 1) + "_" + std::to_string(_cs->getPosY());
			chess = chess_list[c_name];
			if(chess && chess->getType() == Chess::Empty){
				return true;
			}
		}

		if(_cs->hasDown() && !existFromList<Chess*>(_cs->getPosX() + 1, _cs->getPosY(), clist))
		{
			c_name = "c" + std::to_string(_cs->getPosX() + 1) + "_" + std::to_string(_cs->getPosY());
			chess = chess_list[c_name];
			if(chess && chess->getType() == Chess::Empty){
				return true;
			}
		}
	}
	return false;
}

bool Board::isEat(int type, int x, int y, std::vector<Chess*>& clist)
{
	//获得对手类型，如果传进来的是空位置，直接返回
	Chess::ChessType opponent_type;
	if((enum Chess::ChessType)type == Chess::Black)
	{
		opponent_type = Chess::White;
	}else if((enum Chess::ChessType)type == Chess::White)
	{
		opponent_type = Chess::Black;
	}else
		return false;

	bool bEat = false;
	Chess* chess = getChessByPoint(x, y, chess_list);
	if(chess)
	{
		Chess* n_chess;
		if(chess->hasLeft())
		{
			n_chess = getChessByPoint(chess->getPosX(), chess->getPosY() - 1, chess_list);
			if(n_chess && n_chess->getType() == opponent_type)
			{
				bEat = tryEat(n_chess->getType(), n_chess->getPosX(), n_chess->getPosY(), clist) || bEat;
			}
		}

		if(chess->hasRight())
		{
			n_chess = getChessByPoint(chess->getPosX(), chess->getPosY() + 1, chess_list);
			if(n_chess && n_chess->getType() == opponent_type)
			{
				bEat = tryEat(n_chess->getType(), n_chess->getPosX(), n_chess->getPosY(), clist) || bEat;
			}
		}

		if(chess->hasUp())
		{
			n_chess = getChessByPoint(chess->getPosX() - 1, chess->getPosY(), chess_list);
			if(n_chess && n_chess->getType() == opponent_type)
			{
				bEat = tryEat(n_chess->getType(), n_chess->getPosX(), n_chess->getPosY(), clist) || bEat;
			}
		}

		if(chess->hasDown())
		{
			n_chess = getChessByPoint(chess->getPosX() + 1, chess->getPosY(), chess_list);
			if(n_chess && n_chess->getType() == opponent_type)
			{
				bEat = tryEat(n_chess->getType(), n_chess->getPosX(), n_chess->getPosY(), clist) || bEat;
			}
		}
	}
	return bEat;
}

//------------------------------------------------------------------
// 尝试吃子                                                              
//------------------------------------------------------------------
bool Board::tryEat(int type, int x, int y, std::vector<Chess*>& clist)
{
	if((enum Chess::ChessType)type == Chess::Empty) return false;
	Chess* chess = getChessByPoint(x, y, chess_list);
	//如果当前位置是空的，不能被吃
	if(chess->isEmpty())
		return false;

	std::vector<Chess*> &list = clist;

	//获得当前块
	Chess::ChessType chess_type = (enum Chess::ChessType)type;
	std::list<Chess*> slist;
	findBlock(chess_type, x, y, slist);
	if(slist.size() > 0)
	{
		std::list<Chess*>::iterator it;
		for(it = slist.begin(); it != slist.end(); it++)
		{
			Chess *_cs = *it;
			std::string c_name;
			Chess* n_chess;
			
			if(_cs->hasLeft() && !existFromList<Chess*>(_cs->getPosX(), _cs->getPosY() - 1, slist))
			{
				n_chess = getChessByPoint(_cs->getPosX(), _cs->getPosY() - 1, chess_list);
				if(n_chess && (n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White))
				{
					return false;
				}
			}

			if(_cs->hasRight() && !existFromList<Chess*>(_cs->getPosX(), _cs->getPosY() + 1, slist))
			{
				n_chess = getChessByPoint(_cs->getPosX(), _cs->getPosY() + 1, chess_list);
				if(n_chess && (n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White))
				{
					return false;
				}
			}

			if(_cs->hasUp() && !existFromList<Chess*>(_cs->getPosX() - 1, _cs->getPosY(), slist))
			{
				n_chess = getChessByPoint(_cs->getPosX() - 1, _cs->getPosY(), chess_list);
				if(n_chess && (n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White))
				{
					return false;
				}
			}

			if(_cs->hasDown() && !existFromList<Chess*>(_cs->getPosX() + 1, _cs->getPosY(), slist))
			{
				n_chess = getChessByPoint(_cs->getPosX() + 1, _cs->getPosY(), chess_list);
				if(n_chess && (n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White))
				{
					return false;
				}
			}
		}

		//没有活气，把子打到队列里
		for(it = slist.begin(); it != slist.end(); it++)
		{
			Chess *_cs = *it;
			if(!existFromVector<Chess*>(_cs->getPosX(), _cs->getPosY(), list))
			{
				list.push_back(_cs);
			}
		}
	}
	return true;
}

//------------------------------------------------------------------
// 吃子                                                            
//------------------------------------------------------------------
void Board::submitEat(int type, int x, int y)
{
	recorder_ptr the_recorder(new BoardRecorder);
	the_recorder->type = BoardRecorder::EAT;
	the_recorder->cType = (enum Chess::ChessType)type;
	the_recorder->PosX = x;
	the_recorder->PosY = y;
	the_recorder->step = 0;
	recorder_list.push_back(the_recorder);
	cleanChess(x, y);
}

//------------------------------------------------------------------
// 清除                                                             
//------------------------------------------------------------------
void Board::cleanChess(int x, int y)
{
	Chess* chess = getChessByPoint(x, y, chess_list);
	if(chess)
		chess->clean();
}

//------------------------------------------------------------------
// 执行一次悔棋                                                            
//------------------------------------------------------------------
bool Board::carry_out_undo(int type, recorder_ptr& cur_recorder)
{
	if((enum Chess::ChessType)type == Chess::Black && last_BChess == nullptr)
		return false;

	if((enum Chess::ChessType)type == Chess::White && last_WChess == nullptr)
		return false;

	if(recorder_list.empty())return false;
	recorder_ptr find_recorder = recorder_list.back();

	cur_recorder->type = find_recorder->type;
	cur_recorder->PosX = find_recorder->PosX;
	cur_recorder->PosY = find_recorder->PosY;
	cur_recorder->cType = find_recorder->cType;
	cur_recorder->step = find_recorder->step;

	//执行反向操作
	if(cur_recorder->type == BoardRecorder::ADD)
	{
		//如果是加子，那这里就清空
		cleanChess(cur_recorder->PosX, cur_recorder->PosY);
	}
	else if(cur_recorder->type == BoardRecorder::EAT)
	{
		//如果是吃子，那这里就添加
		addChess(cur_recorder->cType, cur_recorder->PosX, cur_recorder->PosY, true);
	}

	if(cur_recorder->cType == (enum Chess::ChessType)type && cur_recorder->type == BoardRecorder::ADD)
	{
		//打标记，下次不执行悔棋
		if(cur_recorder->cType == Chess::Black)
		{
			last_BChess = nullptr;
		}
		else if(cur_recorder->cType == Chess::White)
		{
			last_WChess = nullptr;
		}
	}
	recorder_list.pop_back();
	return true;
}

//------------------------------------------------------------------
// 计算得分                                                            
//------------------------------------------------------------------
bool Board::caclulateResult()
{
	int nBlack = 0;
	int nWhite = 0;
	int nPending = 0;
	
	fBlack = 0;
	fWhite = 0;

	for(int i = 0; i < 19; i++)
	{
		for(int j = 0; j < 19; j++)
		{
			switch(getChessByPoint(i, j, chess_list)->getType())
			{
			case Chess::ChessType::Black:
			case Chess::ChessType::DeadWhite:
			case Chess::ChessType::MaybeBlack:
				nBlack++;
				break;
			case Chess::ChessType::White:
			case Chess::ChessType::DeadBlack:
			case Chess::ChessType::MaybeWhite:
				nWhite++;
				break;
			default:
				nPending++;
				break;
			}
		}
	}

	fBlack = nBlack + (double)nPending / 2;
	fWhite = nWhite + (double)nPending / 2;
	return true;
}




//------------------------------------------------------------------
// 寻找块                                                               
//------------------------------------------------------------------
void Board::findBlock(int type, int x, int y, std::list<Chess*>& clist)
{
	std::list<Chess*> &list = clist;
	//获得当前同类子的列表
	std::list<Chess*> map;
	std::map<std::string, Chess*>::iterator it;
	for(it = chess_list.begin(); it != chess_list.end(); it++)
	{
		Chess *_chess = it->second;
		_chess->_find_visit = 0;
		if(_chess->getType() == type)
		{
			map.push_back(_chess);
			_chess->_find_visit += 4;
		}
	}

	//获得当前x,y为位置
	Chess* cur_chess = getChessByPoint(x, y, chess_list);
	std::list<Chess*> q1;
	q1.push_back(cur_chess);
	cur_chess->_find_visit += 1;

	while (q1.size() > 0)
	{
		//获得chess
		Chess* chess = q1.front();
		int cx = chess->getPosX();
		int cy = chess->getPosY();

		//加入到块列表中
		if(((chess->_find_visit >> 1) & 1) == 0)
		{
			list.push_back(chess);
			chess->_find_visit += 2;
		}

		Chess* n_chess;
		if(chess->hasLeft())
		{
			n_chess = getChessByPoint(cx, cy - 1, chess_list);
			if(n_chess->_find_visit == 4)
			{
				q1.push_back(n_chess);
				n_chess->_find_visit += 1;
			}
		}

		if(chess->hasRight())
		{
			n_chess = getChessByPoint(cx, cy + 1, chess_list);
			if(n_chess->_find_visit == 4)
			{
				q1.push_back(n_chess);
				n_chess->_find_visit += 1;
			}
		}

		if(chess->hasUp())
		{
			n_chess = getChessByPoint(cx - 1, cy, chess_list);
			if(n_chess->_find_visit == 4)
			{
				q1.push_back(n_chess);
				n_chess->_find_visit += 1;
			}
		}

		if(chess->hasDown())
		{
			n_chess = getChessByPoint(cx + 1, cy, chess_list);
			if(n_chess->_find_visit == 4)
			{
				q1.push_back(n_chess);
				n_chess->_find_visit += 1;
			}
		}
		chess->_find_visit^=(chess->_find_visit&(1<<0))^(0<<0);
		q1.pop_front();
	}
}

//获得某一行棋盘信息
int Board::getDataByLine(int y, int type)
{
	int value = 0;
	for(int i = 0; i < 19; i++)
	{
		Chess* chess = getChessByPoint(i, y, chess_list);
		Chess::ChessType _type = (enum Chess::ChessType)type;

		if(_type == Chess::Black)
		{
			if(chess->getType() == Chess::Black || chess->getType() == Chess::DeadBlack)
			{
				value ^= (value&(1 << i)) ^ (1 << i);
			}
		}else
		{
			if(chess->getType() == Chess::White || chess->getType() == Chess::DeadWhite)
			{
				value ^= (value&(1 << i)) ^ (1 << i);
			}
		}
	}
	return value;
}

//获得当前位置上的棋
Chess* Board::getChessByPoint(int x, int y, std::map<std::string, Chess*>& map)
{
	std::map<std::string, Chess*> &_map = map;
	std::string c_name = "c" + std::to_string(x) + "_" + std::to_string(y);
	std::map<std::string, Chess*>::iterator it = _map.find(c_name);
	Chess* &n_chess = it->second;
	return n_chess;
}

//------------------------------------------------------------------
//	点目  
//	param:type 计算黑目还是白目
//------------------------------------------------------------------
std::list<Chess*> Board::goPoint(int type)
{
	std::list<Chess*> list;
	Chess::ChessType oppo_type = getopponent((enum Chess::ChessType)type);
	for(int x = 0; x < 19; ++x)
	{
		for(int y = 0; y < 19; ++y)
		{
			Chess* chess = getChessByPoint(x, y, chess_list);
			chess->_visit = 0;
			if(chess->getType() != Chess::Black && chess->getType() != Chess::DeadBlack && chess->getType() != Chess::White && chess->getType() != Chess::DeadWhite)
				chess->setType(Chess::Empty);
			else
				chess->set_point = false;
		}
	}

	//遍历MAP
	std::map<std::string, Chess*>::iterator it;
	for(it = chess_list.begin(); it != chess_list.end(); ++it)
	{
		Chess *chess = it->second;
		if(chess->_visit == 1)
			continue;

		//如果是空，执行
		if(chess->getType() == Chess::Empty)
		{
			Chess::ChessType chess_type = (enum Chess::ChessType)type;
			std::list<Chess*> clist;
			findBlock(Chess::Empty, chess->getPosX(), chess->getPosY(), clist);

			if(!clist.empty())
			{
				std::list<Chess*>::iterator list_it;
				bool is_point = true;
				for(list_it = clist.begin(); list_it != clist.end(); list_it++)
				{
					Chess *_cs = *list_it;
					//添加处理标记
					_cs->_visit = 1;
					if(!is_point)continue;

					Chess* n_chess;
					if(_cs->hasLeft())
					{
						n_chess = getChessByPoint(_cs->getPosX(), _cs->getPosY() - 1, chess_list);
						if(n_chess && n_chess->getType() == oppo_type && n_chess->_visit == 0)
						{
							n_chess->_visit = 1;
							is_point = false;
							continue;
						}
					}

					if(_cs->hasRight())
					{
						n_chess = getChessByPoint(_cs->getPosX(), _cs->getPosY() + 1, chess_list);
						if(n_chess && n_chess->getType() == oppo_type && n_chess->_visit == 0)
						{
							n_chess->_visit = 1;
							is_point = false;
							continue;
						}
					}

					if(_cs->hasUp())
					{
						n_chess = getChessByPoint(_cs->getPosX() - 1, _cs->getPosY(), chess_list);
						if(n_chess && n_chess->getType() == oppo_type && n_chess->_visit == 0)
						{
							n_chess->_visit = 1;
							is_point = false;
							continue;
						}
					}

					if(_cs->hasDown())
					{
						n_chess = getChessByPoint(_cs->getPosX() + 1, _cs->getPosY(), chess_list);
						if(n_chess && n_chess->getType() == oppo_type && n_chess->_visit == 0)
						{
							n_chess->_visit = 1;
							is_point = false;
							continue;
						}
					}
				}

				if(is_point)
				{
					if(!list.empty())
						list.splice(list.end(), clist);
					else
						list.swap(clist);
				}
			}
		}
	}
	return list;
}

//点目
void Board::setPoint(int x, int y, int type, bool isHide)
{
	Chess::ChessType play_type = (enum Chess::ChessType)type;
	Chess::ChessType oppo_type = getopponent((enum Chess::ChessType)type);
	std::list<Chess*> clist;
	setPoint(type, x, y, clist);

	bool is_clear = false;
	if(clist.size() == chess_list.size())
		is_clear = true;

	if(!clist.empty())
	{
		std::list<Chess*>::iterator it;
		Chess* chess;
		for(it = clist.begin(); it != clist.end(); ++it)
		{
			chess = *it;

			if(isHide)
			{
				//清除
				if(chess->getType() == Chess::Black || chess->getType() == Chess::DeadBlack)
				{
					//如果是自已方的子，恢复
					if(play_type == Chess::Black)
					{
						chess->setType(Chess::Black);
						chess->set_point = false;
					}else{
						chess->setType(Chess::DeadBlack);
					}
				}else if(chess->getType() == Chess::White || chess->getType() == Chess::DeadWhite)
				{
					if(play_type == Chess::White)
					{
						chess->setType(Chess::White);
						chess->set_point = false;
					}else
					{
						chess->setType(Chess::DeadWhite);
					}
				}else if(chess->getType() == Chess::Empty)
				{
					if(oppo_type == Chess::Black)
						chess->setType(Chess::MaybeBlack);
					else if(oppo_type == Chess::White)
						chess->setType(Chess::MaybeWhite);
				}else if(chess->getType() == Chess::MaybeBlack || chess->getType() == Chess::MaybeWhite)
				{
					chess->setType(Chess::Empty);
				}
			}else
			{
				chess->set_point = true;
				if(is_clear)
				{
					if(chess->getType() == Chess::MaybeBlack || chess->getType() == Chess::MaybeWhite)
					{
						chess->setType(Chess::Empty);
					}
				}else
				{
					if(chess->getType() == Chess::Empty)
					{
						if(oppo_type == Chess::Black)
						{
							chess->setType(Chess::MaybeBlack);
						}
						else if(oppo_type == Chess::White)
						{
							chess->setType(Chess::MaybeWhite);
						}
					}
				}

				if(chess->getType() == Chess::Black)
				{
					chess->setType(Chess::DeadBlack);
				}
				else if(chess->getType() == Chess::White)
				{
					chess->setType(Chess::DeadWhite);
				}
			}
			chess->set_point = !isHide;
		}

		if(isHide)
		{
			std::list<Chess*> b_list = goPoint(Chess::Black);
			std::list<Chess*> w_list = goPoint(Chess::White);

			std::list<Chess*>::iterator it;
			Chess* _chess;
			for(it = b_list.begin(); it != b_list.end(); ++it)
			{
				_chess = *it;
				_chess->setType(Chess::MaybeBlack);
			}

			for(it = w_list.begin(); it != w_list.end(); ++it)
			{
				_chess = *it;
				_chess->setType(Chess::MaybeWhite);
			}
		}
	}
}

//重置点目
void Board::clearPoint()
{
	std::map<std::string, Chess*>::iterator it;
	for(it = chess_list.begin(); it != chess_list.end(); it++)
	{
		Chess *_chess = it->second;
		if(_chess->getType() == Chess::DeadBlack)
		{
			_chess->setType(Chess::Black);
		}else if(_chess->getType() == Chess::DeadWhite)
		{
			_chess->setType(Chess::White);
		}else if(_chess->getType() == Chess::MaybeBlack || _chess->getType() == Chess::MaybeWhite || _chess->getType() == Chess::MaybePending)
		{
			_chess->setType(Chess::Empty);
		}
	}
	fBlack = 0;
	fWhite = 0;
}



//点目-点目操作
void Board::setPoint(int type, int x, int y, std::list<Chess*>& clist)
{
	std::list<Chess*> &list = clist;
	//把棋盘信息打入到模拟棋盘中
	for(int i = 0; i < 19; ++i)
	{
		for(int j = 0; j < 19; ++j)
		{
			Chess* chess = getChessByPoint(i, j, chess_list);
			chess->_visit = 0;
		}
	}

	//获得当前所有相邻的棋和气
	Chess* cur_chess = getChessByPoint(x, y, chess_list);
	cur_chess->_visit = 1;
	std::list<Chess*> q1;
	q1.push_back(cur_chess);

	while (q1.size() > 0)
	{
		Chess* p_chess = q1.front();
		int cx = p_chess->getPosX();
		int cy = p_chess->getPosY();

		//加入到块列表中
		if(p_chess->_visit == 1)
			list.push_back(p_chess);

		Chess* n_chess;
		if(p_chess->hasLeft())
		{
			n_chess = getChessByPoint(cx, cy - 1, chess_list);
			if((n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White) || n_chess->getType() == (enum ChessType)type)
			{
				if(n_chess->_visit == 0)
				{
					n_chess->_visit = 1;
					q1.push_back(n_chess);
				}
			}
		}

		if(p_chess->hasRight())
		{
			n_chess = getChessByPoint(cx, cy + 1, chess_list);
			if((n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White) || n_chess->getType() == (enum ChessType)type)
			{
				if(n_chess->_visit == 0)
				{
					n_chess->_visit = 1;
					q1.push_back(n_chess);
				}
			}
		}

		if(p_chess->hasUp())
		{
			n_chess = getChessByPoint(cx - 1, cy, chess_list);
			if((n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White) || n_chess->getType() == (enum ChessType)type)
			{
				if(n_chess->_visit == 0)
				{
					n_chess->_visit = 1;
					q1.push_back(n_chess);
				}
			}
		}

		if(p_chess->hasDown())
		{
			n_chess = getChessByPoint(cx + 1, cy, chess_list);
			if((n_chess->getType() != Chess::Black && n_chess->getType() != Chess::White) || n_chess->getType() == (enum ChessType)type)
			{
				if(n_chess->_visit == 0)
				{
					n_chess->_visit = 1;
					q1.push_back(n_chess);
				}
			}
		}
		q1.pop_front();
	}
}





/*********************************************** private ***********************************************/

//获得棋盘上同一类型的棋子
std::list<Chess*> Board::getChessFormType(int type, std::map<std::string, Chess*>& map)
{
	std::map<std::string, Chess*> &_map = map;
	std::list<Chess*> list;

	std::map<std::string, Chess*>::iterator it;
	for(it = _map.begin(); it != _map.end(); it++)
	{
		Chess *_chess = it->second;
		if(_chess->getType() == type)
		{
			list.push_back(_chess);
		}
	}
	return list;
}






//获得对手棋类型
Chess::ChessType Board::getopponent(Chess::ChessType type)
{
	if(type == Chess::ChessType::Black)
		return Chess::ChessType::White;
	else if(type == Chess::ChessType::White)
		return Chess::ChessType::Black;
	else
		return Chess::Empty;
}