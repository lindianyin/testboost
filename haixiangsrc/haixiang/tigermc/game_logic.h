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
#define			MAX_SEATS 200    //在要"logic_base.h"前
#include "logic_base.h"
class tiger_logic;
typedef boost::shared_ptr<tiger_logic> logic_ptr;
class preset_present;
class tax_info;
class msg_rand_result;

class bet_info
{
public:
	std::string		uid_;
	unsigned int	bet_count_;
};


struct scene_set;
class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;
class tiger_logic : public logic_base<tiger_logic>
{
public:
	int									id_;							//第几个游戏
	std::string					strid_;						
	std::vector<player_ptr>	is_playing_;			//游戏者
	bool								is_waiting_config_;
	scene_set*					scene_set_;
	match_ptr						the_match_;
	tiger_logic(int is_match);

	int							set_bet(player_ptr pp, int pid, longlong count);
	//本轮结束
	void						will_shut_down();
	int							player_login( player_ptr pp, int seat = -1);
	int							leave_game(player_ptr pp, int pos, int why = 0);
	void						save_match_result(player_ptr pp);
	int							get_playing_count(int typ = 0){return 0;}
	void						start_logic(){};
	void						stop_logic(){};
	void						broadcast_msg(msg_base<none_socket>& msg){};
	int							is_ingame(player_ptr pp)
	{
		auto itf = std::find(is_playing_.begin(), is_playing_.end(), pp);
		return itf != is_playing_.end();
	}
	void						join_player(player_ptr pp){};
};

