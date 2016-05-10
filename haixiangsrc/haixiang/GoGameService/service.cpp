#include "service.h"
#include "msg_server.h"
#include "msg_client.h"
#include "game_service_base.hpp"

GoGame_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;
vector<std::string> g_bot_names;


bool get_scene_config()
{
	scene_set cnf;
	cnf.id_ = "";
	cnf.gurantee_set_ = 0;
	cnf.is_free_ = 0;
	the_service.scenes_.push_back(cnf);
	return true;
}

GoGame_service::GoGame_service()
	:game_service_base<service_config, game_player, remote_socket<game_player>, logic_ptr, no_match, scene_set>(the_service.the_config_.game_id)
{

}


void GoGame_service::on_main_thread_update()
{
	static boost::posix_time::ptime  gpt = boost::posix_time::microsec_clock::local_time();
	//更新配置
	boost::posix_time::ptime  pt1 = boost::posix_time::microsec_clock::local_time();
	float dt = (pt1 - gpt).total_milliseconds() / 1000.0;
	if (dt < 0.01) return;

	//刷新每个单独业务
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
}

void GoGame_service::on_sub_thread_update()
{

}


int GoGame_service::on_run()
{
	//更新配置
	the_config_.refresh();
	//获得场次配置
	if (!get_scene_config()) return -1;

	return ERROR_SUCCESS_0;
}

int GoGame_service::on_stop()
{
	return ERROR_SUCCESS_0;
}

player_ptr GoGame_service::pop_onebot(logic_ptr plogic)
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



GoGame_service::msg_ptr GoGame_service::create_msg(unsigned short cmd)
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_url_user_login_req);
	REGISTER_CLS_CREATE(msg_platform_login_req);
	REGISTER_CLS_CREATE(msg_enter_game_req);
	REGISTER_CLS_CREATE(msg_prepare_enter_complete);
	REGISTER_CLS_CREATE(msg_leave_room);

	REGISTER_CLS_CREATE(send_game_make_ready);		//举手准备
	REGISTER_CLS_CREATE(send_game_move_chess);		//走棋
	REGISTER_CLS_CREATE(send_game_ask_undo);			//悔棋询问
	REGISTER_CLS_CREATE(send_game_reply_undo);		//悔棋回复
	REGISTER_CLS_CREATE(send_game_pass_chess);		//pass
	REGISTER_CLS_CREATE(send_game_point_chess);		//点目
	REGISTER_CLS_CREATE(send_game_reply_point);		//点目回复
	REGISTER_CLS_CREATE(send_game_give_up);				//认输							
	REGISTER_CLS_CREATE(send_game_compel_chess);	//强制数子							
	REGISTER_CLS_CREATE(send_game_ask_summation);	//求和询问
	REGISTER_CLS_CREATE(send_game_reply_summation);	//求和回复
	REGISTER_CLS_CREATE(send_set_point);					//设置点目
	REGISTER_CLS_CREATE(send_submit_point);				//提交点目
	REGISTER_CLS_CREATE(send_result_gopoint);			//提交点目结果
	END_MSG_CREATE()
	return ret_ptr;
}

int GoGame_service::handle_msg(msg_ptr msg, remote_socket_ptr from_sock)
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if(pmsg)
	{
		pmsg->from_sock_ = from_sock;
		player_ptr pp = from_sock->the_client_.lock();
		if(pp.get())
		{
			pp->last_msg_ = boost::posix_time::microsec_clock::local_time();
		}

		if(pmsg->head_.cmd_ == GET_CLSID(msg_talk_req))
		{
			return ERROR_SUCCESS_0;
		}else{
			return pmsg->handle_this();
		}
	}else
		return SYS_ERR_CANT_FIND_CONNECTION;
}

