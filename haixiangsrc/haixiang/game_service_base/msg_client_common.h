#pragma once
#include "utility.h"
#include "msg_from_client.h"
#include "game_config.h"
enum
{
	GET_CLSID(msg_koko_trade_inout_ret) = 998,	//id和msg_koko_trade_inout一样没关系
	GET_CLSID(msg_user_login_delegate_ret) = 999,
	GET_CLSID(msg_koko_trade_inout) = 998,
	GET_CLSID(msg_user_login_delegate) = 113,
	GET_CLSID(msg_match_end) = 20003,
	GET_CLSID(msg_player_distribute) = 20004,
	GET_CLSID(msg_ping_req)=0xFFFF,
	GET_CLSID(msg_url_user_login_req) = 500,
	GET_CLSID(msg_game_lst_req),
	GET_CLSID(msg_enter_game_req),
	GET_CLSID(msg_talk_req),
	GET_CLSID(msg_get_prize_req),
	GET_CLSID(msg_leave_room),
	GET_CLSID(msg_get_rank_req),
	GET_CLSID(msg_set_account),
	GET_CLSID(msg_trade_gold_req),
	GET_CLSID(msg_user_info_changed),
	GET_CLSID(msg_account_login_req),
	GET_CLSID(msg_change_pwd_req),
	GET_CLSID(msg_get_recommend_reward),
	GET_CLSID(msg_get_trade_to_lst),
	GET_CLSID(msg_get_trade_from_lst),
	GET_CLSID(msg_get_recommend_lst),

