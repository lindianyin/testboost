#include "msg_client.h"
#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "msg.h"

extern void sql_trim(std::string& str);

int  get_db_egg_level(std::string uid)
{
	int level = 0; 

	std::string sql;

	Query q(*the_service.gamedb_);

	sql =	"SELECT	egg_level"
		" FROM `egg_level_info` WHERE uid = '" + uid+"'";

	if (q.get_result(sql)){
		if (q.fetch_row()){
			level  = q.getuval();
		}
	}
	q.free_result();

	return level;
}



static player_ptr		user_from_173_login(remote_socket_ptr from_sock_, 
								   std::string platform_uid,
								   std::string uname_,
								   std::string headpic, 
								   std::string uidx_, 
								   std::string platform_id)
{
	from_sock_->set_authorized();
	sql_trim(uname_);
	sql_trim(headpic);
	//防止不同平台的uid冲突
	std::string uid_ = platform_id + platform_uid;

	player_ptr pp = the_service.get_player(uid_);

	if(pp.get())
	{
		remote_socket_ptr pold = pp->from_socket_.lock();
		if (pold.get())
		{
			//如果是另一个socket发过来的，则断开旧连接
			if (pold != from_sock_)
			{
				msg_logout msg;
				send_msg(pold, msg, true);

				glb_log.write_log("same account login, the old force offline!");

				pp->from_socket_ = from_sock_;
				from_sock_->the_client_ = pp;
				
			}
	    }
	    else
	    {
			pp->from_socket_ = from_sock_;
			from_sock_->the_client_ = pp;
			pp->name_ = uname_;
			pp->head_ico_ = headpic;
			pp->uidx_ = uidx_;
			pp->platform_uid_ = platform_uid;
			pp->platform_id_ = platform_id;
	    }
		pp->init_egg_game();
	}
	else
	{
		pp.reset(new game_player_type());
		pp->uid_ = uid_;
		pp->name_ = uname_;
		pp->from_socket_ = from_sock_;
		pp->head_ico_ = headpic;
		from_sock_->the_client_ = pp;
		pp->is_bot_ = false;
		pp->uidx_ = uidx_;
		pp->platform_uid_ = platform_uid;
		pp->platform_id_ = platform_id;
		pp->init_egg_game();
		the_service.players_.insert(make_pair(pp->uid_, pp));
	}

	if (!pp.get()){
		return pp;
	}
	//先发送服务器状态
	//pp->send_cur_server_state_to_client(the_service.state_);

	//要去数据库查询玩家最后一次蛋的等级返回给客户端
    pp->cur_egg_level = get_db_egg_level(pp->uid_);

	pp->send_cur_level_to_client();

	pp->is_connection_lost_ = false;

	pp->save_db_egg_on_line();

	pp->set_last_operator_time();

	return pp;
}

int	msg_173_eggs_login::handle_this()
{
	std::string sign_pattn, s;
	s.assign(uid_, max_guid);
	sign_pattn +=s.c_str();
	sign_pattn +="|";

	s.assign(uname_, max_name);
	sign_pattn += s.c_str();
	sign_pattn +="|";

	/*sign_pattn += lex_cast_to_str(vip_lv_);
	sign_pattn +="|";*/

	s.assign(head_icon_, max_guid);
	sign_pattn += s.c_str();
	sign_pattn +="|";

	/*sign_pattn += lex_cast_to_str(uidx_);
	sign_pattn +="|";*/

	s.assign(session_, max_guid);
	sign_pattn += s.c_str();
	sign_pattn +="|";

	/*sign_pattn += lex_cast_to_str(sex_);
	sign_pattn +="|";*/

	sign_pattn += the_service.the_config_.php_sign_key_;

	std::string remote_sign = url_sign_;
	unsigned char out_put[16];
	CMD5 md5;
	md5.MessageDigest((const unsigned char*)sign_pattn.c_str(), sign_pattn.length(), out_put);
	std::string sign_key;
	for (int i = 0; i < 16; i++)
	{
		char aa[4] = {0};
		sprintf_s(aa, 4, "%02x", out_put[i]);
		sign_key += aa;
	}

	if(sign_key != remote_sign){
		glb_log.write_log("wrong sign, local: %s remote:%s", sign_pattn.c_str(), remote_sign.c_str());
		return SYS_ERR_SESSION_TIMEOUT;
	}

	if (the_service.players_.size() > 500){
		return SYS_ERR_SERVER_FULL;
	}

	if (the_service.the_config_.shut_down_){
		return SYS_ERR_SESSION_TIMEOUT;
	}

	boost::system::error_code ec;
	auto rmt = from_sock_->s.remote_endpoint(ec);
	if (ec.value() != 0){
		return SYS_ERR_SESSION_TIMEOUT;
	}

	auto it_sess = the_service.login_sessions_.find(session_);
	if (it_sess != the_service.login_sessions_.end()){
		session& ses = it_sess->second;
		if (ses.remote_.address() != rmt.address()){
			return SYS_ERR_SESSION_TIMEOUT;
		}
		//半小时超时
		else if ((boost::posix_time::second_clock::local_time() - ses.create_time_).total_seconds() > 1800){
			return SYS_ERR_SESSION_TIMEOUT;
		}
	}
	session ses; 
	ses.remote_ = rmt;
	ses.session_key_ = session_;
	ses.create_time_ = boost::posix_time::second_clock::local_time();
	the_service.login_sessions_.insert(make_pair(session_, ses));

	std::string idx = boost::lexical_cast<std::string>(sex_);
	user_from_173_login(from_sock_, uid_, uname_, head_icon_, idx, uidx_);
	return ERROR_SUCCESS_0;
}


