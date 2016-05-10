#pragma once
#include "game_service_base.h"
#include "utility.h"
#include "simple_xml_parse.h"
#include "error_define.h"
#include <unordered_map>
#include <queue>
#include "service_config_base.h"
#include "platform_gameid.h"

class beauty_beast_service;
extern beauty_beast_service the_service;
using namespace nsp_simple_xml;
class beatuy_beast_logic;
typedef boost::shared_ptr<beatuy_beast_logic> logic_ptr;

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

class tax_info
{
public:
	longlong				bound_;
	unsigned int		tax_percent_;
};


#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)

#define WRITE_XML_VALUE(p, n)\
	doc.add_node(p, #n, n)

class service_config : public service_config_base
{
public:
	map<int, preset_present>	presets_;
	map<int, group_set>	groups_;
	map<int, unsigned int>		chips_;	//筹码设置
	map<std::string, std::string>	black_names_;
	int				wait_start_time_;
	int				do_random_time_;
	int				wait_end_time_;
	longlong	banker_deposit_;		//庄家保证金
	int				player_caps_;				//每个游戏最多人数
	longlong	sys_banker_deposit_, sys_banker_deposit_max_;//系统庄保证金
	int				max_banker_turn_;		//连续最大做庄次数
	int				min_banker_turn_;
	vector<tax_info> banker_tax_;	//对庄家抽税
	vector<tax_info> player_tax_;	//对闲家抽税
	std::string		sysbanker_name_;
	int				benefit_banker_, benefit_sysbanker_;		 //利庄概率
	int				sys_banker_tax_;		 //系统庄抽税
	longlong	max_win_, max_lose_;
	longlong	bet_cap_;
	int				set_bet_tax_;				//下注抽税
	int				player_tax2_;				//玩家闲家抽税
	int				enable_sysbanker_;
	int				player_banker_tax_;	//玩家庄家抽税
	longlong	broadcast_set_;			//广播到平台的临界值
	int				shut_down_;					//是否处于关闭状态
	int				smart_mode_;
	int				bot_enable_bet_;		//开启下注
	int				bot_enable_banker_;	//开启排庄
	int       have_banker_;
	int       player_count_random_range_; //在线人数随机数范围

	service_config()
	{
		sys_banker_deposit_ = 0;
		set_bet_tax_ = 0;
		restore_default();
		enable_sysbanker_ = true;
		shut_down_ = 0;
		smart_mode_ = 1;
		bot_enable_bet_ = 1;
		bot_enable_banker_ = 1;
		have_banker_ = 1;
		player_count_random_range_ = 1000;

		presets_.clear();
		groups_.clear();
		chips_.clear();
	}
	int				load_from_file(std::string path)
	{
		service_config_base::load_from_file(path);
		xml_doc doc;
		if (!doc.parse_from_file("niuniu.xml")){
			
		}
		else{
			READ_XML_VALUE("config/", have_banker_);
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
		wait_start_time_ = 10000;
		do_random_time_	= 10000;
		wait_end_time_ = 20000;
		banker_deposit_ = 1000000;		//庄家保证金
		player_caps_ = 200;						//每个游戏最多人数
		sys_banker_deposit_ = 100000;	//系统庄保证金
		max_banker_turn_ = 10;					//连续最大做庄次数

	}

	void			refresh();
	bool			need_update_config( service_config& new_config);
	bool			check_valid();

    int             get_max_rate_pid_by_prior(int prior);  //得到相同押注ID的最大赔率的pid
	int             get_min_rate_pid_by_prior(int prior);  //得到相同押注ID的最小赔率pid
	bool            is_pid(int pid);                       //是否是PID,如果不是PID，应该就是prior（押注ID）
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

class beauty_player;
typedef boost::shared_ptr<remote_socket<beauty_player>> remote_socket_ptr;
class  beauty_beast_service : public game_service_base<service_config, beauty_player, remote_socket<beauty_player>, logic_ptr, no_match, scene_set>
{
public:
	std::unordered_map<std::string, player_ptr>	bots_;
	std::queue<int> lottery_record_;   //中奖记录，只返回最近100次
	beauty_beast_service();

	void							on_main_thread_update();
	void							on_sub_thread_update(); 
	virtual int				on_run();

	virtual int				on_stop()
	{
		return ERROR_SUCCESS_0;
	}
	int								get_free_player_slot(logic_ptr exclude);
	void							player_login(player_ptr pp);
	msg_ptr						create_msg(unsigned short cmd);
	int								handle_msg(msg_ptr msg, remote_socket_ptr from_sock);
	vector<preset_present>		get_present_bysetbet(int setbet);
};
