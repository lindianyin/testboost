#pragma once
#include "msg.h"
#include "error.h"

class	msg_game_data : public msg_base<nosocket>
{
public:
	game_info		inf;
	msg_game_data()
	{
		head_.cmd_ = GET_CLSID(msg_game_data);
	}

	int					write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(inf.id_, data_s);
		write_data(inf.type_, data_s);
		write_data<char>(inf.dir_, max_guid, data_s);
		write_data<char>(inf.exe_, max_guid, data_s);
		write_data<char>(inf.update_url_, 128, data_s);
		write_data<char>(inf.help_url_, 128, data_s);
		write_data<char>(inf.game_name_, 128, data_s);
		write_data<char>(inf.thumbnail_, 128, data_s);
		write_data<char>(inf.solution_, 16, data_s);
		write_data(inf.no_embed_, data_s);
		write_data<char>(inf.catalog_, 128, data_s);
		return 0;
	}
};

class		msg_player_info : public msg_base<nosocket>
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
		head_.cmd_ = GET_CLSID(msg_player_info);
		channel_ = 0;
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(uid_, max_guid, data_s);
		write_data(nickname_, max_name, data_s);
		write_data(iid_, data_s);
		write_data(gold_, data_s);
		write_data(vip_level_, data_s);
		write_data(channel_, data_s);
		write_data(gender_, data_s);
		write_data(level_, data_s);
		return 0;
	}
};

class		msg_user_login_ret : public msg_player_info
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
	msg_user_login_ret()
	{
		head_.cmd_ = GET_CLSID(msg_user_login_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_player_info::write(data_s, l);
		write_data(token_, max_guid, data_s);
		write_data(sequence_, data_s);
		write_data(email_, max_guid,  data_s);
		write_data(phone, 32, data_s);
		write_data(address_, 256, data_s);
		write_data(game_gold_, data_s);
		write_data(game_gold_free_, data_s);
		write_data(email_verified_, data_s);
		write_data(phone_verified_, data_s);
		write_data(byear_, data_s);
		write_data(bmonth_, data_s);
		write_data(bday_, data_s);
		write_data(region1_, data_s);
		write_data(region2_, data_s);
		write_data(region3_, data_s);
		write_data(age_, data_s);
		write_data(idcard_, 32, data_s);
		return 0;
	}
};

class msg_user_login_ret_delegate : public msg_user_login_ret
{
public:
	int		sn_;
	msg_user_login_ret_delegate()
	{
		head_.cmd_ = GET_CLSID(msg_user_login_ret_delegate);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_user_login_ret::write(data_s, l);
		write_data(sn_, data_s);
		return 0;
	}
};


//更改用户数据
class msg_account_info_update : public msg_base<nosocket>
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
	msg_account_info_update()
	{
		head_.cmd_ = GET_CLSID(msg_account_info_update);
		address_[0] = 0;
		mobile_[0] = 0;
		email_[0] = 0;
		idcard_[0] = 0;
	}
	int					write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(gender_, data_s);
		write_data(byear_, data_s);
		write_data(bmonth_, data_s);
		write_data(bday_, data_s);
		write_data(address_, 256, data_s);
		write_data(nick_name_, 64, data_s);	
		write_data(age_, data_s);
		write_data(mobile_, 32, data_s);
		write_data(email_, 32, data_s);
		write_data(idcard_, 32, data_s);
		write_data(region1_, data_s);
		write_data(region2_, data_s);
		write_data(region3_, data_s);
		return 0;
	}
};

class		msg_player_leave : public msg_base<nosocket>
{
public:
	int				channel_;
	char			uid_[max_guid];
	msg_player_leave()
	{
		head_.cmd_ = GET_CLSID(msg_player_leave);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(channel_, data_s);
		write_data(uid_, max_guid, data_s);
		return 0;
	}
};

class		msg_broadcast_base : public msg_base<nosocket>
{
public:
	int			msg_type_;		//0 聊天，1 密语，2 系统消息，4 广播 5 礼物
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(msg_type_, data_s);
		return 0;
	}
};

