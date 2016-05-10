#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
#include "schedule_task.h"
#include "fish_cloud.h"
#include <set>
#include "game_player.h"

#define			MAX_SEATS 6    //在要"logic_base.h"前
#include "logic_base.h"

class fishing_logic;
typedef boost::shared_ptr<fishing_logic> logic_ptr;

class preset_present;
class tax_info;

class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;



struct scene_set;

class task_for_clean_fish_for_cloud_spawn : public task_info
{
public:
	logic_wptr logic_;
	task_for_clean_fish_for_cloud_spawn(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class task_for_clean_fish_for_random_spawn : public task_info
{
public:
	logic_wptr logic_;
	int			wait_time_;
	task_for_clean_fish_for_random_spawn(asio::io_service& ios):task_info(ios)
	{
		wait_time_ = 0;
	}
	virtual int routine();
};

class task_for_spawn_cloud_generate : public task_info
{
public:
	logic_wptr logic_;
	task_for_spawn_cloud_generate(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class task_for_spawn_freely_generate : public task_info
{
public:
	logic_wptr logic_;
	task_for_spawn_freely_generate(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class task_for_clean_fish_for_boss_spwan : public task_info
{
public:
	task_for_clean_fish_for_boss_spwan(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class	task_for_enable_boss_die : public task_info
{
public:
	fish_wptr	the_wboss_;
	task_for_enable_boss_die(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class task_for_forece_boss_die : public task_info
{
public:
	fish_wptr	the_wboss_;
	task_for_forece_boss_die(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class task_for_adjust_rate : public task_info
{
public:
	fish_wptr	the_wboss_;
	int				last_rate_;
	__int64		lotty_inc_;
	task_for_adjust_rate(asio::io_service& ios):task_info(ios)
	{
		last_rate_= 0;
		lotty_inc_ = 0;
	}
	virtual int routine();
};

class task_for_spwan_global_boss_generate : public task_info
{
public:
	task_for_spwan_global_boss_generate(asio::io_service& ios):task_info(ios)
	{

	}
	virtual int routine();
};

class fishing_logic : public logic_base<fishing_logic>
{
public:

	boost::weak_ptr<fish_player>		the_host_;
	unsigned int				turn_;
	std::map<int, fish_ptr>	fishes_;
	fcloud_ptr					the_cloud_generator_;
	task_ptr						current_spwan_;
	int									is_match_game_;	//是否为比赛场
	match_ptr						the_match_;
	int									run_state_;
	int									cur_map_;
	int									is_adding_bot_, is_removing_bot_;
	bool								is_host_game_;
	std::unordered_map<std::string, std::map<unsigned int, unsigned int>> missiles_;

	explicit fishing_logic(int is_match);

	void						join_fish(fish_ptr fh);
	void						join_player(player_ptr p);

	int							update(float dt);
	void						start_logic();
	void						stop_logic();

	void						broadcast_msg(msg_base<none_socket>& msg, int exclude_pos = -1);
	void						fishes_runout();
	//本轮结束
	void						will_shut_down();
	int							player_login( player_ptr pp, int seat = -1);
	//why = 0,玩家主动退出游戏, 1 换桌退出游戏 2 游戏结束清场退出游戏
	void						leave_game( player_ptr pp, int pos, int why = 0);
	bool						is_ingame(player_ptr pp);
	int							get_today_win(string uid, __int64& win);
	void						sync_fish(fish_ptr fh, player_ptr pp);
	void						restore_account(player_ptr pp);
	void						adjust_bot();
};

