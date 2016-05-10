#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "db_delay_helper.h"

int state_wait_start::enter()
{
	glb_log.write_log("state_wait_start::enter()!");
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		if (the_service.wait_for_config_update_){
			plogic->is_waiting_config_ = true;
		}
		//如果没有准备进行更新，则开始本场游戏
		if (!the_service.wait_for_config_update_)	{
			if(plogic->this_turn_start() != ERROR_SUCCESS_0){
				start_success_ = false;
			}
		}
		//暂停游戏，准备游戏配置更新
		else{
			timer_.expires_from_now(boost::posix_time::millisec(2000));
			timer_.async_wait(boost::bind(&state_wait_start::on_wait_config_expired, 
				boost::dynamic_pointer_cast<state_wait_start>(shared_from_this()),
				boost::asio::placeholders::error));
			return ERROR_SUCCESS_0;
		}
	}
	else{
		glb_log.write_log("warning, plogic is null in state_wait_start!");
	}
	if(!start_success_){
		timer_.expires_from_now(boost::posix_time::millisec(3000));
	}
	else{
		timer_.expires_from_now(boost::posix_time::millisec(1000));
	}
	timer_.async_wait(boost::bind(&state_wait_start::on_time_expired,
		boost::dynamic_pointer_cast<state_wait_start>(shared_from_this()),
		boost::asio::placeholders::error));
	state_machine::enter();
	return ERROR_SUCCESS_0;
}

void state_wait_start::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		if(start_success_)
			plogic->change_to_wait_bet();
		else
			plogic->change_to_wait_start();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_wait_start!");
	}
}

state_wait_start::state_wait_start() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_wait_start);
	start_success_ = true;
}

void state_wait_start::on_wait_config_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_wait_start();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_wait_start!");
	}
}

int state_do_random::enter()
{
	glb_log.write_log("state_do_random::enter()!");
	logic_ptr plogic = the_logic_.lock();
	timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.do_random_time_));
	timer_.async_wait(boost::bind(&state_do_random::on_time_expired,
		boost::dynamic_pointer_cast<state_do_random>(shared_from_this()),
		boost::asio::placeholders::error));
	if(plogic.get()){
		plogic->random_result();
	}
	state_machine::enter();		
	return ERROR_SUCCESS_0;
}

void state_do_random::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_gaming();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}
}

state_do_random::state_do_random() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_do_random);
}

unsigned int state_do_random::get_time_total()
{
	return the_service.the_config_.do_random_time_;
}

int state_rest_end::enter()
{
	glb_log.write_log("state_rest_end::enter()!");
	timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.wait_end_time_));
	timer_.async_wait(boost::bind(&state_rest_end::on_time_expired, 
		boost::dynamic_pointer_cast<state_rest_end>(shared_from_this()),
		boost::asio::placeholders::error));

	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->balance_result();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}
	state_machine::enter();
	return ERROR_SUCCESS_0;
}

void state_rest_end::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_wait_start();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}
}


state_rest_end::state_rest_end() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_rest_end);
}

static int get_id()
{
	static int gid = 1;
	return gid++;
}

jinghua_logic::jinghua_logic(int is_match)
{
	is_waiting_config_ = false;
	id_ = get_id();
	strid_ = get_guid();
	turn_ = 0;
	last_win_pos_ = -1;
	force_over_ = 0;
	scene_set_ = NULL;
	shuffle_cards_ = jinghua_card::generate();
}

void jinghua_logic::broadcast_msg( msg_base<none_socket>& msg, bool include_observer)
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		is_playing_[i]->send_msg_to_client(msg);
	}
	
	if (include_observer){
		auto it = observers_.begin();
		while ( it != observers_.end())
		{
			player_ptr pp = *it;		it++;
			pp->send_msg_to_client(msg);
		}
	}
}

void jinghua_logic::random_result()
{
	if (last_win_pos_ < 0 )	{
		next_pos_ = random_choose_start();
	}
	else
		next_pos_ = last_win_pos_;
	
	int indx = 0;
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->is_giveup_) continue;

		player_ptr pp = is_playing_[i];
		pp->turn_cards_.clear();
