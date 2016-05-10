#include "service.h"
#include "msg_server.h"
#include "msg_client.h"
#include "Query.h"
#include "game_logic.h"
#include "game_config.h"
#include "msg_client_common.h"

jinghua_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;

vector<std::string> g_bot_names;
int jinghua_service::on_run()
{
	the_config_.refresh();
	the_config_ = the_new_config_;
	the_config_.shut_down_ = 0;
	wait_for_config_update_ = false;
	
	the_service.scenes_.clear();

	Query q(*gamedb_);

	string sql = "select `is_free`, `bet_cap`, `gurantee_set`,"
		" `to_banker_set`, `player_tax`, `max_win`, `max_lose`"
		" from server_parameters_scene_set";

	if (q.get_result(sql)){
		while (q.fetch_row())
		{
			scene_set sc;
			sc.id_ = scenes_.size() + 1;
			sc.is_free_ = q.getval();
			sc.bet_cap_ = q.getbigint();
			sc.gurantee_set_ = q.getbigint();
			sc.to_banker_set_ = q.getbigint();
			sc.player_tax_ = q.getval();
			sc.max_win_ = q.getbigint();
			sc.max_lose_ = q.getbigint();

			scenes_.push_back(sc);
		}
	}
	q.free_result();

	//删除残留在线用户数据
	sql = "delete from log_online_players";
	q.execute(sql);
	q.free_result();
	
	sql = "update server_parameters set `shut_down` = 0";
	q.execute(sql);


	sql = "select `key`, `value` from player_curreny_cache";
	q.get_result(sql);
	while (q.fetch_row())
	{
		string uid = q.getstr();
		longlong v = q.getbigint();
		int r = give_currency(uid, v, v, 0);
	}
	q.free_result();

	sql = "delete from player_curreny_cache";
	q.execute(sql);
	return ERROR_SUCCESS_0;
}

void jinghua_service::player_login( player_ptr pp )
{

}

jinghua_service::jinghua_service():
	game_service_base<service_config, jinghua_player, remote_socket<jinghua_player>, logic_ptr, no_match_cls, scene_set>(GAME_ID)
{
	wait_for_config_update_ = false;
}

void jinghua_service::on_main_thread_update()
{
}

void jinghua_service::on_sub_thread_update()
{
}

void jinghua_service::save_currency_to_local( string uid, longlong val )
{
	if (the_service.the_config_.is_free_){
		uid += "_FREE";
	}
	Database& db = *gamedb_;
	BEGIN_REPLACE_TABLE("player_curreny_cache");
	SET_STRFIELD_VALUE("key", uid);
	SET_FINAL_VALUE("value", val);
	EXECUTE_IMMEDIATE();
}


int jinghua_service::on_stop()
{
	return ERROR_SUCCESS_0;
}

void jinghua_service::delete_from_local( string uid )
{
	if (the_service.the_config_.is_free_){
		uid += "_FREE";
	}
	string sql = "delete from player_curreny_cache where `key` = '" + uid + "'";
	Query q(*gamedb_);
	q.execute(sql);
}



logic_ptr jinghua_service::get_game( string uid )
{
	for (int i = 0 ; i < the_golden_games_.size(); i++)
	{
		for (int j = 0; j < MAX_SEATS; j++)
		{
			if (!the_golden_games_[i]->is_playing_[j].get()) continue;
			if(the_golden_games_[i]->is_playing_[j]->uid_ == uid){
				return the_golden_games_[i];
			}
		}
	}
	return logic_ptr();
}

jinghua_service::msg_ptr jinghua_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_game_lst_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_bet_operator);
	REGISTER_CLS_CREATE(msg_talk_req);
	REGISTER_CLS_CREATE(msg_want_sit_req);
	REGISTER_CLS_CREATE(msg_ready_req);
	REGISTER_CLS_CREATE(msg_see_card_req);
	END_MSG_CREATE()
		return ret_ptr;
}

int jinghua_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if (pmsg){
		pmsg->from_sock_ = from_sock;
		return pmsg->handle_this();
	}
	else
		return SYS_ERR_CANT_FIND_CONNECTION;
}

//void client_connector::close( close_by why )
//{
//	if(!this->is_connected()) return;
//	flex_orient_connector::close(why);
//	player_ptr pp = get_player<niuniu_player>();
//	if(pp.get()){
//		Database& db(*the_service.gamedb_);
//		{
//			BEGIN_INSERT_TABLE("log_player_login");
//			SET_STRFIELD_VALUE("uid", pp->uid_);
//			SET_STRFIELD_VALUE("login_server", "");
//			SET_STRFIELD_VALUE("uname", pp->name_);
//			SET_FINAL_VALUE("is_logout", 1);
//			EXECUTE_IMMEDIATE();
//		}
//		{
//			string sql = "delete from log_online_players where uid = '" + pp->uid_ + "'";
//			Query q(db);
//			q.execute(sql);
//		}
//	}
//}

void service_config::refresh()
{
	the_service.the_new_config_ = the_service.the_config_;
	string sql;
	Query q(*the_service.gamedb_);
	sql =	"SELECT	`wait_start_time`,`do_random_time`,"
		"`wait_end_time`,	`player_caps`,"
		"`max_win`, `max_lose`,`bet_cap`,"
		"`win_broadcast`, `shut_down`,`player_tax`, `currency_require`, `lowbet_cap`, `total_betcap`"
		" FROM `server_parameters`";
	if (q.get_result(sql)){
		if (q.fetch_row())
		{
			the_service.the_new_config_.wait_start_time_ = q.getval();
			the_service.the_new_config_.do_random_time_ = q.getval();
			the_service.the_new_config_.wait_end_time_ = q.getval();
			the_service.the_new_config_.player_caps_ = q.getval();
			the_service.the_new_config_.max_win_ = q.getbigint();
			the_service.the_new_config_.max_lose_ = q.getbigint();
			the_service.the_new_config_.player_bet_cap_ = q.getbigint();
			the_service.the_new_config_.broadcast_set_ = q.getbigint();
			the_service.the_new_config_.shut_down_ = q.getval();
			the_service.the_new_config_.player_tax_ = q.getval();
			the_service.the_new_config_.currency_require_ = q.getbigint();
			the_service.the_new_config_.player_bet_lowcap_ = q.getbigint();
			the_service.the_new_config_.total_bet_cap_ = q.getbigint();
		}
	}
	q.free_result();

	if (need_update_config(the_service.the_new_config_)){
		the_service.wait_for_config_update_ = true;
	}
}

bool service_config::need_update_config( service_config& new_config )
{
	if (wait_start_time_ != new_config.wait_start_time_) return true;
	if (do_random_time_ != new_config.do_random_time_) return true;
	if (wait_end_time_ != new_config.wait_end_time_) return true;
	if (broadcast_set_ != new_config.broadcast_set_) return true;
	if (shut_down_ != new_config.shut_down_) return true;
	if (max_win_ != new_config.max_win_) return true;
	if (max_lose_ != new_config.max_lose_) return true;
	if (player_tax_ != new_config.player_tax_) return true;
	if (currency_require_ != new_config.currency_require_) return true;
	if (player_bet_cap_ != new_config.player_bet_cap_) return true;
	if (player_bet_lowcap_ != new_config.player_bet_lowcap_) return true;
	if (total_bet_cap_ != new_config.total_bet_cap_) return true;
	return false;
}

bool service_config::check_valid()
{
	return true;
}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}
