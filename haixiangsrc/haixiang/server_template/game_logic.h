#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
#include "game_service_base.h"
#include "game_player.h"

#include "schedule_task.h"

class fishing_logic;
typedef boost::shared_ptr<fishing_logic> logic_ptr;
class preset_present;
class tax_info;

#define MAX_SEATS 5

struct scene_set;
class fishing_logic : public boost::enable_shared_from_this<fishing_logic>
{
public:
	int									id_;							//第几个游戏
	std::string					strid_;						
	player_ptr					is_playing_[MAX_SEATS];			//游戏者

	unsigned int				turn_;
	bool								is_waiting_config_;
	vector<task_ptr>		will_join_;
	scene_set*					scene_set_;						
	fishing_logic();

	void						start_logic();
	void						stop_logic();
	int							get_playing_count(int get_type = 0);

	void						broadcast_msg(msg_base<none_socket>& msg, bool include_observers = true);

	//本轮结束
	void						will_shut_down();
	int							player_login( player_ptr pp, int seat = -1);
	void						leave_game( player_ptr pp, int pos, int why = 0);
	bool						is_ingame(player_ptr pp);
	int							get_today_win(string uid, longlong& win);
};

