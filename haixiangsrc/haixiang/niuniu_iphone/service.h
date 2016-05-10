#pragma once
#include <vector>
#include "utility.h"
//#include "game_player.h"
#include "simple_xml_parse.h"
#include "MemCacheClient.h"
#include "game_ids.h"
#include "game_logic.h"
#include "shared_memory.h"
#include "game_service_base.h"
#include "service_config_base.h"

class niuniu_service;
extern niuniu_service the_service;
using namespace nsp_simple_xml;
typedef boost::shared_ptr<niuniu_logic> logic_ptr;

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

	int				wait_start_time_;
	int				do_random_time_;
	int				wait_end_time_;
	std::string					slave_cache_ip_;
	unsigned short			slave_cache_port_;
	map<int, int>	random_present_;//最后大奖分配概率,金额
	int				base_score_;
	int				enable_bot_;				//启用机器人
	unsigned int	bot_banker_rate_;		//机器人竞庄机率
	std::vector<std::string>	broadcast_telfee_;
	service_config();
	//int				load_from_file();
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

		random_present_[4000] = 500;
		random_present_[3000] = 1000;
		random_present_[2000] = 1500;
		random_present_[990] = 2000;
		random_present_[10] = 4000;
	}

	void			refresh();
	bool			need_update_config( service_config& new_config);
	bool			check_valid();
};

typedef boost::shared_ptr<remote_socket<niuniu_player>> remote_socket_ptr;
class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;
class  niuniu_service : public game_service_base<service_config, niuniu_player, remote_socket<niuniu_player>, logic_ptr, match_ptr, scene_set>
{
public:
	niuniu_service();

	void							on_main_thread_update();
	void							on_sub_thread_update();
	virtual int				on_run();
	virtual int				on_stop();
	void							player_login(player_ptr pp);
	void							save_currency_to_local(string uid, longlong val, int is_free);
	void							delete_from_local(string uid, int is_free);
	int								restore_account(player_ptr pp, int is_free);
	player_ptr				pop_onebot(logic_ptr plogic);
	msg_ptr						create_msg(unsigned short cmd);
	int								handle_msg(msg_ptr msg, remote_socket_ptr from_sock);
	match_ptr					create_match(int tp, int insid);
};

extern 	db_delay_helper<string, int>& get_delay_helper();