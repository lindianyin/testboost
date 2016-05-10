#include "game_player.h"
#include "service.h"
#include "msg_server.h"
#include "game_logic.h"
#include "game_service_base.hpp"
int	tiger_player::update()
{
	return ERROR_SUCCESS_0;
}

void tiger_player::on_connection_lost()
{
	is_connection_lost_ = true;
	std::string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
	the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);
	longlong  out_count = 0;
	//int ret = the_service.cache_helper_.account_cache_.send_cache_cmd(uid_, UID_LOGOUT, 0, out_count, false);
	logic_ptr plogic = the_game_.lock();
	if (plogic.get()){
		plogic->leave_game(shared_from_this(), pos_);
	}

	if (!is_bot_){
		//base::auto_trade_out(shared_from_this());
		the_service.auto_trade_out(shared_from_this());
	}

	the_service.remove_player(shared_from_this());
}
