#include "service.h"
#include "msg_server.h"
#include "Query.h"
#include "game_logic.h"
#include "msg_client.h"
#include "game_service_base.hpp"
#include "platform_7158.h"
#include "platform_17173.h"
#include "service_config_base.hpp"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/time.hpp>
#include <boost/date_time/time_zone_base.hpp>

beauty_beast_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;

extern int glb_everyday_set[7];
extern int glb_lvset[50];

int beauty_beast_service::on_run()
{
	the_config_.refresh();
	the_config_ = the_new_config_;
	the_config_.shut_down_ = 0;
	wait_for_config_update_ = false;
	platform_config_7158& tcnf = platform_config_7158::get_instance();
	//删除残留白名单数据
	string sql = "delete from white_names";
	Query q(*gamedb_);
	q.execute(sql);
	
	//删除残留在线用户数据
	sql = "delete from log_online_players";
	q.execute(sql);
	q.free_result();

	//还原因断线扣掉庄家的钱
	vector<string> ids;
	vector<longlong> deposits;
	vector<string> game_ids;
	sql = "select uid, deposit, game_id from banker_deposit where state = 0";
	if (q.get_result(sql)){
		while (q.fetch_row()){
			ids.push_back(q.getstr());
			deposits.push_back(q.getbigint());
			game_ids.push_back(q.getstr());
		}
	}
	q.free_result();

	for (int i = 0; i < ids.size(); i++)
	{
		longlong out_count = 0;

		sql = "update banker_deposit set state = 1 where uid = '" + ids[i] + "' and game_id = '" + game_ids[i] + "'";
		q.execute(sql);
		the_service.cache_helper_.give_currency(ids[i], deposits[i], out_count);
		sql = "update banker_deposit set state = 2 where uid = '" + ids[i] + "' and game_id = '" + game_ids[i] + "'";
		q.execute(sql);
	}
	q.free_result();
	
	sql = "update server_parameters set `shut_down` = 0";
	q.execute(sql);
	q.free_result();

#ifdef _WHEEL
	logic_ptr pl;
	if(the_golden_games_.empty())
	{
		pl.reset(new game_logic_type(0));
		the_golden_games_.push_back(pl);
	}

	//查询房间参数
	sql = "select `is_free`, `bet_cap`, `gurantee_set`,"
		" `to_banker_set`, `player_tax`, `max_win`, `max_lose`"
		" from server_parameters_scene_set";
	if (q.get_result(sql))
	{
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
		the_golden_games_[0]->scene_set_ = &scenes_[0];
	}
	q.free_result();
#endif



	//添加机器人
	sql = "select uid, uname, sex, head_pic, credits from sysbanker_template";
	q.get_result(sql);
	while (q.fetch_row()){
		player_ptr pbot(new beauty_player());
		pbot->is_bot_ = true;
		pbot->uid_ = q.getstr();
		pbot->name_ = q.getstr();
		pbot->sex_ = q.getval();
		pbot->head_ico_ = q.getstr();
		longlong c = the_service.cache_helper_.get_currency(pbot->uid_);
		if (c == 0){
			the_service.cache_helper_.give_currency(pbot->uid_, rand_r((unsigned int)the_service.the_config_.banker_deposit_, (unsigned int)3 * the_service.the_config_.banker_deposit_), c);
		}
		//机器人
		the_service.bots_.insert(std::make_pair(pbot->uid_, pbot));
	}
	glb_http_log.output_.fname_ = "http_log.log";
	//启动游戏逻辑
	the_golden_games_[0]->start_logic();

	//启动周长任务
	if(the_service.the_config_.weeklyTask_week != -1)
	{
		//获得当前星期
		boost::posix_time::ptime epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));
		boost::posix_time::ptime now_time = boost::posix_time::second_clock::local_time();
		date now_day = now_time.date();
		int _week = now_day.day_of_week();
		boost::posix_time::time_duration time_from_epoch =  now_time - epoch;
		int64_t cur_t = time_from_epoch.total_seconds() - 28800;
		
		time_duration td(now_time.time_of_day().hours(), now_time.time_of_day().minutes(), now_time.time_of_day().seconds());  
		time_duration td2 = duration_from_string(the_service.the_config_.weeklyTask_time);

		int64_t _s1 = td.total_seconds();
		int64_t _s2 = td2.total_seconds();

		int surplus_time = _s2 - _s1;
		int surplus_day = 0;
		//当前天数时间已经不满一天，不计算成一天
		if(_week < the_service.the_config_.weeklyTask_week)
		{
			surplus_day = the_service.the_config_.weeklyTask_week - _week - 1;
		}
		else if(_week > the_service.the_config_.weeklyTask_week)
		{
			surplus_day = 6 - _week + the_service.the_config_.weeklyTask_week;
		}else
		{
			//如果已经成负数，说明已经过期，要到下一周期才开始
			if(surplus_time < 0)
				surplus_day = 6;
		}

		int need_second;
		if(surplus_time < 0)
		{
			need_second = 86400 + surplus_time + surplus_day * 86400;
		}else
		{
			if(_week != the_service.the_config_.weeklyTask_week)surplus_day++;
			need_second = surplus_time + surplus_day * 86400;
		}

		t.expires_from_now(boost::posix_time::milliseconds(need_second * 1000));
		t.async_wait(boost::bind(&beauty_beast_service::run_scheduled_task, this, boost::asio::placeholders::error));  
	}

	//启动服务器开启计划任务
	if(the_service.the_config_.scheduledTaskTime != "")
	{
		//返回当前时间
		boost::posix_time::ptime epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));
		boost::posix_time::time_duration time_from_epoch = boost::posix_time::second_clock::local_time() - epoch;
		int64_t _t = time_from_epoch.total_seconds() - 28800;
		//获得指定的毫秒
		boost::posix_time::ptime spe_time = time_from_string(the_service.the_config_.scheduledTaskTime);
		boost::posix_time::time_duration spe_time_epoch = spe_time - epoch;
		int64_t _t2 = spe_time_epoch.total_seconds() - 28800;

		//剩余时长
		int surplus_time = _t2 - _t;
		
		if(surplus_time > 0 && surplus_time < 2592000)
		{	
			//定时器开启
			t.expires_from_now(boost::posix_time::milliseconds(surplus_time * 1000));
			t.async_wait(boost::bind(&beauty_beast_service::run_scheduled_task, this, boost::asio::placeholders::error));  
		}else{
			//立即开启
			boost::system::error_code error;
			this->run_scheduled_task(error);
		}
	}
	return ERROR_SUCCESS_0;
}

