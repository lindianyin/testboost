#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "db_delay_helper.h"
#include "schedule_task.h"
#include "boost/date_time/posix_time/conversion.hpp"
#include "telfee_match.h"
#include "game_service_base.hpp"

extern log_file<cout_output> glb_log;

using namespace boost;
using namespace boost::posix_time;
using namespace boost::gregorian;

std::map<string, niuniu_score> g_scores;
longlong	 g_cash_pool = 0;
longlong	 total_win_ = 0;
std::map<string, longlong>	g_todaywin;
std::map<string, longlong>	g_debug_performance_total;	//每个函数执行时间累计
std::map<string, longlong>	g_debug_performance_count;
std::map<string, longlong>	g_debug_performance_avg;

enum 
{
	cheat_bot_win,
	cheat_bot_lose,
	cheat_player_win,
	cheat_player_lose,
	cheat_no_cheat,
};

int state_wait_start::enter()
{
	state_machine::enter();
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		if (the_service.wait_for_config_update_){
			plogic->is_waiting_config_ = true;
		}
		plogic->this_turn_init();
		//暂停游戏，准备游戏配置更新
		if (the_service.wait_for_config_update_)	{
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

	timer_.expires_from_now(boost::posix_time::millisec(3000));

	timer_.async_wait(boost::bind(&state_wait_start::on_time_expired,
		boost::dynamic_pointer_cast<state_wait_start>(shared_from_this()),
		boost::asio::placeholders::error));

	return ERROR_SUCCESS_0;
}

void state_wait_start::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		if(plogic->this_turn_start() != ERROR_SUCCESS_0){
			start_success_ = false;
		}
		if(start_success_)
			//如果有庄，则开始
			if(plogic->the_banker_.get()){
				plogic->change_to_wait_bet();
			}
			//如果没有庄，则进入竞庄状态
			else{
				plogic->change_to_vote_banker();
			}
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

class bot_open_card_task : public task_info
{
public:
	player_ptr	pp;
	bot_open_card_task(io_service& ios) : task_info(ios)
	{

	}
	virtual	int	routine()
	{
		logic_ptr plogic = pp->the_game_.lock();
		if(plogic.get()){
			zero_match_result zm = calc_niuniu_point(pp->turn_cards_);
			pp->match_result_ = zm;
			plogic->broadcast_match_result(pp, pp->pos_);
		}
		return routine_ret_break;
	}
};

int state_do_random::enter()
{
	state_machine::enter();
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		int r = plogic->random_result();
		if (r != ERROR_SUCCESS_0){
			plogic->this_turn_over();
			plogic->change_to_wait_start();
		}
		else{

			for (int i = 0; i < MAX_SEATS; i++)
			{
				if (!plogic->is_playing_[i].get()) continue;
				if (!plogic->is_playing_[i]->is_bot_) continue;
				bot_open_card_task* ptask = new bot_open_card_task(the_service.timer_sv_);
				task_ptr task(ptask);
				ptask->pp = plogic->is_playing_[i];
				ptask->schedule(7000 + rand_r(4000)); 
			}

			if (plogic->the_banker_.get())	{
				plogic->the_banker_->not_set_bet_count_ = 0;
			}

			timer_.expires_from_now(boost::posix_time::millisec(500));
			timer_.async_wait(boost::bind(&state_do_random::on_update,
				boost::dynamic_pointer_cast<state_do_random>(shared_from_this()),
				boost::asio::placeholders::error));
		}
	}

	return ERROR_SUCCESS_0;
}

state_do_random::state_do_random() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_do_random);
}

int state_do_random::get_time_total()
{
	return the_service.the_config_.do_random_time_;
}

void state_do_random::on_update( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		if ( plogic->is_all_card_opened())	{
			timer_.expires_from_now(boost::posix_time::millisec(2000));
			timer_.async_wait(boost::bind(&state_do_random::on_time_expired,
				boost::dynamic_pointer_cast<state_do_random>(shared_from_this()),
				boost::asio::placeholders::error));
		}
		else if (get_time_left() < 500)	{
			plogic->change_to_end_rest();
		}
		else{
			timer_.expires_from_now(boost::posix_time::millisec(500));
			timer_.async_wait(boost::bind(&state_do_random::on_update,
				boost::dynamic_pointer_cast<state_do_random>(shared_from_this()),
				boost::asio::placeholders::error));
		}
	}
}

void state_do_random::on_time_expired( boost::system::error_code e )
{
	timer_.cancel(e);
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_end_rest();
	}
}

int state_rest_end::enter()
{
	state_machine::enter();
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){

		plogic->balance_result();

		timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.wait_end_time_));
		timer_.async_wait(boost::bind(&state_rest_end::on_time_expired, 
			boost::dynamic_pointer_cast<state_rest_end>(shared_from_this()),
			boost::asio::placeholders::error));
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}

	return ERROR_SUCCESS_0;
}

void state_rest_end::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->this_turn_over();
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

int state_rest_end::get_time_total()
{
	return the_service.the_config_.wait_end_time_;
}

static int get_id()
{
	static int gid = 1;
	return gid++;
}


class bot_quit_game : public task_info
{
public:
	player_ptr	pp;
	int					join_count_;

	bot_quit_game(io_service& ios) : task_info(ios)
	{

	}
	virtual	int	routine()
	{
		//如果还在当前游戏中.则退出当前游戏.
		//考虑如果是重新加入这个游戏怎么处理.
		if (pp->join_count_ == join_count_){
			pp->on_connection_lost();
		}
		return routine_ret_break;
	}
};


niuniu_logic::niuniu_logic(int ismatch)
{
	is_waiting_config_ = false;
	id_ = get_id();
	strid_ = get_guid();
	turn_ = 0;
	banker_turn = 0;
	shuffle_cards_ = niuniu_card::generate();
	scene_set_ = NULL;
	run_state_ = 0;
	is_match_game_ = ismatch;
}

void niuniu_logic::broadcast_msg( msg_base<none_socket>& msg, bool include_observer)
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

bool card_is_greater(niuniu_card& c1, niuniu_card& c2)
{
	return c1.card_id_ < c2.card_id_;
}

int niuniu_logic::random_result()
{
	//洗牌
	int bet_players = 0;

	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->bet_ > 0){
			is_playing_[i]->not_set_bet_count_ = 0;
			bet_players++;
		}
		else if(is_playing_[i] != the_banker_){
			is_playing_[i]->not_set_bet_count_++;

			msg_player_setbet msg_set;
			msg_set.max_setted_ = 0;
			COPY_STR(msg_set.uid_, is_playing_[i]->uid_.c_str());
			broadcast_msg(msg_set);

		}
		is_playing_[i]->turn_cards_.clear();
	}

	if (bet_players < 1){
		return -1;
	}

	banker_turn++;
	int indx = 0;
	niuniu_card::reset_and_shuffle(shuffle_cards_);
	int ret = the_service.cheat_win();
	glb_log.write_log("##cheat_win ret=%d", ret);

	if(1 == ret){//1系统必赢
		cheat_result(cheat_bot_win, indx);
		cheat_result(cheat_player_lose, indx);
	}
	else if(ret == -1){ //-1 系统必输
		cheat_result(cheat_player_win, indx);
		cheat_result(cheat_bot_lose, indx);
	}

	cheat_result(cheat_no_cheat, indx);
	
	msg_cards_complete msg;
	broadcast_msg(msg);

	if (the_banker_.get()){
		msg_deposit_change2 msg;
		msg.pos_ = the_banker_->pos_;
		msg.display_type_ = 2;
		if (is_match_game_){
			msg.credits_ = (double)the_banker_->free_guarantee_;
		}
		else
			msg.credits_ = (double)the_banker_->guarantee_;
		broadcast_msg(msg);
	}

	return ERROR_SUCCESS_0;
}

