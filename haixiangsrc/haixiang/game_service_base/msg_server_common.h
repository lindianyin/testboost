#pragma once
#include "msg.h"
#include "utility.h"

enum
{
	GET_CLSID(msg_register_to_world)= 19999,
	GET_CLSID(msg_match_score) = 20002,
	GET_CLSID(msg_ping) = 0xFFFF,
	GET_CLSID(msg_user_login_ret)		= 1000,
	GET_CLSID(msg_common_reply)			= 1001,				//这条消息id需要固定下来。它处于公共类里
	GET_CLSID(msg_common_reply_srv)	= 6000,	//这条消息id需要固定下来。它处于公共类里
	GET_CLSID(msg_logout)						= 1034,
	GET_CLSID(msg_currency_change)	= 1007,				//这条消息id需要固定下来。它处于公共类里
	GET_CLSID(msg_server_parameter)	= 1107,
	GET_CLSID(msg_low_currency)		= 1109,
	GET_CLSID(msg_player_seat)		= 1110,
	GET_CLSID(msg_room_info),
	GET_CLSID(msg_player_leave),
	GET_CLSID(msg_deposit_change2),
	GET_CLSID(msg_chat_deliver),

	GET_CLSID(msg_everyday_gift),
	GET_CLSID(msg_levelup),
	GET_CLSID(msg_set_account_ret),
	GET_CLSID(msg_recommend_data),
	GET_CLSID(msg_trade_data),
	GET_CLSID(msg_game_info),
	GET_CLSID(msg_server_info),
	GET_CLSID(msg_rank_data),
	msg_serverid_this_game_begin = 1500,
};



class msg_set_account_ret : public msg_base<none_socket>
{
public:
	char					uid_[max_guid];
	unsigned int	iid_;
	msg_set_account_ret()
	{
		head_.cmd_ = GET_CLSID(msg_set_account_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(iid_, data_s);
		return 0;
	}
};

class msg_user_login_ret:public msg_base<none_socket>
{
public:
	int			iid_;
	int			lv_;
	double	currency_;
	double	exp_;
	double	exp_max_;
	char		uname_[max_name];
	char		uid_[max_guid];
	char		head_pic_[max_guid];
	msg_user_login_ret()
	{
		head_.cmd_ = GET_CLSID(msg_user_login_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data(lv_, data_s);
		write_data(currency_, data_s);
		write_data(exp_, data_s);
		write_data(exp_max_, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		return 0;
	}
};

class msg_ping : public msg_base<none_socket>
{
public:
	msg_ping()
	{
		head_.cmd_ = GET_CLSID(msg_ping);
	}
};

//账号内容改变，这个暂时不用，你收到这个消息时忽略掉。
class msg_currency_change : public msg_base<none_socket>
{
public:
	enum{
		why_sync,
		why_everyday_login,
		why_levelup,
		why_rnd_result,
		why_online_prize,
		why_trade_complete,
	};
	double			credits_;			//玩家资金数 8字节
	int					why_;					//0,同步(总量)，1,每日登录奖励(变化量)，2，等级装奖励(变化量), 3,财神奖金

	msg_currency_change()
	{
		credits_ = 0;
		head_.cmd_ = GET_CLSID(msg_currency_change);
		why_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(credits_, data_s);
		write_data(why_, data_s);
		return 0;
	}
};

class msg_low_currency : public msg_base<none_socket>
{
public:
	long long require_;
	msg_low_currency()
	{
		head_.cmd_ = GET_CLSID(msg_low_currency);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(require_, data_s);
		return 0;
	}
};

//账号在其它地方登录提示
class msg_logout : public msg_base<none_socket>
{
public:
	char uid_[max_guid];	//40字节
	msg_logout()
	{
		head_.cmd_ = GET_CLSID(msg_logout);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		return 0;
	}
};

//玩家保证金变化
class msg_deposit_change2 : public msg_currency_change
{
public:
	int		pos_;
	int		display_type_;		//0，不需要飘字 1,要飘字, 2 保证金变化
	msg_deposit_change2()
	{
		head_.cmd_ = GET_CLSID(msg_deposit_change2);
		display_type_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_currency_change::write(data_s, l);
		write_data(pos_, data_s);
		write_data(display_type_, data_s);
		return 0;
	}
};


class msg_player_leave : public msg_base<none_socket>
{
public:
	int				pos_;		//哪个位置离开游戏
	int				why_;		//why = 0,玩家主动退出游戏, 1 换桌退出游戏 2 游戏结束清场退出游戏， 3，T出游戏
	msg_player_leave()
	{
		head_.cmd_ = GET_CLSID(msg_player_leave);
		why_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(why_, data_s);
		return 0;
	}
};


class msg_chat_deliver : public msg_base<none_socket>
{
public:
	int						channel_;						//频道
	char					name_[max_name];		//谁
	int						sex_;								//暂时没用
	char					content_[256];			//说了什么
	msg_chat_deliver()
	{
		head_.cmd_ = GET_CLSID(msg_chat_deliver);
		sex_ = -1;
		memset(name_, 0, max_name);
		memset(content_, 0, 256);
	}
	void		set_content(string s)
	{
		if(s.length() > 256) return;
		COPY_STR(content_, s.c_str());
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(channel_, data_s);
		write_data<char>(name_, max_name, data_s);
		write_data<int>(sex_, data_s);
		write_data<char>(content_, 256, data_s, false);
		return 0;
	}
};

class msg_everyday_gift : public msg_base<none_socket>
{
public:
	msg_everyday_gift()
	{
		head_.cmd_ = GET_CLSID(msg_everyday_gift);
		getted_ = 0;
		conitnue_days_ = 0;
		type_ = 0;
	}

	int			conitnue_days_;	//如果type==3,这里是要领取等级
	int			getted_;	
	int			type_;		//和msg_get_prize_req里的type相同意义
	int			extra_data_;		//如果type ==3,这里是剩余计时
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(conitnue_days_, data_s);
		write_data(getted_, data_s);
		write_data(type_, data_s);
		write_data(extra_data_, data_s);
		return 0;
	}
};

class msg_levelup : public msg_base<none_socket>
{
public:
	int lv_;
	double exp_;
	double exp_max_;
	int	pos_;
	msg_levelup()
	{
		head_.cmd_ = GET_CLSID(msg_levelup);
		lv_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(lv_, data_s);
		write_data(exp_, data_s);
		write_data(exp_max_, data_s);
		return 0;
	}
};

//推荐信息数据
class msg_recommend_data : public  msg_base<none_socket>
{
public:
	short SS;	//起始标记
	short	SE;	//结束标记	
	char uid_[max_guid];
	char uname_[max_name];
	char head_pic_[max_guid];
	int reward_getted_;	//奖励是否已领取 0，未领取，1领取
	msg_recommend_data()
	{
		head_.cmd_ = GET_CLSID(msg_recommend_data);
		SS = 0;
		SE = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(SS, data_s);
		write_data(SE, data_s);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data(reward_getted_, data_s);
		return 0;
	}
};

class msg_trade_data : public  msg_base<none_socket>
{
public:
	short SS;	//起始标记
	short	SE;	//结束标记	
	int		iid_;					//数字id
	double	gold_;				//钱
	double	time;					//时间