void beauty_beast_service::run_scheduled_task(const boost::system::error_code&)
{
	std::string periodTime = the_service.the_config_.period_time;
	if(periodTime != ""){
		std::vector<std::string> v;
		split_str(periodTime.c_str(), periodTime.length(), "|", v, false);
		if(v.size() == 4){
			int d = boost::lexical_cast<int>(v[0]);
			int h = boost::lexical_cast<int>(v[1]);
			int m = boost::lexical_cast<int>(v[2]);
			int s = boost::lexical_cast<int>(v[3]);

			if(d != 0 || h != 0 || m != 0 || s != 0)
			{
				boost::shared_ptr<task_clear_riches> task;
				task.reset(new task_clear_riches(timer_sv_));
				date_info _date_info;
				_date_info.d = d;
				_date_info.h = h;
				_date_info.m = m;
				_date_info.s = s;
				task->schedule_at_next_days(_date_info);
			}
		}
	}
}



beauty_beast_service::msg_ptr beauty_beast_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_url_user_login_req2);
	REGISTER_CLS_CREATE(msg_platform_login_req);
	REGISTER_CLS_CREATE(msg_cancel_banker_req);
	REGISTER_CLS_CREATE(msg_apply_banker_req);
	REGISTER_CLS_CREATE(msg_set_bets_req);
	REGISTER_CLS_CREATE(msg_7158_user_login_req);
	REGISTER_CLS_CREATE(msg_get_player_count);
	REGISTER_CLS_CREATE(msg_get_lottery_record);
	REGISTER_CLS_CREATE(msg_account_login_req);
	REGISTER_CLS_CREATE(msg_set_account);
	REGISTER_CLS_CREATE(msg_change_pwd_req);
	REGISTER_CLS_CREATE(msg_get_banker_ranking);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_get_prize_req);
	END_MSG_CREATE()
		return ret_ptr;
}