int niuniu_logic::set_bet( player_ptr pp, longlong count )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	if (count < 0)
		return GAME_ERR_NOT_ENOUGH_CURRENCY;

	//如果自己是庄家，不能下注
	if (the_banker_ == pp || !the_banker_.get()){
		return GAME_ERR_CANT_BET_NOW;
	}

	//已经下过注了
	if (pp->bet_ > 0){
		return GAME_ERR_CANT_BET_NOW;
	}

	if (current_state_->state_id_ != GET_CLSID(state_wait_bet))
		return GAME_ERR_CANT_BET_NOW;

	int nc = get_playing_count(0) - 1;
	if (nc <= 0) return GAME_ERR_CANT_BET_NOW;

	longlong out_count = 0;
	if (is_match_game_){
		longlong cc = the_banker_->free_guarantee_ / (/*5*/3 * nc);
		if (count > cc){	count = cc;	}
		if (count > 9000000)		count = 9000000;
		out_count = pp->free_gold_;
		if (out_count < (count * 3/*5*/)){
			return GAME_ERR_NOT_ENOUGH_CURRENCY;
		}

		int ret = pp->guarantee(count * 3/*5*/);
		if (ret != ERROR_SUCCESS_0){
			return ret;
		}
	}
	else{
		get_today_win(pp->uid_, out_count);
		if ((out_count < 0 && abs(out_count) > scene_set_->max_lose_)){
			return GAME_ERR_LOSE_CAP_EXCEED;
		}
		else if ((out_count > 0 && abs(out_count) > scene_set_->max_win_)){
			return GAME_ERR_WIN_CAP_EXCEED;
		}
		longlong cc = the_banker_->guarantee_ / (/*5*/3 * nc);
		if (count > cc){	count = cc;	}
		if (count > 9000000)		count = 9000000;
		out_count = the_service.cache_helper_.get_currency(pp->uid_);
		if (out_count < (count * 3/*5*/)){
			return GAME_ERR_NOT_ENOUGH_CURRENCY;
		}

		int ret = pp->guarantee(count * 3/*5*/);
		if (ret != ERROR_SUCCESS_0){
			return ret;
		}

	}
	
	pp->bet_ = count;

	msg_deposit_change2 msg;
	msg.pos_ = pp->pos_;
	msg.display_type_ = 0;
	msg.credits_ = (double)out_count - count * 3/*5*/;
	broadcast_msg(msg);

	msg.display_type_ = 2;
	msg.credits_ = count * 3/*5*/;
	broadcast_msg(msg);

	msg_player_setbet msg_set;
	COPY_STR(msg_set.uid_, pp->uid_.c_str());
	msg_set.max_setted_ = (double)count;
	broadcast_msg(msg_set);

	return ERROR_SUCCESS_0;
}

class bot_rejoin_game : public task_info
{
public:
	logic_ptr plogic;
	bot_rejoin_game(io_service& ios) : task_info(ios)
	{

	}

	virtual	int	routine()
	{
		player_ptr pp = the_service.pop_onebot(plogic);
		if (pp.get())	{
			if(plogic->player_login(pp) == ERROR_SUCCESS_0){
				auto itf = std::find(plogic->will_join_.begin(), plogic->will_join_.end(), shared_from_this());
				if(itf != plogic->will_join_.end()){
					plogic->will_join_.erase(itf);
				}

				if (plogic->the_match_.get()){
					plogic->the_match_->add_score(pp->uid_, 0);
				}
			}
		}
		return routine_ret_break;
	}
};

int niuniu_logic::this_turn_start()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("this_turn_start");
	int ret = ERROR_SUCCESS_0;
	if (get_playing_count() > 1 && get_playing_count(1) > 0){
		if (the_match_.get()){
			if (the_match_->state_ == telfee_match_info::state_wait_balance){
				turn_++;
				msg_cash_pool msg;
				msg.credits_ = (double)g_cash_pool;
				broadcast_msg(msg);
			}
			else{
				ret = SYS_ERR_CANT_FIND_CHARACTER;
			}
		}
		else{
			turn_++;
			msg_cash_pool msg;
			msg.credits_ = (double)g_cash_pool;
			broadcast_msg(msg);
		}
	}
	else{
		ret = SYS_ERR_CANT_FIND_CHARACTER;
	}
DEBUG_COUNT_PERFORMANCE_END()
	return ret;
}

bool		sort_win(player_ptr p1, player_ptr p2)
{
	if (p1->match_result_.calc_point_ == p2->match_result_.calc_point_){
		return p1->match_result_.niuniu_level_ >= p2->match_result_.niuniu_level_;
	}
	else{
		return p1->match_result_.calc_point_ > p2->match_result_.calc_point_;
	}
}
longlong		calc_will_pay(player_ptr the_banker, player_ptr pp)
{
	if (pp->match_result_.calc_point_ < 10){
		if (pp->match_result_.calc_point_ > 6)	{
				return pp->bet_ * 2;
		}
		else{
			return pp->bet_;
		}
	}
	else{
		int fac = 0;
		if (pp->match_result_.niuniu_level_ == 0){
			fac = 3;
		}
		else if (pp->match_result_.niuniu_level_ == 1)	{
			fac = 3;//4;
		}
		else if (pp->match_result_.niuniu_level_ == 2)	{
			fac = 3;//5;
		}
		return pp->bet_ * fac;
	}
	return ERROR_SUCCESS_0;
}
//结算庄家输 
int		niuniu_logic::balance_banker_lose(player_ptr the_banker, player_ptr pp, double fac1)
{
	if (pp->match_result_.calc_point_ < 10){
		if (pp->match_result_.calc_point_ > 6)	{
			the_banker->lose_bet((longlong)(pp->bet_ * 2 * fac1), the_banker) ;
			pp->win_bet((longlong)(pp->bet_ * 2 * fac1) - scene_set_->player_tax_, the_banker);
		}
		else{
			the_banker->lose_bet((longlong)(pp->bet_ * fac1), the_banker);
			pp->win_bet((longlong)(pp->bet_ * fac1) - scene_set_->player_tax_, the_banker);
		}
	}
	else{
		int fac = 0;
		if (pp->match_result_.niuniu_level_ == 0){
			fac = 3;
		}
		else if (pp->match_result_.niuniu_level_ == 1)	{
			fac = 4;
		}
		else if (pp->match_result_.niuniu_level_ == 2)	{
			fac = 5;
		}
		the_banker->lose_bet((longlong)(pp->bet_ * fac * fac1), the_banker);
		pp->win_bet((longlong)(pp->bet_ * fac * fac1) - scene_set_->player_tax_, the_banker);
	}
	g_cash_pool += scene_set_->player_tax_;
	return ERROR_SUCCESS_0; 
}

