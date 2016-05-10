#pragma once
#include "msg_from_client.h"
#include "utility.h"
#include "error_define.h"
#include "msg_client_common.h"
enum 
{
	GET_CLSID(msg_fire_at_req) = 100,
	GET_CLSID(msg_gun_switch_req),
	GET_CLSID(msg_hit_fish_req),
	GET_CLSID(msg_buy_coin_req),
	GET_CLSID(msg_get_match_report),
	GET_CLSID(msg_lock_fish_req),
	GET_CLSID(msg_enter_host_game_req),
	GET_CLSID(msg_get_host_list),
	GET_CLSID(msg_upload_screenshoot),
	GET_CLSID(msg_send_present_req),
	GET_CLSID(msg_version_verify),
	GET_CLSID(msg_host_start_req),
};

class msg_gun_switch_req : public msg_authrized<remote_socket_ptr>
{
public:
	int			level_;

	MSG_CONSTRUCT(msg_gun_switch_req);
	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(level_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_fire_at_req : public msg_authrized<remote_socket_ptr>
{
public:
	short		x, y;
	int			missile_id_;
	MSG_CONSTRUCT(msg_fire_at_req);
	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(x, data_s);
		read_data(y, data_s);
		read_data(missile_id_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_hit_fish_req : public msg_authrized<remote_socket_ptr>
{
public:
	int			iid_;
	int			missile_id_;
	MSG_CONSTRUCT(msg_hit_fish_req);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(iid_, data_s);
		read_data(missile_id_, data_s);
		return 0;
	}
	int		handle_this();
};

//上分
class msg_buy_coin_req : public msg_authrized<remote_socket_ptr>
{
public:
	int				dir_;	//0上分，1下分
	longlong		count_;
	MSG_CONSTRUCT(msg_buy_coin_req);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(dir_, data_s);
		read_data(count_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_get_match_report : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(msg_get_match_report);
	int		handle_this();
};

class msg_lock_fish_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		iid_;
	MSG_CONSTRUCT(msg_lock_fish_req);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(iid_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_enter_host_game_req : public msg_enter_game_req
{
public:
	MSG_CONSTRUCT(msg_enter_host_game_req);
	int		handle_this();
};
class msg_get_host_list: public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(msg_get_host_list);
	int		handle_this();
};

//主播截图上传
class msg_upload_screenshoot : public msg_authrized<remote_socket_ptr>
{
public:
	int		TAG_;	//1开始，2，结束，0，中间
	int		len_;
	char	data_[512];
	MSG_CONSTRUCT(msg_upload_screenshoot);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(TAG_, data_s);
		read_data(len_, data_s);
		read_data<unsigned char>((unsigned char*)data_, len_, data_s);
		return 0;
	}
	int		handle_this();
};

//
class msg_send_present_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		to_host_;
	int		present_id_;
	int		count_;
	MSG_CONSTRUCT(msg_send_present_req);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(to_host_, data_s);
		read_data(present_id_, data_s);
		read_data(count_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_version_verify : public msg_from_client<remote_socket_ptr>
{
public:
	int		version_;
	MSG_CONSTRUCT(msg_version_verify);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(version_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_host_start_req : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(msg_host_start_req);
	int		handle_this();
};
