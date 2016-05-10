#include "service.h"
#include "game_config.h"
#include "msg.h"
#include "msg_client.h"
#include <utility.h>

game_service  the_service;
log_file<cout_output> glb_log;
log_file<file_output> glb_http_log;
std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;

#define  KICK_OUT_SECONDS 60
#define  UPDATE_ALERT_MAIL_CONFIG_TIME 3600 //半小时更新一次



void    update_alert_email_config(const boost::system::error_code& error)
{
#ifdef ALERT_EMAILS
	the_service.the_config_.load_jinbi_alert_config();

	the_service.t.expires_from_now(boost::posix_time::seconds(UPDATE_ALERT_MAIL_CONFIG_TIME));  
	the_service.t.async_wait(boost::bind(update_alert_email_config,   
		boost::asio::placeholders::error));
#endif // ALERT_EMAILS
}


void service_config::save_jinbi_alert(std::string uid,std::string uname,int type,int value,int game_id)
{
#ifdef ALERT_EMAILS

	JinbiAlertnConfigMap::iterator iter = jinbi_alert_config_map.find(type);

	if(iter == jinbi_alert_config_map.end())
		return;

	if( value > iter->second.value_ )
	{
		/*string sql;
		Query qq(*the_service.email_db_);

		sql = "set names 'gbk';";

		qq.execute(sql);*/

		Database& db = *the_service.email_db_;
		std::string  str;
		if(type == ALERT_TYPE_LOSE)
			str = iter->second.type_name_ + "限制为"+ lex_cast_to_str(iter->second.value_) +",玩家"+ uid + "输" +lex_cast_to_str(value);
		else
			str = iter->second.type_name_ + "限制为"+ lex_cast_to_str(iter->second.value_) +",玩家"+ uid + "赢" +lex_cast_to_str(value);

		BEGIN_INSERT_TABLE("alert_emails");
		SET_STRFIELD_VALUE("fromgame", iter->second.game_name_.c_str());
		SET_STRFIELD_VALUE("alert_type",lex_cast_to_str(type) );
		SET_STRFIELD_VALUE("content",str.c_str());
		SET_STRFIELD_VALUE("uid",uid.c_str() );
		str = uname + " ";
		SET_STRFINAL_VALUE("uname",str.c_str() );
		EXECUTE_IMMEDIATE();
	}

#endif
}

void service_config::load_jinbi_alert_config()
{
#ifdef ALERT_EMAILS
	jinbi_alert_config_map.clear();

	string sql;
	Query q(*the_service.email_db_);

	sql = "set names 'gbk';";
	sql.append("SELECT type,value,gameName,typeName  FROM `alert_config` WHERE gameID = ");
	sql.append(lex_cast_to_str(ALERT_GAME_ID));

	std::string  tmpstr = q.get_string(sql);

	if (q.get_result(sql)){
		while (q.fetch_row()){
			stJinbiAlertnConfig tmp;
			tmp.type_      = q.getval();
			tmp.value_     = q.getval();
			tmp.game_name_ = q.getstr();
			tmp.type_name_ = q.getstr();
			jinbi_alert_config_map.insert(std::make_pair(tmp.type_,tmp));
		}
	}

	q.free_result();
   
	//  
#endif //ALERT_EMAILS
}



bool service_config::need_update_config( service_config& new_config )
{
	if(egg_rate_param_ != new_config.egg_rate_param_)
		return true;

	if(egg_max_level_ != new_config.egg_max_level_)
		return true;

	if(new_config.egg_param_map.size() == 0)
		return false;

	if(egg_param_map.size() != new_config.egg_param_map.size())
		return true;


	for(int i = 1 ;i <= new_config.egg_max_level_ ; ++ i)
	{
		if( egg_param_map[i] != new_config.egg_param_map[i] )
			return true;
	}

	return false;
}