class		msg_chat_deliver : public msg_broadcast_base
{
public:
	int			channel_;
	char		from_uid_[max_guid];
	char		nickname_[max_guid];
	char		content_[512];
	msg_chat_deliver()
	{
		head_.cmd_ = GET_CLSID(msg_chat_deliver);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_broadcast_base::write(data_s, l);
		write_data(channel_, data_s);
		write_data(from_uid_, max_guid, data_s);
		write_data(nickname_, max_guid, data_s);
		write_data(content_, 512, data_s);
		return 0;
	}
};

class		msg_same_account_login : public msg_base<nosocket>
{
public:
	msg_same_account_login()
	{
		head_.cmd_ = GET_CLSID(msg_same_account_login);
	}
};


//通知客户端主播开始。
class msg_live_show_start : public msg_base<nosocket>
{
public:
	int		roomid_;
	char	hostid_[max_guid];
	msg_live_show_start()
	{
		head_.cmd_ = GET_CLSID(msg_live_show_start);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(roomid_, data_s);
		write_data(hostid_, max_guid, data_s);
		return 0;
	}
};

class msg_live_show_stop : public msg_live_show_start
{
public:
	msg_live_show_stop()
	{
		head_.cmd_ = GET_CLSID(msg_live_show_stop);
	}
};



//通知主播可以开播。
class msg_turn_to_show : public msg_base<nosocket>
{
public:
	int			roomid_;
	int			reply_;
	msg_turn_to_show()
	{
		head_.cmd_ = GET_CLSID(msg_turn_to_show);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(roomid_, data_s);
		write_data(reply_, data_s);
		return 0;
	}
};

//通知主播麦序被接受
class msg_host_apply_accept : public msg_player_info
{
public:
	msg_host_apply_accept()
	{
		head_.cmd_ = GET_CLSID(msg_host_apply_accept);
	}
};

class msg_channel_server : public msg_base<nosocket>
{
public:
	char	ip_[max_name];
	int		port_;
	int		for_game_;
	msg_channel_server()
	{
		head_.cmd_ = GET_CLSID(msg_channel_server);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(ip_, max_guid, data_s);
		write_data(port_, data_s);
		write_data(for_game_, data_s);
		return 0;
	}
};

//主播图像
class msg_image_data : public msg_base<none_socket>
{
public:
	int		this_image_for_;		//> 0 avroomid, -1 个人头像, -2 验证码图片
	int		TAG_;	//1开始，2，结束，0，中间
	int		len_;
	char	data_[512];
	msg_image_data()
	{
		head_.cmd_ = GET_CLSID(msg_image_data);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(this_image_for_, data_s);
		write_data(TAG_, data_s);
		write_data(len_, data_s);
		write_data<unsigned char>((unsigned char*)data_, len_, data_s);
		return 0;
	}
};

//继承自msg_host_screenshoot
//此时avroom_id字段的含义:
//0-用户头像，其它以后再扩展
class msg_user_image : public msg_image_data
{
public:
	char		uid_[max_guid];
	msg_user_image()
	{
		head_.cmd_ = GET_CLSID(msg_user_image);
		this_image_for_ = -1;
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_image_data::write(data_s, l);
		write_data(uid_, max_guid, data_s);
		return 0;
	}
};

class msg_verify_code : public msg_base<none_socket>
{
public:
	char	question_[128];
	char	anwsers_[256];
	msg_verify_code()
	{
		head_.cmd_ = GET_CLSID(msg_verify_code);
		anwsers_[0] = 0;
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(question_, 128, data_s);
		write_data(anwsers_, 256, data_s);
		return 0;
	}
};

class msg_check_data_ret : public msg_base<none_socket>
{
public:
	int			query_type_;
	int			result_;
	msg_check_data_ret()
	{
		head_.cmd_ = GET_CLSID(msg_check_data_ret);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(query_type_, data_s);
		write_data(result_, data_s);
		return 0;
	}
};