	msg_trade_data()
	{
		head_.cmd_ = GET_CLSID(msg_trade_data);
		SS = 0;
		SE = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(SS, data_s);
		write_data(SE, data_s);
		write_data(iid_, data_s);
		write_data(gold_, data_s);
		write_data(time, data_s);
		return 0;
	}
};

class msg_server_info : public msg_base<none_socket>
{
public:
	int		player_count_;
	int		is_in_server_;
	msg_server_info()
	{
		head_.cmd_ = GET_CLSID(msg_server_info);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(player_count_, data_s);
		write_data(is_in_server_, data_s);
		return 0;
	}
};

class msg_game_info : public msg_base<none_socket>
{
public:
	char	game_id_[max_guid];
	int		extra_data1_;
	MSG_CONSTRUCT(msg_game_info);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(game_id_, max_guid, data_s);
		write_data(extra_data1_, data_s);
		return 0;
	}
};

class msg_room_info : public msg_base<none_socket>
{
public:
	int			room_id_;		//房间id
	int			free_pos_;	//空余位置
	int			observers_;	//观察者数量

	msg_room_info()
	{
		head_.cmd_ = GET_CLSID(msg_room_info);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(room_id_, data_s);
		write_data(free_pos_, data_s);
		write_data(observers_, data_s);
		return 0;
	}
};

//玩家坐到位置
class msg_player_seat : public msg_base<none_socket>
{
public:
	char			uid_[max_guid];	//谁
	char			head_pic_[max_guid];
	char			uname_[max_name];
	int				pos_;						//坐在哪个位置
	int				iid_;
	char			gun_type_[max_name];	//枪类型
	int				gun_level_;	//枪等级
	int				lv_;
	msg_player_seat()
	{
		head_.cmd_ = GET_CLSID(msg_player_seat);
		memset(gun_type_, 0, max_name);
		gun_level_ = 0; lv_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data(pos_, data_s);
		write_data<char>(gun_type_, max_name, data_s);
		write_data(gun_level_, data_s);
		write_data(lv_, data_s);
		return 0;
	}
};

class msg_rank_data : public msg_base<none_socket>
{
public:
	char	uname_[max_name];
	short	rank_type_;
	short	tag_; //0新的开始，1后续数据
	double	data_;
	MSG_CONSTRUCT(msg_rank_data);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uname_, max_name, data_s);
		write_data(rank_type_, data_s);
		write_data(tag_, data_s);
		write_data(data_, data_s);
		return 0;
	}
};