int beauty_beast_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if (pmsg){
		pmsg->from_sock_ = from_sock;
		return pmsg->handle_this();
	}
	else
		return SYS_ERR_CANT_FIND_CONNECTION;
}

void beauty_beast_service::player_login( player_ptr pp )
{
#ifndef _WHEEL
	msg_enter_game_req msg;
	msg.from_sock_ = pp->from_socket_.lock();
	msg.room_id_ = 0;
	msg.handle_this();
#endif
}

beauty_beast_service::beauty_beast_service():
	game_service_base<service_config, beauty_player, remote_socket<beauty_player>, logic_ptr, no_match, scene_set>(the_service.the_config_.game_id)
	,t(timer_sv_)
{
	logic_ptr pl(new beatuy_beast_logic(0));
#ifndef _WHEEL
	scene_set sc;
	sc.id_ = 1;
	sc.player_tax_ = 0;
	sc.to_banker_set_ = 0;
	sc.max_win_ = 99999999999;
	sc.max_lose_ = sc.max_win_;
	sc.is_free_ = 0;
	sc.bet_cap_ = 99999999;
	sc.gurantee_set_ = 0;
	the_golden_games_.push_back(pl);
	scenes_.push_back(sc);
	wait_for_config_update_ = false;
	pl->scene_set_ = &scenes_[0];
#endif

	riches.reset(new preset_riches());
}

int beauty_beast_service::get_free_player_slot( logic_ptr exclude )
{
	int ret = 0;
	for (int i = 0; i < the_golden_games_.size(); i++){
		if (the_golden_games_[i] == exclude) continue;
		ret += int(the_config_.player_caps_ - the_golden_games_[i]->players_.size());
	}
	return ret;
}

void beauty_beast_service::on_main_thread_update()
{
}

void beauty_beast_service::on_sub_thread_update()
{

}

vector<preset_present> beauty_beast_service::get_present_bysetbet(int setbet)
{
	vector<preset_present> v;
	auto itm = the_config_.presets_.begin();
	while (itm != the_config_.presets_.end())
	{
		if (itm->second.prior_ == setbet)
		{
			v.push_back(itm->second);
		}
		itm++;
	}
	return v;
}

// void client_connector::close( close_by why )
// {
// 	if(!this->is_connected()) return;
// 	flex_orient_connector::close(why);
// 	player_ptr pp = get_player<beauty_player>();
// 	if(pp.get()){
// 		Database& db(*the_service.gamedb_);
// 		{
// 			BEGIN_INSERT_TABLE("log_player_login");
// 			SET_STRFIELD_VALUE("uid", pp->uid_);
// 			SET_STRFIELD_VALUE("login_server", "");
// 			SET_STRFIELD_VALUE("uname", pp->name_);
// 			SET_FINAL_VALUE("is_logout", 1);
// 			EXECUTE_IMMEDIATE();
// 		}
// 		{
// 			string sql = "delete from log_online_players where uid = '" + pp->uid_ + "'";
// 			Query q(db);
// 			q.execute(sql);
// 		}
// 	}
// }