class msg_koko_trade_inout_ret : public msg_base<none_socket>
{
public:
	char			sn_[max_guid];
	int				result_;				//0 处理成功, 1 已处理过				
	__int64		count_;

	msg_koko_trade_inout_ret()
	{
		head_.cmd_ = GET_CLSID(msg_koko_trade_inout_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(sn_, max_guid, data_s);
		write_data(result_, data_s);
		write_data(count_, data_s);
		return 0;
	}
};

class		msg_sync_item : public msg_base<none_socket>
{
public:
	int				item_id_;
	__int64		count_;
	msg_sync_item()
	{
		head_.cmd_ = GET_CLSID(msg_sync_item);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(item_id_, data_s);
		write_data(count_, data_s);
		return 0;
	}
};

class		msg_region_data : public msg_base<none_socket>
{
public:
	int			rid_;
	int			prid_;
	int			rtype_;
	char		rname[max_name];
	int			pprid_;
	msg_region_data()
	{
		head_.cmd_ = GET_CLSID(msg_region_data);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(rid_, data_s);
		write_data(prid_, data_s);
		write_data(rtype_, data_s);
		write_data(rname, max_name, data_s);
		write_data(pprid_, data_s);
		return 0;
	}
};
//服务器进度
class		msg_srv_progress : public msg_base<none_socket>
{
public:
	int		pro_type_;		//类型 0 登录进度
	int		step_;				//进度
	int		step_max_;		//进度最大值
	msg_srv_progress()
	{
		head_.cmd_ = GET_CLSID(msg_srv_progress);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pro_type_, data_s);
		write_data(step_, data_s);
		write_data(step_max_, data_s);
		return 0;
	}
};

class msg_sync_token : public msg_base<none_socket>
{
public:
	msg_sync_token()
	{
		head_.cmd_ = GET_CLSID(msg_sync_token);
	}

	__int64		sequence_;
	char			token_[max_guid];
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(sequence_, data_s);
		write_data<char>(token_, max_guid, data_s);
		return 0;
	}
};

//通知客户端比赛开始了
class msg_confirm_join_game_deliver : public msg_base<nosocket>
{
public:
	int		match_id_;
	char	ins_id_[max_guid];
	int		register_count_;
	char	ip_[max_guid];
	int		port_;
	msg_confirm_join_game_deliver()
	{
		head_.cmd_ = GET_CLSID(msg_confirm_join_game_deliver);
	}
	int					write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(match_id_, data_s);
		write_data<char>(ins_id_, max_guid, data_s);
		write_data(register_count_, data_s);
		write_data<char>(ip_, max_guid, data_s);
		write_data(port_, data_s);
		return 0;
	}
};

class msg_match_data : public msg_base<nosocket>
{
public:
	match_info	inf;
	msg_match_data()
	{
		head_.cmd_ = GET_CLSID(msg_match_data);
	}

	int					write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(inf.id_, data_s);
		write_data(inf.match_type_, data_s);
		write_data(inf.game_id_, data_s);
		write_data<char>((char*)inf.match_name_.c_str(), 128, data_s);
		write_data<char>((char*)inf.thumbnail_.c_str(), 128, data_s);
		write_data<char>((char*)inf.help_url_.c_str(), 128, data_s);
		write_data<char>((char*)inf.prize_desc_.c_str(), 256, data_s);
		write_data(inf.start_type_, data_s);
		write_data(inf.require_count_, data_s);
		write_data(inf.start_stripe_, data_s);
		write_data(inf.end_type_, data_s);
		write_data(inf.end_data_, data_s);
		write_data<char>((char*)inf.srvip_.c_str(), 64, data_s);
		write_data(inf.srvport_, data_s);
		write_data(inf.cost_.size(), data_s);
		auto it = inf.cost_.begin();
		while (it != inf.cost_.end())
		{
			write_data(it->first, data_s);
			write_data(it->second, data_s);
			it++;
		}
		return 0;
	}
};
