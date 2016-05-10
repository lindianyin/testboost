#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "game_service_base.h"
#include "msg.h"
#include "player_base.h"

using namespace std;
class tiger_logic;
class tiger_player;

typedef boost::shared_ptr<tiger_player> player_ptr;


struct stPlayeGameInfo 
{
	int			game_count_;
	longlong    one_game_max_win_;
	longlong    today_win_;
	longlong    month_win_;
	longlong    time_;
};


class tiger_player : public player_base<tiger_player, remote_socket<tiger_player>>
{
public:
	int						join_count_;
	int						login_time_;
	bool					is_connection_lost_;
 	stPlayeGameInfo         game_info_;
	
	//游戏临时数据
	boost::weak_ptr<tiger_logic> the_game_;


	int						is_win_;
	std::map<int, int>			bets_;
	longlong			temp_win_;
	tiger_player()
	{
		is_win_ = -1;
		is_connection_lost_ = false;
		credits_ = 0;
		pos_ = -1;
		is_bot_ = 0;
		login_time_ = 0;
		join_count_ = 0;
		iid_ = 0;
		lv_ = 0;
		
		memset(&game_info_,0,sizeof(game_info_));
	}

	player_ptr					clone()
	{
		player_ptr ret(new tiger_player);
		ret->uid_ = uid_;
		ret->name_							=			name_;
		ret->head_ico_					=			head_ico_;
		ret->sex_								=			sex_;
		ret->iid_								=			iid_;
		ret->the_game_					=			the_game_;
		ret->is_bot_						=			is_bot_;
		ret->is_win_						=			is_win_;
		ret->bets_							=			bets_;
		ret->credits_						=			credits_;
		ret->pos_								=			pos_;
		ret->join_count_				=			join_count_;
		ret->login_time_				=			login_time_;
		ret->is_connection_lost_=			is_connection_lost_;
		ret->lv_								=     lv_;
		return ret;
	}

	int						update();
	void					reset_temp_data()
	{
		is_win_ = -1;
		bets_.clear();
	}
	boost::shared_ptr<tiger_logic> get_game()
	{
		return the_game_.lock();
	}

	void					on_connection_lost();
};
