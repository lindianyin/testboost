#include "game_logic.h"
#include "msg_server.h"
#include "telfee_match.h"


class bot_rejoin_game : public task_info
{
public:
	logic_ptr plogic;

	bot_rejoin_game(io_service& ios) : task_info(ios){}

	virtual	int	routine()
	{
		player_ptr pp = the_service.pop_onebot(plogic);
		if (pp.get())	{
			if(plogic->player_login(pp) != ERROR_SUCCESS_0){
				the_service.free_bots_.insert(std::make_pair(pp->uid_, pp));
			}
			else{
				the_service.players_.insert(std::make_pair(pp->uid_, pp));
				if (plogic->the_match_.get())	{
					plogic->the_match_->add_score(pp->uid_, 0);
				}
			}
		}
		plogic->is_adding_bot_ = 0;
		return routine_ret_break;
	}
};

int task_clock::routine()
{
	logic_ptr plogic = logic_.lock();
	plogic->scheduledTask();
	return routine_ret_continue;
}


GoGame_logic::GoGame_logic(int is_match)
{
	count_down = 0;
	the_state = NOT_START;
	force_number = false;
	the_clock = nullptr;
	//初始化棋盘信息
	the_board_.reset(new Board);
}

void GoGame_logic::start_logic()
{
	count_down = 0;
	the_state = NOT_START;

	//加入机器人
	bot_rejoin_game join(the_service.timer_sv_);
	join.plogic = shared_from_this();
	join.routine();
}

int GoGame_logic::player_login(player_ptr pp, int pos /* = -1 */)
{
	//当前位置不在可用范围内
	if(pos >= MAX_SEATS) return -1;
	//当前房间人数已满
	if(get_playing_count() == MAX_SEATS && pos == -1)return -1;

	pp->the_game_ = shared_from_this();
	//查询玩家游戏数据
	the_service.get_gogame_data(pp);

	//发送配置信息
	{
		msg_server_parameter msg;
		std::map<std::string, level_grading>::iterator it;
		int i = 0;
		for(it = the_service.grade_map.begin(); it != the_service.grade_map.end(); it++)
		{
			level_grading lg = it->second;
			msg.level[i] = lg.level;
			msg.min_score[i] = lg.min_score;
			msg.max_score[i] = lg.max_score;
			i++;
		}
		pp->send_msg_to_client(msg);
	}
	
	//发送玩家游戏数据
	{
		msg_player_game_data msg;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		msg.score = pp->score;
		msg.win_num = pp->win_total;
		msg.fail_num = pp->fail_total;
		msg.draw_num = pp->draw_total;
		pp->send_msg_to_client(msg);
	}

	//发送玩家进入房间消息
	{
		msg_prepare_enter msg;
		if (the_match_.get()){
			msg.match_id_ = the_match_->iid_;
		}
		else{
			msg.match_id_ = 0;
		}
		msg.extra_data1_ = 0;
		pp->send_msg_to_client(msg);
	}

	if(pos == -1)
	{
		for(int i = 0; i < MAX_SEATS; i++)
		{
			if (!is_playing_[i].get() || 
				(is_playing_[i].get() && is_playing_[i]->uid_ == pp->uid_)){
					is_playing_[i] = pp;
					pp->pos_ = i;
					break;
			}
		}

		//广播玩家坐下位置
		msg_player_add_seat msg;
		msg.pos_ = pp->pos_;
		COPY_STR(msg.uname_, pp->name_.c_str());
		COPY_STR(msg.uid_, pp->uid_.c_str());
		COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
		msg.score = pp->score;
		msg.win_num = pp->score;
		msg.fail_num = pp->score;
		msg.draw_num = pp->score;
		broadcast_msg(msg, pp->pos_);
	}else{
		is_playing_[pos] = pp;
		pp->pos_ = pos;
		//广播玩家坐下位置
		msg_player_add_seat msg;
		msg.pos_ = pp->pos_;
		COPY_STR(msg.uname_, pp->name_.c_str());
		COPY_STR(msg.uid_, pp->uid_.c_str());
		COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
		msg.score = pp->score;
		msg.win_num = pp->score;
		msg.fail_num = pp->score;
		msg.draw_num = pp->score;
		broadcast_msg(msg, pp->pos_);
	}
	return ERROR_SUCCESS_0;
}