void service_config::refresh()
{
	refresh_common_param();
	refresh_eggs_param();



	if (need_update_config(the_service.the_new_config_)){
		//the_service.wait_for_config_update_ = true;
		//更新服务器数据
		the_service.the_config_.shut_down_		=  the_service.the_new_config_.shut_down_;
		the_service.the_config_.egg_rate_param_ =  the_service.the_new_config_.egg_rate_param_ ;
		the_service.the_config_.egg_max_level_  =  the_service.the_new_config_.egg_max_level_;
		the_service.the_config_.egg_param_map.clear();
		the_service.the_config_.egg_param_map = the_service.the_new_config_.egg_param_map;
	}

	the_service.the_config_.shut_down_ = the_service.the_new_config_.shut_down_;
	  
}

void  service_config::refresh_common_param()
{
	the_service.the_new_config_ = the_service.the_config_;
	string sql;
	Query q(*the_service.gamedb_);

	sql =	"SELECT"
		" param_1,param_2,param_3"
		" FROM `server_parameters` WHERE game_id = ";

	sql.append(GAME_ID_EGG);


	if (q.get_result(sql)){
		if (q.fetch_row()){
			the_service.the_new_config_.shut_down_ = q.getval();
			the_service.the_new_config_.egg_rate_param_ = q.getval();
			the_service.the_new_config_.egg_max_level_  = q.getval();
		}
	}
	q.free_result();
}


void  service_config::refresh_eggs_param()
{
	string sql;
	Query q(*the_service.gamedb_);

	sql =	"SELECT	*"
		    " FROM `egg_level_congfig` ";

	the_service.the_new_config_.egg_param_map.clear();

	if ( q.get_result(sql) )
	{
		while (q.fetch_row())
		{
			egg_param  t_param;
			t_param.level     = q.getval();
			t_param.up_rate   = q.getval();
			t_param.use_money = q.getubigint();
			t_param.get_money = q.getubigint();
			the_service.the_new_config_.egg_param_map.insert(std::make_pair(t_param.level,t_param));
		}
	}

	q.free_result();
}

int  service_config::load_from_file()
{
	xml_doc doc;
	if (!doc.parse_from_file("173.xml")){
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
		READ_XML_VALUE("config/", alert_email_set_				);
	}
	return ERROR_SUCCESS_0;
}

void  service_config::load_173_eggs_param()
{

	egg_param_map.clear();


	string sql;
	Query q(*the_service.gamedb_);

	sql =	"SELECT *"
		" FROM `egg_level_congfig` ";


	if ( q.get_result(sql) )
	{
		if(q.num_rows() > 0 )
		{
			while (q.fetch_row())
			{
				egg_param  t_param;
				t_param.level     = q.getval();
				t_param.up_rate   = q.getval();
				t_param.use_money = q.getubigint();
				t_param.get_money = q.getubigint();
				egg_param_map.insert(std::make_pair(t_param.level,t_param));
			}
		}
	}

	if(egg_param_map.size() == 0)
	{
		egg_param  t_param;

		t_param.level     = 1;
		t_param.up_rate   = 100;
		t_param.use_money = 5;
		t_param.get_money = 5;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));

		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 2;
		t_param.up_rate   = 90;
		t_param.use_money = 13;
		t_param.get_money = 20;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));

		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 3;
		t_param.up_rate   = 80;
		t_param.use_money = 50;
		t_param.get_money = 80;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));


		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 4;
		t_param.up_rate   = 70;
		t_param.use_money = 160;
		t_param.get_money = 320;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));

		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 5;
		t_param.up_rate   = 60;
		t_param.use_money = 500;
		t_param.get_money = 1280;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));

		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 6;
		t_param.up_rate   = 40;
		t_param.use_money = 900;
		t_param.get_money = 5120;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));

		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 7;
		t_param.up_rate   = 30;
		t_param.use_money = 1500;
		t_param.get_money = 20480;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));

		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 8;
		t_param.up_rate   = 20;
		t_param.use_money = 3000;
		t_param.get_money = 81920;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));


		memset(&t_param,0,sizeof(t_param));
		t_param.level     = 9;
		t_param.up_rate   = 10;
		t_param.use_money = 10000;
		t_param.get_money = 327680;
		egg_param_map.insert(std::make_pair(t_param.level,t_param));
	}

	q.free_result();
}


