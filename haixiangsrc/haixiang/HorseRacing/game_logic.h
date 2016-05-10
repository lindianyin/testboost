#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
#include "game_service_base.h"
#include "game_player.h"

class horse_racing_logic;
typedef boost::shared_ptr<horse_racing_logic> logic_ptr;
class preset_present;
class tax_info;
class msg_rand_result;
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
	virtual		unsigned int		get_time_total() {return 0;};
	virtual ~state_machine()	{}
};
enum
{
	GET_CLSID(state_wait_start) = 1,
	GET_CLSID(state_do_random) = 2,
	GET_CLSID(state_rest_end) = 3,
};
class state_wait_start : public state_machine<horse_racing_logic>
{
public:
	state_wait_start();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	void			on_wait_config_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};
	virtual		unsigned int		get_time_total();
private:
	boost::asio::deadline_timer timer_;
};

class state_do_random : public state_machine<horse_racing_logic>
{
public:
	state_do_random();

	virtual		int		enter();
	void					on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};
	virtual		unsigned int		get_time_total();
private:
	boost::asio::deadline_timer timer_;
};

class state_rest_end : public state_machine<horse_racing_logic>
{
public:
	state_rest_end();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};
	virtual		unsigned int		get_time_total();
private:
	boost::asio::deadline_timer timer_;
};

typedef boost::shared_ptr<state_machine<horse_racing_logic>> ms_ptr;

class bet_info
{
public:
	string				uid_;
	int						pid_;
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
class		horse_group
{
public:
	int		horse1_;
	int		horse2_;
	int		betted_;
	horse_group()
	{
		horse1_ = horse2_ = 0;
		betted_ = 0;
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

struct scene_set;
class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;
class horse_racing_logic : public logic_base<horse_racing_logic>
{
public:
	int									id_;
	std::string					strid_;							//第几个游戏
	vector<player_ptr>	players_;
	unsigned int				turn_;
	vector<bet_info>		bets_;	//押注列表
	bool								is_waiting_config_;
	vector<turn_info>		last_random_;
	int									cheat_count_;
	vector<preset_present>	presets_;		//本局概率表	,id从0开始，和数据库中不同，为了和数组索引匹配
	vector<horse_group>	group_sets_;		//马匹组合
	vector<int>					vhorse;					//本轮马名次
	vector<float>				speeds_;
	vector<int>					ranks_;					//名次表
	match_ptr						the_match_;
	scene_set*					scene_set_;	

	horse_racing_logic(int is_match);

	void						start_logic();
	void						stop_logic(){};
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

	int							set_bet(player_ptr pp, int pid, int present_id);
	//新一轮开始
	void						this_turn_start();

	//结算结果
	void						balance_result();
	//本轮结束
	void						will_shut_down();
	void						player_login( player_ptr pp, int pos );
	void						leave_game( player_ptr pp, int pos, int why = 0 );

	int							get_today_win(string uid, longlong& win);

	int							get_playing_count(int tp = 0){return 0;}
	int							is_ingame(player_ptr pp)
	{
		auto itf = std::find(players_.begin(), players_.end(), pp);
		return itf != players_.end();
	}
	void						join_player(player_ptr pp){};
private:
	vector<game_report_data> turn_reports_;		//本轮结算结果
	ms_ptr							current_state_;
	preset_present*			this_turn_result_;	//本轮结果
	void								change_to_state(ms_ptr ms);
	void								do_random(map<int, preset_present>& vpr);

	void								record_winner(bet_info& bi, int i);
	void								build_rand_result(msg_rand_result& msg);
	map<int , longlong>	sum_bets();
	longlong						get_total_bets(int pid);
};