//玩家进入
void GoGame_logic::join_player(player_ptr pp)
{
	//向玩家广播游戏场内玩家
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (is_playing_[i].get())
		{
			//玩家坐下位置
			{
				msg_player_add_seat msg;
				msg.pos_ = i;
				COPY_STR(msg.uname_, is_playing_[i]->name_.c_str());
				COPY_STR(msg.uid_, is_playing_[i]->uid_.c_str());
				COPY_STR(msg.head_pic_, is_playing_[i]->head_ico_.c_str());
				msg.score = is_playing_[i]->score;
				msg.win_num = is_playing_[i]->score;
				msg.fail_num = is_playing_[i]->score;
				msg.draw_num = is_playing_[i]->score;
				pp->send_msg_to_client(msg);
			}

			//先配先后手了，说明比赛已经开始了
			if(is_playing_[i]->offensive_move > 0)
			{
				//广播当前状态
				msg_assign_type msg;
				msg.pos = is_playing_[i]->pos_;
				msg.type = is_playing_[i]->offensive_move;
				pp->send_msg_to_client(msg);

			}else{
				//广播准备情况
				if(is_playing_[i]->is_ready)
				{
					msg_make_ready_res msg;
					msg.pos = is_playing_[i]->pos_;
					pp->send_msg_to_client(msg);
				}
			}
		}
	}

	//如果已经在比赛了，向玩家发送当前棋盘信息
	if(the_state != Game_state::NOT_START)
	{
		//黑子
		int c_count = 0;
		{
			msg_black_chess_data  msg;
			for(int i = 0; i < 19; i++)
			{
				msg.chess_data[i] = the_board_->getDataByLine(i, (int)Chess::ChessType::Black);
			}
			//获得有棋子的位置
			memset(msg.chess_num, 0, sizeof(msg.chess_num));
			for(int i = 0; i < 19; i++)
			{
				for(int j = 0; j < 19; j++)
				{
					Chess* _chess = the_board_->getChessByPoint(j,i, the_board_->chess_list);
					if(_chess && (_chess->getType() == Chess::Black || _chess->getType() == Chess::DeadBlack))
					{
						msg.chess_num[c_count] = (short)_chess->getStep();
						c_count++;

					}
				}
			}
			msg.chess_count = c_count;
			pp->send_msg_to_client(msg);
		}
		
		//白子
		c_count = 0;
		{
			msg_white_chess_data  msg;
			for(int i = 0; i < 19; i++)
			{
				msg.chess_data[i] = the_board_->getDataByLine(i, (int)Chess::ChessType::White);
			}
			//获得棋子位置
			memset(msg.chess_num, 0, sizeof(msg.chess_num));
			for(int i = 0; i < 19; i++)
			{
				for(int j = 0; j < 19; j++)
				{
					Chess* _chess = the_board_->getChessByPoint(j, i, the_board_->chess_list);
					if(_chess && (_chess->getType() == Chess::ChessType::White || _chess->getType() == Chess::ChessType::DeadWhite))
					{
						msg.chess_num[c_count] = (short)_chess->getStep();
						c_count++;
					}
				}
			}
			msg.chess_count = c_count;
			pp->send_msg_to_client(msg);
		}
	}

	//发送当前游戏状态
	{
		msg_board_data msg;
		msg.cur_status = (int)the_state;
		if(the_state == NOT_START)
		{
			msg.cur_time = 0;
			msg.cur_turn = 0;
		}else
		{
			msg.cur_time = the_board_->cur_timer;
			msg.cur_turn = the_board_->cur_turn;
		}
		msg.player_time = pp->time_out;
		msg.player_eat = pp->eat_number;
		msg.cur_count = the_board_->cur_step;
		player_ptr opp = getopponent(pp);
		if(opp.get())
		{
			msg.oppo_time = opp->time_out;
			msg.oppo_eat = opp->eat_number;
		}else{
			msg.oppo_time = 0;
			msg.oppo_eat = 0;
		}
		pp->send_msg_to_client(msg);
	}

	//回合切换
	{
		msg_round_turn msg;
		msg.status = the_state;
		msg.type = the_board_->cur_turn;
		msg.timer = the_board_->cur_timer;
		msg.count = the_board_->cur_step;
		pp->send_msg_to_client(msg);
	}

}

