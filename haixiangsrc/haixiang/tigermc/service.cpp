#include "service.h"
#include "msg_server.h"
#include "Query.h"
#include "game_logic.h"
#include "schedule_task.h"
#include "msg_client.h"
#include "game_logic.h"
#include "platform_gameid.h"
#include "game_service_base.hpp"
tigermc_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;
vector<std::string> g_bot_names;
extern std::map<std::string, longlong> g_todaywin;


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
// 		boost::shared_ptr<msg_cache_cmd_ret<mem_connector_ptr>> msg_ret(new msg_cache_cmd_ret<mem_connector_ptr>());
// 		COPY_STR(msg_ret->key_, KEY_GETRANK);
// 
// 		//delete by liufapu
// 		int r = the_service.cache_helper_.account_cache_.do_send_cache_cmd(KEY_GETRANK, "CMD_GET", 0, msg_ret,
// 			/*the_service.cache_helper_.account_cache_.s_, false, false,*/ 0, nullptr);
// 		boost::posix_time::ptime p1 = boost::posix_time::microsec_clock::local_time();
// 		while (true){
// 			std::vector<boost::shared_ptr<msg_cache_cmd_ret<mem_connector_ptr>>> v;
// 			//delete by liufapu
// 			//r = the_service.cache_helper_.account_cache_.get_cache_result(v, the_service.cache_helper_.account_cache_.s_, KEY_GETRANK, 200);
// 			bool br = false;
// 			for (int i = 0 ; i < (int)v.size(); i++)
// 			{
// 				msg_ret = v[i];
// 				the_service.glb_goldrank_.push_back(msg_ret);
// 				if (msg_ret->str_value_[1] == 0){
// 					br = true;
// 					break;
// 				}
// 			}
// 			if (br)
// 				break;
// 			if((boost::posix_time::microsec_clock::local_time() - p1).total_milliseconds() > 200){
// 				break;
// 			}
// 		}

		async_optr op = async_optr(new(wealth_op));
		the_service.cache_helper_.async_cmd(KEY_GETRANK, GOLD_RANK, 0, op);

		return task_info::routine_ret_continue;
	}
};

extern std::map<std::string, int> g_online_getted_;
class reset_scores_task : public task_info
{
public:
	reset_scores_task(io_service& io) : task_info(io)
	{

	}
	int			routine()
	{
		g_todaywin.clear();
		g_online_getted_.clear();
		return routine_ret_break;
	}
};
int		match_prize[5][7] = {0};
extern	int glb_giftset[50];
extern	int glb_lvset[50];
extern	int glb_everyday_set[7];


int tigermc_service::on_run()
{
	the_config_.refresh();
	the_config_ = the_new_config_;
	wait_for_config_update_ = false;
	
	the_service.scenes_.clear();



	{
		reset_scores_task* ptast = new reset_scores_task(timer_sv_);
		task_ptr task(ptast);
		date_info di;
		di.d = 1;
		task->schedule_at_next_days(di);
	}
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
		ptask->schedule(12000);
	}






	return ERROR_SUCCESS_0;
}

void tigermc_service::player_login( player_ptr pp )
{

}

tigermc_service::tigermc_service():game_service_base<service_config, tiger_player, remote_socket<tiger_player>, logic_ptr, match_ptr, scene_set>(GAME_ID)
{
	wait_for_config_update_ = false;
}

void tigermc_service::on_main_thread_update()
{
}

void tigermc_service::on_sub_thread_update()
{
}


int tigermc_service::on_stop()
{
	return ERROR_SUCCESS_0;
}

void tigermc_service::delete_from_local( string uid, int is_free )
{
	if (is_free){
		uid += "_FREE";
	}
	string sql = "delete from player_curreny_cache where `key` = '" + uid + "'";
	Query q(*gamedb_);
	q.execute(sql);
}


tigermc_service::msg_ptr tigermc_service::create_msg( unsigned short cmd )
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_set_bets_req);
	REGISTER_CLS_CREATE(msg_game_lst_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_start_random_req);
	REGISTER_CLS_CREATE(msg_get_prize_req);
	REGISTER_CLS_CREATE(msg_leave_room);
	REGISTER_CLS_CREATE(msg_set_account);
	REGISTER_CLS_CREATE(msg_trade_gold_req);
	REGISTER_CLS_CREATE(msg_user_info_changed);
	REGISTER_CLS_CREATE(msg_account_login_req);
	REGISTER_CLS_CREATE(msg_platform_account_login);
	REGISTER_CLS_CREATE(msg_change_pwd_req);
	REGISTER_CLS_CREATE(msg_player_game_info_req);
	REGISTER_CLS_CREATE(msg_auto_tradein_req);
	REGISTER_CLS_CREATE(msg_7158_user_login_req);
	END_MSG_CREATE()
	return ret_ptr;
}

