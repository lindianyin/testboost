#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
//#include "game_service_base.h"
#include "poker_card.h"
#include "schedule_task.h"
#include "game_player.h"
#define MAX_SEATS 3
#include "logic_base.h"

class ddz_logic;
typedef boost::shared_ptr<ddz_logic> logic_ptr;

template<class service_t>
class state_machine : public boost::enable_shared_from_this<state_machine<service_t>>
{
public:
	state_machine()
	{
		state_id_ = 0;
	}
	boost::weak_ptr<service_t> the_logic_;
	int				state_id_; 
	time_t		enter_tick_;
	virtual		int		enter();
	virtual		int		update()	{return 0;}
	virtual		int		leave()		{cancel(); return 0;}
	virtual		void	cancel()	{the_logic_.reset();}
	virtual	  int		get_time_left() 
	{
		int r = get_time_total() - get_time_elapse();
		return r < 0 ? 0 : r;
	};
	virtual		int		get_time_total() {return 0;};
	virtual  int		get_time_elapse() {return int(GetTickCount() - enter_tick_);}
	virtual ~state_machine()	{}
};
enum
{
	GET_CLSID(state_wait_start) = 1, 
	GET_CLSID(state_do_random) = 2,
	GET_CLSID(state_rest_end) = 3,
	GET_CLSID(state_vote_banker) = 4,		//选庄阶段
	GET_CLSID(state_gaming) = 5,
};

class state_vote_banker : public state_machine<ddz_logic>
{
public:
	state_vote_banker();
	int				enter();
	void			on_update(boost::system::error_code e);
	int				get_time_total() {return 15000;};
private:
	boost::asio::deadline_timer timer_;
};

class state_gaming : public state_machine<ddz_logic>
{
public:
	state_gaming();
	int				enter();
	void			on_update(boost::system::error_code e);
	bool			check_game();
	int				get_time_total() {return 20000;};//出牌状态的时间是20秒
	void			next_player(bool bank_first = false, bool islead = false);
private:
	boost::asio::deadline_timer timer_;
};

class state_wait_start : public state_machine<ddz_logic>
{
public:
	state_wait_start();
	bool			start_success_;
	int				enter();
	void			on_time_expired(boost::system::error_code e);
	void			on_wait_config_expired(boost::system::error_code e);
	int				get_time_total() {return 3000;};
private:
	boost::asio::deadline_timer timer_;
};

class state_do_random : public state_machine<ddz_logic>
{
public:
	state_do_random();
	int						enter();
	void					on_time_expired(boost::system::error_code e);
	int						get_time_total();
private:
	boost::asio::deadline_timer timer_;
};

class state_rest_end : public state_machine<ddz_logic>
{
public:
	state_rest_end();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	virtual		 int		get_time_total();
private:
	boost::asio::deadline_timer timer_;
};

typedef boost::shared_ptr<state_machine<ddz_logic>> ms_ptr;

class bet_info
{
public:
	string				uid_;
	unsigned int	bet_count_;
};

class		game_report_data
{
public:
	string				uid_;
	longlong			pay_;
	longlong			actual_win_;		//实际
	longlong			should_win_;		//应该赢多少
	int						present_id_;		//押哪个
	game_report_data()
	{
		pay_ = actual_win_ = should_win_ = present_id_ = 0;
	}
};

class		turn_info
{
public:
	unsigned int	time_;
	unsigned int	present_id_;
	unsigned int  turn_;
	unsigned int	factor_;
	int						first_;
	int						second_;
	turn_info()
	{
		time_ = present_id_ = turn_ = 0;
		first_ = second_ = 0;
	}
};

class		query_banker_info
{
public:
	string			uid_;
	int					replied_;
	query_banker_info()
	{
		replied_ = -1;
	}
};


struct niuniu_score
{
	std::string uid_;
	int		score_;
	niuniu_score()
	{
		score_ = 0;
	}
};

