#include "msg_client.h"
#include "msg_server.h"

int send_game_make_ready::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	if(!pp->is_ready)
	{
		pp->is_ready = true;
	}

	msg_make_ready_res msg;
	msg.pos = pp->pos_;
	plogic->broadcast_msg(msg);
	
	//如果双方都准备，可以开始游戏
	plogic->inNewRound();
	return ERROR_SUCCESS_0;
}

int send_game_move_chess::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	
	//走棋
	plogic->move_chess(pp, pos_x, pos_y);
	return ERROR_SUCCESS_0;
}

//悔棋询问
int send_game_ask_undo::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->ask_undo(pp);
	return ERROR_SUCCESS_0;
}

//悔棋回复
int send_game_reply_undo::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->reply_undo(pp, result);
	return ERROR_SUCCESS_0;
}

//pass
int send_game_pass_chess::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->pass_chess(pp);
	return ERROR_SUCCESS_0;
}

//点目
int send_game_point_chess::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->point_chess(pp);
	return ERROR_SUCCESS_0;
}

//点目回复
int send_game_reply_point::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->reply_point_chess(pp, result);
	return ERROR_SUCCESS_0;
}

//认输
int send_game_give_up::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->give_up(pp);
	return ERROR_SUCCESS_0;
}

//强制数子
int send_game_compel_chess::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->compel_chess(pp);
	return ERROR_SUCCESS_0;
}

//求和询问
int send_game_ask_summation::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->ask_summation(pp);
	return ERROR_SUCCESS_0;
}

//求和回复
int send_game_reply_summation::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->reply_summation(pp, result);
	return ERROR_SUCCESS_0;
}

//设置点目
int send_set_point::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->set_point(pp, posX, posY, is_set);
	return ERROR_SUCCESS_0;
}

//完成点目
int send_submit_point::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	if(!pp->is_point)
		pp->is_point = true;

	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->submit_point(pp);
	return ERROR_SUCCESS_0;
}

//提交点目结果
int send_result_gopoint::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr plogic = pp->the_game_.lock();
	if(!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	plogic->result_point(pp, _result);
	return ERROR_SUCCESS_0;
}