// 		if (i % 2 == 0){
// 			jinghua_card c;
// 			c.card_weight_ = 9;
// 			c.card_id_ = 13 * 3 + 8;
// 			pp->turn_cards_.push_back(c);
// 			c.card_weight_ = 6;
// 			c.card_id_ = 13 * 2 + 5;
// 			pp->turn_cards_.push_back(c);
// 			c.card_weight_ = 7;
// 			c.card_id_ = 13 * 0 + 6;
// 			pp->turn_cards_.push_back(c);
// 		}
// 		else{
// 			jinghua_card c;
// 			c.card_weight_ = 5;
// 			c.card_id_ = 13 * 3 + 4;
// 			pp->turn_cards_.push_back(c);
// 			c.card_weight_ = 8;
// 			c.card_id_ = 13 * 1 + 7;
// 			pp->turn_cards_.push_back(c);
// 			c.card_weight_ = 6;
// 			c.card_id_ = 13 * 1 + 5;
// 			pp->turn_cards_.push_back(c);
// 		}
		pp->turn_cards_.insert(pp->turn_cards_.begin(), shuffle_cards_.begin() + indx * 3, shuffle_cards_.begin() + (indx + 1) * 3);
		indx++;
	}






}
int jinghua_logic::random_choose_start()
{
	vector<player_ptr> vp;
	for (int i = 0; i < MAX_SEATS; i++){
		if(is_playing_[i].get() && !is_playing_[i]->is_giveup_){
			vp.push_back(is_playing_[i]);
		}
	}
	if (!vp.empty()){
		int r = rand_r(vp.size() - 1);
		return vp[r]->pos_;
	}
	return -1;
}

int jinghua_logic::set_bet( player_ptr pp, longlong count )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	if (count < 0)
		return GAME_ERR_NOT_ENOUGH_CURRENCY;

	longlong out_count = 0;
	get_today_win(pp->uid_, out_count);
	if ((out_count < 0 && abs(out_count) > the_service.the_config_.max_lose_)){
		return GAME_ERR_LOSE_CAP_EXCEED;
	}
	else if ((out_count > 0 && abs(out_count) > the_service.the_config_.max_win_)){
		return GAME_ERR_WIN_CAP_EXCEED;
	}

	if (current_state_->state_id_ != GET_CLSID(state_gaming))
		return GAME_ERR_CANT_BET_NOW;
	int ret = the_service.apply_cost(pp->uid_, count, out_count);
	if (ret != ERROR_SUCCESS_0){
		return ret;
	}

	pp->bet_ += count;
	
	msg_deposit_change2 msg;
	msg.pos_ = pp->pos_;
	msg.display_type_ = 0;
	msg.credits_ = out_count;
	broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}

void broadcast_player_leave(player_ptr pp);
int jinghua_logic::this_turn_start()
{
	bets_pool_ = 0;
	force_over_ = 0;
	jinghua_card::reset_and_shuffle(shuffle_cards_);
	last_bet_ = the_service.the_config_.player_bet_lowcap_;
	last_is_openset_ = 1;
	//重置上局游戏数据
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (is_playing_[i].get()){
			is_playing_[i]->reset_temp_data();
			int ret = can_continue_play(is_playing_[i]);
			if (ret != ERROR_SUCCESS_0){
				if (!is_playing_[i]->is_logout_)	{
					
					msg_player_leave msg2;
					msg2.pos_ = i;
					broadcast_msg(msg2);

					msg_change_to_observer msg;
					msg.reason_ = ret;
					is_playing_[i]->send_msg_to_client(msg);
					observers_.push_back(is_playing_[i]);
				}
				is_playing_[i].reset();
			}
		}
	}

	if (get_playing_count() < 2){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	else{
		turn_++;
		return ERROR_SUCCESS_0;
	}
}

void jinghua_logic::send_match_result_to_player(player_ptr pp_src, int winner, player_ptr pp_dest, int is_pk)
{
	msg_card_match_result msg;
	COPY_STR(msg.uid_, pp_src->uid_.c_str());
	msg.result_ = pp_src->match_result_.card_match_;
	msg.winner_pos_ = winner;
	msg.is_pk_ = is_pk;
	pp_dest->send_msg_to_client(msg);
}