struct scene_set;

class ddz_logic : public logic_base<ddz_logic>
{
public:
	vector<player_ptr>	observers_;											//观察者
	player_ptr					the_banker_;										//庄家
	int									current_order_;									//当前出牌的位置
	int									current_move_;
	int									current_csf_order_;							//当前最大牌玩家
	int									lead_card_;											//起始出牌
	int									card_played_;										//当前轮是否有人出牌
	int									pass_count_;										//跟不了牌玩家数
	csf_result					current_csf_;										//当前牌面 当前轮桌面上最后出的牌

	int									current_askbank_order_;					//当前问庄顺序
	int									turn_bank_order_;								//当前局庄的位置
	int									max_bet_pos_;										//下注最大的人
	int									max_bet_;												//最大倍率

	unsigned int				turn_;													//局
	bool								is_waiting_config_;
	vector<ddz_card>		shuffle_cards_;
	vector<query_banker_info>			query_bankers_;				//问庄
	vector<task_ptr>		will_join_;

	int									is_match_game_;									//是否为比赛场
	int									run_state_;											//运行状态 开始:0 运行中：1 结束:2
	std::list<csf_result>				play_records_;					//出牌记录
	ddz_logic(int ismatch);
	int							get_banker_pos(player_ptr p1, player_ptr p2, player_ptr p3);
	void						turn_to_play(bool banker_first = false, bool is_lead = false);
	//得到对手玩家,如果是庄家,对手玩家为闲家,如果是闲家,对手玩家只有一个庄家
	player_ptr			get_next_enemy(int offset);		
	void						auto_play();

	void						play_cards_tips();
	void						start_logic();
	void						stop_logic();
	void						play(player_ptr pp, csf_result& csf);
	int							promote_banker(player_ptr p, int reason);
	//随机选择一个庄家
	int							random_choose_banker(int reason);

	void						change_to_vote_banker()
	{
		ms_ptr pms;
		pms.reset( new state_vote_banker);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}
	void						change_to_wait_start();
	void						change_to_gaming()
	{
		ms_ptr pms;
		pms.reset(new state_gaming);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}
	void						change_to_do_random()
	{
		ms_ptr pms;
		pms.reset( new state_do_random);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}
	void						change_to_end_rest()
	{
		ms_ptr pms;
		pms.reset( new state_rest_end);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}

	void						broadcast_msg(msg_base<none_socket>& msg, bool include_observers = true);
	int							random_result();

	int							set_bet(player_ptr pp, longlong count);
	//新一轮开始
	int							this_turn_start();
	int							this_turn_init();
	//本轮结束
	void						will_shut_down();
	int							player_login( player_ptr pp, int seat = -1);
	//why = 0,玩家主动退出游戏, 1 换桌退出游戏 2 游戏结束清场退出游戏， 3，T出游戏 4 参加比赛场退出游戏
	void						leave_game( player_ptr pp, int pos, int why = 0);
	bool						is_ingame(player_ptr pp);
	int							get_today_win(string uid, longlong& win);
	void						balance_result();
	void						broadcast_match_result(player_ptr pp, int pos);
	void						broadcast_random_result(player_ptr pp, int pos);
	void						save_match_result(player_ptr pp);
	//玩家是否可以继续玩下去
	int							can_continue_play(player_ptr pp);
	void						query_for_become_banker();
	bool						is_all_query_replied();
	bool						this_turn_is_over();
	bool						is_all_bet_setted();
	int							this_turn_over();
	int							join_player(player_ptr pp);
	ms_ptr						current_state_;
private:
	vector<game_report_data>	turn_reports_;		//本轮结算结果

	void						change_to_state(ms_ptr ms);
	void						reset_to_not_banker();
	int							balance_banker_lose(player_ptr the_banker, player_ptr pp, double fac1);
	int							balance_banker_win(player_ptr the_banker, player_ptr pp);
};

