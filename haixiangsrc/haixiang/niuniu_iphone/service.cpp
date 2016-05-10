#include "service.h"
#include "msg_server.h"
#include "Query.h"
#include "game_logic.h"
#include "schedule_task.h"
#include "msg_client.h"
#include "game_logic.h"
#include <unordered_map>
#include "telfee_match.h"
#include <math.h>
#include "game_service_base.hpp"
#include "logic_base.h"
#include "service_config_base.hpp"

niuniu_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;

extern std::map<std::string, niuniu_score> g_scores;
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
		int count = 0;
		return task_info::routine_ret_continue;
	}
};

class reset_scores_task : public task_info
{
public:
	reset_scores_task(io_service& io) : task_info(io)
	{

	}
	int			routine()
	{
		g_scores.clear();
		g_todaywin.clear();
		return routine_ret_continue;
	}
};

int		match_prize[5][7] = {0};
extern	int glb_giftset[50];
extern	int glb_lvset[50];
extern	int glb_everyday_set[7];

int niuniu_service::on_run()
{
	the_config_.refresh();
	the_config_ = the_new_config_;
	wait_for_config_update_ = false;
	
	the_service.scenes_.clear();

	{
		reset_scores_task* ptast = new reset_scores_task(timer_sv_);
		task_ptr task(ptast);
		date_info dat; dat.d = 1;
		task->schedule_at_next_days(dat);
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("online.xml"))
		{
			BOOST_ASSERT(false);
			return -1;
		}
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			xml_node& pn = root->child_lst[i];
			online_config oc;
			oc.time_ = pn.get_value("time", 0);
			oc.prize_ = pn.get_value("reward", 0);
			the_config_.online_prize_.push_back(oc);
		}
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("telfee.xml")) {
			BOOST_ASSERT(false);
			return -1;
		}
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < 5; i++)
		{
			xml_node& pn = root->child_lst[i];
			for (int j = 0; j < 7; j++)
			{
				match_prize[i][j] = pn.get_value(boost::lexical_cast<std::string>(j + 1).c_str(), 0);
			}
		}
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("levelset.xml")) {
			BOOST_ASSERT(false);
			return -1;
		}
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size(); i += 2)
		{
			glb_giftset[i / 2] = root->child_lst[i].get_value<int>();
			glb_lvset[i / 2] = root->child_lst[i + 1].get_value<int>();
		}
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("everyday.xml")) {
			BOOST_ASSERT(false);
			return -1;
		}
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			glb_everyday_set[i] = root->child_lst[i].get_value<int>();
		}
	}


	Query q(*gamedb_);

	string sql;
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
		int r = cache_helper_.give_currency(uid, v, v, false);
	}
	q.free_result();

	sql = "delete from player_curreny_cache";
	q.execute(sql);

	FILE* fp = NULL;
	fopen_s(&fp, "bot_names.txt", "r");
	if(!fp){
		BOOST_ASSERT(false);
		return -1;
	} 
		
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
	scene_set sc;
	sc.bet_cap_ = 1000000;
	sc.gurantee_set_ = 1000;
	sc.max_lose_ = sc.max_lose_ = 9999999999;
	sc.player_tax_ = 0;
	sc.to_banker_set_ = 300000;
	sc.id_ = 10;

	return ERROR_SUCCESS_0;
}

void niuniu_service::player_login( player_ptr pp )
{

}

niuniu_service::niuniu_service():game_service_base<service_config, niuniu_player, remote_socket<niuniu_player>, logic_ptr, match_ptr, scene_set>(IPHONE_NIUINIU)
{
	wait_for_config_update_ = false;
}