void  service_config::load_173_common_param()
{
	//读取游戏参数
	string sql;
	Query q(*the_service.gamedb_);

	sql =	"SELECT"
		" param_1,param_2,param_3"
		" FROM `server_parameters` WHERE game_id = ";

	sql.append(GAME_ID_EGG);

	egg_rate_param_ = 0;
	egg_max_level_  = 0;

	string str ;
	if (q.get_result(sql)){
		if (q.fetch_row()){
			shut_down_ = q.getval();
			egg_rate_param_ = q.getval();
			egg_max_level_  = q.getval();
		}
	}
	else
	{
		str = q.GetError();
	}

	if(egg_rate_param_ == 0)
		egg_rate_param_ = 1;

	if(egg_max_level_ == 0)
		egg_max_level_  = 9;


	q.free_result();

}

bool service_config::check_valid()
{
	return true;
}

game_service::game_service():game_service_base<service_config, game_player, remote_socket<game_player>, logic_ptr, no_match, scene_set>(GAME_ID),t(timer_sv_)
{
	//state_ = msg_173_eggs_service_state::SERVER_STATE_NORMAL;
	the_config_.egg_param_map.clear();
	the_new_config_.egg_param_map.clear();
	
}


game_service::msg_ptr			game_service::create_msg(unsigned short cmd) 
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_173_eggs_login);
	REGISTER_CLS_CREATE(msg_173_eggs_smash); 
	REGISTER_CLS_CREATE(msg_173_eggs_open) ;
	END_MSG_CREATE()	

	return ret_ptr;	
}

int					game_service::handle_msg(msg_ptr msg, remote_socket_ptr from_sock) 
{
	msg_from_client<remote_socket_ptr>* pmsg = dynamic_cast<msg_from_client<remote_socket_ptr>*>(msg.get());
	if (pmsg){
		pmsg->from_sock_ = from_sock;
		return pmsg->handle_this();
	}
	else
		return SYS_ERR_CANT_FIND_CONNECTION;
}


void  game_service::on_main_thread_update()
{

		boost::posix_time::ptime  pt1 = boost::posix_time::microsec_clock::local_time();

		auto iter = players_.begin();


		for(;iter != players_.end();)
		{
			player_ptr pp = iter->second;

		    if(!pp.get())
			{
				++iter;
				continue;
			}

			if(pp->last_op_time.is_not_a_date_time())
			{
				++iter;
				continue;
			}

			if( (pt1-pp->last_op_time).total_seconds() > KICK_OUT_SECONDS )
			{
				
				auto conn = pp->from_socket_.lock();
				if (conn.get()){
					conn->close();
				}

				if(!pp->is_connection_lost_){
					pp->on_connection_lost();
					remove_player(pp);
					iter = players_.erase(iter);
				}
		
			}
			else
			{
				++iter;
			}
		}
}

void 	game_service::on_sub_thread_update()
{

}

int		game_service::on_run()
{
	the_config_.load_173_common_param();
	the_config_.load_173_eggs_param();
	glb_http_log.output_.fname_ = "http_log.log";
#ifdef ALERT_EMAILS
	the_config_.load_jinbi_alert_config();

	
	t.expires_from_now(boost::posix_time::seconds(UPDATE_ALERT_MAIL_CONFIG_TIME));  
	t.async_wait(boost::bind(update_alert_email_config,   
		boost::asio::placeholders::error));
#endif // ALERT_EMAILS
	Query q(*gamedb_);

	string sql = "delete from log_online_players";
	q.execute(sql);
	q.free_result();

	return 0;
}

int			game_service::on_stop() 
{
		return ERROR_SUCCESS_0;
}

