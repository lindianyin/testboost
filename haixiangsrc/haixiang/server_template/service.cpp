#include "service.h"
#include "msg_server.h"
#include "Query.h"
#include "game_logic.h"
#include "schedule_task.h"
#include "msg_client.h"
#include "game_logic.h"

fishing_service  the_service;
log_file<cout_output> glb_log;
vector<std::string> g_bot_names;
extern std::map<std::string, longlong> g_todaywin;

class sync_wealth_task : public task_info
{
public:
	sync_wealth_task(boost::asio::io_service& ios):task_info(ios)
	{

	}
	int		routine()
	{
		the_service.glb_goldrank_.clear();
		boost::shared_ptr<msg_cache_cmd_ret> msg_ret(new msg_cache_cmd_ret());
		COPY_STR(msg_ret->key_, KEY_GETRANK);
		int r = the_service.account_cache_.do_send_cache_cmd(KEY_GETRANK, "CMD_GET", 0, *msg_ret, 
			the_service.account_cache_.s_, false, false);
		if (r == 0){
			r = the_service.account_cache_.get_cache_result(*msg_ret, the_service.account_cache_.s_);
			while(r == 0){

				the_service.glb_goldrank_.push_back(msg_ret);
				if (msg_ret->str_value_[1] == 0){
					break;
				}
				msg_ret.reset(new msg_cache_cmd_ret());
				COPY_STR(msg_ret->key_, KEY_GETRANK);
				r = the_service.account_cache_.get_cache_result(*msg_ret, the_service.account_cache_.s_);
			}
		}

		return task_info::routine_ret_continue;
	}
};

int fishing_service::on_run()
{
	the_config_.refresh();
	the_config_ = the_new_config_;
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


	sql = "select `key`, `value`,	`cur_type` from player_curreny_cache";
	q.get_result(sql);
	while (q.fetch_row())
	{
		string uid = q.getstr();
		longlong v = q.getbigint();
		int tp = q.getval();
		int r = give_currency(uid, v, v, false);
	}
	q.free_result();

	sql = "delete from player_curreny_cache";
	q.execute(sql);

	FILE* fp = NULL;
	fopen_s(&fp, "bot_names.txt", "r");
	if(!fp) return -1;
	vector<char> data_read;
	char c[128];
	while (!feof(fp))
	{
		int s = fread(c, sizeof(char), 128, fp);
		for (int i = 0 ; i < s; i++)
		{
			data_read.push_back(c[i]);
		}
	}
	fclose(fp);

	split_str(data_read.data() + 3, data_read.size(), "\n", g_bot_names, true);

	{
		sync_wealth_task* ptask = new sync_wealth_task(timer_sv_);
		task_ptr task(ptask);
		ptask->routine();
		ptask->schedule(12000);
	}
	return ERROR_SUCCESS_0;
}

void fishing_service::player_login( player_ptr pp )
{

}

fishing_service::fishing_service():game_service_base<service_config, fish_player, remote_socket<fish_player>>(IPHONE_NIUINIU)
{
	wait_for_config_update_ = false;
}

void				fishing_service::update_every_2sec()
{
	game_service_base<service_config, fish_player, remote_socket<fish_player>>::update_every_2sec();

	if (the_config_.shut_down_){
		glb_log.write_log("there are %d games still running!", the_games_.size());
	}
}

void fishing_service::on_main_thread_update()
{
	//更新配置
	bool all_game_waiting = true;
	if (wait_for_config_update_){
		for (unsigned int i = 0 ; i < the_games_.size(); i++)
		{
			if (!the_games_[i]->is_waiting_config_)	{
				all_game_waiting = false;
				break;
			}
		}
		if (all_game_waiting){
			the_config_ = the_new_config_;
			if (!the_config_.shut_down_)
				wait_for_config_update_ = false;
		}
	}

	the_service.account_cache_.pickup_msg(the_service.account_cache_.s_);
	the_service.account_cache_.pickup_msg(the_service.account_cache_.slave_s_);

	auto itm = the_service.account_cache_.msg_lst_.begin();
	while (itm != the_service.account_cache_.msg_lst_.end())
	{
		auto pmsg = *itm; *itm++;
		if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_kick_account)){
			msg_cache_kick_account* pacc = (msg_cache_kick_account*) pmsg.get();
			player_ptr pp = the_service.get_player(pacc->uid_);
			if (pp.get()){
				auto pconn = pp->from_socket_.lock();
				if (pconn.get()){
					msg_logout msg;
					send_msg(pconn, msg);
				}
			}
			glb_log.write_log("kick out account :%s", pacc->uid_);
		}
	}
	the_service.account_cache_.msg_lst_.clear();

	if (the_config_.shut_down_){
		if (the_games_.empty()){
			stoped_ = true;
		}
	}
}