//结算庄家赢
int		niuniu_logic::balance_banker_win(player_ptr the_banker, player_ptr pp)
{
	if (the_banker->match_result_.calc_point_ < 10){
		if (the_banker->match_result_.calc_point_ > 6)	{
			the_banker->win_bet(pp->bet_ * 2 - scene_set_->player_tax_, the_banker);
			pp->lose_bet(pp->bet_ * 2, the_banker);
		}
		else{
			the_banker->win_bet(pp->bet_ - scene_set_->player_tax_, the_banker);
			pp->lose_bet(pp->bet_, the_banker) ;
		}
	}
	else{
		int fac = 0;
		if (the_banker->match_result_.niuniu_level_ == 0){
			fac = 3;
		}
		else if (the_banker->match_result_.niuniu_level_ == 1)	{
			fac = 4;
		}
		else if (the_banker->match_result_.niuniu_level_ == 2)	{
			fac = 5;
		}
		the_banker->win_bet(fac * pp->bet_ - scene_set_->player_tax_, the_banker);
		pp->lose_bet(fac * pp->bet_, the_banker);
	}
	g_cash_pool += scene_set_->player_tax_;
	return ERROR_SUCCESS_0;
}

int niuniu_logic::cheat_result(int cheat_type, int& indx)
{
	//作弊时玩家的点数 < 5
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		player_ptr pp = is_playing_[i];
		if (!pp->turn_cards_.empty()) continue;
		if (pp->bet_ < 1 && pp != the_banker_) continue;
		//如果作弊机器人
		if (cheat_type == cheat_bot_win || cheat_type == cheat_bot_lose){
			if (!pp->is_bot_) continue;
		}
		//如果作弊玩家
		else if(cheat_type == cheat_player_win || cheat_type == cheat_player_lose){
			if (pp->is_bot_) continue;
		}

		int counter = 0;
		zero_match_result zm;
		bool nobr = true;
		do{
			random_shuffle(shuffle_cards_.begin() + indx, shuffle_cards_.end());
			pp->turn_cards_.clear();
			pp->turn_cards_.insert(pp->turn_cards_.begin(), shuffle_cards_.begin() + indx, shuffle_cards_.begin() + indx + 5);

			if (pp->turn_cards_.size() != 5){
				glb_log.write_log("card dispatch is wrong, card size=%d", pp->turn_cards_.size());
			}
			counter++;
			if (cheat_type == cheat_bot_win || cheat_type == cheat_player_win){
				zm = calc_niuniu_point(pp->turn_cards_);
				nobr = zm.calc_point_ < 7 && counter < 500;
			}
			else if(cheat_type == cheat_bot_lose || cheat_type == cheat_player_lose){
				zm = calc_niuniu_point(pp->turn_cards_);
				nobr = zm.calc_point_ > 4 && counter < 500;
			}
			//不作弊
			else{
				nobr = false;
			}
		}while (nobr);

		broadcast_random_result(pp, i);
		indx += 5;
	}
	return ERROR_SUCCESS_0;
}

void	build_match_result_msg(msg_card_match_result& msg, player_ptr pp, int pos, zero_match_result& zm )
{
	msg.pos_ = pos;
	msg.niuniu_point_ = zm.calc_point_;
	msg.niuniu_level_ = zm.niuniu_level_;
	msg.card3_[0] = zm.c1.card_id_;
	msg.card3_[1] = zm.c2.card_id_;
	msg.card3_[2] = zm.c3.card_id_;
	msg.card2_[0] = zm.c4.card_id_;
	msg.card2_[1] = zm.c5.card_id_;
	msg.final_win_ = (double)pp->actual_win_;
	//如果是大牌
	if (pp->match_result_.c1.card_id_ == 53){
		msg.replace_id1_ = zm.c1.replaced_to_id_;
	}
	if (pp->match_result_.c1.card_id_ == 52){
		msg.replace_id2_ = zm.c1.replaced_to_id_;
	}
	if (pp->match_result_.c2.card_id_ == 53){
		msg.replace_id1_ = zm.c2.replaced_to_id_;
	}
	if (pp->match_result_.c2.card_id_ == 52){
		msg.replace_id2_ = zm.c2.replaced_to_id_;
	}
	if (pp->match_result_.c3.card_id_ == 53){
		msg.replace_id1_ = zm.c3.replaced_to_id_;
	}
	if (pp->match_result_.c3.card_id_ == 52){
		msg.replace_id2_ = zm.c3.replaced_to_id_;
	}
	if (pp->match_result_.c4.card_id_ == 53){
		msg.replace_id1_ = zm.c4.replaced_to_id_;
	}
	if (pp->match_result_.c4.card_id_ == 52){
		msg.replace_id2_ = zm.c4.replaced_to_id_;
	}
	if (pp->match_result_.c5.card_id_ == 53){
		msg.replace_id1_ = zm.c5.replaced_to_id_;
	}
	if (pp->match_result_.c5.card_id_ == 52){
		msg.replace_id2_ = zm.c5.replaced_to_id_;
	}
}

void niuniu_logic::broadcast_match_result(player_ptr pp,  int pos)
{
	msg_card_match_result msg;
	build_match_result_msg(msg, pp, pos, pp->match_result_);
	broadcast_msg(msg);
}

void niuniu_logic::save_match_result(player_ptr pp)
{
	BEGIN_INSERT_TABLE("log_player_win");
	SET_FIELD_VALUE("game_id", lex_cast_to_str(strid_));
	SET_FIELD_VALUE("turn", turn_);
	SET_FIELD_VALUE("uid", pp->uid_);
	SET_FIELD_VALUE("iid_", pp->iid_);
	SET_FIELD_VALUE("win",pp->actual_win_);
	SET_FIELD_VALUE("pay", pp->bet_);
	std::string c;
	for (unsigned int i = 0 ; i < pp->turn_cards_.size(); i++)
	{
		c += lex_cast_to_str(pp->turn_cards_[i].card_id_) + ",";
	}
	SET_FIELD_VALUE("cards", c);
	SET_FIELD_VALUE("is_banker", int(pp == the_banker_));
	SET_FIELD_VALUE("niuniu_point", pp->match_result_.calc_point_);
	SET_FIELD_VALUE("tax", scene_set_->player_tax_);
	SET_FINAL_VALUE("niuniu_level", pp->match_result_.niuniu_level_);
	the_service.delay_db_game_.push_sql_exe(pp->uid_ + "match_r", -1, str_field, true);
}

int get_score_prize(player_ptr pp);