bool    service_config::is_pid(int pid)
{
		auto piter = presets_.find(pid);

		if(piter != presets_.end())
		{
			return true;
		}
		else
		{
			auto iter = groups_.find(pid);

			if(iter != groups_.end())
				return true;
		}
			
		return false;
 }

 int  service_config::get_max_rate_pid_by_prior(int prior)
 {
	 auto iter = presets_.begin();
	 int  ifactor_ = 0;
	 int  pid   = 0;
	 
	 for(;iter != presets_.end(); ++iter)
	 {
		 preset_present tpr = iter->second;
		if(tpr.prior_ == prior)
		{
			if(tpr.factor_ > ifactor_)
			{
				pid = tpr.pid_;
				ifactor_ = tpr.factor_;
			}
		}
	 }
	 return pid;
 }

 int service_config::get_min_rate_pid_by_prior(int prior)
 {
	 auto iter = presets_.begin();
	 int  ifactor_ = 100000;
	 int  pid   = 0;

	 for(;iter != presets_.end(); ++iter)
	 {
		 preset_present tpr = iter->second;
		 if(tpr.prior_ == prior)
		 {
			 if(tpr.factor_ < ifactor_)
			 {
				 pid = tpr.pid_;
				 ifactor_ = tpr.factor_;
			 }
		 }
	 }
	 return pid;
 }



 int service_config::load_from_file( std::string path )
 {
	 xml_doc config_doc;
	 if(config_doc.parse_from_file("config.xml"))
	 {
		 xml_node *_config;
		 READ_CONFIG_DATA(config_doc, "context/config", _config);
		 READ_CONFIG_VALUE(_config, game_id);
		 READ_CONFIG_VALUE(_config, game_name);
		 READ_CONFIG_VALUE(_config, db_port_);
		 READ_CONFIG_VALUE(_config, db_host_);
		 READ_CONFIG_VALUE(_config, db_user_);
		 READ_CONFIG_VALUE(_config, db_pwd_);
		 READ_CONFIG_VALUE(_config, db_name_);
		 READ_CONFIG_VALUE(_config, client_port_);
		 READ_CONFIG_VALUE(_config, php_sign_key_);
		 READ_CONFIG_VALUE(_config, accdb_port_);
		 READ_CONFIG_VALUE(_config, accdb_host_);
		 READ_CONFIG_VALUE(_config, accdb_user_);
		 READ_CONFIG_VALUE(_config, accdb_pwd_);
		 READ_CONFIG_VALUE(_config, accdb_name_);
		 READ_CONFIG_VALUE(_config, cache_ip_);
		 READ_CONFIG_VALUE(_config, cache_port_);
		 READ_CONFIG_VALUE(_config, have_banker_);
		 READ_CONFIG_VALUE(_config, register_account_reward_);

		 READ_CONFIG_VALUE(_config, weeklyTask_week);
		 READ_CONFIG_VALUE(_config, weeklyTask_time);
		 READ_CONFIG_VALUE(_config, scheduledTaskTime);
		 READ_CONFIG_VALUE(_config, period_time);
		 READ_CONFIG_VALUE(_config, world_ip_);
		 READ_CONFIG_VALUE(_config, world_port_);
		 READ_CONFIG_VALUE(_config, world_id_);

		 glb_log.write_log("(Server_Config)Game_ID:%d", game_id);
		 glb_log.write_log("(Server_Config)Game_NAME:%s", game_name.c_str());
		 glb_log.write_log("(Server_Config)client_port:%d", client_port_);
		 glb_log.write_log("(Server_Config)db_host:%s", db_host_.c_str());
		 glb_log.write_log("(Server_Config)db_port:%d", db_port_);
		 glb_log.write_log("(Server_Config)cache_ip:%s", cache_ip_.c_str());
		 glb_log.write_log("(Server_Config)cache_port:%d", cache_port_);
		 glb_log.write_log("(Server_Config)world_ip:%s", world_ip_.c_str());
		 glb_log.write_log("(Server_Config)world_port:%d", world_port_);
		 glb_log.write_log("(Server_Config)world_id:%d", world_id_);

		 xml_node *platform_7158_config;
		 READ_CONFIG_DATA(config_doc, "context/platform/config_7158", platform_7158_config);
		 READ_CONFIG_VALUE_2(platform_7158_config, "game_id", platform_config_7158::get_instance().game_id_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "host_", platform_config_7158::get_instance().host_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "port_", platform_config_7158::get_instance().port_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "skey_", platform_config_7158::get_instance().skey_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "verify_param_", platform_config_7158::get_instance().verify_param_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "trade_param_", platform_config_7158::get_instance().trade_param_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "query_param_", platform_config_7158::get_instance().query_param_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "broadcast_param_", platform_config_7158::get_instance().broadcast_param_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "rate_", platform_config_7158::get_instance().rate_);
		 READ_CONFIG_VALUE_2(platform_7158_config, "broadcast_skey_", platform_config_7158::get_instance().broadcast_skey_);

		 glb_log.write_log("(Server_Config)7158_gameID:%s", platform_config_7158::get_instance().game_id_.c_str());
		 glb_log.write_log("(Server_Config)7158_host:%s", platform_config_7158::get_instance().host_.c_str());
		 glb_log.write_log("(Server_Config)7158_port:%d", platform_config_7158::get_instance().port_);

		 xml_node *platform_17173_config;
		 READ_CONFIG_DATA(config_doc, "context/platform/config_17173", platform_17173_config);
		 READ_CONFIG_VALUE_2(platform_17173_config, "game_id", platform_config_173::get_instance().game_id_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "host_", platform_config_173::get_instance().host_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "port_", platform_config_173::get_instance().port_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "skey_", platform_config_173::get_instance().skey_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "trade_param_", platform_config_173::get_instance().trade_param_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "broadcast_param_", platform_config_173::get_instance().broadcast_param_);

		 READ_CONFIG_VALUE_2(platform_17173_config, "appid_", platform_config_173::get_instance().appid_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "broadcast_appid_", platform_config_173::get_instance().broadcast_appid_);
		 READ_CONFIG_VALUE_2(platform_17173_config, "broadcast_key_", platform_config_173::get_instance().broadcast_key_);

		 glb_log.write_log("(Server_Config)173_gameID:%s", platform_config_173::get_instance().game_id_.c_str());
		 glb_log.write_log("(Server_Config)173_host:%s", platform_config_173::get_instance().host_.c_str());
		 glb_log.write_log("(Server_Config)173_port:%d", platform_config_173::get_instance().port_);

		 xml_node *platform_t58_config;
		 READ_CONFIG_DATA(config_doc, "context/platform/config_t58", platform_t58_config);
		 READ_CONFIG_VALUE_2(platform_t58_config, "fromid_", platform_config_t5b::get_instance().fromid_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "game_id_", platform_config_t5b::get_instance().game_id_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "host_", platform_config_t5b::get_instance().host_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "port_", platform_config_t5b::get_instance().port_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "skey_", platform_config_t5b::get_instance().skey_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "verify_param_", platform_config_t5b::get_instance().verify_param_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "trade_param_", platform_config_t5b::get_instance().trade_param_);
		 READ_CONFIG_VALUE_2(platform_t58_config, "rate_", platform_config_t5b::get_instance().rate_);

	 }else{
		 restore_default();
		 save_default();
	 }

	 {
		 //加载喇叭配置
		 xml_doc broadcast_doc;
		 if(broadcast_doc.parse_from_file("broadcast.xml"))
		 {
			 broadcast_telfee.clear();
			 xml_node *_config;
			 READ_CONFIG_DATA(broadcast_doc, "config", _config);
			 for (int i = 0; i < (int)_config->child_lst.size(); i++)
			 {
				 xml_node& pn = _config->child_lst[i];
				 broadcast_telfee.insert(std::map<std::string, std::string>::value_type(pn.name_, pn.get_value<std::string>()));
			 }
		 }
	 }
	 return ERROR_SUCCESS_0;
 }

 void  service_config::refresh()
{
	the_service.the_new_config_ = the_service.the_config_;
	string sql = "select `pid`, `cost` from chip_set";
	Query q(*the_service.gamedb_);
	if(q.get_result(sql)){
		the_service.the_new_config_.chips_.clear();
		while (q.fetch_row())
		{
			int pid = q.getval();
			int cost = q.getval();
			the_service.the_new_config_.chips_.insert(make_pair(pid,  cost));
		}
	}
	q.free_result();

	sql =	"SELECT	`wait_start_time`,`do_random_time`,"
		"`wait_end_time`,`banker_deposit`,	`player_caps`,"
		"`max_banker_turn`,	`sys_banker_deposit`, "
		"`sys_banker_deposit_max`, `benefit_banker`, `sysbanker_name`,"
		"`sysbanker_tax`,`min_banker_turn`,`max_win`, `max_lose`,`bet_cap`,"
		"`player_tax`, `enable_sysbanker`, `player_banker_tax`, `benefit_sysbanker`, `set_bet_cap`,"
		"`win_broadcast`, `shut_down`, `smart_mode`, `bot_enable_bet`,`bot_enable_banker`,"
		"`broadcase_gold`"
		" FROM `server_parameters`";
	if (q.get_result(sql)){
		if (q.fetch_row())
		{
			the_service.the_new_config_.wait_start_time_ = q.getval();
			the_service.the_new_config_.do_random_time_ = q.getval();
			the_service.the_new_config_.wait_end_time_ = q.getval();
			the_service.the_new_config_.banker_deposit_ = q.getbigint();
			the_service.the_new_config_.player_caps_ = q.getval();
			the_service.the_new_config_.max_banker_turn_ = q.getval();
			the_service.the_new_config_.sys_banker_deposit_ = q.getbigint();
			the_service.the_new_config_.sys_banker_deposit_max_ = q.getbigint();
			the_service.the_new_config_.benefit_banker_ = q.getval();
			the_service.the_new_config_.sysbanker_name_ = q.getstr();
			the_service.the_new_config_.sys_banker_tax_ = q.getval();
			the_service.the_new_config_.min_banker_turn_ = q.getval();
			the_service.the_new_config_.max_win_ = q.getbigint();
			the_service.the_new_config_.max_lose_ = q.getbigint();
			the_service.the_new_config_.bet_cap_ = q.getbigint();
			the_service.the_new_config_.player_tax2_ = q.getval();
			the_service.the_new_config_.enable_sysbanker_ = q.getval();
			the_service.the_new_config_.player_banker_tax_ = q.getval();
			the_service.the_new_config_.benefit_sysbanker_ = q.getval();
			the_service.the_new_config_.set_bet_tax_ = q.getval();
			the_service.the_new_config_.broadcast_set_ = q.getbigint();
			the_service.the_new_config_.shut_down_ = q.getval();
			the_service.the_new_config_.smart_mode_ = q.getval();
			the_service.the_new_config_.bot_enable_bet_ = q.getval();
			the_service.the_new_config_.bot_enable_banker_ = q.getval();
			the_service.the_new_config_.broadcase_gold = q.getval();
			//the_service.the_new_config_.player_count_random_range_ = q.getval();
			//if(the_service.the_new_config_.player_count_random_range_  == 0 )
			//	the_service.the_new_config_.player_count_random_range_ = 1000;
		}
	}
	q.free_result();


	sql = "select `bound`, `tax_percent`, `is_banker_tax` from tax_set";
	if (q.get_result(sql)){
		the_service.the_new_config_.banker_tax_.clear();
		the_service.the_new_config_.player_tax_.clear();
		while (q.fetch_row())
		{
			tax_info tax;
			tax.bound_ = q.getbigint();
			tax.tax_percent_ = q.getval();
			//如果是闲家抽税
			if (q.getval() == 0){
				the_service.the_new_config_.player_tax_.push_back(tax);
			}
			else{
				the_service.the_new_config_.banker_tax_.push_back(tax);
			}
		}
	}
	q.free_result();

	sql = "select `pid`, `rate`, `factor`, `prior`, `name` from presets";
	if (q.get_result(sql)){
		the_service.the_new_config_.presets_.clear();
		the_service.the_new_config_.groups_.clear();
		while (q.fetch_row())
		{
			preset_present pr;
			pr.pid_ = q.getval();
			pr.rate_ = q.getval();
			pr.factor_ = q.getnum();
			std::string ids = q.getstr();
			if (pr.rate_ < 0){
				group_set gs;
				gs.pid_ = pr.pid_;
				gs.factor_ = pr.factor_;
				split_str<int>(ids.c_str(), ids.length(), ",", gs.group_by_, false);
				the_service.the_new_config_.groups_.insert(make_pair(pr.pid_, gs));
			}
			else{
				pr.prior_ = boost::lexical_cast<int>(ids.c_str());
				the_service.the_new_config_.presets_.insert(make_pair(pr.pid_, pr));
			}
		}
	}
	q.free_result();
	
	sql = "select uid from black_names";
	if (q.get_result(sql)){
		the_service.the_new_config_.black_names_.clear();
		while (q.fetch_row())
		{
			string str_uid = q.getstr();
			the_service.the_new_config_.black_names_.insert(make_pair(str_uid, str_uid));
		}
	}
	q.free_result();

	sql = "select `min_gain`, `max_gain`, `ratio` from tax_turn";
	if(q.get_result(sql))
	{
		the_service.the_new_config_.tax_turn_.clear();
		while (q.fetch_row())
		{
			tax_turn t_tax;
			t_tax.min_gain = q.getbigint();
			t_tax.max_gain = q.getbigint();
			t_tax.ratio = q.getval();
			the_service.the_new_config_.tax_turn_.push_back(t_tax);
		}
	}
	q.free_result();

	service_config_base::refresh();

	if (need_update_config(the_service.the_new_config_)){
		the_service.wait_for_config_update_ = true;
	}
}