void jinghua_logic::save_match_result(player_ptr pp)
{
	BEGIN_INSERT_TABLE("log_player_win");
	SET_STRFIELD_VALUE("game_id", lex_cast_to_str(strid_));
	SET_FIELD_VALUE("turn", turn_);
	SET_STRFIELD_VALUE("uid", pp->uid_);
	SET_FIELD_VALUE("win",pp->actual_win_);
	SET_FIELD_VALUE("pay", pp->bet_);
	string c;
	for (int i = 0 ; i < pp->turn_cards_.size(); i++)
	{
		c += lex_cast_to_str(pp->turn_cards_[i].card_id_) + ",";
	}
	SET_STRFIELD_VALUE("cards", c);
	SET_FIELD_VALUE("niuniu_point", pp->match_result_.card_match_);
	SET_FINAL_VALUE("tax", the_service.the_config_.player_tax_);
	EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
}

void jinghua_logic::balance_result()
{
	player_ptr winner;
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->turn_cards_.empty()) continue;
		//保存压注信息

		BEGIN_INSERT_TABLE("set_bets");
		SET_FIELD_VALUE("bet_count", is_playing_[i]->bet_);
		SET_FIELD_VALUE("turn", turn_);
		SET_STRFIELD_VALUE("uid", is_playing_[i]->uid_);
		SET_STRFINAL_VALUE("game_id", strid_);
		EXECUTE_NOREPLACE_DELAYED("",the_service.delay_db_game_);

		if (is_playing_[i]->is_giveup_) continue;
		is_playing_[i]->match_result_ = analyse_cards(is_playing_[i]->turn_cards_);
		if (!winner.get()){
			winner = is_playing_[i];
		}
		else{
			if (is_greater(is_playing_[i]->match_result_, winner->match_result_)){
				winner = is_playing_[i];
			}
		}
		glb_log.write_log("uid:%s, card:%d,%d,%d result:%d",
			is_playing_[i]->uid_.c_str(), 
			is_playing_[i]->turn_cards_[0].card_weight_,
			is_playing_[i]->turn_cards_[1].card_weight_,
			is_playing_[i]->turn_cards_[2].card_weight_,
			is_playing_[i]->match_result_.card_match_);
	}
	if (!winner.get()) return;
	glb_log.write_log("winner:%s", winner->uid_.c_str());
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->is_giveup_) continue;
		if (is_playing_[i]->turn_cards_.empty()) continue;

		for (int j = 0; j < MAX_SEATS; j++)
		{
			if (!is_playing_[j].get()) continue;
			if (is_playing_[j]->is_giveup_) continue;
			//发送player1的牌到player2
			send_card_to_player(is_playing_[i], is_playing_[j]);
			send_match_result_to_player(is_playing_[i], winner->pos_, is_playing_[j]);
		}
	}

	msg_card_match_result msg_r;
	COPY_STR(msg_r.uid_, "");
	msg_r.result_ = -1;
	msg_r.winner_pos_ = winner->pos_;
	broadcast_msg(msg_r);

	winner->actual_win_ = bets_pool_ - the_service.the_config_.player_tax_;
	
	if (winner->actual_win_ < 0){
		winner->actual_win_ = 0;
	}

	longlong out_count = 0;
	int ret = the_service.give_currency(winner->uid_, winner->actual_win_, out_count);

	msg_deposit_change2 msg;
	msg.pos_ = winner->pos_;
	msg.display_type_ = 1;
	msg.credits_ = winner->actual_win_;
	broadcast_msg(msg);

	msg.display_type_ = 0;
	msg.credits_ = out_count;
	broadcast_msg(msg);

	save_match_result(winner);
}

void jinghua_logic::will_shut_down()
{

}

void jinghua_logic::change_to_state( ms_ptr ms )
{
	if (current_state_.get()){
		current_state_->leave();
	}
	current_state_ = ms;
	current_state_->enter();
}

void			get_credits(msg_currency_change& msg, player_ptr pp, jinghua_logic* logic)
{
	msg.credits_ = the_service.get_currency(pp->uid_);
}