class bot_take_prize_task : public task_info
{
public:
	player_ptr pp;
	bot_take_prize_task(boost::asio::io_service& ios) : task_info(ios)
	{

	}
	int		routine()
	{
		get_score_prize(pp);
		return routine_ret_break;
	}
};


void niuniu_logic::balance_result()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("balance_result");
	if (!the_banker_.get()) return;

	vector<player_ptr> winner;
	for (int i = 0; i < MAX_SEATS; i++)
	{
		player_ptr pp = is_playing_[i];
		if (!pp.get()) continue;
		//如果玩家放弃牌
		if (pp->match_result_.calc_point_ == -1 && !pp->turn_cards_.empty())	{
			zero_match_result zm;
			zm.c1 = pp->turn_cards_[0];
			zm.c2 = pp->turn_cards_[1];
			zm.c3 = pp->turn_cards_[2];
			zm.c4 = pp->turn_cards_[3];
			zm.c5 = pp->turn_cards_[4];
			zm = calc_niuniu_point(pp->turn_cards_);
			zm.calc_niuniu_level();
			pp->match_result_ = zm;
			broadcast_match_result(pp, pp->pos_);
		}
		
		if (pp->match_result_.calc_point_ >=0)	{
			//如果是庄家自己，则跳过
			if (pp == the_banker_) continue;

			pp->is_win_ = is_greater(pp->match_result_, the_banker_->match_result_);

			//如果闲家输了	
			if (pp->is_win_ == 0){
				balance_banker_win(the_banker_, pp);
				//玩家庄赢了,系统输了
				if (!the_banker_->is_bot_){
					the_service.add_stock(-std::abs(pp->actual_win_));
				}
				//玩家输了,系统赢了
				if (!pp->is_bot_){
					the_service.add_stock(std::abs(pp->actual_win_));
				}
			}
			else{
				winner.push_back(pp);
			}
		}
	}

	longlong will_pay = 0;

	for (unsigned int i = 0; i < winner.size(); i++)
	{
		player_ptr pp = winner[i];
		will_pay += calc_will_pay(the_banker_, pp);
	}

	float fac = 1.0;
	if (is_match_game_){
		//如果庄家钱不够赔，则按百分比赔
		if (will_pay > the_banker_->free_guarantee_ && will_pay >= 1){
			fac = the_banker_->free_guarantee_ / float(will_pay);
		}
	}
	else{
		//如果庄家钱不够赔，则按百分比赔
		if (will_pay > the_banker_->guarantee_ && will_pay >= 1){
			fac = the_banker_->guarantee_ / float(will_pay);
		}
	}

	//从大到小赔
	for (unsigned int i = 0; i < winner.size(); i++)
	{
		player_ptr pp = winner[i];
		balance_banker_lose(the_banker_, pp, fac);
		//玩家庄输了,系统赢了
		if (!the_banker_->is_bot_){
			the_service.add_stock(std::abs(pp->actual_win_));
		}
		//玩家赢了,系统输了
		if (!pp->is_bot_){
			the_service.add_stock(-std::abs(pp->actual_win_));
		}
	}
	
	for (int i = 0; i < MAX_SEATS; i++)
	{
		player_ptr pp = is_playing_[i];
		if (!pp.get()) continue;
		if (pp->turn_cards_.empty()) continue;

		if (!is_match_game_){
			if (pp->match_result_.calc_point_ == 0)	{
				g_scores[pp->uid_].score_ += 1;
			}
			else if (pp->match_result_.calc_point_ < 10)	{
				g_scores[pp->uid_].score_ += pp->match_result_.calc_point_ + 1;
			}
			else if (pp->match_result_.niuniu_level_ >= 0){
				g_scores[pp->uid_].score_ += 10 * (pp->match_result_.niuniu_level_ + 1);
			}

			msg_niuniu_score msg;
			msg.pos_ = i;
			msg.score = g_scores[pp->uid_].score_;
			msg.base_score_ = the_service.the_config_.base_score_;
			broadcast_msg(msg);

			save_match_result(pp);

			longlong out_count = 0;
			//如果不是庄家，则马上保存玩家的钱到账户服务器
			if (pp != the_banker_){
				the_service.restore_account(pp, scene_set_->is_free_);
			}
			else if(the_banker_.get()){
				the_service.save_currency_to_local(the_banker_->uid_, the_banker_->guarantee_, scene_set_->is_free_);
			}

			g_todaywin[pp->uid_] += pp->actual_win_;

			longlong lret = 0;
			the_service.cache_helper_.account_cache_.send_cache_cmd(pp->uid_ + KEY_CUR_TRADE_CAP, CMD_COST, scene_set_->player_tax_, lret);
		}
		else{
 			if (the_match_.get()){
				if (pp == the_banker_){
					if (pp->actual_win_ > 0)
						the_match_->add_score(pp->uid_, pp->actual_win_);
				}
				else{
					pp->free_gold_ += pp->free_guarantee_;
					pp->free_guarantee_ = 0;
					if (pp->actual_win_ > 0)
						the_match_->add_score(pp->uid_, pp->actual_win_);
				}
			}
		}
		//如果是机器人，安排时间领奖
		if (pp->is_bot_){
			bot_take_prize_task* ptask = new bot_take_prize_task(the_service.timer_sv_);
			ptask->pp = pp;
			task_ptr task(ptask);
			ptask->schedule(2000 + rand_r(10000));
		}
		
		{
			msg_deposit_change2 msg;
			msg.pos_ = i;
			msg.display_type_ = 1;
			msg.credits_ = (double)pp->actual_win_;
			broadcast_msg(msg);

			if (is_match_game_){
				if (pp == the_banker_){
					msg.display_type_ = 2;
					msg.credits_ = (double)the_banker_->free_guarantee_;
				}
				else{
					msg.display_type_ = 0;
					msg.credits_ = (double)pp->free_gold_;
				}
				broadcast_msg(msg);
			}
			else{
				if (pp == the_banker_){
					msg.display_type_ = 2;
					msg.credits_ = (double)the_banker_->guarantee_;
				}
				else{
					msg.display_type_ = 0;
					msg.credits_ = (double)pp->credits_;
				}
				broadcast_msg(msg);
			}
		}
	}
DEBUG_COUNT_PERFORMANCE_END()
}

void niuniu_logic::will_shut_down()
{
	auto itf = std::find(the_service.the_golden_games_.begin(),
		the_service.the_golden_games_.end(), shared_from_this());
	if (itf != the_service.the_golden_games_.end()){
		the_service.the_golden_games_.erase(itf);
	}
}

void niuniu_logic::change_to_state( ms_ptr ms )
{
	if (current_state_.get()){
		current_state_->leave();
	}
	current_state_ = ms;
	current_state_->enter();
}