//玩家离开
void GoGame_logic::leave_game(player_ptr pp, int pos, int why /* = 0 */)
{
	msg_player_leave msg;
	msg.pos_ = pos;
	msg.why_ = why;
	broadcast_msg(msg);
	if(the_state != NOT_START || the_state != END)
	{
		game_result(pp->offensive_move, 1);//结算
	}
}

//玩家掉线
void GoGame_logic::player_lost_connection(player_ptr pp)
{
	//广播对手掉线
	msg_call_board msg;
	msg.cmd = Error_cmd::PLAYER_LOST_CONNECTION;
	broadcast_msg(msg, pp->pos_);
	
	if(the_state == GoGame_logic::POINT)
	{
		//如果其中一个玩家掉线且不在强制数子状态下，回到初始化数目
		if(!force_number)
		{
			the_board_->clearPoint();
			for(int i = 0; i < MAX_SEATS; i++)
			{
				is_playing_[i]->is_point = false;
			}

			//广播消息
			msg_inform_point_data msg;
			msg.type = 0;
			broadcast_msg(msg);
		}else{
			//如果进入强制数目阶段，直接显示结果
			game_result();
		}
	}
}




bool GoGame_logic::is_ingame( player_ptr pp )
{
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->uid_ == pp->uid_){
			return true;
		}
	}
	return false;
}

//逻辑时间线
int GoGame_logic::update(float dt)
{
	if(last_time.is_not_a_date_time())
	{
		last_time = posix_time::microsec_clock::local_time();
	}else
	{
		if((posix_time::microsec_clock::local_time() - last_time).total_seconds() < 1)
		{
			return 0;
		}else{
			last_time = posix_time::microsec_clock::local_time();
		}
	}

	//掉线的人一定时间内没有上，踢掉
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->is_connection_lost_ ==  true)
		{
			if ((boost::posix_time::microsec_clock::local_time() - is_playing_[i]->conn_lost_tick_).total_milliseconds() > the_service.the_config_.wait_reconnect)
			{
				//如果在比赛中，则进入结算
				if(the_state != NOT_START && the_state != END)
				{
					game_result(is_playing_[i]->offensive_move, 1);
				}else{
					leave_game(is_playing_[i], i, 0);
				}
			}
		}
	}
	return 0;
}

//发送消息到客户端
void GoGame_logic::broadcast_msg( msg_base<none_socket>& msg, int exclude_pos)
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (i == exclude_pos) continue;
		is_playing_[i]->send_msg_to_client(msg);
	}
}

int fake_telfee_score()
{
	return rand_r(20, 500);
}

//计划任务
void GoGame_logic::scheduledTask()
{
	//新回合开始
	if(count_down > 0)
	{
		count_down--;
		if(count_down == 0)startNewRound();		
	}

	if(the_state == GoGame_logic::IN_GAME)
	{
		//走棋阶段
		if(the_board_->cur_timer > 0)
		{
			for (int i = 0; i < MAX_SEATS; i++)
			{
				if (!is_playing_[i].get()) continue;
				if (is_playing_[i]->offensive_move == the_board_->cur_turn)
				{

					is_playing_[i]->time_out++;
					break;
				}
			}

			the_board_->cur_timer--;
			if(the_board_->cur_timer == 0)
			{
				player_ptr giveUp_player;
				for (int i = 0; i < MAX_SEATS; i++)
				{
					if (!is_playing_[i].get()) continue;
					if (is_playing_[i]->offensive_move == the_board_->cur_turn)
					{
						giveUp_player = is_playing_[i];
						break;
					}
				}
				if(giveUp_player.get())
				{
					give_up(giveUp_player);
				}
				//eatChessState();
			}
		}
	}else if(the_state == GoGame_logic::REMOVE_DEAD)
	{
		//吃子阶段
		if(the_board_->cur_timer > 0)
		{
			the_board_->cur_timer--;
			if(the_board_->cur_timer == 0)
			{
				moveChessState();
			}
		}
	}else if(the_state == GoGame_logic::POINT)
	{
		//如果是强制数子阶段，十秒后跳到结算状态
		if(force_number)
		{
			the_board_->cur_timer--;
			if(the_board_->cur_timer == 0)
			{

			}
		}
	}
}


//====================================================================
// 业务逻辑
//====================================================================

