#pragma once
#include "net_service.h"
#include <string>
#include <boost/weak_ptr.hpp>
#include "boost/shared_ptr.hpp"
#include "game_service_base.h"
#include "msg.h"
#include "poker_card.h"

using namespace std;
class jinghua_logic;
class jinghua_player : public player_base<jinghua_player, remote_socket<jinghua_player>>
{
public:
	vector<jinghua_card>		turn_cards_;
	boost::weak_ptr<jinghua_logic> the_game_;

	jinhua_match_result		match_result_;
	int								is_win_;
	int								is_giveup_;
	longlong					bet_;
	longlong					actual_win_;
	longlong					credits_;
	int								pos_;
	int								card_opened_;	//ÅÆÒÑ¿ª¹ý
	int								is_ready_;
	vector<longlong >	vbet_;
	jinghua_player()
	{
		is_logout_ = false;
		is_connection_lost_ = false;
		reset_temp_data();
	}

	int						update();
	void					reset_temp_data()
	{
		turn_cards_.clear();
		is_win_ = -1;
		match_result_ = jinhua_match_result();
		bet_ = 0;
		actual_win_ = 0;
		is_giveup_ = 0;
		card_opened_ = 0;
		is_ready_ = 0;
		vbet_.clear();
	}
	boost::shared_ptr<jinghua_logic> get_game()
	{
		return the_game_.lock();
	}
	void					sync_account(longlong count);
	bool					is_logout_;
	void					on_connection_lost();
};
typedef boost::shared_ptr<jinghua_player> player_ptr;