void			get_credits(msg_currency_change& msg, player_ptr pp, niuniu_logic* logic)
{
	msg.credits_ = (double) the_service.cache_helper_.get_currency(pp->uid_);
}
extern int get_level(longlong& exp);
extern int glb_lvset[50];
class update_online_time : public task_info
{
public:
	boost::weak_ptr<niuniu_player> the_pla_;
	update_online_time(io_service& ios) : task_info(ios)
	{

	}
	int		 routine()
	{
		DEBUG_COUNT_PERFORMANCE_BEGIN("update_online_time");
		int ret = routine_ret_continue;
		player_ptr pp = the_pla_.lock();
		if (pp.get() && !pp->is_connection_lost_){
			total_win_ -= 500;
			longlong exp = the_service.cache_helper_.add_var(pp->uid_ + KEY_EXP, IPHONE_NIUINIU, 100);
			int lv = get_level(exp);
			if (pp->lv_ != lv){
				msg_levelup msg;
				msg.lv_ = lv;
				msg.exp_ = exp;
				if (lv > 29) lv = 29;
				msg.exp_max_ = glb_lvset[lv - 1];
				pp->send_msg_to_client(msg);
				pp->lv_ = lv;
			}
			else{
				msg_levelup msg;
				msg.lv_ = lv;
				msg.exp_ = exp;
				if (lv > 29) lv = 29;
				msg.exp_max_ = glb_lvset[lv - 1];
				pp->send_msg_to_client(msg);
			}
		}
		else{
			ret = routine_ret_break;
		}
		DEBUG_COUNT_PERFORMANCE_END();
		return ret;
	}
};


int niuniu_logic::player_login( player_ptr pp, int seat)
{
	//坐位满了
	if (get_playing_count() == MAX_SEATS){
		return -1;
	}
	else{
		msg_common_reply<none_socket> rpl;
		rpl.err_ = GAME_ERR_ENTER_GAME_SUCCESS;
		pp->send_msg_to_client(rpl);

		pp->the_game_ = shared_from_this();
		{
			msg_cash_pool msg;
			msg.credits_ = (double)g_cash_pool;
			pp->send_msg_to_client(msg);
		}
		if (is_match_game_){
			if (the_match_.get() && the_match_->current_schedule_.get()){
 				
				msg_time_left msg;
				msg.state_ = the_match_->state_;
				msg.left_ = the_match_->current_schedule_->time_left();
				pp->send_msg_to_client(msg);
				
				for (int i = 0; i < (int)the_match_->vscores_.size(); i++)
				{
					if (the_match_->vscores_[i].uid_ == pp->uid_ && !pp->is_connection_lost_){
						the_match_->vscores_.erase(the_match_->vscores_.begin() + i);
						break;
					}
				}

			}

			if(seat == -1) //只在重新进入时加钱，断线重连不加钱
				pp->free_gold_ = 1000000;
		}
		{
			msg_server_parameter msg;
			msg.is_free_ = scene_set_->is_free_;
			msg.min_guarantee_ = (int)scene_set_->gurantee_set_;
			msg.banker_set_ = (int)scene_set_->to_banker_set_;
			msg.low_cap_ = (int)(scene_set_->gurantee_set_ - scene_set_->player_tax_) / 50;
			msg.player_tax_ = scene_set_->player_tax_;
			msg.enter_scene_ = 1;
			pp->send_msg_to_client(msg);
		}
		pp->join_count_++;
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
					if (is_match_game_){
						msg2.credits_ = pp->free_gold_;
					}
					else{
						get_credits(msg2, pp, this);
					}
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
				if (is_match_game_){
					msg2.credits_ = pp->free_gold_;
				}
				else{
					get_credits(msg2, pp, this);
				}
				broadcast_msg(msg2);
				
				
				msg_player_hint hint_msg;
				hint_msg.hint_type_ = 1;
				pp->send_msg_to_client(hint_msg);
			}
		}

		if (pp->is_bot_)	{
			bot_quit_game* ptask = new bot_quit_game(the_service.timer_sv_);
			task_ptr task(ptask);
			ptask->pp = pp;
			ptask->join_count_ = pp->join_count_;
			ptask->schedule(15 * 60 * 1000 * rand_r(1, 4));
		}

		if (!pp->is_bot_)	{
			update_online_time* ptask = new update_online_time(the_service.timer_sv_);
			task_ptr task(ptask);
			ptask->the_pla_ = pp;
			ptask->schedule(60000);
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
			
			if(the_banker_.get() && is_playing_[i] == the_banker_){
				msg_promote_banker msg;
				COPY_STR(msg.uid_,the_banker_->uid_.c_str());
				msg.reason_ = 1;
				pp->send_msg_to_client(msg);
			}

			{
				msg_deposit_change2 msg;
				msg.pos_ = i;
				if (is_match_game_){
					msg.credits_ = is_playing_[i]->free_gold_;
				}
				else{
					get_credits(msg, is_playing_[i], this);
				}
				pp->send_msg_to_client(msg);

				msg.display_type_ = 2;
				if (is_match_game_){
					msg.credits_ = is_playing_[i]->free_guarantee_;
				}
				else{
					msg.credits_ = is_playing_[i]->guarantee_;
				}
				

				pp->send_msg_to_client(msg);
			}
			{
				msg_niuniu_score msg;
				msg.pos_ = i;
				msg.score = g_scores[is_playing_[i]->uid_].score_;
				msg.base_score_ = the_service.the_config_.base_score_;
				pp->send_msg_to_client(msg);
			}
			if (is_playing_[i]->bet_ > 0){
				msg_player_setbet msg;
				COPY_STR(msg.uid_, is_playing_[i]->uid_.c_str());
				msg.max_setted_ = (double)is_playing_[i]->bet_;
				pp->send_msg_to_client(msg);
			}

			if (!is_playing_[i]->turn_cards_.empty()){
				msg_cards msg;
				msg.pos_ = i;
				for (unsigned int ii = 0; ii < is_playing_[i]->turn_cards_.size() && ii < 5; ii++)
				{
					msg.cards_[ii] = is_playing_[i]->turn_cards_[ii].card_id_;
				}
				pp->send_msg_to_client(msg);
			}
			//广播开牌信息
			if( (pp->match_result_.calc_point_ != -1) && (pp->match_result_.niuniu_level_ != -1) )
				broadcast_match_result(is_playing_[i],i);
		}
	}

	//发送游戏状态机
	if (current_state_.get()){
		msg_state_change msg;
		msg.change_to_ = current_state_->state_id_;
		msg.time_left = current_state_->get_time_left();
		msg.time_total_ = current_state_->get_time_total();
		pp->send_msg_to_client(msg);

		if(current_state_->state_id_ == GET_CLSID(state_do_random))
		{
			msg_cards_complete msg2;
			pp->send_msg_to_client(msg2);
		}
	}

	return ERROR_SUCCESS_0;
}