//进入新的一局游戏
void GoGame_logic::inNewRound()
{
	for(int i = 0; i < MAX_SEATS; i++)
	{
		//人数不满，不开
		if (!is_playing_[i].get()) return;
		//没人准备，不开
		if(!is_playing_[i]->is_ready) return;
	}

	//抽先手签
	int random = rand_r(100);
	int res = random % 2;
	player_ptr pp = is_playing_[res];
	if(pp.get())
	{
		pp->offensive_move = 1;
		//广播抽签结果
		msg_assign_type msg;
		msg.pos = pp->pos_;
		msg.type = pp->offensive_move;
		broadcast_msg(msg);

		if(res == 0 && is_playing_[1].get())
			is_playing_[1]->offensive_move = 2;
		else if(res == 1 && is_playing_[0].get())
			is_playing_[0]->offensive_move = 2;
	}

	//初始化棋盘信息
	the_board_.reset(new Board);
	//初始化玩家信息
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		is_playing_[i]->reset_player();
	}

	//开始游戏
	count_down = COUNT_DOWN;
	if(the_clock == nullptr)
	{
		the_clock = new task_clock(the_service.timer_sv_);
		the_clock->logic_ = shared_from_this();
		task_ptr ptask(the_clock);
		the_clock->schedule(1000);
	}
}

//新回合开始
void GoGame_logic::startNewRound()
{
	force_number = false;
	the_board_->cur_turn = 0;
	the_board_->cur_timer = the_service.the_new_config_.turn_time;
	the_board_->history = 0;
	the_board_->cur_step = 0;
	the_state = IN_GAME;
	turn_convert();
}

//轮数转换
void GoGame_logic::turn_convert()
{
	//如果是新的开始，黑先手
	if(the_board_->cur_turn == 0)
		the_board_->cur_turn = 1;
	else{
		if(the_board_->cur_turn == 1)
			the_board_->cur_turn = 2;
		else
			the_board_->cur_turn = 1;
	}

	player_ptr pp = getPlayerByType(the_board_->cur_turn);
	if(pp.get())
	{
		pp->is_pass = false;
	}

	msg_round_turn msg;
	msg.status = the_state;
	msg.type = the_board_->cur_turn;
	msg.timer = the_board_->cur_timer;
	msg.count = the_board_->cur_step;
	broadcast_msg(msg);
}


//走棋状态
void GoGame_logic::moveChessState()
{
	the_board_->cur_timer = the_service.the_new_config_.turn_time;
	the_board_->history++;
	the_state = IN_GAME;
	turn_convert();
}

//提子状态
void GoGame_logic::eatChessState(int type /* = 0 */, int posX /* = 0 */, int posY /* = 0 */)
{
	if(type == 0)
	{
		//直接回到走棋状态
		moveChessState();
	}else{
		//查询当前位置四周是否有对方的子
		int curType = type;
		int px = posX;
		int py = posY;

		//有子可以吃
		std::vector<Chess*> cList;
		if(the_board_->isEat(curType, px, py, cList))
		{
			if(cList.size() > 0)
			{
				msg_eat_chess msg;
				msg.type = curType == 1 ? 2 : 1;
				msg.count = cList.size();
				for(int i = 0; i < msg.count; i++)
				{
					Chess* &cs = cList[i];
					msg.posX[i] = cs->getPosX();
					msg.posY[i] = cs->getPosY();
					the_board_->submitEat(cs->getType(), cs->getPosX(), cs->getPosY());
				}
				broadcast_msg(msg);

				player_ptr pp = getPlayerByType(curType);
				pp->eat_number += cList.size();
				
				the_board_->cur_timer = TIMEWAIT;
				the_state = REMOVE_DEAD;
			}else{
				moveChessState();
			}
		}else{
			moveChessState();
		}
	}
}

