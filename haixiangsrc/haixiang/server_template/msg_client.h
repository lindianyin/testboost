#pragma once
#include "msg_from_client.h"
#include "error_define.h"
#include "utility.h"
#include "service.h"

enum
{
	GET_CLSID(msg_url_user_login_req) = 22,
	GET_CLSID(msg_game_lst_req) = 6,
	GET_CLSID(msg_enter_game_req) = 7,
	GET_CLSID(msg_talk_req) = 9,
	GET_CLSID(msg_get_prize_req) = 13,
	GET_CLSID(msg_leave_room) = 14,
	GET_CLSID(msg_get_wealth_rank_req) = 15,
	GET_CLSID(msg_set_account) = 16,
	GET_CLSID(msg_trade_gold_req) = 17,
	GET_CLSID(msg_user_info_changed) = 18,
	GET_CLSID(msg_account_login_req) = 19,
	GET_CLSID(msg_change_pwd_req) = 20,
	GET_CLSID(msg_get_recommend_reward) = 30,
	GET_CLSID(msg_get_trade_to_lst) = 31,
	GET_CLSID(msg_get_trade_from_lst) = 32,
	GET_CLSID(msg_get_recommend_lst) = 33,
};

class msg_change_pwd_req : public msg_authrized<remote_socket_ptr>
{
public:
	char	pwd_old_[max_name];
	char	pwd_[max_name];
	msg_change_pwd_req()
	{
		head_.cmd_ = GET_CLSID(msg_change_pwd_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data<char>(pwd_old_, max_name , data_s);
		read_data<char>(pwd_,max_name , data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_account_login_req : public msg_from_client<remote_socket_ptr>
{
public:
	char	acc_name_[max_name];
	char	pwd_[max_name];
	msg_account_login_req()
	{
		head_.cmd_ = GET_CLSID(msg_account_login_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(acc_name_, max_name, data_s);
		read_data<char>(pwd_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_set_account : public msg_authrized<remote_socket_ptr>
{
public:
	char	acc_name_[max_name];
	char	pwd_[max_name];
	char	nick_name[max_name];
	double relate_iid_;	//推荐人id
	msg_set_account()
	{
		head_.cmd_ = GET_CLSID(msg_set_account);
		relate_iid_ = 0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(acc_name_, max_name, data_s);
		read_data<char>(pwd_, max_name, data_s);
		read_data<char>(nick_name, max_name, data_s);
		read_data<double>(relate_iid_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_url_user_login_req : public msg_from_client<remote_socket_ptr>
{
public:
	char		uid_[max_guid];
	char		uname_[max_name];
	int			vip_lv_;
	char		head_icon_[max_guid];
	int			uidx_;//php用到
	char		session_[max_guid];
	char		url_sign_[max_guid];
	int			sex_;
	msg_url_user_login_req()
	{
		head_.cmd_ = GET_CLSID(msg_url_user_login_req);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data(vip_lv_, data_s);
		write_data<char>(head_icon_, max_guid, data_s);
		write_data(uidx_, data_s);
		write_data<char>(session_, max_guid, data_s);
		write_data<char>(url_sign_, max_guid, data_s);
		write_data(sex_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		read_data<char>(uname_, max_name, data_s);
		read_data(vip_lv_, data_s);
		read_data<char>(head_icon_, max_guid, data_s);
		read_data(uidx_, data_s);
		read_data<char>(session_, max_guid, data_s);
		read_data<char>(url_sign_, max_guid, data_s);
		read_data(sex_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_game_lst_req : public msg_from_client<remote_socket_ptr>
{
public:
	unsigned int			page_;						//第几页
	unsigned int			page_set_;				//每页多少个
	msg_game_lst_req()
	{
		head_.cmd_ = GET_CLSID(msg_game_lst_req);
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

class msg_enter_game_req : public msg_authrized<remote_socket_ptr>
{
public:
	unsigned int			room_id_;
	msg_enter_game_req()
	{
		head_.cmd_ = GET_CLSID(msg_enter_game_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(room_id_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_get_prize_req : public msg_authrized<remote_socket_ptr>
{
public:
	int			type_;		//0,领取等级奖励, 1领取每日登录奖励
	int			data_;		//要领取第几日的登录奖励
	msg_get_prize_req()
	{
		head_.cmd_ = GET_CLSID(msg_get_prize_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(type_, data_s);
		read_data(data_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_leave_room : public msg_authrized<remote_socket_ptr>
{
public:
	msg_leave_room()
	{
		head_.cmd_ = GET_CLSID(msg_leave_room);
	}
	int			handle_this();
};

class msg_get_wealth_rank_req : public msg_from_client<remote_socket_ptr>
{
public:
	msg_get_wealth_rank_req()
	{
		head_.cmd_ = GET_CLSID(msg_get_wealth_rank_req);
	}
	int			handle_this();
};

class msg_trade_gold_req : public msg_authrized<remote_socket_ptr>
{
public:
	unsigned int	iid_;
	unsigned int	gold_;
	msg_trade_gold_req()
	{
		head_.cmd_ = GET_CLSID(msg_trade_gold_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(iid_, data_s);
		read_data(gold_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_user_info_changed : public msg_authrized<remote_socket_ptr>
{
public:
	char	uname_[max_name];			//用户名
	char	head_icon_[max_guid];	//头像
	msg_user_info_changed()
	{
		head_.cmd_ = GET_CLSID(msg_user_info_changed);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data<char>(uname_, max_name , data_s);
		read_data<char>(head_icon_,max_guid , data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

//得到推荐人列表
class msg_get_recommend_lst : public msg_authrized<remote_socket_ptr>
{
public:
	msg_get_recommend_lst()
	{
		head_.cmd_ = GET_CLSID(msg_get_recommend_lst);
	}
	int			handle_this();
};

//得到赠送记录
class msg_get_trade_to_lst : public msg_authrized<remote_socket_ptr>
{
public:
	msg_get_trade_to_lst()
	{
		head_.cmd_ = GET_CLSID(msg_get_trade_to_lst);
	}
	int			handle_this();
};

//得到被赠送记录
class msg_get_trade_from_lst : public msg_authrized<remote_socket_ptr>
{
public:
	msg_get_trade_from_lst()
	{
		head_.cmd_ = GET_CLSID(msg_get_trade_from_lst);
	}
	int			handle_this();
};

//领取推荐奖励
class msg_get_recommend_reward : public msg_authrized<remote_socket_ptr>
{
public:
	char	uid_[max_guid];//得到哪个被推荐人的奖励
	msg_get_recommend_reward()
	{
		head_.cmd_ = GET_CLSID(msg_get_recommend_reward);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data<char>(uid_,max_guid , data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

//发送喇叭
class msg_talk_req : public msg_authrized<remote_socket_ptr>
{
public:
	enum
	{
		CHAT_CHANNEL_ROOM = 1,	
		CHAT_CHANNEL_WHISPER = 2,
		CHAT_CHANNEL_BROADCAST = 3,
		CHAT_CHANNEL_SYSTEM = 4,
		CHAT_CHANNEL_ERROR = 5,
	};
	char							to_name_[max_name];	//发送给谁
	unsigned int			channel_;		//这里填3
	unsigned char			len_;				//内容长度字节数
	char		content_[256];				//内容
	msg_talk_req()
	{
		head_.cmd_ = GET_CLSID(msg_talk_req);
	}
	void		set_content(std::string s)
	{
		if(s.length() > 256) return;
		len_ = s.length();
		COPY_STR(content_, s.c_str());
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data<char>(to_name_, max_name, data_s);
		read_data(channel_, data_s);
		read_data(len_, data_s);
		read_data<char>(content_, std::min<int>(len_, 256), data_s, false);
		return ERROR_SUCCESS_0;
	}

	int		handle_this();
};
