#include "msg_server_common.h"
#include "msg_client.h"

#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "utility.h"
#include "error_define.h"
#include "error.h"

//ÉêÇëÏÂ×¯
int msg_cancel_banker_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	
	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	return pgame->apply_cancel_banker(pp);
}

int msg_apply_banker_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	return pgame->apply_banker(pp);
}

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

int msg_get_player_count::handle_this()
{
	msg_return_player_count msg;
	msg.player_count_ = rand_r(1,the_service.the_config_.player_count_random_range_) + the_service.players_.size() 
		              + the_service.bots_.size();

	player_ptr pp = from_sock_->the_client_.lock();
	if (pp.get())
		pp->send_msg_to_client(msg);
	
	return ERROR_SUCCESS_0;
}

int msg_get_lottery_record::handle_this()
{
	msg_return_lottery_record msg;
	std::queue<int> temp = the_service.lottery_record_;
	msg.count_ = temp.size();
	int i = 0;
	
	while (!temp.empty() && i < msg_return_lottery_record::MAX_RECORD)
	{
		msg.record_[i] = temp.front();
		temp.pop();
		++ i;
	}
	
	
	player_ptr pp = from_sock_->the_client_.lock();
	if (pp.get())
		pp->send_msg_to_client(msg);

	return ERROR_SUCCESS_0;
}