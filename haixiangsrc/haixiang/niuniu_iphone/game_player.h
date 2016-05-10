#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
//#include "game_service_base.h"
#include "msg.h"
#include "poker_card.h"
#include "player_base.h"

using namespace std;
class niuniu_logic;
class niuniu_player;
typedef long long longlong;
typedef boost::shared_ptr<niuniu_player> player_ptr;

class niuniu_player : public player_base<niuniu_player, remote_socket<niuniu_player>, niuniu_logic>
{
public:
	int						join_count_;
	int						login_time_;
	int						not_set_bet_count_;
	longlong			guarantee_;
	longlong			free_gold_;
	longlong			free_guarantee_;

	//游戏临时数据
	vector<niuniu_card>		turn_cards_;
	boost::weak_ptr<niuniu_logic> the_game_;
	zero_match_result		match_result_;
	int						is_win_;
	longlong			bet_;
	longlong			actual_win_;
	bool					is_copy_;
	int					agree_join_;

	niuniu_player()
	{
		is_win_ = -1;
		is_connection_lost_ = false;
		credits_ = 0;
		actual_win_ = 0;
		guarantee_ = 0;
		pos_ = -1;
		bet_ = 0;
		not_set_bet_count_ = 0;
		is_bot_ = 0;
		login_time_ = 0;
		join_count_ = 0;
		iid_ = 0;
		lv_ = 0;
		is_copy_ = false;
		agree_join_ = 0;
		free_guarantee_ = 0;
	}

	player_ptr					clone()
	{
		player_ptr ret(new niuniu_player);
		ret->uid_ = uid_;
		ret->name_							=			name_;
		ret->head_ico_					=			head_ico_;
		ret->sex_								=			sex_;
		ret->iid_								=			iid_;
		ret->turn_cards_				=			turn_cards_;
		ret->the_game_					=			the_game_;
		ret->is_bot_						=			is_bot_;
		ret->not_set_bet_count_	=			not_set_bet_count_;
		ret->match_result_			=			match_result_;
		ret->is_win_						=			is_win_;
		ret->bet_								=			bet_;
		ret->guarantee_					=			guarantee_;
		ret->actual_win_				=			actual_win_;
		ret->credits_						=			credits_;
		ret->pos_								=			pos_;
		ret->join_count_				=			join_count_;
		ret->login_time_				=			login_time_;
		ret->is_connection_lost_=			is_connection_lost_;
		ret->lv_								=     lv_;
		ret->is_copy_ = true;
		ret->free_guarantee_		=			free_guarantee_;
		ret->free_gold_					=     free_gold_;
		ret->platform_id_ = platform_id_;
		ret->platform_uid_ = platform_uid_;
		return ret;
	}

	int						update();
	void					reset_temp_data()
	{
		turn_cards_.clear();
		is_win_ = -1;
		match_result_ = zero_match_result();
		bet_ = 0;
		actual_win_ = 0;
	}
	boost::shared_ptr<niuniu_logic> get_game()
	{
		return the_game_.lock();
	}
	void					win_bet(longlong v, player_ptr banker);
	void					lose_bet(longlong v, player_ptr banker);
	int						guarantee(longlong v, bool take_all = false, bool sync_to_client = true);
	void					on_connection_lost();
	bool					has_card(niuniu_card& c);
};
