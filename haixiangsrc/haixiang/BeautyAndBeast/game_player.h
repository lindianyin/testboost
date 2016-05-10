#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "msg.h"
#include "player_base.h"

using namespace std;
class beatuy_beast_logic;
class beauty_player : public player_base<beauty_player, remote_socket<beauty_player>, beatuy_beast_logic>
{
public:
	int						banker_turns_;
	bool					in_white_name_;
	boost::weak_ptr<beatuy_beast_logic> the_game_;
	beauty_player()
	{
		banker_turns_ = 0;
		in_white_name_ = 0;
		is_bot_ = false;
		is_connection_lost_ = false;
		sex_ = 0;
	}

	int						update();

	boost::shared_ptr<beatuy_beast_logic> get_game()
	{
		return the_game_.lock();
	}

	void					sync_account(__int64 count);
	void					on_connection_lost();
};
typedef boost::shared_ptr<beauty_player> player_ptr;