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

fishing_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
extern fish_ptr glb_boss;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;

vector<std::string> g_bot_names;
extern std::map<std::string, longlong> g_todaywin;
extern bool		get_fish_config(std::string path, std::map<std::string, fish_config_ptr>& the_fish_config);
extern bool		get_path_config(std::string path, std::map<std::string, path_config>& the_path, std::map<std::string, fish_config_ptr>& the_fish_config);
extern bool		get_cloud_config(std::string path, std::map<std::string, cloud_config>& the_cloud_config);
extern longlong			total_pay_;

class wealth_op : public cache_async_callback
{
public:
	bool is_complete_;
	virtual void	routine(boost::shared_ptr<msg_cache_cmd_ret<mem_connector_ptr>> msg_ret)
	{
		the_service.glb_goldrank_.push_back(msg_ret);
		if (msg_ret->str_value_[1] == 0) {
			is_complete_ = true;
		}
	}
	virtual bool	is_complete() {
		return is_complete_;
	}

	wealth_op()
	{
		is_complete_ = false;
	}
};

class sync_wealth_task : public task_info
{
public:
	sync_wealth_task(boost::asio::io_service& ios):task_info(ios)
	{

	}
	int		routine()
	{
		the_service.glb_goldrank_.clear();
		async_optr op = async_optr(new(wealth_op));
		the_service.cache_helper_.async_cmd(KEY_GETRANK, GOLD_RANK, 0, op);
		if (the_service.add_stock(0) < the_service.the_config_.alert_email_set_){
			Database& db = *the_service.email_db_;
			BEGIN_INSERT_TABLE("alert_emails");
			SET_FIELD_VALUE("fromgame", "fishing");
			SET_FIELD_VALUE("alert_type", 1);
			SET_FINAL_VALUE("content", "warning: system win " + boost::lexical_cast<std::string>(the_service.add_stock(0)));
			EXECUTE_IMMEDIATE();
		}
		return task_info::routine_ret_continue;
	}
};

bool		get_scene_config()
{
	xml_doc doc;
	if (!doc.parse_from_file("scene_config.xml"))return false;
	xml_node* pn = doc.get_node("config");
	for (int i = 0 ; i < (int)pn->child_lst.size(); i++)
	{
		xml_node& fish_node = pn->child_lst[i];
		scene_set cnf;
		cnf.id_ = fish_node.get_value<std::string>("iid", "");
		cnf.gurantee_set_ = fish_node.get_value("lowcap", 0);
		cnf.rate_ = fish_node.get_value("exchange", 0);
		cnf.is_free_ = 0;

		if (cnf.id_ == "1"){
			get_fish_config("fish/fish_config_normal.xml", cnf.the_fish_config_);
			get_cloud_config("fish/cloud_config.xml", cnf.the_cloud_config_);
		}
		else if (cnf.id_ == "10"){
			get_fish_config("fish/fish_config_telfee_1_5_10.xml", cnf.the_fish_config_);
			get_cloud_config("fish/cloud_config.xml", cnf.the_cloud_config_);
			cnf.is_free_ = 1;
			cnf.gurantee_set_ = 0;
		}
		else if (cnf.id_ == "50"){
			get_fish_config("fish/fish_config_normal.xml", cnf.the_fish_config_);
			get_cloud_config("fish/cloud_config.xml", cnf.the_cloud_config_);
		}
		the_service.scenes_.push_back(cnf);
	}
	return true;
}

bool		get_cloud_config(std::string path, std::map<std::string, cloud_config>& the_cloud_config)
{
	xml_doc doc;
	if (!doc.parse_from_file(path.c_str()))return false;
	xml_node* pn = doc.get_node("config");
	for (int i = 0 ; i < (int)pn->child_lst.size(); i++)
	{
		xml_node& fish_node = pn->child_lst[i];
		cloud_config cnf;
		cnf.type_ = fish_node.get_value<std::string>("name", "");
		cnf.rate_ = fish_node.get_value("rate", 0);
		the_cloud_config.insert(std::make_pair(cnf.type_, cnf));
	}
	return true;
}


extern std::map<std::string, int> g_online_getted_;
extern longlong			g_total_pay_;
class everyday_reset_task : public task_info
{
public:
	everyday_reset_task(io_service& io) : task_info(io)
	{

	}
	int			routine()
	{
		g_online_getted_.clear();
		g_total_pay_ = 0;

		return routine_ret_continue;
	}
};