void jinghua_logic::player_login( player_ptr pp, int seat)
{
	pp->the_game_ = shared_from_this();

	msg_deposit_change msg_ds;
	msg_ds.room_id_ = id_;
	get_credits(msg_ds, pp, this);
	pp->send_msg_to_client(msg_ds);

	//发送游戏状态机
	{
		msg_state_change msg;
		msg.change_to_ = current_state_->state_id_;
		msg.time_left = current_state_->get_time_left();
		msg.time_total_ = current_state_->get_time_total();
		pp->send_msg_to_client(msg);
	}
	//坐位满了
	if (get_playing_count() == MAX_SEATS){
		observers_.push_back(pp);
		msg_change_to_observer msg;
		msg.reason_ = 0;
		pp->send_msg_to_client(msg);
	}
	else{
		if (seat == -1){
			for (int i = 0 ; i < MAX_SEATS; i++)
			{
				if (!is_playing_[i].get() && pp != is_playing_[i]){
					is_playing_[i] = pp;
					pp->pos_ = i;
					//广播玩家坐下位置
					msg_player_seat msg;
					msg.pos_ = i;
					COPY_STR(msg.uname_, pp->name_.c_str());
					COPY_STR(msg.uid_, pp->uid_.c_str());
					COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
					broadcast_msg(msg);

					msg_deposit_change2 msg2;
					msg2.pos_ = i;
					get_credits(msg2, pp, this);
					broadcast_msg(msg2);
					break;
				}
			}
		}
		else{
			if(seat >= 0 && seat < MAX_SEATS){
				is_playing_[seat] = pp;
				pp->pos_ = seat;
				msg_player_seat msg;
				msg.pos_ = seat;
				COPY_STR(msg.uname_, pp->name_.c_str());
				COPY_STR(msg.uid_, pp->uid_.c_str());
				COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
				pp->send_msg_to_client(msg);

				msg_deposit_change2 msg2;
				msg2.pos_ = seat;
				get_credits(msg2, pp, this);
				broadcast_msg(msg2);
			}
		}
	}
	//向玩家广播游戏场内玩家
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (is_playing_[i].get()){
			{
				//广播玩家坐下位置
				msg_player_seat msg;
				msg.pos_ = i;
				COPY_STR(msg.uname_, is_playing_[i]->name_.c_str());
				COPY_STR(msg.uid_, is_playing_[i]->uid_.c_str());
				COPY_STR(msg.head_pic_, is_playing_[i]->head_ico_.c_str());
				pp->send_msg_to_client(msg);
			}

			if (current_state_->state_id_ == GET_CLSID(state_wait_bet) && is_playing_[i]->is_ready_){
				msg_player_is_ready msg;
				msg.pos_ = i;
				pp->send_msg_to_client(msg);
			}

			{
				msg_deposit_change2 msg;
				msg.pos_ = i;
				get_credits(msg, is_playing_[i], this);
				pp->send_msg_to_client(msg);
			}
		}
	}
	if (pp->is_ready_){
		msg_player_is_ready msg;
		msg.pos_ = pp->pos_;
		pp->send_msg_to_client(msg);
	}

	if (current_state_->state_id_ == GET_CLSID(state_gaming)){
		msg_gaming_info msg;
		msg.cur_betcount_ = last_bet_;
		msg.cur_cardopen_ = last_is_openset_;
		msg.total_bets_ = bets_pool_;
		msg.cur_waitbet_ = query_inf.pos_;
		for (int i = 0, n = 0; i < MAX_SEATS; i++)
		{
			if (!is_playing_[i].get()) continue;
			msg.pla_inf_[n].pos_ = i;
			msg.pla_inf_[n].is_playing_ = !is_playing_[i]->turn_cards_.empty();
			msg.pla_inf_[n].card_open_ = is_playing_[i]->card_opened_;
			msg.pla_inf_[n].is_giveup_ = is_playing_[i]->is_giveup_;
			msg.pla_inf_[n].bet_ = is_playing_[i]->bet_;
			n++;
		}
		pp->send_msg_to_client(msg);
		if (!pp->turn_cards_.empty() && pp->card_opened_){
			send_card_to_player(pp, pp);
		}
	}

}

void jinghua_logic::leave_game( player_ptr pp, int pos, int why)
{
	pp->the_game_.reset();
	auto itf = std::find(observers_.begin(), observers_.end(), pp);
	if(itf != observers_.end()){
		observers_.erase(itf);
	}
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i] == pp){
			msg_player_leave msg;
			msg.pos_ = i;
			broadcast_msg(msg);
			is_playing_[i].reset();
		}
	}
}

void jinghua_logic::start_logic()
{
	change_to_wait_start();
}

