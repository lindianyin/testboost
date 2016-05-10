#pragma once
#include "boost/thread.hpp"

#define GET_CLSID(m) m##_id

enum 
{
	error_wrong_sign = -1000,	//数字签名不对
	error_wrong_password,			//密码不对
	error_sql_execute_error,	//sql语句执行失败
	error_no_record,					//数据库中没有相应记录
	error_user_banned,				//用户被禁止
	error_account_exist,			//注册账户名已存在
	error_server_busy,				//服务器正忙
	error_cant_find_player,		//找不到玩家
	error_cant_find_match,
	error_cant_find_server,
	error_msg_ignored,
	error_cancel_timer,
	error_cannt_regist_more,  //不能再注册了
	error_email_inusing,
	error_mobile_inusing,
	error_wrong_verify_code,
	error_time_expired,
	error_invalid_data,
	error_success = 0,
	error_business_handled,
	error_invalid_request,
	error_not_enough_gold,
	error_not_enough_gold_game,
	error_not_enough_gold_free,
	error_cant_find_coordinate,
};

enum 
{
	GET_CLSID(msg_koko_trade_inout_ret) = 998,
	GET_CLSID(msg_user_login_ret_delegate) = 999,
	GET_CLSID(msg_user_login_ret) = 1000,
	GET_CLSID(msg_common_reply) = 1001,//占位符。
	GET_CLSID(msg_common_reply_internal) = 6000,//占位符。
	GET_CLSID(msg_player_info) = 1002,
	GET_CLSID(msg_player_leave),
	GET_CLSID(msg_chat_deliver),
	GET_CLSID(msg_same_account_login),
	GET_CLSID(msg_live_show_start),
	GET_CLSID(msg_turn_to_show),
	GET_CLSID(msg_host_apply_accept),
	GET_CLSID(msg_channel_server),
	GET_CLSID(msg_image_data),
	GET_CLSID(msg_user_image),
	GET_CLSID(msg_account_info_update),
	GET_CLSID(msg_verify_code),
	GET_CLSID(msg_check_data_ret),
	GET_CLSID(msg_live_show_stop),
	GET_CLSID(msg_sync_item),
	GET_CLSID(msg_region_data),
	GET_CLSID(msg_srv_progress),
	GET_CLSID(msg_sync_token),
	GET_CLSID(msg_confirm_join_game_deliver),
};

enum
{
	GET_CLSID(msg_koko_trade_inout) = 998,
	GET_CLSID(msg_user_login) = 100,
	GET_CLSID(msg_user_register),
	GET_CLSID(msg_join_channel),
	GET_CLSID(msg_chat),
	GET_CLSID(msg_leave_channel),
	GET_CLSID(msg_host_apply),
	GET_CLSID(msg_show_start),
	GET_CLSID(msg_user_login_channel),
	GET_CLSID(msg_upload_screenshoot),
	GET_CLSID(msg_show_stop),
	GET_CLSID(msg_change_account_info),
	GET_CLSID(msg_get_verify_code),
	GET_CLSID(msg_check_data),
	GET_CLSID(msg_user_login_delegate),
	GET_CLSID(msg_send_present),
	GET_CLSID(msg_get_token),
	GET_CLSID(msg_action_record),
	GET_CLSID(msg_get_game_coordinate),
};

enum 
{
	GET_CLSID(msg_register_to_world) = 19999,
	GET_CLSID(msg_match_info) = 20000,
	GET_CLSID(msg_match_start) = 20001,
	GET_CLSID(msg_match_score) = 20002,
	GET_CLSID(msg_match_end) = 20003,
	GET_CLSID(msg_player_distribute) = 20004,
	GET_CLSID(msg_confirm_join_game) = 20005,
	GET_CLSID(msg_confirm_join_game_reply) = 20006,
	GET_CLSID(msg_match_register) = 20007,
	GET_CLSID(msg_match_cancel) = 20008,
	GET_CLSID(msg_query_info) = 20009,
	GET_CLSID(msg_match_data) = 20010,
	GET_CLSID(msg_game_data) = 20011,
	GET_CLSID(msg_host_data) = 20012,
	GET_CLSID(msg_game_room_data) = 20013,
	GET_CLSID(msg_my_match_score) = 20014,
	GET_CLSID(msg_my_game_data) = 20015,
	GET_CLSID(msg_match_champion) = 20016,
	GET_CLSID(msg_match_report) = 20017,
	GET_CLSID(msg_match_over) = 20018,
	GET_CLSID(msg_query_data_begin),
	GET_CLSID(msg_player_eliminated_by_game),
};

enum 
{
	item_id_gold,
	item_id_gold_game,
	item_id_gold_free,
	item_id_gold_game_lock,
};

enum 
{
	client_action_logingame,
	client_action_logoutgame,

};
struct end_point
{
	char	ip_[32];
	int		port_;
};

//平台启动时，会下载主播，游戏，比赛的配置文件
//客户端就有了处理依据