	GET_CLSID(msg_auto_tradein_req) = 991,
	GET_CLSID(msg_platform_account_login) = 992,
	GET_CLSID(msg_enter_match_game_req) = 993,
	GET_CLSID(msg_platform_login_req) = 994,
	GET_CLSID(msg_device_id) = 995,
	GET_CLSID(msg_url_user_login_req2) = 996,
	GET_CLSID(msg_prepare_enter_complete) = 997,
	GET_CLSID(msg_get_server_info)=998,
	GET_CLSID(msg_7158_user_login_req) = 999,
	msg_client_id_this_game_begin = 1000,
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

class		msg_account_login_req : public msg_from_client<remote_socket_ptr>
{
public:
	char	acc_name_[max_name];
	char	pwd_[max_name];
	char	platform_id[max_name];
	msg_account_login_req()
	{
		head_.cmd_ = GET_CLSID(msg_account_login_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(acc_name_, max_name, data_s);
		read_data<char>(pwd_, max_name, data_s);
		read_data<char>(platform_id, max_name, data_s);
		return ERROR_SUCCESS_0;
	}

	int			handle_this();
};

class msg_set_account : public msg_from_client<remote_socket_ptr>
{
public:
	char	acc_name_[max_name];
	char	pwd_[max_name];
	char	head_icon_[max_name];
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
		read_data<char>(head_icon_, max_name, data_s);
		read_data<double>(relate_iid_, data_s);
		return 0;
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
	char		platform_[max_name];//php用到
	char		sn_[max_guid];
	char		url_sign_[max_guid];
	int			iid_;
	msg_url_user_login_req()
	{
		head_.cmd_ = GET_CLSID(msg_url_user_login_req);
		platform_[4] = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data(vip_lv_, data_s);
		write_data<char>(head_icon_, max_guid, data_s);
		write_data<char>(platform_, max_name, data_s);
		write_data<char>(sn_, max_guid, data_s);
		write_data<char>(url_sign_, max_guid, data_s);
		write_data(iid_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		read_data<char>(uname_, max_name, data_s);
		read_data(vip_lv_, data_s);
		read_data<char>(head_icon_, max_guid, data_s);
		read_data<char>(platform_, max_name, data_s);
		read_data<char>(sn_, max_guid, data_s);
		read_data<char>(url_sign_, max_guid, data_s);
		read_data(iid_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

//为了兼容老版登录,只有7158可以用这样的方式登录，其它平台不可以用这个登录。
class msg_url_user_login_req2 : public msg_url_user_login_req
{
public:
	msg_url_user_login_req2()
	{
		head_.cmd_ = GET_CLSID(msg_url_user_login_req2);
		iid_ = 0;
	}
	int			handle_this();
};

class msg_7158_user_login_req : public msg_from_client<remote_socket_ptr>
{
public:
	char		FromID[max_guid];
	char		uidx_[max_guid];
	char		uid_[max_guid];
	char		pwd[max_guid];
	char		uname_[max_guid];
	char		head_pic_[max_guid];
	char		session_[max_guid];
	msg_7158_user_login_req()
	{
		head_.cmd_ = GET_CLSID(msg_7158_user_login_req);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data<char>(FromID, max_guid, data_s);
		write_data<char>(uidx_, max_guid, data_s);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(pwd, max_guid, data_s);
		write_data<char>(uname_, max_guid, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data<char>(session_, max_guid, data_s);
		return 0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(FromID, max_guid, data_s);
		read_data<char>(uidx_, max_guid, data_s);
		read_data<char>(uid_, max_guid, data_s);
		read_data<char>(pwd, max_guid, data_s);
		read_data<char>(uname_, max_guid, data_s);
		read_data<char>(head_pic_, max_guid, data_s);
		read_data<char>(session_, max_guid, data_s);
		return 0;
	}
	int			handle_this();
};

class		msg_platform_login_req : public msg_url_user_login_req
{
public:
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


class msg_enter_match_game_req : public msg_authrized<remote_socket_ptr>
{
public:
	int				match_id_;
	char			game_ins_[max_guid];
	msg_enter_match_game_req()
	{
		head_.cmd_ = GET_CLSID(msg_enter_match_game_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(match_id_, data_s);
		read_data(game_ins_, max_guid, data_s);
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
	int			type_;		//0,领取等级奖励, 1领取每日登录奖励 2,在线奖励
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


class msg_get_rank_req : public msg_from_client<remote_socket_ptr>
{
public:
	int		type_;	//0财富榜，1等级榜，2幸运榜
	int		page_;	//
	int		page_count_;//	
	msg_get_rank_req()
	{
		head_.cmd_ = GET_CLSID(msg_get_rank_req);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(type_, data_s);
		read_data(page_, data_s);
		read_data(page_count_, data_s);
		return ERROR_SUCCESS_0;
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
	char		content_[256];				//内容
	msg_talk_req()
	{
		head_.cmd_ = GET_CLSID(msg_talk_req);
	}
	void		set_content(std::string s)
	{
		if(s.length() > 256) return;
		COPY_STR(content_, s.c_str());
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data<char>(to_name_, max_name, data_s);
		read_data(channel_, data_s);
		read_data<char>(content_, 256, data_s);
		return ERROR_SUCCESS_0;
	}

	int		handle_this();
};
class msg_get_server_info : public msg_from_client<remote_socket_ptr>
{
public:
	char	uid_[max_guid];
	msg_get_server_info()
	{
		head_.cmd_ = GET_CLSID(msg_get_server_info);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		return ERROR_SUCCESS_0;
	}
	int		handle_this();
};

class msg_prepare_enter_complete : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(msg_prepare_enter_complete);
	int		handle_this();
};


enum 
{
	GET_CLSID(msg_query_match_req) = msg_client_id_this_game_begin,
	GET_CLSID(msg_register_match_req),
	GET_CLSID(msg_agree_to_join_match),
};

//查询比赛信息
class msg_query_match_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		match_id_;				//比赛类型
	MSG_CONSTRUCT(msg_query_match_req);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(match_id_, data_s);
		return 0;
	}
	int		handle_this();
};

//报名
class msg_register_match_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		match_id_;				//比赛类型
	int		cancel_;					//取消报名
	MSG_CONSTRUCT(msg_register_match_req);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(match_id_, data_s);
		read_data(cancel_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_agree_to_join_match : public msg_authrized<remote_socket_ptr>
{
public:
	int		agree_;
	MSG_CONSTRUCT(msg_agree_to_join_match);
	int					read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(agree_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_ping_req : public msg_authrized<remote_socket_ptr>
{
public:
	MSG_CONSTRUCT(msg_ping_req);
};

class msg_device_id : public msg_from_client<remote_socket_ptr>
{
public:
	char		device_id[max_name];
	MSG_CONSTRUCT(msg_device_id);
	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(device_id, max_name, data_s);
		return 0;
	}
	int		handle_this();
};

//KOKO平台比赛场消息
//通知游戏服务器玩家分配进入游戏
class msg_player_distribute : public msg_base<boost::shared_ptr<world_socket>>
{
public:
	char	uid_[max_guid];
	int		match_id_;
	char	game_ins_[max_guid];
	int		score_;
	int		time_left_;
	msg_player_distribute()
	{
		head_.cmd_ = GET_CLSID(msg_player_distribute);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		read_data(match_id_, data_s);
		read_data<char>(game_ins_, max_guid, data_s);
		read_data(score_,  data_s);
		read_data(time_left_,  data_s);
		return 0;
	}
	int		handle_this();
};

//通知游戏服务器比赛结束了
class msg_match_end : public msg_base<boost::shared_ptr<world_socket>>
{
public:
	int		match_id_;
	msg_match_end()
	{
		head_.cmd_ = GET_CLSID(msg_match_end);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data(match_id_, data_s);
		return 0;
	}
	int		handle_this();
};

//合作平台用户登录
class msg_platform_account_login : public msg_account_login_req
{
public:
	msg_platform_account_login()
	{
		head_.cmd_ = GET_CLSID(msg_platform_account_login);
	}
	int		handle_this();
};

//发送给KOKO平台的登录消息
class msg_user_login_delegate : public msg_from_client<remote_socket_ptr>
{
public:
	msg_user_login_delegate()
	{
		head_.cmd_ = GET_CLSID(msg_user_login_delegate);
	}
	char		acc_name_[max_guid];
	char		pwd_hash_[max_guid];
	char		machine_mark_[max_guid];
	int			sn_;
	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data(acc_name_, max_guid, data_s);
		write_data(pwd_hash_, max_guid, data_s);
		write_data(machine_mark_, max_guid, data_s);
		write_data(sn_, data_s);
		return 0;
	}
};

class		msg_koko_trade_inout : public msg_base<none_socket>
{
public:
	char		uid_[max_guid];
	int			dir_;						//0 从平台兑入游戏部分货币， 1 从平台兑入游戏所有货币,  2 从游戏兑入平台。
	__int64	count_;					//货币量
	__int64	time_stamp_;		//时间戳
	char		sn_[max_guid];
	char		sign_[max_guid];
	msg_koko_trade_inout()
	{
		head_.cmd_ = GET_CLSID(msg_koko_trade_inout);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(uid_, max_guid, data_s);
		write_data(dir_, data_s);
		write_data(count_, data_s);
		write_data(time_stamp_, data_s);
		write_data(sn_, max_guid, data_s);
		write_data(sign_, max_guid, data_s);
		return 0;
	}
};

class		world_socket;
class		msg_player_info : public msg_base<boost::shared_ptr<world_socket>>
{
public:
	char			uid_[max_guid];
	char			nickname_[max_name];
	__int64		iid_;
	__int64		gold_;
	int				vip_level_;
	int				channel_;	
	int				gender_;
	int				level_;
	msg_player_info()
	{
		channel_ = 0;
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data(uid_, max_guid, data_s);
		read_data(nickname_, max_name, data_s);
		read_data(iid_, data_s);
		read_data(gold_, data_s);
		read_data(vip_level_, data_s);
		read_data(channel_, data_s);
		read_data(gender_, data_s);
		read_data(level_, data_s);
		return 0;
	}
};

class		msg_player_info_ex : public msg_player_info
{
public:
	char			token_[max_guid];
	__int64		sequence_;

	char			email_[max_guid];		//email
	char			phone[32];					//手机号
	char			address_[256];			//地址
	__int64		game_gold_;					//游戏币
	__int64		game_gold_free_;
	int				email_verified_;		//邮箱地址已验证 0未验证，1已验证
	int				phone_verified_;		//手机号验已验证 0未验证，1已验证
	int				byear_, bmonth_, bday_;	//出生年月日
	int				region1_,region2_,region3_;
	int				age_;
	char			idcard_[32];				//身份证号
	int			read(const char*& data_s, unsigned int l)
	{
		msg_player_info::read(data_s, l);
		read_data(token_, max_guid, data_s);
		read_data(sequence_, data_s);
		read_data(email_, max_guid,  data_s);
		read_data(phone, 32, data_s);
		read_data(address_, 256, data_s);
		read_data(game_gold_, data_s);
		read_data(game_gold_free_, data_s);
		read_data(email_verified_, data_s);
		read_data(phone_verified_, data_s);
		read_data(byear_, data_s);
		read_data(bmonth_, data_s);
		read_data(bday_, data_s);
		read_data(region1_, data_s);
		read_data(region2_, data_s);
		read_data(region3_, data_s);
		read_data(age_, data_s);
		read_data(idcard_, 32, data_s);
		return 0;
	}
};

class msg_user_login_delegate_ret : public msg_player_info_ex
{
public:
	int		sn_;
	msg_user_login_delegate_ret()
	{
		head_.cmd_ = GET_CLSID(msg_user_login_delegate_ret);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_player_info_ex::read(data_s, l);
		read_data(sn_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_koko_trade_inout_ret : public msg_base<boost::shared_ptr<world_socket>>
{
public:
	char			sn_[max_guid];
	int				result_;				//0 处理成功, 1 已处理过				
	__int64		count_;					

	msg_koko_trade_inout_ret()
	{
		head_.cmd_ = GET_CLSID(msg_koko_trade_inout_ret);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data(sn_, max_guid, data_s);
		read_data(result_, data_s);
		read_data(count_, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_auto_tradein_req : public msg_authrized<remote_socket_ptr>
{
public:
	char		platform_id_[max_guid];
	msg_auto_tradein_req()
	{
		head_.cmd_ = GET_CLSID(msg_auto_tradein_req);
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(platform_id_, max_guid, data_s);
		return 0;
	}
	int		handle_this();
};

class msg_common_reply_srv : public msg_common_reply_internal<boost::shared_ptr<world_socket>>
{
public:
	int			handle_this();
};