void niuniu_service::on_main_thread_update()
{
	//更新配置
	bool all_game_waiting = true;
	if (wait_for_config_update_){
		for (unsigned int i = 0 ; i < the_golden_games_.size(); i++)
		{
			if (!the_golden_games_[i]->is_waiting_config_)	{
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

	/*for (auto mat : the_matches_)
	{
	mat.second->update();
	}*/

	if (the_config_.shut_down_){
		if (the_golden_games_.empty()){
			stoped_ = true;
		}
	}
}

void niuniu_service::on_sub_thread_update()
{
}

void niuniu_service::save_currency_to_local( string uid, longlong val, int is_free )
{
	if (is_free){
		uid += "_FREE";
	}
	BEGIN_REPLACE_TABLE("player_curreny_cache");
	SET_FIELD_VALUE("key", uid);
	SET_FIELD_VALUE("cur_type", is_free);
	SET_FINAL_VALUE("value", val);
	the_service.delay_db_game_.push_sql_exe(uid + "cur_cache", -1, str_field, true);
}

int niuniu_service::on_stop()
{
	for (unsigned int i = 0 ; i < the_golden_games_.size(); i++)
	{
		logic_ptr plogic = the_golden_games_[i];
		plogic->stop_logic();
	}
	return ERROR_SUCCESS_0;
}

void niuniu_service::delete_from_local( string uid, int is_free )
{
	if (is_free){
		uid += "_FREE";
	}
	string sql = "delete from player_curreny_cache where `key` = '" + uid + "'";
	Query q(*gamedb_);
	q.execute(sql);
}


int niuniu_service::restore_account( player_ptr pp, int is_free )
{
	longlong out_count = 0;
	int r = cache_helper_.give_currency(pp->uid_, pp->guarantee_, out_count, true);
	pp->guarantee_ = 0;
	pp->credits_ = out_count;
	if (r != ERROR_SUCCESS_0){
		glb_log.write_log("!!!!!!!!!!!!!!!!!!!restore_account error uid = %s, err = %d", pp->uid_.c_str(), r);
		return r;
	}
	return ERROR_SUCCESS_0;
}

player_ptr niuniu_service::pop_onebot(logic_ptr plogic)
{
	player_ptr pp;
	longlong c = 0;
	if (!pp.get() && !g_bot_names.empty()){
		pp.reset(new niuniu_player);
		pp->is_bot_ = 1;
		pp->uid_ = "ipbot_test" + lex_cast_to_str(rand_r(9999));
		pp->name_ = g_bot_names[rand_r(g_bot_names.size() - 1)];
		pp->head_ico_ = "role_" + lex_cast_to_str(1 + (rand() % 2)) + ".png";
		pp->credits_ = the_service. cache_helper_.get_currency(pp->uid_);
		if (pp->credits_ < plogic->scene_set_->to_banker_set_){
			pp->credits_ += (longlong)(plogic->scene_set_->to_banker_set_ * (rand_r(200, 1000) / 100.0));
			int r = the_service. cache_helper_.give_currency(pp->uid_, pp->credits_, c, 0);
		}
		else if (pp->credits_ > plogic->scene_set_->to_banker_set_ * 10){
			c = (longlong)(plogic->scene_set_->to_banker_set_ * (rand_r(200, 1000) / 100.0));
			int r = the_service. cache_helper_.apply_cost(pp->uid_, pp->credits_ - c, c, false);
		}
	}
	return pp;
}

niuniu_service::msg_ptr niuniu_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_set_bets_req);
	REGISTER_CLS_CREATE(msg_game_lst_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_query_to_become_banker_rsp);
	REGISTER_CLS_CREATE(msg_talk_req);
	REGISTER_CLS_CREATE(msg_want_sit_req);
	REGISTER_CLS_CREATE(msg_open_card_req);
	REGISTER_CLS_CREATE(msg_start_random_req);
	REGISTER_CLS_CREATE(msg_get_prize_req);
	REGISTER_CLS_CREATE(msg_leave_room);
	REGISTER_CLS_CREATE(msg_set_account);
	REGISTER_CLS_CREATE(msg_trade_gold_req);
	REGISTER_CLS_CREATE(msg_user_info_changed);
	REGISTER_CLS_CREATE(msg_account_login_req);
	REGISTER_CLS_CREATE(msg_platform_login_req);
	REGISTER_CLS_CREATE(msg_platform_account_login);
	REGISTER_CLS_CREATE(msg_7158_user_login_req);
	REGISTER_CLS_CREATE(msg_change_pwd_req);
	REGISTER_CLS_CREATE(msg_get_recommend_reward);
	REGISTER_CLS_CREATE(msg_get_trade_to_lst);
	REGISTER_CLS_CREATE(msg_get_trade_from_lst);
	REGISTER_CLS_CREATE(msg_get_recommend_lst);
	REGISTER_CLS_CREATE(msg_query_match_req);
	REGISTER_CLS_CREATE(msg_register_match_req);
	REGISTER_CLS_CREATE(msg_agree_to_join_match);
	REGISTER_CLS_CREATE(msg_get_server_info);
	REGISTER_CLS_CREATE(msg_get_rank_req);
	REGISTER_CLS_CREATE(msg_url_user_login_req2);
	REGISTER_CLS_CREATE(msg_misc_data_req);
	REGISTER_CLS_CREATE(msg_enter_match_game_req);
	END_MSG_CREATE()
	return ret_ptr;
}

int niuniu_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
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
	service_config_base::refresh();
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

	sql = "select `is_free`, `bet_cap`, `gurantee_set`,"
		" `to_banker_set`, `player_tax`, `max_win`, `max_lose`"
		" from server_parameters_scene_set";
	the_service.scenes_.clear();
	if (q.get_result(sql)){
		while (q.fetch_row())
		{
			scene_set sc;
			sc.id_ = the_service.scenes_.size() + 1;
			sc.is_free_ = q.getval();
			sc.bet_cap_ = q.getbigint();
			sc.gurantee_set_ = q.getbigint();
			sc.to_banker_set_ = q.getbigint();
			sc.player_tax_ = q.getval();
			sc.max_win_ = q.getbigint();
			sc.max_lose_ = q.getbigint();

			the_service.scenes_.push_back(sc);
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
	register_fee_ = 0;
}

//int service_config::load_from_file()
//{
	//service_config_base::load_from_file("niuniu.xml");
	//xml_doc doc;
	//if (!doc.parse_from_file("niuniu.xml")){

	//}
	//else{
	//	READ_XML_VALUE("config/", slave_cache_ip_	);
	//	READ_XML_VALUE("config/", slave_cache_port_	);
	//}
//	return ERROR_SUCCESS_0;
//}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}

match_ptr niuniu_service::create_match(int tpid, int insid)
{
	//内置比赛场
	if (tpid == 1){
		match_ptr pmc(new telfee_match_info());
		pmc->iid_ = tpid;
		pmc->time_type_ = 0;
		pmc->register_time_ = 10000;
		pmc->enter_time_ = 60000;
		pmc->matching_time_ = 10 * 60000;
		pmc->prizes_.insert(pmc->prizes_.begin(), match_prize[0], (match_prize[0] + 7));

		the_matches_.insert(std::make_pair(pmc->iid_, pmc));
		return pmc;
	}
	//平台比赛场
	else if (tpid == 5){
		match_ptr pmc(new telfee_match_info(0));
		//如果没有指定实例id
		if (insid <= 0){insid = tpid;}
		pmc->iid_ = insid;
		pmc->time_type_ = 0;
		pmc->register_time_ = 0;
		pmc->enter_time_ = 5000;
		pmc->matching_time_ = 5 * 60000 - 5000;
		pmc->prizes_.insert(pmc->prizes_.begin(), match_prize[0], (match_prize[0] + 7));
		pmc->scene_set_.bet_cap_ = 1000000;
		pmc->scene_set_.gurantee_set_ = 1000;
		pmc->scene_set_.max_lose_ = 9999999999;
		pmc->scene_set_.player_tax_ = 0;
		pmc->scene_set_.to_banker_set_ = 500000;
		pmc->scene_set_.id_ = 10;
		the_matches_.insert(std::make_pair(pmc->iid_, pmc));
		return pmc;
	}
	return match_ptr();
}
