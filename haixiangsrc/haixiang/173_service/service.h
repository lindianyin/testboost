#pragma once
#include "game_service_base.h"
#include "utility.h"
#include "game_player.h"
#include "game_logic.h"
#include "simple_xml_parse.h"
#include "error_define.h"
#include <unordered_map>

class game_service;
extern game_service the_service;
using namespace nsp_simple_xml;

extern std::map<string, longlong>	g_debug_performance_total;	//每个函数执行时间累计
extern std::map<string, longlong>	g_debug_performance_count;
extern std::map<string, longlong>	g_debug_performance_avg;

extern void broadcast_17173_msg(player_ptr pp, int level, longlong money);

#ifdef _DEBUG

#define DEBUG_COUNT_PERFORMANCE_BEGIN(name)\
	std::string local_func_name = name;\
	boost::posix_time::ptime local_ptm = boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time();\


#define DEBUG_COUNT_PERFORMANCE_END()\
	g_debug_performance_total[local_func_name] += (boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time() - local_ptm).total_microseconds();\
	g_debug_performance_count[local_func_name]++;\
	g_debug_performance_avg[local_func_name] = g_debug_performance_total[local_func_name] / g_debug_performance_count[local_func_name];

#else

#define DEBUG_COUNT_PERFORMANCE_BEGIN(name)
#define DEBUG_COUNT_PERFORMANCE_END()	

#endif

#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)


#define  ALERT_GAME_ID 105

struct scene_set
{
	std::string			id_;
	int				is_free_;						//是否免费场
	int				rate_;							//兑换费率
	longlong	gurantee_set_;
	scene_set()
	{
		is_free_ = 1;
		rate_ = 0;
		gurantee_set_ = 0;
	}
};


struct egg_param
{
	int level;        //等级  
	int up_rate;      //升级概率
	int use_money;    //消耗金币
	int get_money;    //奖励金币

	egg_param()
	{
		level     = 0;
		up_rate   = 0;     
		use_money = 0;    
		get_money = 0;    
	}

	bool operator != (const egg_param& temp)
	{
		if(level != temp.level)
			return true;

		if(up_rate != temp.up_rate)
			return true;

		if(use_money != temp.use_money)
			return true;

		if(get_money != temp.get_money)
			return true;

		return false;
	}
};

/*异常类型
alert_type	1	兑入预警	
alert_type	2	兑出预警	
alert_type	3	玩家负预警	
alert_type	4	玩家正预警	
alert_type	5	系统负预警	
alert_type	6	系统正预警	
*/

enum 
{
	ALERT_TYPE_TRADE_IN  = 1,
	ALERT_TYPE_TRADE_OUT = 2,
	ALERT_TYPE_LOSE      = 3,
	ALERT_TYPE_WIN       = 4,
	ALERT_TYPE_SYS_LOSE  = 5,
	ALERT_TYPE_SYS_WIN   = 6
};
struct stJinbiAlertnConfig
{
	std::string  game_name_; 
	std::string  type_name_;
	int          type_;
	int          value_;

	stJinbiAlertnConfig()
	{
		game_name_.clear();
		type_name_.clear();
		type_   = 0 ;
		value_  = 0;
	}
};



class platform_config_173
{
public:
	
	std::string		game_id_;//接入平台约定的游戏id
	std::string		host_;
	std::string		skey_;
	std::string		port_;
	std::string		trade_param_;
	std::string     appid_;   //173指定要用
	std::string     broadcast_appid_;
	std::string		broadcast_param_;
	std::string     broadcast_key_;
	
	platform_config_173()
	{
		game_id_ = GAME_ID_EGG;
		port_ = "80";
		host_ = "v.17173.com";
		skey_ = "C2546726272942A294CEDE3E19258C6C";
		trade_param_ = "/show/open/t_one.action?";
		broadcast_param_ = "/show/open/t_award.action?";
		appid_       ="600014";
		broadcast_appid_="600014";
		broadcast_key_  ="C2546726272942A294CEDE3E19258C6C";
	}