void niuniu_logic::leave_game( player_ptr pp, int pos, int why )
{
	if (why == 0 || why == 1 || why == 4){
		if (why == 1 && the_match_){
			return;
		}

		if (current_state_->state_id_ != state_wait_start_id){
			player_ptr new_pp = pp->clone();
			new_pp->on_connection_lost();
			new_pp->from_socket_.reset();
			for (int i = 0; i < MAX_SEATS; i++)
			{
				if (is_playing_[i] == pp){
					is_playing_[i] = new_pp;
					break;
				}
			}

			if (the_banker_ == pp){
				the_banker_ = new_pp;
			}
		}
		else{
			is_playing_[pos].reset();
		}
		msg_player_leave msg_leave;
		msg_leave.pos_ = pp->pos_;
		msg_leave.why_ = why;
		pp->send_msg_to_client(msg_leave);

	}
	else{
		msg_player_leave msg;
		msg.pos_ = pos;
		msg.why_ = why;
		broadcast_msg(msg);

		if (is_match_game_){
			pp->free_gold_ += pp->free_guarantee_;
			pp->free_guarantee_ = 0;
		}
		else{
			if (pp->guarantee_ > 0 && pp == the_banker_){
				reset_to_not_banker();
			}
		}
		is_playing_[pos].reset();

		msg_player_leave msg_leave;
		msg_leave.pos_ = pp->pos_;
		msg_leave.why_ = why;
		pp->send_msg_to_client(msg_leave);
	}

	auto itf = std::find(observers_.begin(), observers_.end(), pp);
	if(itf != observers_.end()){
		observers_.erase(itf);
	}

	pp->the_game_.reset();
	pp->reset_temp_data();
	pp->guarantee_ = 0;
	pp->not_set_bet_count_ = 0;
	pp->pos_ = -1;
	pp->credits_ = 0;

	//只有主动退出游戏，或是因为长时间没有打炮被清出场，才要清除分数 
	//主动退出游戏也要清除比赛场积分
	if (why == 1 || why == 3 || why == 0){
		if (is_match_game_ && the_match_.get() && why != 2){
			for (int i = 0; i < (int)the_match_->vscores_.size(); i++)
			{
				if (the_match_->vscores_[i].uid_ == pp->uid_ && !pp->is_connection_lost_){
					the_match_->vscores_.erase(the_match_->vscores_.begin() + i);
					break;
				}
			}
		}
	}
	//不是换桌，不是机器人，退出游戏是兑出。
	//if (!pp->is_bot_ && why != 1 /*&& why != GAME_ERR_NOT_ENOUGH_BET*/){
	if (pp->is_copy_ && !the_service.get_player(pp->uid_).get()){
		the_service.auto_trade_out(pp);
	}
}

void niuniu_logic::start_logic()
{
	change_to_wait_start();
	run_state_ = 1;
}

int niuniu_logic::get_today_win(string uid, longlong& win )
{
	longlong n = g_todaywin[uid];
	win = n;
	return ERROR_SUCCESS_0;
}

