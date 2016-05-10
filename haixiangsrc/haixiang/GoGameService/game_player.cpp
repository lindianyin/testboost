#include "game_player.h"
#include "service.h"
#include "msg_server.h"
#include "game_logic.h"

void game_player::reset_player()
{
	eat_number = 0;
	time_out = 0;
	total_undo = 0;
}


int game_player::update()
{
	return ERROR_SUCCESS_0;
}

void game_player::sync_account(__int64 count)
{
	msg_currency_change msg;
	msg.credits_ = count;
	send_msg_to_client(msg);
}

//玩家连接关闭
void game_player::on_connection_lost()
{
	is_connection_lost_ = true;
	conn_lost_tick_ = boost::posix_time::microsec_clock::local_time();
	logic_ptr pgame = get_game();
	if(pgame.get())
	{
		pgame->player_lost_connection(this->shared_from_this());
	}
}

//清空玩家信息
void game_player::reset_temp_data()
{
	pos_ = -1;
	is_point = false;
	is_result_point = -1;
	is_pass = false;
	offensive_move = 0;
	is_ready = false;
	time_out = 0;
	is_undo = 0;
	total_undo = 0;
	is_connection_lost_ = false;
	sex_ = 0;
	the_game_.reset();
}