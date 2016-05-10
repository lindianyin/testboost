#include "msg_client.h"
#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "utility.h"
#include "error_define.h"
#include "error.h"

int msg_set_bets_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	return pgame->set_bet(pp, pid_, present_id_);
}

int msg_setbet_his_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	string sql = "SELECT turn, Unix_Timestamp(time), pay, win, present_id, factor FROM log_player_win where uid = '" + pp->uid_ +
		"' order by time desc limit " + lex_cast_to_str(page_ * page_set_) + ", " + lex_cast_to_str(page_set_);
	Query q(*the_service.gamedb_);
	if (q.get_result(sql)){
		msg_my_setbet_history msg_his;
		int n = 0;
		while(q.fetch_row() && n < 10)
		{
			msg_his.turn_[n] = q.getval();
			msg_his.time_[n] = q.getval();
			msg_his.pay_[n] = q.getval();
			msg_his.win_[n] = q.getval();
			msg_his.result_[n] = q.getval();
			msg_his.factor_[n] = q.getval();
			n++;
		}
		pp->send_msg_to_client(msg_his);
	}
	return ERROR_SUCCESS_0;
}