//走棋
int GoGame_logic::move_chess(player_ptr pp, int posX, int posY)
{
	int refuse = 0;
	int why = 0;

	//pass状态取消
	pp->is_pass = false;

	//位置上已有子
	if(!the_board_->ChessIsEmpty(posX, posY))
	{
		refuse = 1;
		why = 1;

		//通知玩家当前位置上的棋子是什么
		Chess* cur_chess = the_board_->getChessByPoint(posX, posY, the_board_->chess_list);
		if(cur_chess)
		{
			msg_move_chess_res msg;
			msg.type = cur_chess->getType();
			msg.posX = posX;
			msg.posY = posY;
			msg.step = cur_chess->getStep();
			msg.refuse = refuse;
			msg.why = why;
			pp->send_msg_to_client(msg);
		}
		return 0;
	}

	//没有气
	if(!the_board_->isLive(pp->offensive_move, posX, posY))
	{
		refuse = 1;
		why = 2;

		Chess* cur_chess = the_board_->getChessByPoint(posX, posY, the_board_->chess_list);
		if(cur_chess)
		{
			msg_move_chess_res msg;
			msg.type = cur_chess->getType();
			msg.posX = posX;
			msg.posY = posY;
			msg.step = cur_chess->getStep();
			msg.refuse = refuse;
			msg.why = why;
			pp->send_msg_to_client(msg);
		}
		return 0;
	}

	//不是当前方走棋
	if(the_board_->cur_turn != pp->offensive_move)
	{
		refuse = 1;
		why = 3;

		Chess* cur_chess = the_board_->getChessByPoint(posX, posY, the_board_->chess_list);
		if(cur_chess)
		{
			msg_move_chess_res msg;
			msg.type = cur_chess->getType();
			msg.posX = posX;
			msg.posY = posY;
			msg.step = cur_chess->getStep();
			msg.refuse = refuse;
			msg.why = why;
			pp->send_msg_to_client(msg);
		}
		return 0;
	}
		
	//广播走棋
	if(refuse == 0)
	{
		if(the_board_->addChess(pp->offensive_move, posX, posY) == 0)
		{
			msg_move_chess_res msg;
			msg.type = pp->offensive_move;
			msg.posX = posX;
			msg.posY = posY;
			msg.step = the_board_->cur_step;
			msg.refuse = refuse;
			msg.why = why;
			broadcast_msg(msg);
			//进入提子判断
			eatChessState(pp->offensive_move, posX, posY);

			if(pp->is_undo > 0)
				pp->is_undo--;
		}
	}
	return 0;
}

//悔棋询问
int GoGame_logic::ask_undo(player_ptr pp)
{
	if(the_state == NOT_START || the_board_->cur_step <= 2)
	{
		msg_undo_res msg;
		msg.result = 2;
		pp->send_msg_to_client(msg);
		return -1;
	}

	if(pp->is_undo > 0)
	{
		msg_undo_res msg;
		msg.result = 3;
		pp->send_msg_to_client(msg);
		return -1;
	}

	if(pp->total_undo >= 3)
	{
		msg_undo_res msg;
		msg.result = 4;
		pp->send_msg_to_client(msg);
		return -1;
	}

	player_ptr opp = getopponent(pp);
	if(opp.get())
	{
		msg_opposite_undo msg;
		opp->send_msg_to_client(msg);
	}
	return 0;
}

//悔棋回复
int GoGame_logic::reply_undo(player_ptr pp, int result)
{
	player_ptr opp = getopponent(pp);
	if(!opp.get()) return -1;

	//必须在游戏中，且有前两轮情况下才能悔棋
	if(the_state == NOT_START || the_board_->cur_step <= 2)
	{
		msg_undo_res msg;
		msg.result = 2;
		opp->send_msg_to_client(msg);
	}
	
	if(result == 0)
	{
		//被拒绝,通知对手	
		msg_undo_res msg;
		msg.result = 0;
		opp->send_msg_to_client(msg);
	}
	else if(result == 1)
	{
		//执行悔棋
		recorder_ptr t_recorder(new BoardRecorder);
		while (the_board_->carry_out_undo(opp->offensive_move, t_recorder))
		{
			//广播客户端执行悔棋
			if(t_recorder->type == BoardRecorder::PASS)
				continue;

			msg_undo_res msg;
			msg.result = 1;
			msg.operate = (t_recorder->type == BoardRecorder::ADD) ? 2 : 1;
			msg.type = t_recorder->cType;
			msg.oPosX = t_recorder->PosX;
			msg.oPosY = t_recorder->PosY;
			msg.step = t_recorder->step;
			broadcast_msg(msg);
		}

		opp->is_undo = 2;
		opp->total_undo++;
	}
	return 0;
}

//pass
int GoGame_logic::pass_chess(player_ptr pp)
{
	player_ptr opp = getopponent(pp);

	//如果对方已经pass,进入结算
	if(opp->is_pass)
	{
		return game_result();
	}

	pp->is_pass = true;
	eatChessState();
	return 0;
}

