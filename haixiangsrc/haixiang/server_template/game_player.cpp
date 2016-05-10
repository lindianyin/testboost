#include "game_player.h"
#include "service.h"
#include "msg_server.h"
#include "game_logic.h"
extern longlong total_win_;
int	fish_player::update()
{
	return ERROR_SUCCESS_0;
}

void fish_player::on_connection_lost()
{
	is_connection_lost_ = true;
	string str_field = "delete from log_online_players where uid = '" + uid_ + "'";
	the_service.delay_db_game_.push_sql_exe(uid_ + "_logout", -1, str_field, true);
}