int niuniu_logic::can_continue_play( player_ptr pp )
{
	if (pp->is_connection_lost_ && pp != the_banker_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	if (pp->not_set_bet_count_ > 1){
		return GAME_ERR_NOT_ENOUGH_BET;
	}

	if (!is_match_game_){
		if (pp == the_banker_ && pp->guarantee_ < scene_set_->gurantee_set_){
			return GAME_ERR_NOT_ENOUGH_CURRENCY;
		}
		else if(pp != the_banker_){
			longlong c = the_service.cache_helper_.get_currency(pp->uid_);
			if (c < scene_set_->gurantee_set_){
				return GAME_ERR_NOT_ENOUGH_CURRENCY;
			}
		}
	}
	else{
		if (pp == the_banker_ && pp->free_guarantee_ < scene_set_->gurantee_set_){
			return GAME_ERR_NOT_ENOUGH_CURRENCY;
		}
		else if(pp != the_banker_){
			if (pp->free_gold_ < scene_set_->gurantee_set_){
				return GAME_ERR_NOT_ENOUGH_CURRENCY;
			}
		}
	}
	//如果机器人是庄家，不要踢出
	if (get_playing_count() == MAX_SEATS && pp->is_bot_ && pp != the_banker_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	if (pp->is_bot_ && !the_service.the_config_.enable_bot_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	//只有机器的人游戏，机器人退出去
	if (pp->is_bot_ && get_playing_count(1) == 0){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	//如果报名了比赛场，而且不是庄家，进入比赛场
	if (pp->agree_join_ == 2 && pp != the_banker_)
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (the_service.the_config_.shut_down_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	return ERROR_SUCCESS_0;
}

class banker_rsp_task : public task_info
{
public:
	player_ptr pp;
	banker_rsp_task(boost::asio::io_service& ios):task_info(ios)
	{

	}
	int	 routine()
	{
		logic_ptr pgame = pp->the_game_.lock();
		if (!pgame.get())
			return routine_ret_break;

		int agree = rand_r(10000) < the_service.the_config_.bot_banker_rate_ ? 1 : 0; //70%机率做庄
		for (unsigned int i = 0 ; i < pgame->query_bankers_.size(); i++)
		{
			if (pgame->query_bankers_[i].uid_ == pp->uid_){
				pgame->query_bankers_[i].replied_ = agree;
			}
		}
		msg_query_banker_rsp msg;
		msg.agree_ = agree;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		pgame->broadcast_msg(msg);
		return routine_ret_break;
	}
};


void niuniu_logic::query_for_become_banker()
{
	msg_query_to_become_banker msg;
	for (int i = 0 ; i < MAX_SEATS ;i++)
	{
		if (!is_playing_[i].get()) continue;
		longlong out_count = the_service.cache_helper_.get_currency(is_playing_[i]->uid_);
		if (out_count < scene_set_->to_banker_set_) continue;
		msg.pos_ = i;
		broadcast_msg(msg);

		query_banker_info qi;
		qi.uid_ = is_playing_[i]->uid_;
		query_bankers_.push_back(qi);

		if (is_playing_[i]->is_bot_){
			banker_rsp_task* task = new banker_rsp_task(the_service.timer_sv_);
			task->pp = is_playing_[i];
			task_ptr ptask(task);
			task->schedule(500 + rand_r(1500));
		}
	}
}

bool niuniu_logic::is_all_query_replied()
{
	for (unsigned int i = 0; i < query_bankers_.size(); i++){
		if (query_bankers_[i].replied_ < 0)	
			return false;
	}
	return true;
}

int niuniu_logic::promote_banker( player_ptr p, int reason )
{
	if (is_match_game_){
		longlong bset = 0;
		int ret = p->guarantee(bset, true, false);
		if (ret == ERROR_SUCCESS_0){
			the_banker_ = p;
			msg_promote_banker msg;
			COPY_STR(msg.uid_,p->uid_.c_str());
			msg.reason_ = reason;
			broadcast_msg(msg);
		}
		else{
		}
	}
	else{
		longlong c = the_service.cache_helper_.get_currency(p->uid_);
		longlong bset = std::min<longlong>(c, scene_set_->to_banker_set_);
		int ret = p->guarantee(bset, true, false);
		if (ret == ERROR_SUCCESS_0){
			the_service.save_currency_to_local(p->uid_, p->guarantee_, scene_set_->is_free_);
			the_banker_ = p;
			msg_promote_banker msg;
			COPY_STR(msg.uid_,p->uid_.c_str());
			msg.reason_ = reason;
			broadcast_msg(msg);
		}
		else{
			glb_log.write_log("warning!!!, promote_banker falied on ret = %d", ret);
			return ret;
		}
	}

	return ERROR_SUCCESS_0;
}

void niuniu_logic::broadcast_random_result( player_ptr pp, int pos )
{
	msg_cards msg;
	msg.pos_ = pos;
	for (unsigned int i = 0; i < pp->turn_cards_.size() && i < 5; i++)
	{
		msg.cards_[i] = pp->turn_cards_[i].card_id_;
	}
	broadcast_msg(msg);
}

void niuniu_logic::stop_logic()
{
	run_state_ = 2;
	if (!is_match_game_){
		if (the_banker_.get()){
			if(the_service.restore_account(the_banker_, scene_set_->is_free_) == ERROR_SUCCESS_0){
				the_service.delete_from_local(the_banker_->uid_, scene_set_->is_free_);
			}
		}
	}
	else{
		for (int i = 0 ; i < MAX_SEATS; i++)
		{
			if (!is_playing_[i].get()) continue;
			leave_game(is_playing_[i], i, 2);
		}
	}
}

int niuniu_logic::random_choose_banker( int reason )
{
	vector<player_ptr> vp;
	for (int i = 0; i < MAX_SEATS; i++){
		if(is_playing_[i].get() && !is_playing_[i]->is_copy_){
			vp.push_back(is_playing_[i]);
		}
	}
	if (!vp.empty()){
		int r = rand_r(vp.size() - 1);
		return promote_banker(vp[r], reason);
	}
	return GAME_ERR_NOT_BANKER;
}

bool niuniu_logic::is_all_card_opened()
{
	for(int i = 0 ;i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->turn_cards_.empty()) continue;
		if (is_playing_[i]->is_copy_) continue;
		if (is_playing_[i]->match_result_.calc_point_ == -1)	{
			return false;
		}
	}
	return true;
}

bool niuniu_logic::is_all_bet_setted()
{
	for(int i = 0 ;i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->bet_ == 0 && is_playing_[i] != the_banker_)	{
			return false;
		}
	}
	return true;
}

int niuniu_logic::get_playing_count( int get_type /*= 0*/ )
{
	int ret = 0;
	for (int i = 0; i < MAX_SEATS; i++){
		if(is_playing_[i].get()){
			if (get_type == 2){
				if (is_playing_[i]->is_bot_)	{
					ret++;
				}
			}
			else if(get_type == 1){
				if (!is_playing_[i]->is_bot_)	{
					ret++;
				}
			}
			else
				ret++;
		}
	}
	return ret;
}

bool niuniu_logic::is_ingame( player_ptr pp )
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

void niuniu_logic::reset_to_not_banker()
{
	
	//
	if (!is_match_game_){
		if(the_service.restore_account(the_banker_, scene_set_->is_free_) == ERROR_SUCCESS_0){
			the_service.delete_from_local(the_banker_->uid_, scene_set_->is_free_);
		}
	}
	else if(the_banker_.get()){
		the_banker_->free_gold_ += the_banker_->free_guarantee_;
		the_banker_->free_guarantee_ = 0;
	}

	banker_turn = 0;

	msg_promote_banker msg;
	if (the_banker_.get()){
		COPY_STR(msg.uid_, the_banker_->uid_.c_str());
	}
	msg.reason_ = -1;
	broadcast_msg(msg);

	if(the_banker_.get())
	{
		msg_deposit_change2 msg_dp;
		msg_dp.display_type_ = 0;
		msg_dp.pos_ = the_banker_->pos_;
		msg_dp.credits_ = (double)the_service.cache_helper_.get_currency(the_banker_->uid_);
		broadcast_msg(msg_dp);
	}
	
	the_banker_.reset();
	//
}

int niuniu_logic::this_turn_over()
{
	//庄家不能再做庄
	if (the_banker_.get()){
		if (banker_turn > 2 ||
			get_playing_count() <= 1 ||
			(!is_match_game_ && the_banker_->guarantee_ < scene_set_->to_banker_set_) ||
			is_match_game_ && the_banker_->free_guarantee_ < scene_set_->to_banker_set_){
			reset_to_not_banker();
		}
	}

	//重置上局游戏数据
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		int ret = can_continue_play(is_playing_[i]);
		if (ret != ERROR_SUCCESS_0){
			leave_game(is_playing_[i], i, ret);
		}
	}
	return ERROR_SUCCESS_0;
}

int niuniu_logic::this_turn_init()
{
	query_bankers_.clear();

	int plac = get_playing_count();
	int plab = get_playing_count(2);
	int plap = get_playing_count(1);
	if (plap > 0 && plac < 3 && will_join_.empty() && plab < 2 && the_service.the_config_.enable_bot_){
		int i = 0;
		while (i < 2){
			bot_rejoin_game* ptask = new bot_rejoin_game(the_service.timer_sv_);
			task_ptr task(ptask);
			ptask->plogic = shared_from_this();
			ptask->schedule(1000 + rand_r(4000));
			will_join_.push_back(task);
			i++;
		}
	}

	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (is_playing_[i].get()){
			is_playing_[i]->reset_temp_data();
		}
	}
	
	//重置上局游戏数据
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		int ret = can_continue_play(is_playing_[i]);
		if (is_playing_[i]->is_connection_lost_){
			if (!is_playing_[i]->is_copy_){
				leave_game(is_playing_[i], i, 3);
			}
		}
	}

	return ERROR_SUCCESS_0;
}

void niuniu_logic::change_to_wait_start()
{
	if (the_service.the_config_.shut_down_){
		current_state_.reset();
		will_shut_down();
	}
	else{
		ms_ptr pms;
		pms.reset( new state_wait_start);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}
}

int niuniu_logic::join_player(player_ptr pp)
{
	return 0;
}

template<class service_t>
int state_machine<service_t>::enter()
{	
	enter_tick_ = GetTickCount();

	msg_state_change msg;
	msg.change_to_ = state_id_;
	msg.time_left = get_time_left();
	msg.time_total_	= get_time_total();
	boost::shared_ptr<service_t> plogic = the_logic_.lock();
	if(plogic.get())
		plogic->broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}

int state_vote_banker::enter()
{
	state_machine::enter();
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->query_for_become_banker();
	}

	timer_.expires_from_now(boost::posix_time::millisec(500));
	timer_.async_wait(boost::bind(&state_vote_banker::on_update, 
			boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
			boost::asio::placeholders::error));


	return ERROR_SUCCESS_0;
}

void state_vote_banker::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
 	if(plogic.get()){
		timer_.cancel(e);
		if (plogic->the_banker_.get()){
			msg_deposit_change2 msg_dep;
			msg_dep.pos_ = plogic->the_banker_->pos_;
			msg_dep.display_type_ = 2;
			if (plogic->is_match_game_){
				msg_dep.credits_ = plogic->the_banker_->free_guarantee_;
			}
			else{
				plogic->the_banker_->sync_account(0);
				msg_dep.credits_ = plogic->the_banker_->guarantee_;
			}
			plogic->broadcast_msg(msg_dep);
		}
		plogic->change_to_wait_bet();
	}
}

