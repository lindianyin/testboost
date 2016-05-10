#include "game_player.h"
#include "service.h"
#include "msg_server.h"
#include "game_logic.h"

int	beauty_player::update()
{
	return ERROR_SUCCESS_0;
}

void beauty_player::sync_account( __int64 count )
{
	msg_currency_change msg;
	msg.credits_ = count;
	send_msg_to_client(msg);
}

void beauty_player::on_connection_lost()
{
	is_connection_lost_ = true;
	logic_ptr pgame = get_game();
	if (pgame.get()){
		pgame->leave_game(shared_from_this(), pos_);
	}
	if (!is_bot_){
		the_service.auto_trade_out(shared_from_this());
	}
	std::string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
	the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);
}
