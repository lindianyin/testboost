#pragma once
#include "msg_from_client.h"
#include "utility.h"
#include "error_define.h"
//#include "service.h"
#include "msg_client_common.h"

enum
{
	GET_CLSID(msg_set_bets_req) = 100,
	GET_CLSID(msg_query_to_become_banker_rsp),
	GET_CLSID(msg_want_sit_req),
	GET_CLSID(msg_open_card_req),
	GET_CLSID(msg_start_random_req),
	GET_CLSID(msg_misc_data_req),   //杂项数据

};

class msg_set_bets_req : public msg_authrized<remote_socket_ptr>
{
public:
	int			count_;
	msg_set_bets_req()
	{
		head_.cmd_ = GET_CLSID(msg_set_bets_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(count_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			handle_this();
};

class msg_query_to_become_banker_rsp : public msg_authrized<remote_socket_ptr>
{
public:
	int			agree_;		//是否同意
	msg_query_to_become_banker_rsp()
	{
		head_.cmd_ = GET_CLSID(msg_query_to_become_banker_rsp);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(agree_, data_s);
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

class msg_open_card_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		card3_[3];
	int		card2_[2];
	int		give_up_;				//0 开牌，1 放弃，2 自动开牌， 3 提示牌型
	msg_open_card_req()
	{
		head_.cmd_ = GET_CLSID(msg_open_card_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data<int>(card3_, 3, data_s, true);
		read_data<int>(card2_, 2, data_s, true);
		read_data(give_up_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_start_random_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		view_present_;
	msg_start_random_req()
	{
		head_.cmd_ = GET_CLSID(msg_start_random_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(view_present_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_misc_data_req : public msg_authrized<remote_socket_ptr>
{
public:
	msg_misc_data_req()
	{
		head_.cmd_ = GET_CLSID(msg_misc_data_req);
	}

	int			handle_this();
};