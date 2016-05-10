#pragma once
#include "game_service_base.h"
#include "utility.h"
#include "game_player.h"
#include "game_logic.h"
#include "simple_xml_parse.h"
#include "MemCacheClient.h"
#include "error_define.h"

class horse_racing_service;
extern horse_racing_service the_service;
using namespace nsp_simple_xml;

typedef boost::shared_ptr<horse_racing_logic> logic_ptr;

extern std::map<string, longlong>	g_debug_performance_total;	//每个函数执行时间累计
extern std::map<string, longlong>	g_debug_performance_count;
extern std::map<string, longlong>	g_debug_performance_avg;
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

#define  GAME_ID 2

class preset_present
{
public:
	int			pid_;				//奖项id
	int			rate_;			//概率
	int			factor_;		//赔率
	int			prior_;			//赔付优先级
	int			db_pid_;
	preset_present()
	{
		pid_ = 0;
		rate_ = 0;
		factor_ = 0;
		prior_ = 0;
		db_pid_ = 0;
	}
	bool	operator == (preset_present& pr);
};

class tax_info
{
public:
	longlong				bound_;
	unsigned int		tax_percent_;
};

//场次设定
struct scene_set
{
	int				id_;
	int				is_free_;						//是否免费场
	longlong	bet_cap_;						//下注上限
	longlong	gurantee_set_;			//最低金钱设置
	longlong	to_banker_set_;			//竞庄要求
	int				player_tax_;				//流水费
	longlong	max_win_;						//输赢上下限
	longlong	max_lose_;
	scene_set()
	{
		memset(this, 0, sizeof(scene_set));
	}
};

#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)

#define WRITE_XML_VALUE(p, n)\
	doc.add_node(p, #n, n)

class service_config : public service_config_base
{
public:
	map<int, preset_present>	presets_;
	map<int, unsigned int>		chips_;	//筹码设置
	int				wait_start_time_;
	int				do_random_time_;
	int				wait_end_time_;
	int				player_caps_;				//每个游戏最多人数
	longlong	max_win_, max_lose_;
	longlong	bet_cap_;
	int				benefit_banker_;
	longlong	broadcast_set_;			//广播到平台的临界值
	int				shut_down_;					//是否处于关闭状态
	int				riding_road_length_;//跑道长度
	int				riding_time_;				//赛马时间
	service_config()
	{
		restore_default();
		shut_down_ = 0;
	}
	int				load_from_file()
	{
		xml_doc doc;
		if (!doc.parse_from_file("HorseRacing.xml")){
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
		}
		return ERROR_SUCCESS_0;
	}
	void			restore_default()
	{
		db_host_ = "192.168.17.224";
		db_user_ = "root";
		db_pwd_ = "123456";
		db_port_ = 3306;
		db_name_ = "beauty_beast";

		accdb_host_ = "192.168.17.224";
		accdb_user_ = "root";
		accdb_pwd_ = "123456";
		accdb_port_ = 3306;
		accdb_name_ = "account_server";

		client_port_ = 15100;
		php_sign_key_ = "z5u%wi31^_#h=284u%keg+ovc+j6!69wpwqe#u9*st5os5$=j2";
		wait_start_time_ = 30000;
		do_random_time_	= 20000;
		wait_end_time_ = 20000;
		player_caps_ = 200;						//每个游戏最多人数
		riding_road_length_ = 5000;
		riding_time_ = 15000;
		bet_cap_ = 99999999;
		max_lose_ = 99999999;
		max_win_ = 99999999;
	}

	void			refresh();
	bool			need_update_config( service_config& new_config);
	bool			check_valid();
};

typedef boost::shared_ptr<remote_socket<horserace_player>> remote_socket_ptr;
class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;
class  horse_racing_service : public game_service_base<service_config, horserace_player, remote_socket<horserace_player>, logic_ptr, match_ptr, scene_set >
{
public:
	vector<logic_ptr>	the_golden_games_;
	bool							wait_for_config_update_;
	service_config		the_new_config_;

	vector<boost::shared_ptr<msg_cache_cmd_ret<mem_connector_ptr>>>	glb_goldrank_;
	vector<scene_set>	scenes_;
	horse_racing_service();

	void							on_main_thread_update();
	void							on_sub_thread_update();
	virtual int				on_run();

	virtual int				on_stop()
	{
		return ERROR_SUCCESS_0;
	}
	int								get_free_player_slot(logic_ptr exclude);

	msg_ptr						create_msg(unsigned short cmd);
	int								handle_msg(msg_ptr msg, remote_socket_ptr from_sock);
	void							player_login( player_ptr pp );
};

extern 	db_delay_helper<string, int>& get_delay_helper();



#define  GAME_ID_EGG       "1"    //砸蛋    
#define  GAME_ID_FBOOK     "2"    //四大名著
#define  GAME_ID_BGIRL     "10"   //四大美女
#define  GAME_ID_FRUIT     "11"   //水果
#define  GAME_ID_HORES     "12"   //赛马

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

		game_id_ = GAME_ID_HORES; 
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