int jinghua_logic::get_today_win(string uid, longlong& win )
{
	string sql = "select sum(win - pay) from log_player_win where uid = '" + uid +"' and time > date(now())";
	Query q(*the_service.gamedb_);
	if (q.get_result(sql) && q.fetch_row()){
		win = q.getbigint();
		return ERROR_SUCCESS_0;
	}
	else
		return SYS_ERR_SQL_EXE_FAIL;
}

int jinghua_logic::can_continue_play( player_ptr pp )
{
	if (pp->is_connection_lost_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	if (pp->is_logout_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	longlong c = the_service.get_currency(pp->uid_);
	if (c < the_service.the_config_.currency_require_){
		return GAME_ERR_NOT_ENOUGH_CURRENCY;
	}

	return ERROR_SUCCESS_0;
}

void jinghua_logic::send_card_to_player( player_ptr pp_src, player_ptr pp_dest)
{
	msg_cards msg;
	msg.pos_ = pp_src->pos_;
	for (int i = 0; i < pp_src->turn_cards_.size() && i < 3; i++)
	{
		msg.cards_[i] = pp_src->turn_cards_[i].card_id_;
	}
	pp_dest->send_msg_to_client(msg);
}

void jinghua_logic::stop_logic()
{
}

void jinghua_logic::pay_deposit()
{
	longlong out_count = 0;
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		
		//如果没有准备好，放弃该局
		if (!is_playing_[i]->is_ready_){
			is_playing_[i]->is_giveup_ = 1;
		}

		if (is_playing_[i]->is_giveup_) continue;

		int ret =	the_service.apply_cost(is_playing_[i]->uid_, the_service.the_config_.player_bet_lowcap_, out_count, false);
		
		msg_deposit_change2 msg;
		msg.pos_ = i;
		msg.display_type_ = 1;
		msg.credits_ = out_count;
		broadcast_msg(msg);

		is_playing_[i]->bet_ = the_service.the_config_.player_bet_lowcap_;
		is_playing_[i]->vbet_.push_back(the_service.the_config_.player_bet_lowcap_);
		bets_pool_ +=the_service.the_config_.player_bet_lowcap_;
	}
}

bool jinghua_logic::this_turn_overed()
{
	//如果玩家少于2个玩家
	int n = get_still_gaming_count();
	if (n < 2){
		return true;
	}
	//达到整局上限
	if (bets_pool_ >= the_service.the_config_.total_bet_cap_){
		return true;
	}

	int		not_giveup_count = 0;
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (!is_playing_[i]->is_giveup_)	{
			not_giveup_count++;
		}
	}

	//如果大于1人没放弃
	if (not_giveup_count <= 1){
		return true;
	}
	return force_over_;
}

bool jinghua_logic::query_next_bet()
{
	int	pos = get_next_valid_pos();
	if (pos < 0) return false;
	query_inf = query_bet();
	query_inf.uid_ = is_playing_[pos]->uid_;
	query_inf.pos_ = pos;
	next_pos_ = pos + 1;

	msg_query_bet msg;
	msg.pos_ = pos;
	broadcast_msg(msg);
	return true;
}

int jinghua_logic::get_next_valid_pos()
{
	for (int i = next_pos_; i < next_pos_ + MAX_SEATS; i++)
	{
		int pos = i % MAX_SEATS;
		if (!is_playing_[pos].get())		continue;
		if (is_playing_[pos]->is_giveup_) continue;
		if (is_playing_[pos]->turn_cards_.empty()) continue;
		return pos;
	}
	return -1;
}

void jinghua_logic::pk( int pos1, int pos2 )
{
	if(!is_playing_[pos1].get() || !is_playing_[pos2].get()) return;

	msg_player_pk msg;
	msg.pos1_ = pos1;
	msg.pos2_ = pos2;

	is_playing_[pos1]->match_result_ = analyse_cards(is_playing_[pos1]->turn_cards_);
	is_playing_[pos2]->match_result_ = analyse_cards(is_playing_[pos2]->turn_cards_);
	if(is_greater(is_playing_[pos1]->match_result_, is_playing_[pos2]->match_result_))	{
		is_playing_[pos2]->is_giveup_ = 1;
		msg.winner_ = pos1;
	}
	else{
		is_playing_[pos1]->is_giveup_ = 1;
		msg.winner_ = pos2;
	}
	
	broadcast_msg(msg);

	send_match_result_to_player(is_playing_[pos1], msg.winner_, is_playing_[pos2], true);
 	send_match_result_to_player(is_playing_[pos2], msg.winner_, is_playing_[pos1], true);
}

