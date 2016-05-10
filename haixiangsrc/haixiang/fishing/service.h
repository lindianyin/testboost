#pragma once
#include <vector>
#include "utility.h"
#include "simple_xml_parse.h"
#include "MemCacheClient.h"
#include "game_ids.h"
#include "fish_cloud.h"
#include "game_logic.h"
#include "shared_memory.h"

#include "game_service_base.h"
#include "service_config_base.h"

class fishing_service;
extern fishing_service the_service;
using namespace nsp_simple_xml;
typedef boost::shared_ptr<fishing_logic> logic_ptr;

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

struct control_rate
{
	int             rate_;
	longlong        max_gold_;
	longlong        min_gold_;

	control_rate()
	{
		memset(this,0,sizeof(this));
	}
};

struct inventory_control
{
	longlong  gold_;
	int       rate_;
	
	inventory_control()
	{
		memset(this,0,sizeof(this));
	}

};


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
	std::map<std::string, fish_config_ptr> the_fish_config_;
	std::map<std::string, path_config> the_path_;
	std::map<std::string,	cloud_config> the_cloud_config_;
};

#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)

#define WRITE_XML_VALUE(p, n)\
	doc.add_node(p, #n, n)

class service_config : public service_config_base
{
public:
	int				fish_max_amount;					//同屏鱼的最大显示数量


	map<std::string, int>	fish_rate;

	std::map<std::string, fish_config_ptr> the_fish_config_;
	std::map<std::string, path_config> the_path_;
	std::map<int,	int>				beauty_host_presentset_;
	std::vector<std::string>	broadcast_gold_;
	std::vector<std::string>	broadcast_telfee_;
	std::vector<std::string>	broadcast_boss_;
	std::vector<std::string>	broadcast_bomb_;
	int				enable_bot_;				//启用机器人
	double		rate_control_;		//全局概率控制
	longlong	lottery_max_;
	longlong	lottery_min_;
	double		lottery_rate_;		//彩金占赢利率
	int				rmb_rate_;

	longlong    inventory_;          //库存
	
	service_config();

	void			restore_default()
	{
		service_config_base::restore_default();
		fish_max_amount = 100;//默认同屏最多100条鱼
		wait_reconnect = 1000;
	}

	void			refresh();
	bool			need_update_config( service_config& new_config);
	bool			check_valid();
};

//美女主播
struct beauty_host
{
	beauty_host()
	{
		popular_ = 0;
	}
	std::string				uid_;
	longlong					iid_;
	int								avroom_id_;
	std::string				avserver_ip_;
	unsigned short		avserver_port_;
	int								is_online_;
	int								popular_;
	std::map<std::string, boost::weak_ptr<fishing_logic>> games_;
	std::vector<char>	screen_shoot_;
	std::vector<char>	temp_shoot_;
	std::string				host_name_;
	void							broadcast_msg(msg_base<none_socket>& msg)
	{
		auto it = games_.begin();
		while (it != games_.end())
		{
			auto logic = it->second.lock(); it++;
			if (logic.get()) {
				logic->broadcast_msg(msg);
			}
		}
	}
};	

struct gm_set
{
	std::string uid_;
	int					limitation_;
};

class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;
class  fishing_service : public game_service_base<service_config, fish_player, remote_socket<fish_player>, logic_ptr, match_ptr, scene_set>
{
public:
	typedef game_service_base<service_config, fish_player, remote_socket<fish_player>, logic_ptr, match_ptr, scene_set> base;
	std::map<longlong, beauty_host> beauty_hosts_;
	std::map<std::string, gm_set>		gm_sets_;
//	shared_object<longlong>					backup_user_coins_;
	std::map<std::string,control_rate>  control_rate_map_;
	std::vector<inventory_control>      inventory_control_vt;

	fishing_service();

	void							on_main_thread_update();
	void							on_sub_thread_update();
	virtual int				on_run();
	virtual int				on_stop();
	void							save_currency_to_local(std::string uid, longlong val, int is_free);
	void							delete_from_local(std::string uid, int is_free);
	player_ptr				pop_onebot(logic_ptr plogic);
	msg_ptr						create_msg(unsigned short cmd);
	int								handle_msg(msg_ptr msg, remote_socket_ptr from_sock);
	void							update_boss_path(std::list<Point>& path);
	void							update_fish_path(fish_ptr fh, std::list<Point>& path, logic_ptr logic);
	void							boss_runout();
	void							restore_random_spwan();
	void							on_beauty_host_login(player_ptr pp);
	match_ptr					create_match(int tp, int insid);
};

extern 	db_delay_helper<string, int>& get_delay_helper();
