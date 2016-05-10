#pragma once
#include "msg_from_client.h"
#include "error_define.h"
#include "utility.h"
#include "service.h"

enum
{
	GET_CLSID(msg_bet_operator) = 8,
	GET_CLSID(msg_want_sit_req) = 10,
	GET_CLSID(msg_ready_req) = 11,
	GET_CLSID(msg_see_card_req) = 12,
};

class msg_bet_operator : public msg_authrized<remote_socket_ptr>
{
public:
	int			agree_;			//跟注多少钱，0表示放弃
	int			op_code_;		//操作码,0无特殊操作, 1开牌，2 单PK
	int			pk_pos_;		//PK位置

	msg_bet_operator()
	{
		head_.cmd_ = GET_CLSID(msg_bet_operator);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(agree_, data_s);
		read_data(op_code_, data_s);
		read_data(pk_pos_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_want_sit_req : public msg_authrized<remote_socket_ptr>
{
public:
	int				pos_;
	msg_want_sit_req()
	{
		head_.cmd_ = GET_CLSID(msg_want_sit_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(pos_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_ready_req : public msg_authrized<remote_socket_ptr>
{
public:
	msg_ready_req()
	{
		head_.cmd_ = GET_CLSID(msg_ready_req);
	}
	int			handle_this();
};

class msg_see_card_req : public msg_authrized<remote_socket_ptr>
{
public:
	msg_see_card_req()
	{
		head_.cmd_ = GET_CLSID(msg_see_card_req);
	}
	int			handle_this();
};