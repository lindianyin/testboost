#pragma once

class Chess
{
public:
	enum ChessType
	{
		Empty = 0,			//空
		Black = 1,			//黑
		White = 2,			//白
		MaybeBlack = 3,
		MaybeWhite = 4,
		MaybePending = 5,
		DeadBlack = 6,
		DeadWhite = 7
	};

	bool	set_point;				//是否被点目
	int		_visit;						//访问符
	int		_find_visit;			//查找访问符





	explicit Chess(int x, int y);
	~Chess();
	bool addChess(ChessType _type, int _step);
	int getPosX();
	int getPosY();

	ChessType getType();					//获得当前棋子类型
	void setType(ChessType _type);

	int getStep();

	bool	isEmpty();							//是否为空
	bool	hasLeft();							//是否靠边
	bool	hasRight();
	bool	hasUp();
	bool	hasDown();
	void	reset();
	void	clean();								//清除

private:
	int x;
	int y;
	ChessType type;
	int step;
};