#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
#include "platform_gameid.h"
#include "msg.h"

class beauty_player;
typedef boost::shared_ptr<beauty_player> player_ptr;
#include "logic_base.h"


class beatuy_beast_logic;
typedef boost::shared_ptr<beatuy_beast_logic> logic_ptr;
class tax_info;
typedef __int64 longlong;
enum
{
	state_wait_start_id = 1,
	state_do_random_id = 2,
	state_rest_end_id= 3,
};
//状态机
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
	virtual		int		enter();
	virtual		int		update()	{return 0;}
	virtual		int		leave()		{cancel(); return 0;}
	virtual		void	cancel()	{the_logic_.reset();}
	virtual		unsigned int		get_time_left() {return 0;};
	virtual ~state_machine()	{}
};

class state_wait_start : public state_machine<beatuy_beast_logic>
{
public:
	state_wait_start();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	void			on_wait_config_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_seconds();
	};
private:
	boost::asio::deadline_timer timer_;
};

class state_do_random : public state_machine<beatuy_beast_logic>
{
public:
	state_do_random();

	virtual		int		enter();
	void					on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_seconds();
	};
private:
	boost::asio::deadline_timer timer_;
};

class state_rest_end : public state_machine<beatuy_beast_logic>
{
public:
	state_rest_end();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_seconds();
	};
private:
	boost::asio::deadline_timer timer_;
};
typedef boost::shared_ptr<state_machine<beatuy_beast_logic>> ms_ptr;
struct scene_set;
class beatuy_beast_logic : public logic_base<beatuy_beast_logic>
{
public:
	int									id_;
	std::string					strid_;							//第几个游戏
	player_ptr					the_banker_;			//玩家庄家,如果是空，则是系统庄家
	vector<player_ptr>	players_;
	vector<player_ptr>	bots_;
	scene_set*					scene_set_;						
	__int64						banker_deposit_;
	__int64						sysbanker_initial_deposit_;
	unsigned int				turn_;
	unsigned int				cheat_count_;
	vector<bet_info>		bets_;	//押注列表
	bool								is_waiting_config_;
	vector<int>					last_random_;
	int									bot_bet_count_;
	beatuy_beast_logic(int ismatch);
	~beatuy_beast_logic()
	{
	}

	void						start_logic();

	void						change_to_wait_start()
	{
		ms_ptr pms;
		pms.reset( new state_wait_start);
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
	//可否下注
	int							can_bet(int pid, unsigned int count);
	
	void						broadcast_msg(msg_base<none_socket>& msg);
	void						random_result();

	int							apply_banker(player_ptr pp);

	int							apply_cancel_banker(player_ptr pp);

	int							set_bet(player_ptr pp, int pid, int present_id);
	//新一轮开始
	void						this_turn_start();

	//结算结果
	void						balance_result();
	//本轮结束
	void						will_shut_down();
	void						bot_join(player_ptr pp );
	void						player_login( player_ptr pp, int pos = -1);
	void						join_player(player_ptr pp);
	void						leave_game( player_ptr pp, int pos, int why = 0);
	bool						is_ingame(player_ptr pp);
	int							get_today_win(string uid, longlong& win);
	int							can_promote_banker(player_ptr pp);

	void						bot_random_bet();
	void						random_bet();
	player_ptr			get_player(string uid);
	void						join_bots();		//随机加入一批机器人
	void						clear_bots();		//清除现有机器人，必须在新的一轮开始前
	bool						is_bot(string uid);
	//玩家下注注数，用来限制机器人下注，玩家下得多了，机器人就少下点
	int							get_player_betcount();
	int							get_playing_count()
	{
		return players_.size();
	}
	void						stop_logic(){}


private:
	boost::asio::deadline_timer timer_;
	vector<string>			banker_applications_;		//申请上庄列表
	map<string, game_report_data> turn_reports_;		//本轮结算结果
	ms_ptr							current_state_;
	bool								apply_cancel_;
	preset_present			this_turn_result_;	//本轮结果
	bool								last_is_sysbanker_;	//上轮是
	void								change_to_state(ms_ptr ms);
	int									restore_deposit();
	void								do_random(map<int, preset_present>& vpr);
	//下庄
	void								cancel_banker();	
	//推一个新玩家做庄
	void								next_banker();
	//提升为庄家
	int									promote_to_banker(player_ptr pp);

	void								record_winner(bet_info& bi, double fator);

	void								record_lose(bet_info& bi);

	static bool					sort_tax_fun(tax_info& t1, tax_info& t2);

	longlong						calc_sys_tax(longlong n);
	longlong						calc_player_tax(longlong n);
	map<int , longlong>	sum_bets(bool include_bot = false);
	longlong						get_present_chips(int pid);

	//发送玩家赢钱的广播消息给7158
	void                broadcast_msg_to_7158(game_report_data report_);

};

