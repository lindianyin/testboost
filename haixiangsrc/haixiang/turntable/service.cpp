#include "service.h"
#include "msg_server.h"
#include "Query.h"
#include "game_logic.h"
#include "msg_client.h"
#include "game_service_base.hpp"

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
	tcnf.game_id_ = "8011";

	//签到奖励
 	{
 		xml_doc doc;
 		if(!doc.parse_from_file("everyday.xml")) return -1;
 		xml_node* root = doc.get_node("config");
 		for (int i = 0; i < (int)root->child_lst.size(); i++)
 		{
 			glb_everyday_set[i] = root->child_lst[i].get_value<int>();
		}
 	}

	//等级经验
	{
		xml_doc doc;
		if(!doc.parse_from_file("levelset.xml")) return -1;
		xml_node* root = doc.get_node("config");
		for (int i = 0; i < (int)root->child_lst.size(); i += 2)
		{
			glb_lvset[i / 2] = root->child_lst[i + 1].get_value<int>();
		}
	}

	 logic_ptr pl(new beatuy_beast_logic(0));
	 the_golden_games_.push_back(pl);

	Query q(*gamedb_);

// 	string sql1 = "alter table server_parameters_scene_set add column room_multiple int not null";
// 	if (q.get_result(sql1))
// 	{
// 		q.free_result();
// 		return 0;
// 	}

	//查询房间参数
	string sql = "select `is_free`, `bet_cap`, `gurantee_set`,"
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
		pl->scene_set_ = &scenes_[0];
	}
	q.free_result();

	//删除残留白名单数据
	sql = "delete from white_names";
	//Query q(*gamedb_);
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

	return ERROR_SUCCESS_0;
}

beauty_beast_service::msg_ptr beauty_beast_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_url_user_login_req2);
	REGISTER_CLS_CREATE(msg_cancel_banker_req);
	REGISTER_CLS_CREATE(msg_apply_banker_req);
	REGISTER_CLS_CREATE(msg_set_bets_req);
	REGISTER_CLS_CREATE(msg_7158_user_login_req);
	REGISTER_CLS_CREATE(msg_get_player_count);
	REGISTER_CLS_CREATE(msg_get_lottery_record);
	REGISTER_CLS_CREATE(msg_account_login_req);
	REGISTER_CLS_CREATE(msg_set_account);
	REGISTER_CLS_CREATE(msg_change_pwd_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_get_prize_req);
	REGISTER_CLS_CREATE(msg_leave_room);
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

extern int get_level(longlong& exp);
class update_online_time : public task_info
{
public:
	boost::weak_ptr<beauty_player> the_pla_;
	update_online_time(io_service& ios) : task_info(ios)
	{

	}
	int		 routine()
	{
		DEBUG_COUNT_PERFORMANCE_BEGIN("update_online_time");
		int ret = routine_ret_continue;
		player_ptr pp = the_pla_.lock();
		if (pp.get() && !pp->is_connection_lost_){
			longlong exp = the_service.cache_helper_.add_var(pp->uid_ + KEY_ONLINE_TIME, GAME_ID, 10);
			exp = the_service.cache_helper_.add_var(pp->uid_ + KEY_EXP, GAME_ID, 10);
			int lv = get_level(exp);
			/*if (pp->lv_ != lv)*/{
				msg_levelup msg;
				msg.exp_ = (double)exp;
				extern int glb_lvset[50];
				msg.exp_max_ = glb_lvset[lv > 29 ? 29 : lv];
				msg.lv_ = lv;
				pp->send_msg_to_client(msg);
				pp->lv_ = lv;
			}
		}
		else{
			ret = routine_ret_break;
		}
		DEBUG_COUNT_PERFORMANCE_END();
		return ret;
	}
};
void beauty_beast_service::player_login( player_ptr pp )
{
	update_online_time* ptask = new update_online_time(the_service.timer_sv_);
	task_ptr task(ptask);
	ptask->the_pla_ = pp;
	ptask->schedule(60000);
// 	msg_enter_game_req msg;
// 	msg.from_sock_ = pp->from_socket_.lock();
// 	msg.room_id_ = 0;
// 	msg.handle_this();
}

beauty_beast_service::beauty_beast_service():game_service_base<service_config, beauty_player, remote_socket<beauty_player>, logic_ptr, no_match, scene_set>(GAME_ID)
{
// 	logic_ptr pl(new beatuy_beast_logic(0));
// 	scene_set sc;
// 	sc.id_ = 1;
// 	sc.player_tax_ = 0;
// 	sc.to_banker_set_ = 0;
// 	sc.max_win_ = 99999999999;
// 	sc.max_lose_ = sc.max_win_;
// 	sc.is_free_ = 0;
// 	sc.bet_cap_ = 99999999;
// 	sc.gurantee_set_ = 0;
// 	the_golden_games_.push_back(pl);
// 	scenes_.push_back(sc);
 	wait_for_config_update_ = false;
	the_config_.register_account_reward_ = 500;
// 	pl->scene_set_ = &scenes_[0];
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
		if (itm->second.prior_ == setbet){
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
		"`win_broadcast`, `shut_down`, `smart_mode`, `bot_enable_bet`,`bot_enable_banker`,`player_count_random_range`"
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
			the_service.the_new_config_.player_count_random_range_ = q.getval();
			if(the_service.the_new_config_.player_count_random_range_  == 0 )
				the_service.the_new_config_.player_count_random_range_ = 1000;
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