	static platform_config_173& get_instance()
	{
		static platform_config_173	conf_173_;
		return conf_173_;
	}
};

typedef map<int ,egg_param>   EggParamMap;
typedef map<int ,stJinbiAlertnConfig> JinbiAlertnConfigMap;

class  service_config : public service_config_base
{
public:
	int             egg_rate_param_;    //砸蛋参数
	int             egg_max_level_;      //砸蛋最高等级
	EggParamMap     egg_param_map;


	//金币异常记录
	JinbiAlertnConfigMap  jinbi_alert_config_map;

	service_config()
	{
		egg_rate_param_ = 100;
		egg_max_level_  = 9;
		egg_param_map.clear();
		jinbi_alert_config_map.clear();
	}

	int				load_from_file();
	void			refresh();
	bool			check_valid();

	void            load_173_common_param();
	void            load_173_eggs_param();

	void            refresh_common_param();
	void            refresh_eggs_param();

	bool            need_update_config( service_config& new_config );

	//金币异常相关操作
	void           load_jinbi_alert_config();
	void           save_jinbi_alert(std::string uid,std::string uname,int type,int value,int game_id = ALERT_GAME_ID);

	//更新警告邮件设置
	//void          update_alert_email_config( const boost::system::error_code& error);

};





typedef boost::shared_ptr<remote_socket<game_player>> remote_socket_ptr;
class  game_service : public game_service_base<service_config, game_player, remote_socket<game_player>, logic_ptr, no_match, scene_set>
{
public:
	game_service();

	int  state_;   //服务器状态


	msg_ptr			create_msg(unsigned short cmd) ;
	int				handle_msg(msg_ptr msg, remote_socket_ptr from_sock) ;

	 void				on_main_thread_update();
	 void 				on_sub_thread_update();
	 int					on_run();
	 int					on_stop() ;

	 boost::asio::deadline_timer t;  
};




template<class service_t, class config_t>
class platform_broadcast_17173 :  public  http_request< platform_broadcast_17173 < service_t , config_t > >
{
public:
	platform_broadcast_17173(service_t& srv , config_t & cnf ): http_request< platform_broadcast_17173<service_t, config_t > > (srv.http_sv_),
		time_out_(srv.http_sv_), platform_config_(cnf)

	{

	};

	~platform_broadcast_17173() {

	};

	boost::asio::deadline_timer time_out_;
	string          uid_;     //用户ID
	string          rel_uid_; //真实ID

	string         award_;   //广播消息内容
	string         award_utf8;// utf8的编码

	config_t&		platform_config_;


	int send_request()
	{
		std::string param,		str_sign;
		//appid
		add_http_params(param, "appId", platform_config_.broadcast_appid_);
		str_sign = "appId="+platform_config_.broadcast_appid_;
		//award
		add_http_params(param, "award", award_utf8);
		str_sign += "award="+HttpUtility::URLEncode(award_utf8);

		////gameid
		add_http_params(param, "gid", platform_config_.game_id_);
		str_sign += "gid="+platform_config_.game_id_;
		//time
		time_t tmp;
		time(&tmp);
		add_http_params(param, "time", tmp);
		str_sign += "time="+boost::lexical_cast<std::string>(tmp);
		//uid
		add_http_params(param, "uid",rel_uid_ );
		str_sign += "uid="+rel_uid_;

		//
		str_sign += platform_config_.broadcast_key_;

		unsigned char out_put[16];
		CMD5 md5;
		md5.MessageDigest((const unsigned char*)str_sign.c_str(), str_sign.length(), out_put);
		std::string sign_key;
		for (int i = 0; i < 16; i++)
		{
			char aa[4] = {0};
			sprintf_s(aa, 4, "%02x", out_put[i]);
			sign_key += aa;
		}
		add_http_params(param, "sig", sign_key);

		std::string str_req = build_http_request(platform_config_.host_, 
			platform_config_.broadcast_param_ + param);

		request(str_req, 
			platform_config_.host_,
			platform_config_.port_);

		return ERROR_SUCCESS_0;
	}

protected:

	void	handle_http_body(std::string str_body)
	{
			/*{"code":"000000","msg":"ok","obj":""}
			code:100003 消费失败
			code:100004 余额不足
			code:900002 请求已被处理
			code:900002 请求已被处理
			code:000000 成功
		*/
	}

	void	on_request_failed(int reason, std::string err_msg)
	{

	}

};

template<class service_t, class config_t>
class user_173_auto_trade :  public  http_request< user_173_auto_trade < service_t , config_t > >
{
public:
	user_173_auto_trade(service_t& srv , config_t & cnf ): http_request< user_173_auto_trade<service_t, config_t > > (srv.http_sv_),
		the_service_(srv),time_out_(srv.http_sv_), platform_config_(cnf)
	
	{
	
	};

	~user_173_auto_trade() {

	};

	service_t&	the_service_;
	//std::string	sn_;
	std::vector<std::string> v;
	boost::asio::deadline_timer time_out_;
	config_t&		platform_config_;
	//
	int             in_come_; //加钱
	int             out_lay_; //扣钱
	string          uid_;     //用户ID
	string          rel_uid_; //真实ID
	string          sync_;    //订单号
	int            is_succeed_;
	int            op_type_;  //玩家操作类型 0代表砸蛋，1代表开蛋



	void parse_173_result(const char* src,const char * sep, vector<string>& res)
	{
		char * token;
		token = strtok((char*)src,sep);
		
		res.push_back(token);

		while (token != NULL)
		{
			//printf("%s \n",token);
			token = strtok(NULL,sep);

			if(token == NULL)
				break;

			res.push_back(token);
		}
	}



	int  send_request()
	{
		/* gid 1  业务id
		income 加钱
		outlay 扣钱
		uid 用户id*/

		std::string param,		str_sign;
		//appid
		add_http_params(param, "appId", platform_config_.appid_);
		str_sign = "appId="+platform_config_.appid_;
		////gameid
		//add_http_params(param, "gid", platform_config_.game_id_);
		//str_sign += "gid="+platform_config_.game_id_;
		//income
		add_http_params(param, "income", in_come_);
		str_sign += "income="+boost::lexical_cast<std::string>(in_come_);
		//outlay
		add_http_params(param, "outlay", out_lay_);
		str_sign += "outlay="+boost::lexical_cast<std::string>(out_lay_);
		/*add_http_params(param, "outlay", 1);
		str_sign += "outlay="+boost::lexical_cast<std::string>(1);*/
		//sync
		add_http_params(param, "sync", sync_);
		str_sign += "sync="+sync_;
		//time
		time_t tmp;
		time(&tmp);
		add_http_params(param, "time", tmp);
		str_sign += "time="+boost::lexical_cast<std::string>(tmp);
		//uid
		add_http_params(param, "uid",rel_uid_ );
		str_sign += "uid="+rel_uid_;
		//
		str_sign += platform_config_.skey_;

		unsigned char out_put[16];
		CMD5 md5;
		md5.MessageDigest((const unsigned char*)str_sign.c_str(), str_sign.length(), out_put);
		std::string sign_key;
		for (int i = 0; i < 16; i++)
		{
			char aa[4] = {0};
			sprintf_s(aa, 4, "%02x", out_put[i]);
			sign_key += aa;
		}
		add_http_params(param, "sig", sign_key);

		std::string str_req = build_http_request(platform_config_.host_, 
			platform_config_.trade_param_ + param);

		request(str_req, 
			platform_config_.host_,
			platform_config_.port_);

		return ERROR_SUCCESS_0;
	}
	
private:


protected:

	void	handle_http_body(std::string str_body)
	{
			/*{"code":"000000","msg":"ok","obj":""}
			code:100003 消费失败
			code:100004 余额不足
			code:900002 请求已被处理
			code:900002 请求已被处理
			code:000000 成功
		*/

		player_ptr pp = the_service.get_player(uid_);

		if(!pp.get())
		{
			on_request_failed(http_business_error, "not find player by uid_!!!!");
			return;
		}

		vector<string>  res_map;

		parse_173_result(str_body.c_str(),"\r\n{:,}\"",res_map);
		msg_173_eggs_smash_ret msg_ret;
		msg_ret.code_ = 1;
		msg_ret.egg_ret_ = 0;
		msg_ret.cur_level_ = 0;
		msg_ret.op_type_   = op_type_;
		msg_ret.in_come_   = 0;
		msg_ret.out_lay_   = 0;
	
		//直接返回失败
		if( res_map.size() == 0 )
		{
			on_request_failed(http_business_error, str_body);
			pp->send_msg_to_client(msg_ret);
			return;
		}


		auto iter = res_map.begin();
	
		for(;iter != res_map.end(); ++ iter)
		{
			string str = *iter;

			if(str.compare("code") == 0)
			{
				++iter;
				if(iter == res_map.end())
					break;
				str = *iter;

				if(str.compare("100003")==0)
					msg_ret.code_ = 2;
				else if(str.compare("100004")==0)
					msg_ret.code_ = 3;
				else if(str.compare("900002")==0)
					msg_ret.code_ = 4;
				else if(str.compare("000000")==0)
					msg_ret.code_ = 0;
			}
			else if (str.compare("jinbi") == 0)
			{
				++iter;
				if(iter == res_map.end())
					break;
				str = *iter;
				
				pp->cur_jinbi = boost::lexical_cast<longlong>(str);
				
				
			}
		}
	
		if(msg_ret.code_ == 0)
		{   
			if(op_type_ == 0)
			{
				if(is_succeed_)
				{
					pp->cur_egg_level += 1;
					msg_ret.egg_ret_ = 1;
					
					//广播消息
					if(pp->cur_egg_level >=6)
					{
						EggParamMap::iterator  iter = the_service.the_config_.egg_param_map.find(pp->cur_egg_level);

						if(iter == the_service.the_config_.egg_param_map.end())
							return;

						broadcast_17173_msg(pp,0,iter->second.get_money);
					}
					
				}
				else
				{
					pp->cur_egg_level -= 2;
					if(pp->cur_egg_level < 0 )
						pp->cur_egg_level  = 0;
				}
				msg_ret.out_lay_ = out_lay_;
				the_service.the_config_.save_jinbi_alert(pp->uid_,pp->name_,ALERT_TYPE_LOSE,out_lay_);
			}
			else if (op_type_ == 1)
			{
				pp->old_egg_level = pp->cur_egg_level;
				pp->cur_egg_level = 0;
				msg_ret.in_come_ = in_come_;
				the_service.the_config_.save_jinbi_alert(pp->uid_,pp->name_,ALERT_TYPE_WIN,in_come_);
			}


			pp->save_db_egg_level();
			pp->save_db_egg_jinbi(op_type_);

			int gold = 0 ;
			if(op_type_ == 0)
				gold = out_lay_;
			else if (op_type_ ==1)
				gold = in_come_;
			pp->save_db_egg_data(op_type_,gold,is_succeed_);
		}
	
		//
		
	    the_service.the_config_.save_jinbi_alert(pp->uid_,pp->name_,ALERT_TYPE_WIN,100);
		msg_ret.cur_level_ = pp->cur_egg_level;
		pp->send_msg_to_client(msg_ret);

	}

	void	on_request_failed(int reason, std::string err_msg)
	{

		player_ptr pp = the_service.get_player(uid_);

		if (pp.get())
		{
			msg_173_eggs_smash_ret msg_ret;
			msg_ret.code_      = 6;
			msg_ret.egg_ret_   = 0;
			msg_ret.cur_level_ = 0;
			msg_ret.op_type_   = op_type_;
			msg_ret.in_come_   = 0;
			msg_ret.out_lay_   = 0;
			pp->send_msg_to_client(msg_ret);
		}
	}
};