state_vote_banker::state_vote_banker() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_vote_banker);
	enter_tick_ = 0;
}

void state_vote_banker::on_update( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		//所有人都回复了要不要做庄
		if (plogic->is_all_query_replied()){
			timer_.expires_from_now(boost::posix_time::millisec(1000));
			timer_.async_wait(boost::bind(&state_vote_banker::on_time_do_choose, 
				boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
				boost::asio::placeholders::error));
		}
		else if (get_time_left() < 3000){
			timer_.expires_from_now(boost::posix_time::millisec(50));
			timer_.async_wait(boost::bind(&state_vote_banker::on_time_do_choose, 
				boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
				boost::asio::placeholders::error));
		}
		else{
			timer_.expires_from_now(boost::posix_time::millisec(500));
			timer_.async_wait(boost::bind(&state_vote_banker::on_update, 
				boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
				boost::asio::placeholders::error));
		}
	}
}

int state_vote_banker::do_choose_banker(logic_ptr plogic)
{
	int ret = ERROR_SUCCESS_0;
	auto it = plogic->query_bankers_.begin();
	while (it != plogic->query_bankers_.end())
	{
		query_banker_info& qi = *it;
		if (qi.replied_ < 1){
			msg_query_banker_rsp msg_rsp;
			msg_rsp.agree_ = 0;
			COPY_STR(msg_rsp.uid_, qi.uid_.c_str());
			plogic->broadcast_msg(msg_rsp);

			it = plogic->query_bankers_.erase(it);
		}
		else{
			it++;
		}
	}
	//没有人愿意做庄，则随机一个人做庄
	if (plogic->query_bankers_.empty()){
		if(!plogic->random_choose_banker(2) == ERROR_SUCCESS_0)
			ret = SYS_ERR_CANT_FIND_CHARACTER;
	}
	else{
		bool finded = false;

		//钱最多的做庄。
		std::string uid; longlong max_c = 0;
		for(int i = 0; i < (int) plogic->query_bankers_.size(); i++)
		{
// 			longlong c = the_service.get_currency(plogic->query_bankers_[i].uid_);
// 			if (max_c < c){
// 				uid = plogic->query_bankers_[i].uid_;
// 				max_c = c;
// 			}
// 			//如果两个玩家钱一样的,则随机一下看要不要变
// 			else if (max_c == c && c > 0){
				if (rand_r(100) > 50 || uid == ""){
					uid = plogic->query_bankers_[i].uid_;
				}
//			}
		}

		for (int i = 0 ; i < MAX_SEATS; i++){
			if (plogic->is_playing_[i].get() && plogic->is_playing_[i]->uid_ == uid)	{
				if(plogic->promote_banker(plogic->is_playing_[i], 1) == ERROR_SUCCESS_0)
				finded = true;
			}
		}
		//如果选庄失败
		if (!finded)	{
			if(!plogic->random_choose_banker(0) == ERROR_SUCCESS_0)
				ret = SYS_ERR_CANT_FIND_CHARACTER;
		}
	}
	return ret;
}

void state_vote_banker::on_time_do_choose( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		int ret = do_choose_banker(plogic);
		if (ret == ERROR_SUCCESS_0){
			timer_.expires_from_now(boost::posix_time::millisec(3000));
			timer_.async_wait(boost::bind(&state_vote_banker::on_time_expired, 
				boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
				boost::asio::placeholders::error));
		}
		else{
			plogic->change_to_wait_start();
		}
	}
}

class bot_set_bets_task : public task_info
{
public:
	player_ptr	pp;
	bot_set_bets_task(io_service& ios) : task_info(ios)
	{

	}
	virtual	int	routine()
	{
		logic_ptr		plogic = pp->the_game_.lock() ;
		if (plogic.get() && plogic->the_banker_.get()){
			longlong c = plogic->the_banker_->guarantee_ / 12/*20*/;
			longlong c1 = the_service.cache_helper_.get_currency(pp->uid_);
			if (plogic->is_match_game_){
				c = plogic->the_banker_->free_guarantee_ / 12/*20*/;
				c1 = pp->free_gold_;
			}

			c = std::min<longlong>(c1 / 3/*5*/, c);
			int n[4] = {10, 30, 50, 100};
			plogic->set_bet(pp, n[rand_r(3)] / 100.0 * c);
		}
		return routine_ret_break;
	}
};

int state_wait_bet::enter()
{
	state_machine::enter();

	logic_ptr plogic = the_logic_.lock();
	if(plogic.get() && plogic->the_banker_.get()){
		//扣庄家水费
		plogic->the_banker_->guarantee_ -= plogic->scene_set_->player_tax_;
		extern longlong	 g_cash_pool;
		g_cash_pool += plogic->scene_set_->player_tax_;
		//the_service.add_stock(plogic->scene_set_->player_tax_);

		msg_deposit_change2 msg_dp;
		msg_dp.display_type_ = 2;
		msg_dp.pos_ = plogic->the_banker_->pos_;
		if (plogic->is_match_game_){
			msg_dp.credits_ = (double)plogic->the_banker_->free_guarantee_;
		}
		else{
			msg_dp.credits_ = (double)plogic->the_banker_->guarantee_;
		}

		plogic->broadcast_msg(msg_dp);

		msg_banker_deposit msg;
		if (plogic->is_match_game_){
			msg.deposit_ = (double)plogic->the_banker_->free_guarantee_;
		}
		else{
			msg.deposit_ = (double)plogic->the_banker_->guarantee_;
		}
		plogic->broadcast_msg(msg);

		for (int i = 0; i < MAX_SEATS; i++)
		{
			if (!plogic->is_playing_[i].get()) continue;
			if (!plogic->is_playing_[i]->is_bot_) continue;
			if (plogic->is_playing_[i] == plogic->the_banker_) continue;
			
			bot_set_bets_task* ptask = new bot_set_bets_task(the_service.timer_sv_);
			task_ptr task(ptask);
			ptask->pp = plogic->is_playing_[i];
			ptask->schedule(2000 + rand_r(2000)); 
		}
	}

	timer_.expires_from_now(boost::posix_time::millisec(500));
	timer_.async_wait(boost::bind(&state_wait_bet::on_update, 
		boost::dynamic_pointer_cast<state_wait_bet>(shared_from_this()),
		boost::asio::placeholders::error));

	return ERROR_SUCCESS_0;
}

state_wait_bet::state_wait_bet() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_wait_bet);
}

void state_wait_bet::on_update( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){

		if (plogic->is_all_bet_setted() || get_time_left() < 500){
			timer_.cancel();
			plogic->change_to_do_random();
		}
		else{
			timer_.expires_from_now(boost::posix_time::millisec(500));
			timer_.async_wait(boost::bind(&state_wait_bet::on_update, 
				boost::dynamic_pointer_cast<state_wait_bet>(shared_from_this()),
				boost::asio::placeholders::error));
		}
	}
}

int fake_telfee_score()
{
	return rand_r(500, 5000);
}