template<class service_t>
int state_machine<service_t>::enter()
{
	msg_state_change msg;
	msg.change_to_ = state_id_;
	msg.time_left = get_time_left();
	msg.time_total_	= get_time_total();
	boost::shared_ptr<service_t> plogic = the_logic_.lock();
	if(plogic.get())
		plogic->broadcast_msg(msg);
	return ERROR_SUCCESS_0;
}

int state_wait_bet::enter()
{
	timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.wait_start_time_));
	timer_.async_wait(boost::bind(&state_wait_bet::on_time_expired, 
		boost::dynamic_pointer_cast<state_wait_bet>(shared_from_this()),
		boost::asio::placeholders::error));

	timer2_.expires_from_now(boost::posix_time::millisec(100));
	timer2_.async_wait(boost::bind(&state_wait_bet::check_all_ready, 
		boost::dynamic_pointer_cast<state_wait_bet>(shared_from_this()),
		boost::asio::placeholders::error));

	state_machine::enter();	

	return ERROR_SUCCESS_0;
}

void state_wait_bet::on_time_expired( boost::system::error_code e )
{
	timer2_.cancel(e);

	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		//人数不够
		if (plogic->get_playing_count(true) < 2)	{
			plogic->change_to_wait_start();
		}
		else{
			plogic->pay_deposit();
			plogic->change_to_do_random();
		}
	}
}

state_wait_bet::state_wait_bet() :timer_(the_service.timer_sv_), timer2_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_wait_bet);
}

void state_wait_bet::check_all_ready( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		//人数不够
		if ((plogic->get_playing_count(true) == plogic->get_playing_count()) &&
			(plogic->get_playing_count(true) >= 2))	{
			plogic->pay_deposit();
			plogic->change_to_do_random();
		}
		timer2_.expires_from_now(boost::posix_time::millisec(100));
		timer2_.async_wait(boost::bind(&state_wait_bet::check_all_ready, 
			boost::dynamic_pointer_cast<state_wait_bet>(shared_from_this()),
			boost::asio::placeholders::error));
	}
}

void state_gaming::on_time_expired( boost::system::error_code e )
{
	count++;
	static DWORD aa = GetTickCount();
	DWORD da = GetTickCount() - aa;
	aa = GetTickCount();
	if (da > 500){
		glb_log.write_log("state_gaming::on_time_expired da = %d", da);
	}
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		//如果本局要结束了
		if (plogic->this_turn_overed())	{
			plogic->change_to_end_rest();
		}
		else{
			if (plogic->query_inf.replied_)	{
				count = 0;
				plogic->query_next_bet();

				timer_.expires_from_now(boost::posix_time::millisec(3000));
				timer_.async_wait(boost::bind(&state_gaming::on_time_expired, 
					boost::dynamic_pointer_cast<state_gaming>(shared_from_this()),
					boost::asio::placeholders::error));
			}
			//10秒内
			else {
				if(count >= 200){
					count = 0;
					player_ptr pp = the_service.get_player(plogic->query_inf.uid_);
					if (pp.get()){
						pp->is_giveup_ = 1;
					}
					msg_player_setbet msg;
					msg.max_setted_ = 0;
					msg.card_opened_ = 0;
					COPY_STR(msg.uid_, plogic->query_inf.uid_.c_str());
					plogic->broadcast_msg(msg);

					plogic->query_next_bet();
				}
				timer_.expires_from_now(boost::posix_time::millisec(100));
				timer_.async_wait(boost::bind(&state_gaming::on_time_expired, 
					boost::dynamic_pointer_cast<state_gaming>(shared_from_this()),
					boost::asio::placeholders::error));
			}
		}
	}
}

state_gaming::state_gaming() : timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_gaming);
	count = 0;
}

int state_gaming::enter()
{
	state_machine<jinghua_logic>::enter();
	timer_.expires_from_now(boost::posix_time::millisec(100));
	timer_.async_wait(boost::bind(&state_gaming::on_time_expired, 
		boost::dynamic_pointer_cast<state_gaming>(shared_from_this()),
		boost::asio::placeholders::error));
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->query_next_bet();
	}
	return ERROR_SUCCESS_0;
}