//主播配置信息
struct host_info
{
	int			roomid_;	//房间ID
	char		roomname_[128];	//房间名称
	char		host_uid_[64]; //主播ID
	char		thumbnail_[128]; //缩略图
	end_point	avaddr_; //视频服务器地址
	end_point	game_addr_; //游戏服务器地址
	int			gameid_;//关联游戏id
	int			popular_;
	int			online_players_;
	int			is_online_;
	int			avshow_start_;
	host_info()
	{
		popular_ = 0;
		online_players_ = 0;
		is_online_ = 0;
		gameid_ = 0;
		roomid_ = 0;
		roomname_[0] = 0;
		host_uid_[0] = 0;
		thumbnail_[0] = 0;
		avshow_start_ = 0;
	}
};


struct game_room_inf
{
	int						game_id_;
	int						room_id_;
	char					room_name_[64];
	char					room_desc_[64];
	char					ip_[32];
	int						port_;
	char					thumbnail_[128];			//缩略图
	char					require_[256];
	game_room_inf()
	{
		require_[0] = 0;
	}
};

//游戏配置信息，平台启动时，会下载
struct game_info
{
	enum 
	{
		game_cat_classic,
		game_cat_match,
		game_cat_host,
	};
	enum 
	{

	};
	int						id_;							//游戏id	
	int						type_;						//0 客户端游戏，1 flash游戏
	char					dir_[64];					//目录，只允许填一个目录名，不允许填路径
	char					exe_[64];					//可执行程序名
	char					update_url_[128];	//更新路径
	char					help_url_[128];		//介绍路径
	char					game_name_[128];	//游戏名称
	char					thumbnail_[128];	//略缩图
	char					solution_[16];		//游戏的设计分辨率,如果为noembed,表示不嵌入平台运行
	int						no_embed_;
	char					catalog_[128];
	std::vector<game_room_inf> rooms_;
};

struct channel_server
{
	std::string ip_;
	int		port_;
	int		online_;
	std::vector<int> game_ids_;
};

struct match_info : public boost::enable_shared_from_this<match_info>
{
	int						id_;						//比赛id
	int						match_type_;		//比赛类型 0-定时赛, 1-个人闯关赛,2-淘汰赛
	int						game_id_;				//游戏id，对应game_info的游戏id
	std::string		match_name_;		//比赛名称
	std::string		thumbnail_;			//缩略图
	std::string		help_url_;			//比赛介绍	
	std::string		prize_desc_;		//奖励描述
	int						start_type_	;		//开赛时机,0按时开赛,1人满即开 2随报随开
	int						require_count_;	//开赛最低人数要求

	int						start_stripe_;	//start_type_ = 0时，这个字段有效, 0 间隔时间开, 1 固定时间开
	union 
	{
		int					wait_register_; //start_strip_ = 0 时， 等待注册时间,秒为单位
		struct											//start_strip_ = 1 时，
		{
			int				day, h, m, s;		//间隔天数, 于那天的 时，分，秒
		};
	};

	int						need_players_per_game_;					//每个游戏需要几人参于
	int						eliminate_phase1_basic_score_;	//淘汰赛海选阶段初始分
	int						eliminate_phase1_inc;						//淘汰赛海选阶段基础分增量(低于基础分的被淘汰,以分钟为单位计)
	int						elininate_phase2_count_;				//淘汰赛进入决赛阶段人数

	int						end_type_;		//结束时机 0定时结束，1 决出胜者
	int						end_data_;		//数据 end_type_ = 0, 这里是结束时长, = 1，是决出胜者数

	std::vector<std::vector<std::pair<int, int>>>	prizes_;		//名次(按在数组的次序)，物品id,　数量
	std::map<int, int>	cost_;										//报名费 物品id,　数量
	std::string		srvip_;
	int						srvport_;
	match_info() 
	{
		eliminate_phase1_inc = 0;
		elininate_phase2_count_ = 0;
		eliminate_phase1_basic_score_ = 0;
		need_players_per_game_ = 4;
		end_type_ = 0;
		end_data_ = 0;
		start_stripe_ = 0;
		wait_register_ = 0;
		h = m = s = 0;
	};
	virtual ~match_info(){}
};

template<class data_type>
class atomic_value
{
public:
	void				store(data_type v)
	{
		boost::lock_guard<boost::mutex> lk(mtu_);
		dat_ = v;
	}
	data_type		load()
	{
		data_type ret;
		boost::lock_guard<boost::mutex> lk(mtu_);
		ret = dat_;
		return ret;
	}
protected:
	data_type		dat_;
	boost::mutex	mtu_;
};

template<class socket_t>
void	send_all_game_to_player(socket_t psock)
{
	auto it = all_games.begin();
	while (it !=  all_games.end())
	{
		gamei_ptr pgame = it->second; it++;
		msg_game_data msg;
		msg.inf = *pgame;
		send_msg(psock, msg);
	}
}

template<class socket_t>
void	send_all_match_to_player(socket_t psock)
{
	auto it = match_manager::get_instance()->all_matches.begin();
	while (it !=  match_manager::get_instance()->all_matches.end())
	{
		match_ptr pmatch = it->second; it++;
		msg_match_data msg;
		msg.inf = *pmatch;
		send_msg(psock, msg);
	}
}