int		match_prize[5][7] = {0};
extern	int glb_giftset[50];
extern	int glb_lvset[50];
extern	int glb_everyday_set[7];
extern  longlong glb_costs[10];

bool restore_all_coins_success = true;
void		restore_unsaved_coins(std::string uid, longlong& coins)
{
	longlong out_count = 0;
	int ret = the_service.cache_helper_.give_currency(uid, coins, out_count, false);
	if (ret == 0){
		coins = 0;
	}
	else
		restore_all_coins_success = false;
}

int fishing_service::on_run()
{
// 	backup_user_coins_.open_shared_object();
// 	backup_user_coins_.visit(restore_unsaved_coins);
// 	if (restore_all_coins_success){
// 		backup_user_coins_.clean();
// 	}

	//更新配置
	the_config_.refresh();
	the_config_ = the_new_config_;
	wait_for_config_update_ = false;
	
	everyday_reset_task* ptask = new everyday_reset_task(the_service.timer_sv_);
	task_ptr task(ptask);
	date_info di; di.d = 1;
	ptask->schedule_at_next_days(di);

	{
		xml_doc doc;
		if(!doc.parse_from_file("online.xml")) return -1;
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
		if(!doc.parse_from_file("broadcast.xml")) return -1;
		xml_node* root = doc.get_node("config/gold");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			xml_node& pn = root->child_lst[i];
			the_config_.broadcast_gold_.push_back(pn.get_value<std::string>());
		}

		root = doc.get_node("config/telfee");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			xml_node& pn = root->child_lst[i];
			the_config_.broadcast_telfee_.push_back(pn.get_value<std::string>());
		}

		root = doc.get_node("config/boss");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			xml_node& pn = root->child_lst[i];
			the_config_.broadcast_boss_.push_back(pn.get_value<std::string>());
		}

		root = doc.get_node("config/bomb");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			xml_node& pn = root->child_lst[i];
			the_config_.broadcast_bomb_.push_back(pn.get_value<std::string>());
		}
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("telfee.xml")) return -1;
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
		if(!doc.parse_from_file("levelset.xml")) return -1;
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size(); i += 2)
		{
			glb_giftset[i / 2] = root->child_lst[i].get_value<int>();
			glb_lvset[i / 2] = root->child_lst[i + 1].get_value<int>();
		}
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("everyday.xml")) return -1;
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size(); i++)
		{
			glb_everyday_set[i] = root->child_lst[i].get_value<int>();
		}
	}

	{
		xml_doc doc;
		if(!doc.parse_from_file("fire_set.xml")) return -1;
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size() && i < 10; i++)
		{
			glb_costs[i] = root->child_lst[i].get_value<int>();
		}
	}

	Query q(*gamedb_);

	string sql = "delete from log_online_players";
	q.execute(sql);
	q.free_result();
	
	sql = "update server_parameters set `shut_down` = 0";
	q.execute(sql);


	sql = "select `key`, `value`,	`cur_type` from player_curreny_cache";
	q.get_result(sql);
	while (q.fetch_row())
	{
		std::string uid = q.getstr();
		longlong v = q.getbigint();
		int tp = q.getval();
		int r = cache_helper_.give_currency(uid, v, v, false);
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
		ptask->schedule(boost::posix_time::seconds(3600).total_milliseconds());
	}

	get_fish_config("fish/fish_boss.xml", the_config_.the_fish_config_);
//	get_path_config("fish/path_boss.xml", the_config_.the_path_, the_config_.the_fish_config_);
	//生成boss鱼的游走路径
	for (int i = 0 ; i < 4; i++)
	{
		path_config pcf;
		pcf.type_ = "boss_path" + boost::lexical_cast<std::string>(i);
		pcf.rate_ = 2500;

		float r = 200;
		Point center(1600 / 2.0, 900 / 2.0);
		float angle = 0;
		for (int i = 0; i <= 40; i++)
		{
			float x = r * cos((angle + i * 9.0) / 180.0 * M_PI);
			float y = r * sin((angle + i * 9.0) / 180.0 * M_PI);
			pcf.path_.push_back(center + Point(x, y));
		}
		the_config_.the_path_.insert(std::make_pair(pcf.type_, pcf));
	}
		
	{
		task_for_clean_fish_for_boss_spwan* spn = new task_for_clean_fish_for_boss_spwan(timer_sv_);
		task_ptr task(spn);
		boost::posix_time::minutes mins(30);//test:30
		spn->schedule(mins.total_milliseconds());
	}

	if (!get_scene_config()) return -1;
	
	for (int i = 0; i < 20; i++)
	{
		logic_ptr logic(new fishing_logic(0));
		logic->scene_set_ = &scenes_[0];
		logic->start_logic();
		the_golden_games_.push_back(logic);
	}

	create_match(1, 0);