enum 
{
	GET_CLSID(msg_match_info) = msg_serverid_this_game_begin,
	GET_CLSID(msg_ask_for_join_match),
	GET_CLSID(msg_prepare_enter),
	GET_CLSID(msg_time_left),
	GET_CLSID(msg_match_result),
	GET_CLSID(msg_match_progress),
	GET_CLSID(msg_broadcast_info),
};

//发送给客户端询问是否加入比赛
class msg_ask_for_join_match : public msg_base<none_socket>
{
public:
	int		match_id_;				//比赛类型
	MSG_CONSTRUCT(msg_ask_for_join_match);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(match_id_, data_s);
		return 0;
	}	
};

class msg_time_left : public msg_base<none_socket>
{
public:
	int		left_;
	int		state_;
	MSG_CONSTRUCT(msg_time_left);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(left_, data_s);
		write_data(state_, data_s);
		return 0;
	}	
};

//通知客户端比赛进程
class msg_match_progress : public msg_base<none_socket>
{
public:
	int		my_score_;
	int		player_count_;
	int		high_score_;
	int		my_rank_;
	MSG_CONSTRUCT(msg_match_progress);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(my_score_, data_s);
		write_data(player_count_, data_s);
		write_data(high_score_, data_s);
		write_data(my_rank_, data_s);
		return 0;
	}	
};

class msg_match_result : public msg_base<none_socket>
{
public:
	int		rank_;		//第几名
	int		money_;		//获得多少钱
	int		telfee_;	//获得话费，值是比赛场次id,根据id算话费值
	char	key_[max_guid];	//兑换码
	MSG_CONSTRUCT(msg_match_result);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(rank_, data_s);
		write_data(money_, data_s);
		write_data(telfee_, data_s);
		write_data<char>(key_, max_guid, data_s);
		return 0;
	}	
};

class msg_broadcast_info : public msg_base<none_socket>
{
public:
	char				who_[max_name]; //谁
	int					at_where_;		//在哪里 0,金币场，1话费场
	int					at_data_;			//具体场
	long long			get_what_;		//获得了什么
	long long			get_what2_;		//还获得了什么
	MSG_CONSTRUCT(msg_broadcast_info);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(who_, max_name, data_s);
		write_data(at_where_, data_s);
		write_data(at_data_, data_s);
		write_data(get_what_, data_s);
		write_data(get_what2_, data_s);
		return 0;
	}
};

class msg_match_info : public msg_base<none_socket>
{
public:
	int		match_id_;				//比赛类型
	int		self_registered_;	//已报过名了
	int		register_count_;	//已报名人数
	int		state_;						//状态,0 接受报名，1 等待开始，2 比赛中
	unsigned int match_time_left_;		//比赛开始剩余时间
	MSG_CONSTRUCT(msg_match_info);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(match_id_, data_s);
		write_data(self_registered_, data_s);
		write_data(register_count_, data_s);
		write_data(state_, data_s);
		write_data(match_time_left_, data_s);
		return 0;
	}
};

class msg_prepare_enter : public msg_base<none_socket>
{
public:
	int		match_id_;
	int		extra_data1_;
	MSG_CONSTRUCT(msg_prepare_enter);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(match_id_, data_s);
		write_data(extra_data1_, data_s);
		return 0;
	}	
};

//KOKO平台比赛场消息
class msg_match_score : public msg_base<none_socket>
{
public:
	char	uid_[max_guid];
	int		match_id_;
	int		score_;
	int		operator_; // 0 - set, 1 - add
	msg_match_score()
	{
		head_.cmd_ = GET_CLSID(msg_match_score);
	}

	int					write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(match_id_, data_s);
		write_data(score_, data_s);
		write_data(operator_, data_s);
		return 0;
	}
};

class msg_register_to_world : public msg_base<none_socket>
{
public:
	int			gameid_;
	MSG_CONSTRUCT(msg_register_to_world);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(gameid_, data_s);
		return 0;
	}
};