int tigermc_service::handle_msg( msg_ptr msg, remote_socket_ptr from_sock )
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
	
	the_service.the_new_config_.presets_.clear();
	sql = "select * from preset_present";
	if (q.get_result(sql)){
		while(q.fetch_row())
		{
			preset_present pst;
			pst.pid_ = q.getval();
			pst.rate_ = q.getval();
			pst.factor_ = q.getval();
			pst.prior_ = q.getval();
			the_service.the_new_config_.presets_.push_back(pst);
		}
	}
	q.free_result();

	sql =	"SELECT	`shut_down`,`max_trade_cap`, `trade_tax`"
		" FROM `server_parameters`";

	if (q.get_result(sql)){
		if (q.fetch_row())
		{
			the_service.the_new_config_.shut_down_ = q.getval();
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
	if (max_trade_cap_ != new_config.max_trade_cap_) return true;
	if (trade_tax_ != new_config.trade_tax_) return true;

	if (presets_.size() != new_config.presets_.size())
		return true;
	else if(presets_.size() > 0){
		for(unsigned int i = 0 ; i < presets_.size(); i++)
		{
			if (memcmp(&presets_[i], &new_config.presets_[i],sizeof(preset_present)) != 0){
				return true;
			}
		}
	}
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
}

int service_config::load_from_file(std::string path)
{
	xml_doc doc;
	if (!doc.parse_from_file("niuniu.xml")){
		restore_default();
		save_default();
	}
	else{
		READ_XML_VALUE("config/", db_port_);
		READ_XML_VALUE("config/", db_host_);
		READ_XML_VALUE("config/", db_user_);
		READ_XML_VALUE("config/", db_pwd_);
		READ_XML_VALUE("config/", db_name_);
		READ_XML_VALUE("config/", client_port_);
		READ_XML_VALUE("config/", php_sign_key_);
		READ_XML_VALUE("config/", accdb_port_);
		READ_XML_VALUE("config/", accdb_host_);
		READ_XML_VALUE("config/", accdb_user_);
		READ_XML_VALUE("config/", accdb_pwd_);
		READ_XML_VALUE("config/", accdb_name_);
		READ_XML_VALUE("config/", cache_ip_);
		READ_XML_VALUE("config/", cache_port_);
		READ_XML_VALUE("config/", world_ip_);
		READ_XML_VALUE("config/", world_port_);
		READ_XML_VALUE("config/", world_id_);
		READ_XML_VALUE("config/", alert_email_set_);
		READ_XML_VALUE("config/", register_fee_);
		READ_XML_VALUE("config/", register_account_reward_);
		READ_XML_VALUE("config/", game_id);


	}
	return ERROR_SUCCESS_0;
}

void     tigermc_service::set_player_game_info(player_ptr pp)
{
	if(!pp.get())
		return;

	string sql;
	Query q(*the_service.gamedb_);

	sql = "SELECT game_count,one_game_max_win,today_win,month_win,UNIX_TIMESTAMP(time) from log_player_game_info WHERE uid = '"+pp->uid_+"';";


	if (q.get_result(sql)){
		//如果没有记录，就要插入一条初始数据
		if(q.num_rows() == 0)
		{
			save_player_game_info(pp);
			return;
		}
		else
		{
			while (q.fetch_row()){
				pp->game_info_.game_count_          = q.getval();
				pp->game_info_.one_game_max_win_    = q.getbigint();
				pp->game_info_.today_win_           = q.getbigint();
				pp->game_info_.month_win_           = q.getbigint();
				pp->game_info_.time_                = q.getbigint();
			}
			q.free_result();
		}
	}

	//根据时间更新玩家gameinfo
	date today = day_clock::local_day();

	//date pday(date_time::NumSpecialValues)
	tm   tm_today  = to_tm(today);
	time_t time_ = pp->game_info_.time_;


	tm*   tm_player =  localtime(&time_);

	if(!tm_player)
		return;

	if( ( tm_today.tm_year != tm_player->tm_year) || ( tm_today.tm_mon != tm_player->tm_mon ) )
	{  
		//跨年和跨月都是更新数据
		pp->game_info_.month_win_ = 0;
		pp->game_info_.today_win_ = 0;
		save_player_game_info(pp);
		return;
	}

	if(( tm_today.tm_mday != tm_player->tm_mday ))
	{
		//日期变化了要更新数据
		pp->game_info_.today_win_ = 0;
		save_player_game_info(pp);
	}

}

void     tigermc_service::save_player_game_info(player_ptr pp)
{
	std::string sql;

	int icout			= pp->game_info_.game_count_;
	longlong  imax_win	= pp->game_info_.one_game_max_win_;
	longlong  itoday    = pp->game_info_.today_win_;
	longlong  imonth    = pp->game_info_.month_win_;

	sql = "call save_player_game_info('" + pp->uid_ + "',"+ lex_cast_to_str(icout)+","
						+ lex_cast_to_str(imax_win)+","+ lex_cast_to_str(itoday)+","
						+ lex_cast_to_str(imonth)+");";
	
	Query q(*the_service.gamedb_);

	if (q.execute(sql))
	{

	}
	else
	{

	}
}

bool preset_present::operator==( preset_present& pr )
{
	return (pr.pid_ == pid_) && (pr.rate_ == rate_) && (pr.factor_ == factor_) && (pr.prior_ == prior_);
}


