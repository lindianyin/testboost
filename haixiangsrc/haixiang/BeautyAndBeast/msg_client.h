#pragma once
#include "msg_from_client.h"
#include "error_define.h"
#include "utility.h"
#include "service.h"
#include "msg_client_common.h"

enum
{
	GET_CLSID(msg_cancel_banker_req) = 3,
	GET_CLSID(msg_apply_banker_req) = 4,
	GET_CLSID(msg_set_bets_req) = 5,
	GET_CLSID(msg_get_player_count) = 6,
	GET_CLSID(msg_get_lottery_record) =  7,
	GET_CLSID(msg_get_banker_ranking) = 8

};


class msg_get_lottery_record : public msg_from_client<remote_socket_ptr>
{
public:
	msg_get_lottery_record()
	{
		head_.cmd_ = GET_CLSID(msg_get_lottery_record);
	}
	int			handle_this();
};

class msg_cancel_banker_req : public msg_from_client<remote_socket_ptr>
{
public:
	msg_cancel_banker_req()
	{
			head_.cmd_ = GET_CLSID(msg_cancel_banker_req);
	}
	int			handle_this();
};

class msg_apply_banker_req : public msg_from_client<remote_socket_ptr>
{
public:
	msg_apply_banker_req()
	{
		head_.cmd_ = GET_CLSID(msg_apply_banker_req);
	}
	int			handle_this();
};

class msg_set_bets_req : public msg_from_client<remote_socket_ptr>
{
public:
	int			pid_;
	int			present_id_;
	msg_set_bets_req()
	{
		head_.cmd_ = GET_CLSID(msg_set_bets_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(pid_, data_s);
		read_data(present_id_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			handle_this();
};


class msg_get_player_count:public msg_from_client<remote_socket_ptr>
{
public:
	msg_get_player_count()
	{
		head_.cmd_ = GET_CLSID(msg_get_player_count);
	}

	int	 read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		return ERROR_SUCCESS_0;
	}

	int			handle_this();
};


class msg_get_banker_ranking :public msg_from_client<remote_socket_ptr>
{
public:
	msg_get_banker_ranking()
	{
		head_.cmd_ = GET_CLSID(msg_get_banker_ranking);
	}

	int handle_this();
};