// 	pmc.reset(new telfee_match_info());
// 	pmc->iid_ = 10;
// 	pmc->time_type_ = 0;
// 	pmc->register_time_ = 60 * 60000;	//60分钟
// 	pmc->enter_time_ = 60000;
// 	pmc->matching_time_ = 10 * 60000;
// 	pmc->prizes_.insert(pmc->prizes_.begin(), match_prize[2], (match_prize[2] + 7));
// 	get_fish_config("fish/fish_config_telfee_1_5_10.xml", pmc->scene_set_.the_fish_config_);
// 	get_path_config("fish/path_config.xml", pmc->scene_set_.the_path_, pmc->scene_set_.the_fish_config_);
// 	get_cloud_config("fish/cloud_config.xml", pmc->scene_set_.the_cloud_config_);

//	the_matches_.insert(std::make_pair(pmc->iid_, pmc));

// 	pmc.reset(new telfee_match_info());
// 	pmc->iid_ = 50;
// 	pmc->time_type_ = 1;
// 
// 	{
// 		date_info dat;
// 		dat.d = 1; dat.h = 11; dat.m = 30;
// 		pmc->vsche_.push_back(dat);
// 
// 		dat.d = 1; dat.h = 12; dat.m = 30;
// 		pmc->vsche_.push_back(dat);
// 
// 		dat.d = 1; dat.h = 13; dat.m = 30;
// 		pmc->vsche_.push_back(dat);
// 
// 		dat.d = 1; dat.h = 20; dat.m = 30;
// 		pmc->vsche_.push_back(dat);
// 
// 		dat.d = 1; dat.h = 21; dat.m = 30;
// 		pmc->vsche_.push_back(dat);
// 
// 		dat.d = 1; dat.h = 22; dat.m = 30;
// 		pmc->vsche_.push_back(dat);
// 	}
// 
// 	pmc->enter_time_ = 60000;
// 	pmc->matching_time_ = 20 * 60000;
// 	pmc->prizes_.insert(pmc->prizes_.begin(), match_prize[3], (match_prize[3] + 7));
// 	get_fish_config("fish/fish_config_telfee_50_100.xml", pmc->scene_set_.the_fish_config_);
// 	get_path_config("fish/path_config.xml", pmc->scene_set_.the_path_, pmc->scene_set_.the_fish_config_);
// 	get_cloud_config("fish/cloud_config.xml", pmc->scene_set_.the_cloud_config_);
// 	the_matches_.insert(std::make_pair(pmc->iid_, pmc));

// 	pmc.reset(new telfee_match_info());
// 	pmc->iid_ = 100;
// 	pmc->time_type_ = 1;
// 	{
// 		date_info dat;
// 		dat.d = 1; dat.h = 12;
// 		pmc->vsche_.push_back(dat);
// 
// 		dat.d = 1; dat.h = 22;
// 		pmc->vsche_.push_back(dat);
// 	}
// 
// 	pmc->enter_time_ = 60000;
// 	pmc->matching_time_ = 20 * 60000;
// 	pmc->prizes_.insert(pmc->prizes_.begin(), match_prize[4], (match_prize[4] + 7));
// 	get_fish_config("fish/fish_config_telfee_50_100.xml", pmc->scene_set_.the_fish_config_);
// 	get_path_config("fish/path_config.xml", pmc->scene_set_.the_path_, pmc->scene_set_.the_fish_config_);
// 	get_cloud_config("fish/cloud_config.xml", pmc->scene_set_.the_cloud_config_);
// 	the_matches_.insert(std::make_pair(pmc->iid_, pmc));

	auto itm = the_matches_.begin();
	while (itm != the_matches_.end())
	{
		itm->second->enter_wait_register_state();
		itm++;
	}

	glb_http_log.output_.fname_ = "http_log.log";

	return ERROR_SUCCESS_0;
}

