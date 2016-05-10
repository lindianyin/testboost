#pragma once
#include "msg_from_client.h"
#include "error_define.h"
#include "utility.h"
#include "service.h"
#include "msg_client_common.h"

enum
{
	GET_CLSID(msg_set_bets_req) = 5,
	GET_CLSID(msg_setbet_his_req) = 6,
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

class msg_setbet_his_req : public msg_from_client<remote_socket_ptr>
{
public:
	int		page_;
	int		page_set_;
	msg_setbet_his_req()
	{
		head_.cmd_ = GET_CLSID(msg_setbet_his_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(page_, data_s);
		read_data(page_set_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