//点目
int GoGame_logic::point_chess(player_ptr pp)
{
	if(the_state == NOT_START)
	{
		msg_point_chess_res msg;
		msg.result = 2;
		pp->send_msg_to_client(msg);
		return -1;
	}

	player_ptr opp = getopponent(pp);
	if(opp.get())
	{
		msg_opposite_point msg;
		opp->send_msg_to_client(msg);
	}
	return 0;
}

//点目回复
int GoGame_logic::reply_point_chess(player_ptr pp, int result)
{
	player_ptr opp = getopponent(pp);
	if(!opp.get()) return -1;

	//必须在游戏中，且有前两轮情况下才能悔棋
	if(the_state == NOT_START)
	{
		msg_point_chess_res msg;
		msg.result = 2;
		opp->send_msg_to_client(msg);
		return -1;
	}

	if(result == 0)
	{
		//被拒绝,通知对手	
		msg_point_chess_res msg;
		msg.result = 0;
		opp->send_msg_to_client(msg);
	}
	else if(result == 1)
	{
		//执行点目
		the_board_->cur_timer = 10;
		the_state = POINT;
		
		msg_round_turn msg;
		msg.status = the_state;
		msg.type = the_board_->cur_turn;
		msg.timer = the_board_->cur_timer;
		msg.count = the_board_->cur_step;
		broadcast_msg(msg);

		//获得当前点目结果
		std::list<Chess*> b_list = the_board_->goPoint(Chess::ChessType::Black);
		std::list<Chess*> w_list = the_board_->goPoint(Chess::ChessType::White);
		std::list<Chess*>::iterator it;
		Chess* _chess;
		for(it = b_list.begin(); it != b_list.end(); ++it)
		{
			_chess = *it;
			_chess->setType(Chess::MaybeBlack);
		}

		for(it = w_list.begin(); it != w_list.end(); ++it)
		{
			_chess = *it;
			_chess->setType(Chess::MaybeWhite);
		}

		if(force_number)
		{
			if(the_board_->caclulateResult())
			{
				msg_result_point_data msg;
				msg.fBlack = the_board_->fBlack;
				msg.fWhite = the_board_->fWhite;
				broadcast_msg(msg);

				for(int i = 0; i < MAX_SEATS; i++)
				{
					if (!is_playing_[i].get()) return -1;
					is_playing_[i]->is_point = false;
				}
			}
		}
	}
	return 0;
}

//认输
int GoGame_logic::give_up(player_ptr pp)
{
	if(!the_board_.get())
	{
		return -1;
	}

	//进入结算
	if(the_board_->caclulateResult())
	{
		game_result(pp->offensive_move);
	}
	return 0;
}

//强制数子
int GoGame_logic::compel_chess(player_ptr pp)
{
	if(!force_number)
		force_number = true;

	msg_coercive_number msg;
	msg.type = pp->offensive_move;
	broadcast_msg(msg);
	return 0;
}

//求和
int GoGame_logic::ask_summation(player_ptr pp)
{
	player_ptr opp = getopponent(pp);
	if(opp.get())
	{
		msg_opposite_summation msg;
		opp->send_msg_to_client(msg);
	}
	return 0;
}

//求和回复
int GoGame_logic::reply_summation(player_ptr pp, int result)
{
	player_ptr opp = getopponent(pp);
	if(!opp.get()) return -1;

	if(the_state == NOT_START)
	{
		//不在游戏中
		msg_summation_res msg;
		msg.result = 2;
		opp->send_msg_to_client(msg);
		return -1;
	}

	//被拒绝,通知对手	
	if(result == 0)
	{
		msg_summation_res msg;
		msg.result = 0;
		opp->send_msg_to_client(msg);
		return 0;
	}

	//和棋
	if(result == 1) game_result(3);
	return 0;
}


//------------------------------------------------------------------
//设置点目
//------------------------------------------------------------------
int GoGame_logic::set_point(player_ptr pp, int posX, int posY, int isSet)
{
	bool flag = (isSet == 1) ? false : true;
	the_board_->setPoint(posX, posY, pp->offensive_move, flag);
	
	msg_inform_point_data msg;
	msg.type = pp->offensive_move;
	msg.posX = posX;
	msg.posY = posY;
	msg.is_set = isSet;
	broadcast_msg(msg);

	return 0;
}