fishing_service::fishing_service():
	game_service_base<service_config, fish_player, remote_socket<fish_player>, logic_ptr, match_ptr, scene_set>(IPHONE_FINSHING)
{
	control_rate_map_.clear();
}

void fishing_service::on_main_thread_update()
{
	static boost::posix_time::ptime  gpt = boost::posix_time::microsec_clock::local_time();
	//更新配置
	boost::posix_time::ptime  pt1 = boost::posix_time::microsec_clock::local_time();
	float dt = (pt1 - gpt).total_milliseconds() / 1000.0;
	if (dt < 0.01) return;
	
	vector<logic_ptr> cpl = the_golden_games_;
	for (int i = 0 ; i < (int)cpl.size(); i++)
	{
		logic_ptr plogic = cpl[i];
		int r = plogic->update(dt);
		if (r == 1){
			auto itf = std::find(the_golden_games_.begin(), the_golden_games_.end(), plogic);
			if (itf != the_golden_games_.end()){
				the_golden_games_.erase(itf);
			}
		}
	}

	if(glb_boss.get() && glb_boss->update(dt) == 1){
		if (glb_boss->die_enable_ == 2){

			restore_random_spwan();

			msg_fish_clean msg;
			msg.iid_ = glb_boss->iid_;

			for (int i = 0; i < (int) the_golden_games_.size(); i++)
			{
				if (the_golden_games_[i]->is_match_game_) continue;

				the_golden_games_[i]->fishes_.clear();
				the_golden_games_[i]->broadcast_msg(msg);
			}

			glb_boss.reset();
		}
		else{
			glb_boss->path_ = glb_boss->path_backup_;
			update_boss_path(glb_boss->path_backup_);
		}
	}

	gpt = pt1;

	auto it_bot = free_bots_.begin();
	while (it_bot != free_bots_.end())
	{
		player_ptr p = it_bot->second;
		if (p->last_msg_.is_not_a_date_time()){
			it_bot++;
			continue;
		}
		//如果10秒没发消息过来,这个机器人挂掉了
		if ((pt1 - p->last_msg_).total_seconds() > 10){
			auto conn = p->from_socket_.lock();
			if (conn.get()){
				conn->close();
			}
			if(!p->is_connection_lost_){
				p->on_connection_lost();
				it_bot = free_bots_.erase(it_bot);
			}
			else{
				it_bot++;
			}
		}
		else
			it_bot++;
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
	SET_FIELD_VALUE("key", uid);
	SET_FIELD_VALUE("cur_type", is_free);
	SET_FINAL_VALUE("value", val);
	the_service.delay_db_game_.push_sql_exe(uid + "cur_cache", -1, str_field, true);
}

int fishing_service::on_stop()
{
	beauty_hosts_.clear();
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

player_ptr fishing_service::pop_onebot(logic_ptr plogic)
{
	if (!free_bots_.empty()){
		int r = rand_r(free_bots_.size() - 1);
		auto itf = free_bots_.begin();
		for (int i = 1 ; i <= r; i++)
		{
			itf++;
		}

		player_ptr pp = itf->second;
		free_bots_.erase(itf);
		return pp;
	}
	else{
		return player_ptr();
	}
}

fishing_service::msg_ptr fishing_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_ping_req);
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_game_lst_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_talk_req);
	REGISTER_CLS_CREATE(msg_get_prize_req);
	REGISTER_CLS_CREATE(msg_leave_room);
	REGISTER_CLS_CREATE(msg_get_rank_req);
	REGISTER_CLS_CREATE(msg_set_account);
	REGISTER_CLS_CREATE(msg_trade_gold_req);
	REGISTER_CLS_CREATE(msg_user_info_changed);
	REGISTER_CLS_CREATE(msg_account_login_req);
	REGISTER_CLS_CREATE(msg_change_pwd_req);
	REGISTER_CLS_CREATE(msg_get_recommend_reward);
	REGISTER_CLS_CREATE(msg_get_trade_to_lst);
	REGISTER_CLS_CREATE(msg_get_trade_from_lst);
	REGISTER_CLS_CREATE(msg_get_recommend_lst);

	REGISTER_CLS_CREATE(msg_fire_at_req);
	REGISTER_CLS_CREATE(msg_gun_switch_req);
	REGISTER_CLS_CREATE(msg_hit_fish_req);
	REGISTER_CLS_CREATE(msg_query_match_req);
	REGISTER_CLS_CREATE(msg_register_match_req);
	REGISTER_CLS_CREATE(msg_agree_to_join_match);
	REGISTER_CLS_CREATE(msg_get_match_report);
	REGISTER_CLS_CREATE(msg_prepare_enter_complete);
	REGISTER_CLS_CREATE(msg_7158_user_login_req);
	REGISTER_CLS_CREATE(msg_buy_coin_req);
	REGISTER_CLS_CREATE(msg_get_server_info);
	REGISTER_CLS_CREATE(msg_lock_fish_req);
	REGISTER_CLS_CREATE(msg_enter_host_game_req);
	REGISTER_CLS_CREATE(msg_get_host_list);
	REGISTER_CLS_CREATE(msg_upload_screenshoot);
	REGISTER_CLS_CREATE(msg_send_present_req);
	REGISTER_CLS_CREATE(msg_device_id);
	REGISTER_CLS_CREATE(msg_version_verify);
	REGISTER_CLS_CREATE(msg_host_start_req);
	REGISTER_CLS_CREATE(msg_platform_login_req);
	REGISTER_CLS_CREATE(msg_enter_match_game_req);
	REGISTER_CLS_CREATE(msg_platform_account_login);
	REGISTER_CLS_CREATE(msg_auto_tradein_req);
	END_MSG_CREATE()
	return ret_ptr;
}