int	  msg_173_eggs_smash::handle_this()
{
	 std::string p_udi;
	 p_udi +=  uidx_;
	 p_udi +=  uid_;
	 player_ptr pp = the_service.get_player(p_udi);

	 if(!pp.get())
		 return ERROR_SYS_BEGIN;

	 pp->set_last_operator_time();

	 int  cur_level  = pp->cur_egg_level;
	 pp->old_egg_level = cur_level;
	 
	 //已经是最高级别
	 if( cur_level >= the_service.the_config_.egg_max_level_ )
		  return ERROR_SYS_BEGIN;

	 int  next_level  = cur_level + 1;

	 EggParamMap::iterator  iter = the_service.the_config_.egg_param_map.find(next_level);

	 if(iter == the_service.the_config_.egg_param_map.end())
	 {
			return ERROR_SYS_BEGIN;
	 }
	 else
	 {
		 int i = rand() % 100;
		 
		 user_173_auto_trade< game_service::this_type, platform_config_173 >* tradeout = 
			 new user_173_auto_trade< game_service::this_type, platform_config_173 >(the_service, platform_config_173::get_instance());

		 boost::shared_ptr< user_173_auto_trade< game_service::this_type, platform_config_173 > > ptrade(tradeout);

		 tradeout->op_type_ = 0; 

		 tradeout->uid_ = pp->uid_;
		 tradeout->rel_uid_ = pp->platform_uid_;
		 
		 tradeout->out_lay_ = iter->second.use_money ;
		 tradeout->in_come_  = 0;
		 tradeout->is_succeed_ = 0;
		 tradeout->sync_ = get_guid();

		 pp->old_jinbi = pp->cur_jinbi;

		 //升级成功
		 if( ( iter->second.up_rate * the_service.the_config_.egg_rate_param_/100 ) > i )
			tradeout->is_succeed_ = 1;
		 else
			 tradeout->is_succeed_ = 0;

		 //发送请求
		 tradeout->send_request();
	 }


	 return ERROR_SUCCESS_0;
}

int	  msg_173_eggs_open::handle_this()
{

	std::string p_udi;

	p_udi +=  uidx_;
	p_udi +=  uid_;
	player_ptr pp = the_service.get_player(p_udi);

	pp->old_jinbi = pp->cur_jinbi;

	if(!pp.get())
		return ERROR_SYS_BEGIN;

	pp->set_last_operator_time();

	//0级蛋，没钱可收
	if(pp->cur_egg_level == 0) 
		return ERROR_SYS_BEGIN;

	

	 EggParamMap::iterator  iter = the_service.the_config_.egg_param_map.find(pp->cur_egg_level);

	 if(iter == the_service.the_config_.egg_param_map.end())
		 return ERROR_SYS_BEGIN;

	user_173_auto_trade< game_service::this_type, platform_config_173 >* tradeout = 
		new user_173_auto_trade< game_service::this_type, platform_config_173 >(the_service, platform_config_173::get_instance());

	boost::shared_ptr< user_173_auto_trade< game_service::this_type, platform_config_173 > > ptrade(tradeout);

	tradeout->op_type_ = 1; //收蛋

	tradeout->uid_ = pp->uid_;
	tradeout->rel_uid_ = pp->platform_uid_;

	tradeout->out_lay_ = 0;
	tradeout->in_come_  = iter->second.get_money;
	tradeout->is_succeed_ = 0;
	tradeout->sync_ = get_guid();

	tradeout->send_request();

	return ERROR_SUCCESS_0;
}