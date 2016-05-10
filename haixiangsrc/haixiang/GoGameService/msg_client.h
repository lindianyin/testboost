#pragma once
#include "msg_from_client.h"
#include "utility.h"
#include "error_define.h"
#include "msg_client_common.h"

enum
{
	GET_CLSID(send_game_make_ready)		= 30001,				//举手准备
	GET_CLSID(send_game_move_chess)		= 30002,				//走棋
	GET_CLSID(send_game_ask_undo)			= 30003,				//悔棋询问
	GET_CLSID(send_game_reply_undo)		=	30004,				//悔棋回复
	GET_CLSID(send_game_pass_chess)		=	30005,				//pass
	GET_CLSID(send_game_point_chess),									//点目
	GET_CLSID(send_game_reply_point),									//点目回复
	GET_CLSID(send_game_give_up),											//认输
	GET_CLSID(send_game_compel_chess),								//强制数子
	GET_CLSID(send_game_ask_summation),								//求和询问
	GET_CLSID(send_game_reply_summation),							//求和回复
	GET_CLSID(send_set_point),												//设置点目坐标
	GET_CLSID(send_submit_point),											//完成点目
	GET_CLSID(send_result_gopoint),										//提交点目结果
};

class send_game_make_ready : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_make_ready);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_move_chess : public msg_authrized<remote_socket_ptr>
{
public:
	int pos_x;
	int pos_y;

	MSG_CONSTRUCT(send_game_move_chess);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(pos_x, data_s);
		read_data(pos_y, data_s);
		return 0;
	}
	int handle_this();
};

class send_game_ask_undo : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_ask_undo);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_reply_undo : public msg_authrized<remote_socket_ptr>
{
public:
	int result;					//结果:0,拒绝;1,同意	
	MSG_CONSTRUCT(send_game_reply_undo);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(result, data_s);
		return 0;
	}
	int handle_this();
};

class send_game_pass_chess : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_pass_chess);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_point_chess : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_point_chess);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_reply_point : public msg_authrized<remote_socket_ptr>
{
public:
	int result;					//结果:0,拒绝;1,同意	
	MSG_CONSTRUCT(send_game_reply_point);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(result, data_s);
		return 0;
	}
	int handle_this();
};

class send_game_give_up : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_give_up);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_compel_chess : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_compel_chess);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_ask_summation : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_game_ask_summation);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

class send_game_reply_summation : public msg_authrized<remote_socket_ptr>
{
public:
	int result;					//结果:0,拒绝;1,同意	
	MSG_CONSTRUCT(send_game_reply_summation);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(result, data_s);
		return 0;
	}
	int handle_this();
};

//设置点目
class send_set_point : public msg_authrized<remote_socket_ptr>
{
public:
	int posX;
	int posY;
	int is_set;
	MSG_CONSTRUCT(send_set_point);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(posX, data_s);
		read_data(posY, data_s);
		read_data(is_set, data_s);
		return 0;
	}
	int handle_this();
};

//完成点目
class send_submit_point : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(send_submit_point);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		return 0;
	}
	int handle_this();
};

//提交点目结果
class send_result_gopoint : public msg_authrized<remote_socket_ptr>
{
public:
	int _result;	//0,同意; 1,继续点目; 2,继续走棋
	MSG_CONSTRUCT(send_result_gopoint);
	int read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(_result, data_s);
		return 0;
	}
	int handle_this();
};