void fishing_service::on_sub_thread_update()
{
}

void fishing_service::save_currency_to_local( string uid, longlong val, int is_free )
{
	if (is_free){
		uid += "_FREE";
	}
	BEGIN_REPLACE_TABLE("player_curreny_cache");
	SET_STRFIELD_VALUE("key", uid);
	SET_FIELD_VALUE("cur_type", is_free);
	SET_FINAL_VALUE("value", val);
	the_service.delay_db_game_.push_sql_exe(uid + "cur_cache", -1, str_field, true);
}

player_ptr fishing_service::get_player_in_game( string uid )
{
	for (unsigned int i = 0 ; i < the_games_.size(); i++)
	{
		for (int j = 0; j < MAX_SEATS; j++)
		{
			if (!the_games_[i]->is_playing_[j].get()) continue;
			if(the_games_[i]->is_playing_[j]->uid_ == uid){
				return the_games_[i]->is_playing_[j];
			}
		}
	}
	return player_ptr();
}

int fishing_service::on_stop()
{
	for (unsigned int i = 0 ; i < the_games_.size(); i++)
	{
		logic_ptr plogic = the_games_[i];
		plogic->stop_logic();
	}
	return ERROR_SUCCESS_0;
}

void fishing_service::delete_from_local( string uid, int is_free )
{
	if (is_free){
		uid += "_FREE";
	}
	string sql = "delete from player_curreny_cache where `key` = '" + uid + "'";
	Query q(*gamedb_);
	q.execute(sql);
}


int fishing_service::restore_account( player_ptr pp, int is_free )
{
	longlong out_count = 0;
	return ERROR_SUCCESS_0;
}

logic_ptr fishing_service::get_game( string uid )
{
	for (unsigned int i = 0 ; i < the_games_.size(); i++)
	{
		for (int j = 0; j < MAX_SEATS; j++)
		{
			if (!the_games_[i]->is_playing_[j].get()) continue;
			if(the_games_[i]->is_playing_[j]->uid_ == uid){
				return the_games_[i];
			}
		}
	}
	return logic_ptr();
}

player_ptr fishing_service::pop_onebot(logic_ptr plogic)
{
	player_ptr pp;
	longlong c = 0;
	if (!pp.get()){
		pp.reset(new fish_player);
		pp->is_bot_ = 1;
		pp->uid_ = "ipbot_test" + lex_cast_to_str(rand_r(9999));
		pp->name_ = g_bot_names[rand_r(g_bot_names.size() - 1)];
		pp->head_ico_ = "role_" + lex_cast_to_str(1 + (rand() % 10)) + ".png";
		pp->credits_ = the_service.get_currency(pp->uid_);
		if (pp->credits_ < plogic->scene_set_->to_banker_set_){
			pp->credits_ += (longlong)(plogic->scene_set_->to_banker_set_ * (rand_r(110, 400) / 100.0));
			int r = the_service.give_currency(pp->uid_, pp->credits_, c, 0);
		}
		else if (pp->credits_ > plogic->scene_set_->to_banker_set_ * 4){
			c = (longlong)(plogic->scene_set_->to_banker_set_ * (rand_r(110, 400) / 100.0));
			int r = the_service.apply_cost(pp->uid_, pp->credits_ - c, c, false);
		}
	}
	return pp;
}

