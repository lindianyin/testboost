#include "Chess.h"

Chess::Chess(int x, int y)
{
	this->x = x;
	this->y = y;
	this->type = ChessType::Empty;
	this->step = 0;
}

Chess::~Chess() {}

bool Chess::addChess(ChessType _type, int _step)
{
	this->type = _type;
	this->step = _step;
	return true;
}

int Chess::getPosX()
{
	return this->x;
}

int Chess::getPosY()
{
	return this->y;
}

Chess::ChessType Chess::getType()
{
	return this->type;
}

void Chess::setType(ChessType _type)
{
	this->type = _type;
}

int Chess::getStep()
{
	return this->step;
}

bool Chess::isEmpty()
{
	if(this->type == ChessType::Empty)
	{
		return true;
	}else
	{
		return false;
	}
}

bool Chess::hasLeft()
{
	if(this->y > 0)
		return true;
	else 
		return false;
}

bool Chess::hasRight()
{
	if(this->y < 18)
		return true;
	else
		return false;
}

bool Chess::hasUp()
{
	if(this->x > 0)
		return true;
	else
		return false;
}

bool Chess::hasDown()
{
	if(this->x < 18)
		return true;
	else
		return false;
}

void Chess::reset()
{

}

void Chess::clean()
{
	this->type = ChessType::Empty;
	this->step = 0;
}