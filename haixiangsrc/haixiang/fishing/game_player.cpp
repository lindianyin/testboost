#include "game_player.h"
#include "service.h"
#include "msg_server.h"
#include "game_logic.h"

int	fish_player::update()
{
	return ERROR_SUCCESS_0;
}

void fish_player::on_connection_lost()
{
	is_connection_lost_ = true;
	conn_lost_tick_ = boost::posix_time::microsec_clock::local_time();
	{
		std::string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
		the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);
		logic_ptr plogic = the_game_.lock();
		if (plogic.get()){
			plogic->restore_account(shared_from_this());
		}
	}
	if (!is_bot_){
		BEGIN_INSERT_TABLE("log_player_win");
		SET_FIELD_VALUE("uid", uid_);
		SET_FIELD_VALUE("uname", name_);
		SET_FIELD_VALUE("iid_", iid_);
		SET_FIELD_VALUE("win", total_win_);
		SET_FINAL_VALUE("pay", total_pay_);
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		total_pay_ = total_win_ = 0;
	}
}

extern int get_level(longlong& exp);
extern int glb_lvset[50];
void fish_player::add_exp(int cnt)
{
	longlong exp = the_service.cache_helper_.add_var(uid_ + KEY_EXP, IPHONE_FINSHING, cnt);
	int lv = get_level(exp);

	msg_levelup msg;
	msg.pos_ = pos_;
	msg.lv_ = lv;	
	msg.exp_max_ = glb_lvset[lv > 29 ? 29 : lv];
	msg.exp_ = exp;
	logic_ptr plogic = the_game_.lock();
	if (plogic.get()){
		plogic->broadcast_msg(msg);
	}
	if (lv_ != lv){
		BEGIN_REPLACE_TABLE("rank");
		SET_FIELD_VALUE("uid", uid_);
		SET_FIELD_VALUE("uname", name_);
		SET_FIELD_VALUE("rank_type", 1);
		SET_FINAL_VALUE("rank_data", lv);
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
	}
	lv_ = lv;
}

void fish_player::reset_temp_data()
{
	last_fire_ = boost::posix_time::ptime();
	pos_ = -1;
	is_quiting_ = false;
	the_game_.reset();
}

void fish_player::add_coin(__int64 count)
{
	coins_ += count;
}
