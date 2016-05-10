#include "game_player.h"
#include "service.h"
#include "msg_server.h"


int	horserace_player::update()
{
	return ERROR_SUCCESS_0;
}

void horserace_player::sync_account( longlong count )
{
	msg_currency_change msg;
	msg.credits_ = count;
	send_msg_to_client(msg);
}

void horserace_player::on_connection_lost()
{
	is_connection_lost_ = true;
	string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
	the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);

	logic_ptr pgame = get_game();
	if (pgame.get()){
		pgame->leave_game(boost::dynamic_pointer_cast<horserace_player>(shared_from_this()), pos_); 
	}
	if (!is_bot_){
		the_service.auto_trade_out(shared_from_this());
	}
}