bool service_config::need_update_config( service_config& new_config )
{
	if (wait_start_time_ != new_config.wait_start_time_) return true;
	if (do_random_time_ != new_config.do_random_time_) return true;
	if (wait_end_time_ != new_config.wait_end_time_) return true;
	if (banker_deposit_ != new_config.banker_deposit_) return true;
	if (max_banker_turn_ != new_config.max_banker_turn_) return true;
	if (min_banker_turn_ != new_config.min_banker_turn_) return true;
	if (sys_banker_deposit_ != new_config.sys_banker_deposit_) return true;
	if (sys_banker_deposit_max_ != new_config.sys_banker_deposit_max_) return true;
	if (benefit_banker_ != new_config.benefit_banker_) return true;
	if (sysbanker_name_ != new_config.sysbanker_name_) return true;
	if (sys_banker_tax_ != new_config.sys_banker_tax_) return true;
	if (bet_cap_ != new_config.bet_cap_) return true;
	if (player_tax2_ != new_config.player_tax2_) return true;
	if (enable_sysbanker_ != new_config.enable_sysbanker_) return true;
	if (player_banker_tax_ != new_config.player_banker_tax_) return true;
	if (benefit_sysbanker_ != new_config.benefit_sysbanker_) return true;
	if (broadcast_set_ != new_config.broadcast_set_) return true;
	if (set_bet_tax_ != new_config.set_bet_tax_) return true;
	if (shut_down_ != new_config.shut_down_) return true;
	if (smart_mode_ != new_config.smart_mode_) return true;
	if (bot_enable_bet_ != new_config.bot_enable_bet_) return true;
	if (bot_enable_banker_ != new_config.bot_enable_banker_) return true;
	if (presets_.size() != new_config.presets_.size()) return true;
	if (chips_ != new_config.chips_) return true;
	if (new_config.black_names_ != black_names_) return true;
	if (new_config.broadcase_gold != broadcase_gold) return true;
	auto itf = presets_.begin();
	while (itf != presets_.end())
	{
		if (!(itf->second == new_config.presets_[itf->first])) return true;
		itf++;
	}

	auto itg = groups_.begin();
	while (itg != groups_.end()){
		auto itgnew = new_config.groups_.find(itg->first);
		if (itgnew == new_config.groups_.end()){
			itg++;
		}
		if (!(itg->second == itgnew->second)){
			return true;
		}
		itg++;
	}

	for ( int i = 0; i < banker_tax_.size(); i++)
	{
		if ((banker_tax_[i].bound_ != new_config.banker_tax_[i].bound_) ||
			(banker_tax_[i].tax_percent_ != new_config.banker_tax_[i].tax_percent_)) return true;
	}

	for ( int i = 0; i < player_tax_.size(); i++)
	{
		if ((player_tax_[i].bound_ != new_config.player_tax_[i].bound_) ||
			(player_tax_[i].tax_percent_ != new_config.player_tax_[i].tax_percent_)) return true;
	}
	
	if (new_config.personal_rate_control_.size() != personal_rate_control_.size()){
		return true;
	}
	return false;
}

bool service_config::check_valid()
{
	if (presets_.size() <= 0)	{
		glb_log.write_log("present not set!");
		return false;       
	}

	if (sys_banker_deposit_ > sys_banker_deposit_max_){
		glb_log.write_log("sys_banker_deposit_ can not > sys_banker_deposit_max_ !");
		return false;
	}
	return true;
}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}