fishing_service::msg_ptr fishing_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_game_lst_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_talk_req);
	REGISTER_CLS_CREATE(msg_get_prize_req);
	REGISTER_CLS_CREATE(msg_leave_room);
	REGISTER_CLS_CREATE(msg_get_wealth_rank_req);
	REGISTER_CLS_CREATE(msg_set_account);
	REGISTER_CLS_CREATE(msg_trade_gold_req);
	REGISTER_CLS_CREATE(msg_user_info_changed);
	REGISTER_CLS_CREATE(msg_account_login_req);
	REGISTER_CLS_CREATE(msg_change_pwd_req);
	REGISTER_CLS_CREATE(msg_get_recommend_reward);
	REGISTER_CLS_CREATE(msg_get_trade_to_lst);
	REGISTER_CLS_CREATE(msg_get_trade_from_lst);
	REGISTER_CLS_CREATE(msg_get_recommend_lst);
	END_MSG_CREATE()
	return ret_ptr;
}

int fishing_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if (pmsg){
		pmsg->from_sock_ = from_sock;
		return pmsg->handle_this();
	}
	else
		return SYS_ERR_CANT_FIND_CONNECTION;
}

void service_config::refresh()
{
	the_service.the_new_config_ = the_service.the_config_;
	string sql;
	Query q(*the_service.gamedb_);
	sql =	"SELECT	`wait_start_time`,`do_random_time`,"
		"`wait_end_time`, `shut_down`,`enable_bot`,"
		"`bot_banker_rate`, `max_trade_cap`, `trade_tax`"
		" FROM `server_parameters`";

	if (q.get_result(sql)){
		if (q.fetch_row())
		{
			the_service.the_new_config_.wait_start_time_ = q.getval();
			the_service.the_new_config_.do_random_time_ = q.getval();
			the_service.the_new_config_.wait_end_time_ = q.getval();
			the_service.the_new_config_.shut_down_ = q.getval();
			the_service.the_new_config_.enable_bot_ = q.getval();
			the_service.the_new_config_.bot_banker_rate_ = q.getval();
			the_service.the_new_config_.max_trade_cap_ = q.getbigint();
			the_service.the_new_config_.trade_tax_ = q.getval();
		}
	}
	q.free_result();

	if (need_update_config(the_service.the_new_config_)){
		the_service.wait_for_config_update_ = true;
	}
	the_service.the_config_.shut_down_ = the_service.the_new_config_.shut_down_;
}

bool service_config::need_update_config( service_config& new_config )
{
	if (wait_start_time_ != new_config.wait_start_time_) return true;
	if (do_random_time_ != new_config.do_random_time_) return true;
	if (wait_end_time_ != new_config.wait_end_time_) return true;
	if (enable_bot_ != new_config.enable_bot_) return true;
	if (bot_banker_rate_ != new_config.bot_banker_rate_) return true;
	if (max_trade_cap_ != new_config.max_trade_cap_) return true;
	if (trade_tax_ != new_config.trade_tax_) return true;
	return false;
}

bool service_config::check_valid()
{
	return true;
}

service_config::service_config()
{
	restore_default();
	shut_down_ = 0;
	enable_bot_ = 1;
	base_score_ = 10;
	bot_banker_rate_ = 10000;
}

int service_config::load_from_file()
{
	xml_doc doc;
	if (!doc.parse_from_file("niuniu.xml")){
		restore_default();
		save_default();
	}
	else{
		READ_XML_VALUE("config/", db_port_								);
		READ_XML_VALUE("config/", db_host_								);
		READ_XML_VALUE("config/", db_user_								);
		READ_XML_VALUE("config/", db_pwd_									);
		READ_XML_VALUE("config/", db_name_								);
		READ_XML_VALUE("config/", client_port_						);
		READ_XML_VALUE("config/", php_sign_key_						);
		READ_XML_VALUE("config/", accdb_port_							);
		READ_XML_VALUE("config/", accdb_host_							);
		READ_XML_VALUE("config/", accdb_user_							);
		READ_XML_VALUE("config/", accdb_pwd_							);
		READ_XML_VALUE("config/", accdb_name_							);
		READ_XML_VALUE("config/", cache_ip_								);
		READ_XML_VALUE("config/", cache_port_							);
		READ_XML_VALUE("config/", slave_cache_ip_					);
		READ_XML_VALUE("config/", slave_cache_port_				);

	}
	return ERROR_SUCCESS_0;
}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}
