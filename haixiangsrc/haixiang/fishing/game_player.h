#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "msg.h"
#include "player_base.h"

using namespace std;
class fishing_logic;
class fish_player;
class fish;
typedef boost::shared_ptr<fish_player> player_ptr;
class fish_player : public player_base<fish_player, remote_socket<fish_player>, fishing_logic>
{
public:

	int						join_count_;
	int						login_time_;

	bool					is_copy_;

	int						gun_level_;
	std::string		gun_type_;
	__int64				coins_, free_coins_;
	boost::posix_time::ptime		last_fire_, last_msg_;
	__int64			lucky_;
	int						is_quiting_;
	__int64			total_pay_, total_win_;

	task_ptr			update_task_;
	boost::weak_ptr<fish> the_lockfish_;
	fish_player()
	{
		is_connection_lost_ = false;
		credits_ = 0;
		pos_ = -1;
		is_bot_ = 0;
		login_time_ = 0;
		join_count_ = 0;
		iid_ = 0;
		lv_ = 0;
		is_copy_ = false;
		gun_level_ = 0;
		coins_ = 0;
		free_coins_ = 0;
		gun_type_ = "gun1";
		online_prize_avilable_ = 0;
		online_prize_ = 0;
		is_quiting_ = 0;
		lucky_ = 0;
		total_pay_ = 0;
		total_win_ = 0;
	}

	int						update();
	void					reset_temp_data();

	boost::shared_ptr<fishing_logic> get_game()
	{
		return the_game_.lock();
	}
	void					add_exp(int cnt);
	void					on_connection_lost();
	void					add_coin(__int64 count);
};
