#pragma once
#include "msg_from_client.h"
#include "error_define.h"
#include "utility.h"
#include "service.h"
#include "msg_client_common.h"

enum
{
	GET_CLSID(msg_set_bets_req) = 5,
	GET_CLSID(msg_start_random_req) = 12,
	GET_CLSID(msg_player_game_info_req) = 13,
};

class msg_set_bets_req : public msg_authrized<remote_socket_ptr>
{
public:
	int			pid_;	//奖项id,顺时针递增。从1开始，12点方向
	int			count_;
	msg_set_bets_req()
	{
		head_.cmd_ = GET_CLSID(msg_set_bets_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(pid_, data_s);
		read_data(count_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			handle_this();
};

class msg_start_random_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		view_present_; //0正常开始， 1押大，2押小, 3 退币, 4 加币
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

class msg_player_game_info_req : public msg_authrized<remote_socket_ptr>
{
public:
	msg_player_game_info_req()
	{
		head_.cmd_ = GET_CLSID(msg_player_game_info_req);
	}


	int			handle_this();
};