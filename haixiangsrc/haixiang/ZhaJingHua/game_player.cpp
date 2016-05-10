#include "game_player.h"
#include "service.h"
#include "msg_server.h"
int	jinghua_player::update()
{
	return ERROR_SUCCESS_0;
}

void jinghua_player::sync_account( longlong count )
{
	msg_currency_change msg;
	msg.credits_ = count;
	send_msg_to_client(msg);
}

void jinghua_player::on_connection_lost()
{
	is_connection_lost_ = true;
	string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
	the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);
}