void	proccess_gm_command(string cmd)
{
	if (cmd.find_first_of("/kickout ") == 0){
		std::string kick_uid = cmd.c_str() + strlen("/kickout ");
		player_ptr pp = the_service.get_player(kick_uid);
		if (pp.get()){
			auto cnn = pp->from_socket_.lock();
			if (cnn.get()){
				cnn->close();
			}
		}
	}
}

int fishing_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if (pmsg){
		pmsg->from_sock_ = from_sock;
		player_ptr pp = from_sock->the_client_.lock();
		if (pp.get()){
			pp->last_msg_ = boost::posix_time::microsec_clock::local_time();
		}

		if (pmsg->head_.cmd_ == GET_CLSID(msg_talk_req)){
			msg_talk_req* talk = (msg_talk_req*)pmsg;
			string content = talk->content_;
			if (content.find_first_of("/") == 0 &&
				gm_sets_.find(pp->uid_) != gm_sets_.end()){
				proccess_gm_command(content);
				return ERROR_SUCCESS_0;
			}
			else{
				return pmsg->handle_this();
			}
		}
		else{
			return pmsg->handle_this();
		}
	}
	else
		return SYS_ERR_CANT_FIND_CONNECTION;
}

void fishing_service::update_fish_path(fish_ptr fh, std::list<Point>& path, logic_ptr logic)
 {
	if (!path.empty()){
		fh->path_.clear();
		fh->path_ = path;
		fh->move_to_next();
	}

	msg_update_path msg;
	msg.speed_ = fh->acc_;
	msg.iid_ = fh->iid_;
	msg.x = fh->pos_.x;
	msg.y = fh->pos_.y;
	msg.path_count_ = fh->path_.size();
	for (auto pt : fh->path_){
		msg.path_.push_back(pt.x);
		msg.path_.push_back(pt.y);
	}
	if (!logic.get()){
		for (int i = 0 ; i < (int)the_service.the_golden_games_.size(); i++)
		{
			logic_ptr plogic = the_service.the_golden_games_[i];
			if (plogic->is_match_game_) continue;
			plogic->broadcast_msg(msg);
		}
	}
	else{
		logic->broadcast_msg(msg);
	}
}
void fishing_service::update_boss_path(std::list<Point>& path)
{
	update_fish_path(glb_boss, path, logic_ptr());
}

void fishing_service::boss_runout()
{
	if (glb_boss.get()){
		glb_boss->die_enable_ = 2;

		glb_boss->pmov_->speed_up(16.0);
		glb_boss->acc_ = 16.0;

		std::list<Point> path;
		path.push_back(glb_boss->pos_);
		path.push_back(Point(1800, 450));
		update_boss_path(path);
	}
}

