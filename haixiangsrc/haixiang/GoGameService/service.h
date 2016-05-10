#pragma once
#include <vector>
#include "utility.h"
#include "simple_xml_parse.h"
#include "game_logic.h"
#include "game_service_base.h"
#include "service_config_base.h"

#define READ_CONFIG_DATA(s, k, v)\
	v = s.get_node(k)

#define READ_CONFIG_VALUE(s, v)\
	v = s->get_value(#v, v)

#define READ_CONFIG_VALUE_2(s, k, v)\
	v = s->get_value(k, v);

class GoGame_service;
extern GoGame_service the_service;
class GoGame_logic;

typedef boost::shared_ptr<GoGame_logic> logic_ptr;

//等级评定表
struct level_grading
{
	int level;
	int min_score;
	int max_score;
};

//玩家游戏数据
struct goplayer_data
{
	int score;
	int win_total;
	int fail_total;
	int draw_total;
};


class service_config : public service_config_base
{
public:

	//计划任务开启时间
	std::string					scheduledTaskTime;
	//计划周期时间
	std::string					period_time;

	//每局用时
	int turn_time;
	//哪一方先开局:0,随机;1,黑;2,白;
	int which_start;


	service_config()
	{

	}
	int		load_from_file(std::string path);
	bool	check_valid();
	void	refresh();
};


struct scene_set
{
	std::string id_;
	longlong		gurantee_set_;
	int					is_free_;						//是否免费场


	scene_set()
	{
		is_free_ = 1;
	}
};


class GoGame_service : public game_service_base<service_config, game_player, remote_socket<game_player>, logic_ptr, no_match, scene_set>
{
public:
	std::map<std::string, level_grading> grade_map;
	std::map<std::string, goplayer_data> goplayer_map;

	GoGame_service();

	void on_main_thread_update();
	void on_sub_thread_update();
	virtual int on_run();
	virtual int on_stop();
	player_ptr pop_onebot(logic_ptr plogic);
	msg_ptr create_msg(unsigned short cmd);
	int handle_msg(msg_ptr msg, remote_socket_ptr from_sock);

	void get_gogame_data(player_ptr pp);

	void set_gogame_data(std::string _uid, int offensive_move, int the_win, int get_score);
};
