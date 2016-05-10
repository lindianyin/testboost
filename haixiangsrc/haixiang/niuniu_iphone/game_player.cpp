#include "game_player.h"
#include "service.h"
#include "msg_server.h"
#include "game_logic.h"
extern longlong total_win_;
int	niuniu_player::update()
{
	return ERROR_SUCCESS_0;
}

void niuniu_player::on_connection_lost()
{
	is_connection_lost_ = true;
	string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
	the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);

	conn_lost_tick_ = boost::posix_time::microsec_clock::local_time();
}

void niuniu_player::win_bet( longlong v, player_ptr banker )
{
	if (v < 0) return;
	actual_win_ += v;

	logic_ptr plogic = the_game_.lock();
	if (plogic.get()){
		if (plogic->is_match_game_)	{
			free_guarantee_ += v;
		}
		else{
			guarantee_ += v;
		}
	}
}

void niuniu_player::lose_bet( longlong v, player_ptr banker )
{
	if (v < 0) return;
	actual_win_ -= v;

	logic_ptr plogic = the_game_.lock();
	if (plogic.get()){
		if (plogic->is_match_game_)	{
			free_guarantee_ -= v;
		}
		else{
			guarantee_ -= v;
			if (guarantee_ < 0){
				guarantee_ = 0;
			}
			the_service.cache_helper_.cost_var(uid_ + KEY_CUR_TRADE_CAP, -1, v);
		}
	}
}

int niuniu_player::guarantee( longlong v, bool take_all, bool sync_to_client)
{
	if (v < 0) return 5;
	longlong out_count = 0;
	logic_ptr plogic = the_game_.lock();

	if (!plogic.get()) return 5;
	if (plogic->is_match_game_){
		if (take_all){
			free_guarantee_ = free_gold_;
			free_gold_ = 0;
		}
		else{
			if (v <= free_gold_){
				free_guarantee_ += v;
				free_gold_ -= v;
			}
			else{
				return -1;
			}
		}
	}
	else{
		int ret = the_service.cache_helper_.apply_cost(uid_, v, out_count, take_all, sync_to_client);
		if(ret == ERROR_SUCCESS_0){
			if (take_all){
				guarantee_ += out_count;
			}
			else
				guarantee_ += v;
			
		}
		else{
			return ret;
		}
	}
	return ERROR_SUCCESS_0;
}

bool niuniu_player::has_card( niuniu_card& c )
{
	for (unsigned int i = 0; i < turn_cards_.size(); i++)
	{
		if (turn_cards_[i].card_id_ == c.card_id_){
			return true;
		}
	}
	return false;
}