void fishing_service::restore_random_spwan()
{
	for (int i = 0; i < (int)the_service.the_golden_games_.size(); i++)
	{
		logic_ptr plogic = the_service.the_golden_games_[i];
		//task_for_clean_fish_for_cloud_spawn * psch = new task_for_clean_fish_for_cloud_spawn(the_service.timer_sv_);
		task_for_clean_fish_for_random_spawn* psch = new task_for_clean_fish_for_random_spawn(the_service.timer_sv_);
		task_ptr ptask(psch);
		psch->logic_ = plogic;
		psch->schedule(1000);//2分钟

		plogic->the_cloud_generator_.reset();

		if (plogic->current_spwan_.get()){
			plogic->current_spwan_->cancel();
		}
		plogic->current_spwan_ = ptask;
	}
}

void fishing_service::on_beauty_host_login(player_ptr pp)
{
}

match_ptr fishing_service::create_match(int tpid, int insid)
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
		get_fish_config("fish/fish_config_telfee_1_5_10.xml", pmc->scene_set_.the_fish_config_);
		get_path_config("fish/path_config.xml", pmc->scene_set_.the_path_, pmc->scene_set_.the_fish_config_);
		get_cloud_config("fish/cloud_config.xml", pmc->scene_set_.the_cloud_config_);

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
		get_fish_config("fish/fish_config_telfee_1_5_10.xml", pmc->scene_set_.the_fish_config_);
		get_path_config("fish/path_config.xml", pmc->scene_set_.the_path_, pmc->scene_set_.the_fish_config_);
		get_cloud_config("fish/cloud_config.xml", pmc->scene_set_.the_cloud_config_);

		the_matches_.insert(std::make_pair(pmc->iid_, pmc));
		return pmc;
	}
	return match_ptr();
}

void service_config::refresh()
{
	the_service.the_new_config_ = the_service.the_config_;
	service_config_base::refresh();

	string sql;
	Query q(*the_service.gamedb_);
	sql =	"SELECT	"
		"`shut_down`,`enable_bot`,"
		"`max_trade_cap`, `trade_tax`, rate_control,"
		"min_lottery, max_lottery, lottery_rate, inventory "
		" FROM `server_parameters`";

	if (q.get_result(sql)){
		if (q.fetch_row()){
			the_service.the_new_config_.shut_down_ = q.getval();
			the_service.the_new_config_.enable_bot_ = q.getval();
			the_service.the_new_config_.max_trade_cap_ = q.getbigint();
			the_service.the_new_config_.trade_tax_ = q.getval();
			the_service.the_new_config_.rate_control_ = q.getnum();
			the_service.the_new_config_.lottery_min_ = q.getbigint();
			the_service.the_new_config_.lottery_max_ = q.getbigint();
			the_service.the_new_config_.lottery_rate_ = q.getnum();
			the_service.the_new_config_.inventory_    = q.getbigint();
		}
	}
	q.free_result();

	sql = "select uid, limitation from server_parameters_gmset";
	q.get_result(sql);
	the_service.gm_sets_.clear();
	while (q.fetch_row())
	{
		gm_set gs;
		gs.uid_ = q.getstr();
		gs.limitation_ = q.getval();
		the_service.gm_sets_.insert(std::make_pair(gs.uid_, gs));
	}
	q.free_result();

	if (need_update_config(the_service.the_new_config_)){
		the_service.wait_for_config_update_ = true;
	}
	the_service.the_config_.shut_down_ = the_service.the_new_config_.shut_down_;
}

bool service_config::need_update_config( service_config& new_config )
{
	if(inventory_     != new_config.inventory_)  return true;

	//不正常数值
	if (new_config.rate_control_ > 10.0 || new_config.rate_control_ < 0.0){
		new_config.rate_control_ = 10.0;
	}
	//不正常数值。
	if (new_config.lottery_rate_ > 0.9 || new_config.lottery_rate_ < 0.0){
		new_config.lottery_rate_ = 0.3;
	}

	return true;
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
	rate_control_ = 0.9;
	lottery_max_ = 500000;
	lottery_min_ = 50000;
	lottery_rate_ = 0.3;
	rmb_rate_ = 10000;
	inventory_ = 0;          //库存
 
}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}
