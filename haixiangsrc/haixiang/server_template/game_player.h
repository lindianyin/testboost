#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "game_service_base.h"
#include "msg.h"

using namespace std;
class fishing_logic;
class fish_player;

typedef boost::shared_ptr<fish_player> player_ptr;

class fish_player : public player_base<fish_player, remote_socket<fish_player>>
{
public:
	std::string		uid_;
	std::string		name_;
	std::string		head_ico_;
	int						sex_;
	longlong			iid_;
	int						is_bot_;
	int						lv_;

	int						join_count_;
	int						login_time_;
	bool					is_connection_lost_;
	longlong			credits_;
	int						pos_;

	//游戏临时数据
	boost::weak_ptr<fishing_logic> the_game_;

	bool					is_copy_;
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
	}

	player_ptr					clone()
	{
		player_ptr ret(new fish_player);
		ret->uid_ = uid_;
		ret->name_							=			name_;
		ret->head_ico_					=			head_ico_;
		ret->sex_								=			sex_;
		ret->iid_								=			iid_;
		ret->the_game_					=			the_game_;
		ret->is_bot_						=			is_bot_;
		ret->credits_						=			credits_;
		ret->pos_								=			pos_;
		ret->join_count_				=			join_count_;
		ret->login_time_				=			login_time_;
		ret->is_connection_lost_=			is_connection_lost_;
		ret->lv_								=     lv_;
		ret->is_copy_ = true;
		return ret;
	}

	int						update();
	void					reset_temp_data()
	{
	}
	boost::shared_ptr<fishing_logic> get_game()
	{
		return the_game_.lock();
	}

	void					on_connection_lost();
};
