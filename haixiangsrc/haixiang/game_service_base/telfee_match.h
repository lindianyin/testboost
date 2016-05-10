#pragma once
#include "schedule_task.h"
#include "boost/smart_ptr.hpp"
#include "game_config.h"

class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;

class match_wait_begin_task : public task_info
{
public:
	match_ptr the_match_;
	match_wait_begin_task(asio::io_service& ios) : task_info(ios)
	{

	}
	int		routine();
};

class match_wait_end_task : public task_info
{
public:
	match_ptr the_match_;
	match_wait_end_task(asio::io_service& ios) : task_info(ios)
	{

	}
	int		routine();
};

class match_wait_register_task : public task_info
{
public:
	match_ptr the_match_;
	match_wait_register_task(asio::io_service& ios) : task_info(ios)
	{

	}

	int		routine();
};

class match_wait_start_task : public task_info
{
public:
	match_ptr the_match_;
	match_wait_start_task(asio::io_service& ios) : task_info(ios)
	{

	}

	int		routine();
};

struct match_score
{
	std::string   uid_;
	long long			score_;
	long long			fake_;
	match_score()
	{
		score_ = 0;
		fake_ = 0;
	}
};

class		telfee_match_info : public boost::enable_shared_from_this<telfee_match_info>
{
public:
	enum 
	{
		state_init,
		state_wait_register,
		state_wait_start_,
		state_wait_balance,
		state_wait_end,
	};
	int			iid_;
	int			state_;

	int			time_type_;

	std::vector<date_info>   vsche_;
	std::vector<logic_ptr>   vgames_;
	std::vector<match_score> vscores_;
	int			register_time_;		//注册时间

	int			enter_time_;			//入场时间
	int			matching_time_;		//比赛时间
	task_ptr current_schedule_;
	std::map<std::string, int>	registers_; //int意义: 0初始，1询问已发送,2同意，3不同意
	std::map<std::string, std::pair<std::string, int>>	init_scores_; 	//比赛初始积分
	std::vector<int> prizes_;
	scene_set		scene_set_;
	int			fake_register_count_;
	telfee_match_info(int auto_change_state = 1)
	{
		state_ = state_init;
		time_type_ = 0;
		indx_ = 0;
		first_run_ = 1;
		fake_register_count_ = 0;
		auto_change_state_ = auto_change_state;
	}
	//进入接受报名状态
	void		enter_wait_register_state();
	//进入等待开始
	void		enter_wait_start_state();
	//进入等待结算
	void		enter_wait_balance_state();
	void		enter_wait_end_state();
	logic_ptr get_free_game(player_ptr pp);
	logic_ptr get_specific_game(std::string strid);
	void		update();

	void		add_score(std::string uid, int score);
	void		register_this(std::string uid);
	void		do_balance();
protected:
	int		indx_;
	boost::posix_time::ptime	pt_last_refresh_;
	boost::posix_time::ptime	pt_last;
	int		first_run_;
	int		auto_change_state_;
	std::vector<logic_ptr>	distribute_player_to_match();

};
