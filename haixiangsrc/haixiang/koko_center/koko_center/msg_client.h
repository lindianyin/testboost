#pragma once
#include "koko_socket.h"
#include "msg_from_client.h"

template<class remote_t>
class msg_user_login_t : public msg_from_client<remote_t>
{
public:
	char		acc_name_[max_guid];
	char		pwd_hash_[max_guid];
	char		machine_mark_[max_guid];
	int					read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(acc_name_, max_guid, data_s);
		read_data(pwd_hash_, max_guid, data_s);
		read_data(machine_mark_, max_guid, data_s);
		return 0;
	}
};

//登录
class msg_user_login : public msg_user_login_t<koko_socket_ptr>
{
public:
	msg_user_login()
	{
		head_.cmd_ = GET_CLSID(msg_user_login);
	}
	int			handle_this();
};

//注册账号
class msg_user_register : public msg_user_login
{
public:
	int		type_;	//0用户名,1手机，2邮箱
	char	verify_code[64];
	msg_user_register()
	{
		head_.cmd_ = GET_CLSID(msg_user_register);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_user_login::read(data_s, l);
		read_data(type_, data_s);
		read_data(verify_code, 64, data_s);
		return 0;
	}
	int			handle_this();
};

//更改用户数据
class msg_change_account_info : public msg_authrized<koko_socket_ptr>
{
public:
	int		gender_;
	int		byear_;
	int		bmonth_;
	int		bday_;
	char	address_[256];
	char	nick_name_[64];
	int		age_;
	char	mobile_[32];
	char	email_[32];
	char	idcard_[32];
	int		region1_, region2_,region3_;
	msg_change_account_info()
	{
		head_.cmd_ = GET_CLSID(msg_change_account_info);
	}
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(gender_, data_s);
		read_data(byear_, data_s);
		read_data(bmonth_, data_s);
		read_data(bday_, data_s);
		read_data(address_, 256, data_s);
		read_data(nick_name_, 64, data_s);
		read_data(age_, data_s);
		read_data(mobile_, 32, data_s);
		read_data(email_, 32, data_s);
		read_data(idcard_, 32, data_s);
		read_data(region1_, data_s);
		read_data(region2_, data_s);
		read_data(region3_, data_s);
		return 0;
	}
	int		handle_this();
};

//生成验证码
class	msg_get_verify_code : public msg_from_client<koko_socket_ptr>
{
public:
	int			type_;		//0图片验证码 1手机验证码 2 邮件验证码
	char		mobile_[256];
	msg_get_verify_code()
	{
		head_.cmd_ = GET_CLSID(msg_get_verify_code);
	}
	int					read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(type_, data_s);
		read_data(mobile_, 256, data_s);
		return 0;
	}
	int		handle_this();
};

class	 msg_check_data : public msg_from_client<koko_socket_ptr>
{
public:
	enum 
	{
		check_account_name_exist,
		check_email_exist,
		check_mobile_exist,
		check_verify_code,		//图片验证码
		check_mverify_code,		//短信验证码
	};

	int					query_type_;
	__int64			query_idata_;	
	char				query_sdata_[64];
	msg_check_data()
	{
		head_.cmd_ = GET_CLSID(msg_check_data);
		query_sdata_[0] = 0;
	}
	int					read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(query_type_, data_s);
		read_data(query_idata_, data_s);
		read_data(query_sdata_, 64, data_s);
		return 0;
	}
	int		handle_this();
};

class		msg_action_record : public msg_from_client<koko_socket_ptr>
{
public:
	int		action_id_;
	int		action_data_;		//是不离开游戏
	msg_action_record()
	{
		head_.cmd_ = GET_CLSID(msg_action_record);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(action_id_, data_s);
		read_data(action_data_, data_s);
		return 0;
	}
	int		handle_this();
};

class		msg_get_token : public msg_authrized<koko_socket_ptr>
{
public:
	msg_get_token()
	{
		head_.cmd_ = GET_CLSID(msg_get_token);
	}
	int		handle_this();
};

//得到游戏协同服务器
class		msg_get_game_coordinate : public msg_authrized<koko_socket_ptr>
{
public:
	int		gameid_;
	msg_get_game_coordinate()
	{
		head_.cmd_ = GET_CLSID(msg_get_game_coordinate);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(gameid_, data_s);
		return 0;
	}
	int		handle_this();
};
