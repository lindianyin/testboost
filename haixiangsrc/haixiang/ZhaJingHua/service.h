#pragma once
#include "game_service_base.h"
#include "utility.h"
#include "game_player.h"
#include "game_logic.h"
#include "simple_xml_parse.h"
#include "MemCacheClient.h"
#include "error_define.h"

class jinghua_service;
extern jinghua_service the_service;
using namespace nsp_simple_xml;

typedef boost::shared_ptr<jinghua_logic> logic_ptr;

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

class preset_present
{
public:
	int			pid_;				//奖项id
	int			rate_;			//概率
	int			factor_;		//赔率
	int			prior_;			//赔付优先级

	preset_present()
	{
		pid_ = 0;
		rate_ = 0;
		factor_ = 0;
		prior_ = 0;
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
	int				is_free_;						//是否为免费服务器
	int				wait_start_time_;
	int				do_random_time_;
	int				wait_end_time_;
	int				player_caps_;				//每个游戏最多人数
	longlong	max_win_, max_lose_;

	longlong	broadcast_set_;			//广播到平台的临界值
	int				shut_down_;					//是否处于关闭状态

	int				player_tax_;				//流水费
	longlong	currency_require_;	//最低金钱设置
	longlong	player_bet_lowcap_;	//最低下注额
	longlong	player_bet_cap_;		//玩家单注上限	
	longlong	total_bet_cap_;

	service_config()
	{
		restore_default();
		shut_down_ = 0;
	}
	int				load_from_file()
	{
		xml_doc doc;
		if (!doc.parse_from_file("zhajinghua.xml")){
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
			READ_XML_VALUE("config/", is_free_								);
		}
		return ERROR_SUCCESS_0;
	}
	void			restore_default()
	{
		service_config_base::restore_default();
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

		php_sign_key_ = "z5u%wi31^_#h=284u%keg+ovc+j6!69wpwqe#u9*st5os5$=j2";
		wait_start_time_ = 30000;
		do_random_time_	= 20000;
		wait_end_time_ = 20000;
		player_caps_ = 200;						//每个游戏最多人数
		max_lose_ = 99999999;
		max_win_ = 99999999;
		currency_require_ = 9000;
		is_free_ = 0;
		player_bet_lowcap_ = 100;	//最低下注额
		player_bet_cap_ = 10000;		//玩家单注上限	
		total_bet_cap_ = 10000000;
	}

	void			refresh();
	bool			need_update_config( service_config& new_config);
	bool			check_valid();
};

typedef boost::shared_ptr<remote_socket<jinghua_player>> remote_socket_ptr;

class  jinghua_service : public game_service_base<service_config, jinghua_player, remote_socket<jinghua_player>, logic_ptr, no_match_cls, scene_set>
{
public:
	jinghua_service();

	void							on_main_thread_update();
	void							on_sub_thread_update();
	virtual int				on_run();

	virtual int				on_stop();
	void							player_login(player_ptr pp);
	void							save_currency_to_local(std::string uid, longlong val);
	void							delete_from_local(std::string uid);
	logic_ptr					get_game(std::string uid);

	msg_ptr						create_msg(unsigned short cmd);
	int								handle_msg(msg_ptr msg, remote_socket_ptr from_sock);
};

extern 	db_delay_helper<std::string, int>& get_delay_helper();