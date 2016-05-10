#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
#include "schedule_task.h"
//#include "fish_cloud.h"
#include <set>
#include "game_player.h"



class game_logic;
typedef boost::shared_ptr<game_logic> logic_ptr;

#define MAX_SEATS 4

struct scene_set;

class game_logic:public logic_base<game_logic>
{
public:
	int							id_;							//第几个游戏
	scene_set*					scene_set_;
	bool						is_waiting_config_;
	
	explicit		game_logic(int is_match);
	int				player_login( player_ptr pp, int seat = -1);
	int				get_playing_count(int get_type = 0);
	bool			is_ingame(player_ptr pp);
	void			start_logic();
	void			stop_logic();
	//why = 0,玩家主动退出游戏, 1 换桌退出游戏 2 游戏结束清场退出游戏
	void			leave_game( player_ptr pp, int pos, int why = 0);
	void			join_player(player_ptr p);

	void           broadcast_msg( msg_base<none_socket>& msg ) {};
};