//获得player游戏数据
void GoGame_service::get_gogame_data(player_ptr pp)
{
	std::string c_name = "go_" + pp->uid_;
	goplayer_data gd;
	std::map<std::string, goplayer_data>::iterator it;
	it = goplayer_map.find(c_name);
	//如果缓存中有玩家数据，直接取
	if(it != goplayer_map.end())
	{
		gd = it->second;
		pp->score = gd.score;
		pp->win_total = gd.win_total;
		pp->fail_total = gd.fail_total;
		pp->draw_total = gd.draw_total;
		return;
	}

	string sql;
	Query q1(*the_service.gamedb_);
	sql = "call role_info_data (0,'"+ pp->uid_ +"', 0, 0, 0, 0);";
	if(q1.get_result(sql))
	{
		if(q1.fetch_row())
		{
			gd.score = q1.getval();
			gd.win_total = q1.getuval();
			gd.fail_total = q1.getuval();
			gd.draw_total = q1.getuval();

			c_name = "go_" + pp->uid_;
			pp->score = gd.score;
			pp->win_total = gd.win_total;
			pp->fail_total = gd.fail_total;
			pp->draw_total = gd.draw_total;
			goplayer_map.insert(std::make_pair(c_name, gd));
		}
	}
	q1.free_result();
	return;
}

void GoGame_service::set_gogame_data(std::string _uid, int offensive_move, int the_win, int get_score)
{
	std::string c_name = "go_" + _uid;
	goplayer_data gd;
	int t_w = 0;
	int t_f = 0;
	int t_d = 0;

	std::map<std::string, goplayer_data>::iterator it;
	it = goplayer_map.find(c_name);
	if(it != goplayer_map.end())
	{
		gd = it->second;
		gd.score += get_score;
		if(the_win == 0)
		{
			gd.draw_total++;
			t_d++;
		}
		else
		{
			if(the_win == offensive_move)
			{
				gd.win_total++;
				t_w++;
			}
			else
			{
				gd.fail_total++;
				t_d++;
			}
		}
	}

	string sql;
	Query q1(*the_service.gamedb_);
	sql = "call role_info_data (1,'"+ _uid +"', " + to_string(get_score) + ", " +to_string(t_w) + ", " + to_string(t_f) + ", " + to_string(t_d) + ");";
	if(q1.get_result(sql))
	{
		if(q1.fetch_row()){};
	}
	q1.free_result();
	return;
}




//=================================================================
// service_config
//=================================================================
//加载配置文件
int service_config::load_from_file(std::string path)
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

		READ_CONFIG_VALUE(_config, world_ip_);
		READ_CONFIG_VALUE(_config, world_port_);
		READ_CONFIG_VALUE(_config, world_id_);

		READ_CONFIG_VALUE(_config, register_account_reward_);
		READ_CONFIG_VALUE(_config, scheduledTaskTime);
		READ_CONFIG_VALUE(_config, period_time);

		READ_CONFIG_VALUE(_config, wait_reconnect);

		glb_log.write_log("(Server_Config)Game_ID:%d", game_id);
		glb_log.write_log("(Server_Config)Game_NAME:%s", game_name.c_str());
		glb_log.write_log("(Server_Config)client_port:%d", client_port_);
		glb_log.write_log("(Server_Config)db_host:%s", db_host_.c_str());
		glb_log.write_log("(Server_Config)db_port:%d", db_port_);
		glb_log.write_log("(Server_Config)cache_ip:%s", cache_ip_.c_str());
		glb_log.write_log("(Server_Config)cache_port:%d", cache_port_);

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

bool service_config::check_valid()
{
	return true;
}

void service_config::refresh()
{
	the_service.the_new_config_ = the_service.the_config_;
	//service_config_base::refresh();
	
	//获得游戏配置
	string sql;
	Query q(*the_service.gamedb_);
	sql = "SELECT turn_time, which_start FROM game_parameters";
	if(q.get_result(sql))
	{
		if(q.fetch_row())
		{
			the_service.the_new_config_.turn_time = q.getuval();
			the_service.the_new_config_.which_start = q.getuval();
		}
	}
	q.free_result();

	//获得评级数据
	the_service.grade_map.clear();
	sql = "SELECT level, min_score, max_score FROM level_grading";
	q.get_result(sql);
	while (q.fetch_row())
	{
		level_grading lg;
		lg.level = q.getuval();
		lg.min_score = q.getuval();
		lg.max_score = q.getuval();
		std::string l_name = to_string(lg.level);
		the_service.grade_map.insert(std::make_pair(l_name, lg));
	}
	q.free_result();
}