//------------------------------------------------------------------
//完成点目
//------------------------------------------------------------------
int GoGame_logic::submit_point(player_ptr pp)
{
	for(int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) return -1;
		if(!is_playing_[i]->is_point) return -1;
	}

	//结算数分,通知客户端
	if(the_board_->caclulateResult())
	{
		msg_result_point_data msg;
		msg.fBlack = the_board_->fBlack;
		msg.fWhite = the_board_->fWhite;
		broadcast_msg(msg);

		for(int i = 0; i < MAX_SEATS; i++)
		{
			if (!is_playing_[i].get()) return -1;
			is_playing_[i]->is_point = false;
		}
	}
	return 0;
}

//------------------------------------------------------------------
// 客户端提交点目结果
//------------------------------------------------------------------
int GoGame_logic::result_point(player_ptr pp, int result)
{
	pp->is_result_point = result;

	int _result = -1;
	for(int i = 0; i < MAX_SEATS; i++)
	{
		//如果其中一个人不在，则显示结果
		if (!is_playing_[i].get())
		{
			_result = 0;
			break;
		}

		//如果其中一个人还没确认，则等待
		if(is_playing_[i]->is_result_point == -1)return 0;

		if(is_playing_[i]->is_result_point > _result)
			_result = is_playing_[i]->is_result_point;
	}

	if(_result == 0)
	{
		//显示结果
		game_result();
	}else if(_result == 1)
	{
		//继续点目
		the_board_->clearPoint();
		for(int i = 0; i < MAX_SEATS; i++)
		{
			if (!is_playing_[i].get())
			{
				//如果其中有人离开，则显示结果

				return 0;
			}
			is_playing_[i]->is_point = false;
		}

		//广播消息
		msg_inform_point_data msg;
		msg.type = 0;
		broadcast_msg(msg);

	}else if(_result == 2)
	{
		//继续走棋
		the_board_->clearPoint();
		//返回到走棋状态
		the_state = IN_GAME;
		msg_round_turn msg;
		msg.status = the_state;
		msg.type = the_board_->cur_turn;
		msg.timer = the_board_->cur_timer;
		msg.count = the_board_->cur_step;
		broadcast_msg(msg);
	}
	return 0;
}


