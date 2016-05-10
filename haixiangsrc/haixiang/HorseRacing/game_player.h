#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "game_service_base.h"
#include "msg.h"
using namespace std;
class horse_racing_logic;
class horserace_player : public player_base<horserace_player, remote_socket<horserace_player>>
{
public:
	boost::weak_ptr<horse_racing_logic> the_game_;
	horserace_player()
	{
		credits_ = 0;
		is_connection_lost_ = false;
	}

	int						update();

	boost::shared_ptr<horse_racing_logic> get_game()
	{
		return the_game_.lock();
	}
	void					sync_account(longlong count);
	void					on_connection_lost();
};
typedef boost::shared_ptr<horserace_player> player_ptr;