#include "service.h"
#include "msg_server.h"
#include "Query.h"
#include "game_logic.h"
#include "msg_client.h"

horse_racing_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;

int horse_racing_service::on_run()
{
	the_config_.refresh();
	the_config_ = the_new_config_;
	the_config_.shut_down_ = 0;
	wait_for_config_update_ = false;
	
	Query q(*gamedb_);
	//删除残留在线用户数据
	string sql = "delete from log_online_players";
	q.execute(sql);
	q.free_result();
	
	sql = "update server_parameters set `shut_down` = 0";
	q.execute(sql);
	return ERROR_SUCCESS_0;
}

horse_racing_service::msg_ptr horse_racing_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_url_user_login_req2);
	REGISTER_CLS_CREATE(msg_set_bets_req);
	REGISTER_CLS_CREATE(msg_setbet_his_req);
	END_MSG_CREATE()
		return ret_ptr;
}

int horse_racing_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if (pmsg){
		pmsg->from_sock_ = from_sock;
		return pmsg->handle_this();
	}
	else
		return SYS_ERR_CANT_FIND_CONNECTION;
}

horse_racing_service::horse_racing_service():game_service_base<service_config, horserace_player, remote_socket<horserace_player>, logic_ptr, match_ptr, scene_set>(GAME_ID)
{
	wait_for_config_update_ = false;
	scene_set sc;
	sc.id_ = 0;
	sc.gurantee_set_ = 0;
	sc.is_free_ = 0;
	sc.to_banker_set_ = 0;
	scenes_.push_back(sc);
}

int horse_racing_service::get_free_player_slot( logic_ptr exclude )
{
	int ret = 0;
	for (int i = 0; i < the_golden_games_.size(); i++){
		if (the_golden_games_[i] == exclude) continue;
		ret += int(the_config_.player_caps_ - the_golden_games_[i]->players_.size());
	}
	return ret;
}

void horse_racing_service::on_main_thread_update()
{
	//更新配置
	bool all_game_waiting = true;
	if (wait_for_config_update_){
		for (int i = 0 ; i < the_golden_games_.size(); i++)
		{
			if (!the_golden_games_[i]->is_waiting_config_)	{
				all_game_waiting = false;
				break;
			}
		}
		if (all_game_waiting){
			the_config_ = the_new_config_;
			msg_server_parameter msg;
			msg.racing_time_ = the_config_.riding_time_;
			msg.road_length_ = the_config_.riding_road_length_;
			auto it2 = the_config_.chips_.begin();
			int i = 0;
			while (it2 != the_config_.chips_.end() && i < 5)
			{
				msg.chip_id_[i] = it2->first;
				msg.chip_cost_[i] = it2->second;
				it2++;
				i++;
			}
			broadcast_msg(msg);
			if (!the_config_.shut_down_)
				wait_for_config_update_ = false;
		}
	}

	boost::system::error_code ec;
	timer_sv_.poll_one(ec);
}

void horse_racing_service::on_sub_thread_update()
{
}

void horse_racing_service::player_login(player_ptr pp)
{
	msg_enter_game_req msg;
	msg.from_sock_ = pp->from_socket_.lock();
	msg.room_id_ = 0;
	msg.handle_this();
}


void service_config::refresh()
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
		"`wait_end_time`,	`player_caps`,"
		"`max_win`, `max_lose`,`bet_cap`,"
		"`win_broadcast`, `shut_down`, `benefit_sysbanker`"
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
			the_service.the_new_config_.bet_cap_ = q.getbigint();
			the_service.the_new_config_.broadcast_set_ = q.getbigint();
			the_service.the_new_config_.shut_down_ = q.getval();
			the_service.the_new_config_.benefit_banker_ = q.getbigint();
		}
	}
	q.free_result();

	sql = "select `pid`, `rate`, `factor`, `prior`, `name` from presets";
	if (q.get_result(sql)){
		the_service.the_new_config_.presets_.clear();
		while (q.fetch_row())
		{
			preset_present pr;
			pr.pid_ = q.getval();
			pr.db_pid_ = pr.pid_;
			pr.rate_ = q.getval();
			pr.factor_ = q.getval();
			pr.prior_ = q.getval();
			the_service.the_new_config_.presets_.insert(make_pair(pr.pid_, pr));
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
	if (benefit_banker_ != new_config.benefit_banker_) return true;
	if (bet_cap_ != new_config.bet_cap_) return true;
	if (broadcast_set_ != new_config.broadcast_set_) return true;
	if (shut_down_ != new_config.shut_down_) return true;
	if (max_win_ != new_config.max_win_) return true;
	if (max_lose_ != new_config.max_lose_) return true;

	if (presets_.size() != new_config.presets_.size()) return true;
	if (chips_.size() != new_config.chips_.size()) return true;

	auto itf = presets_.begin();
	while (itf != presets_.end())
	{
		if (!(itf->second == new_config.presets_[itf->first])) return true;
		itf++;
	}

	auto itf2 = chips_.begin();
	while (itf2 != chips_.end())
	{
		if (itf2->second != new_config.chips_[itf2->first]) return true;
		itf2++;
	}

	return false;
}

bool service_config::check_valid()
{
	if (presets_.size() <= 0)	{
		glb_log.write_log("present not set!");
		return false;       
	}
	if (chips_.empty()){
		glb_log.write_log("chip not set!");
		return false;  
	}
	return true;
}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}