//------------------------------------------------------------------
// 结算
//------------------------------------------------------------------
int GoGame_logic::game_result(int is_lost, int is_flee)
{
	if(the_state == NOT_START)
	{
		stop_logic();
		return 0;
	}

	the_state = END;
	msg_round_turn msg;
	msg.status = the_state;
	msg.type = 0;
	msg.timer = 0;
	msg.count = 0;
	broadcast_msg(msg);

	player_ptr black_player;
	player_ptr white_player;

	for(int i = 0; i < MAX_SEATS; i++)
	{
		if (is_playing_[i].get())
		{
			if(is_playing_[i]->offensive_move == 1)
			{
				black_player = is_playing_[i];
			}else if(is_playing_[i]->offensive_move == 2)
			{
				white_player = is_playing_[i];
			}
		}
	}
	
	int theWin = 0;														//获胜方:0,和棋;1,黑胜;2,白胜
	int total_turn = the_board_->cur_step;		//总步数
	int black_score = 0;											//黑方获得分数
	int white_score = 0;											//白方获得分数
	float sBlack = the_board_->fBlack / 2;
	float sWhite = the_board_->fWhite / 2 + 3.75;
	int difference = sBlack - sWhite;					//分差

	switch (is_lost)
	{
	case 0://不判定
		{
			if(sBlack > sWhite)
			{
				theWin = 1;
			}else if(sBlack < sWhite)
			{
				theWin = 2;
			}else
			{
				theWin = 0;
			}
		}
		break;
	case 1://黑输
		{
			theWin = 2;
		}
		break;
	case 2://白输
		{
			theWin = 1;
		}
		break;
	case 3://和棋
		{
			theWin = 0;
		}
		break;
	}

	if(theWin == 0)
	{
		black_score = 0;
		white_score = 0;
	}else
	{
		if(total_turn < 40)
		{
			black_score = 0;
			white_score = 0;
		}else{
			
			if(abs(difference) >= 200)
			{
				if(black_player->score > white_player->score)
				{
					black_score = (theWin == 1) ? 0 : (theWin == 2) ? -10 : 0;
					white_score = (theWin == 1) ? -1 : (theWin == 2) ? 10 : 0;
				}else{
					black_score = (theWin == 1) ? 10 : (theWin == 2) ? -1 : 0;
					white_score = (theWin == 1) ? -10 : (theWin == 2) ? 0 : 0;
				}
			}else if(abs(difference) >= 100 && abs(difference) < 200)
			{
				if(black_player->score > white_player->score)
				{
					black_score = (theWin == 1) ? 1 : (theWin == 2) ? -10 : 0;
					white_score = (theWin == 1) ? -1 : (theWin == 2) ? 10 : 0;
				}else{
					black_score = (theWin == 1) ? 10 : (theWin == 2) ? -1 : 0;
					white_score = (theWin == 1) ? -10 : (theWin == 2) ? 1 : 0;
				}
			}else if(abs(difference) >= 50 && abs(difference) < 100)
			{
				if(black_player->score > white_player->score)
				{
					black_score = (theWin == 1) ? 10 - (int)(abs(difference) / 10) : (theWin == 2) ? (-1 * (int)(abs(difference) / 10)) : 0;
					white_score = (theWin == 1) ? (-1 * (10 - (int)(abs(difference) / 10))) : (theWin == 2) ? (int)(abs(difference) / 10) : 0;
				}else{
					black_score = (theWin == 1) ? (int)(abs(difference) / 10) : (theWin == 2) ? (-1 * (10 - (int)(abs(difference) / 10))) : 0;
					white_score = (theWin == 1) ? (-1 * (int)(abs(difference) / 10)) : (theWin == 2) ? 10 - (int)(abs(difference) / 10) : 0;
				}
			}else if(abs(difference) < 50)
			{
				if(black_player->score > white_player->score)
				{
					black_score = (theWin == 1) ? 5 : (theWin == 2) ? -5 : 0;
					white_score = (theWin == 1) ? -5 : (theWin == 2) ? 5 : 0;
				}else{
					black_score = (theWin == 1) ? 5 : (theWin == 2) ? -5 : 0;
					white_score = (theWin == 1) ? -5 : (theWin == 2) ? 5 : 0;
				}
			}else
			{
				black_score = 0;
				white_score = 0;
			}
		}
	}

	//如是强退，额外再扣10分
	if(is_flee == 1)
	{
		if(theWin == 2){
			black_score -= 10;
		}else if(theWin == 1)
		{
			white_score -= 10;
		}
	}

	if(black_player.get())
	{
		msg_result_data msg;
		msg.win_type = theWin;
		msg.total_turn = total_turn;
		msg.role_score = black_score;
		msg.oppo_score = white_score;
		msg.role_totle = black_player->score + black_score;
		msg.oppo_totle = white_player->score + white_score;
		black_player->send_msg_to_client(msg);
	}

	if(white_player.get())
	{
		msg_result_data msg;
		msg.win_type = theWin;
		msg.total_turn = total_turn;
		msg.role_score = white_score;
		msg.oppo_score = black_score;
		msg.role_totle = white_player->score + white_score;
		msg.oppo_totle = black_player->score + black_score;
		white_player->send_msg_to_client(msg);
	}


	if(black_player.get())
	{
		the_service.set_gogame_data(black_player->uid_, black_player->offensive_move, theWin, black_score);
	}
	
	if(white_player.get())
	{
		the_service.set_gogame_data(white_player->uid_, white_player->offensive_move, theWin, white_score);
	}
	
	stop_logic();
	return 0;
}


void GoGame_logic::stop_logic()
{
	the_state = END;
	for(int i = 0; i < MAX_SEATS; i++)
	{
		if(!is_playing_[i].get()) continue;
		is_playing_[i]->reset_temp_data();	
		is_playing_[i].reset();
	}

	//重置当前房间状态，等待新的玩家进入
	reset_logic();
}

void GoGame_logic::reset_logic()
{
	the_state = NOT_START;
	force_number = false;
	the_board_.reset(new Board);
	count_down = 0;
	if(the_clock != nullptr)
	{
		the_clock->cancel();
		the_clock = nullptr;
	}
}



/************************************************************************/
/* private                                                              */
/************************************************************************/
//获得对手
player_ptr GoGame_logic::getopponent(player_ptr pp)
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) return nullptr;
		if(i != pp->pos_)
		{
			return is_playing_[i];
		}
	}
	return nullptr;
}

//根据先后手获得玩家
player_ptr GoGame_logic::getPlayerByType(int type)
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if(is_playing_[i].get())
		{
			if(is_playing_[i]->offensive_move == type)
			{
				return is_playing_[i];
			}
		}
	}
	return nullptr;
}