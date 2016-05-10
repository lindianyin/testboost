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
extern log_file<debug_output> debug_log;
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
void		build_card_played_msg(msg_cards_deliver& msg_dlv, csf_result& ret);
bool is_lower(ddz_card& c1, ddz_card& c2)
{
	return c1.card_weight_ < c2.card_weight_;
}

std::string		to_string(std::vector<ddz_card> cards)
{
	std::string ret;
	for (unsigned i = 0; i < cards.size(); i++)
	{
		ret += cards[i].face_;
		ret += ",";
	}
	if (ret == ""){
		ret = "none";
	}
	return ret;
}

void print_cards(csf_result& csf)
{
	debug_log.write_log_no_break("cards:%s att:%s", to_string(csf.main_cards_).c_str(), to_string(csf.attachments_).c_str());
}

void print_cards(std::vector<ddz_card>& cards, bool isbanker = false)
{
	std::string fmt = "cards:%s ";
	if (isbanker){
		fmt += "<==banker";
	}
	fmt += "\r\n";
	debug_log.write_log_no_break(fmt.c_str(), to_string(cards).c_str());
}

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
	if (e.value() == 0){
		logic_ptr plogic = the_logic_.lock();
		if(plogic.get()){
			if(plogic->this_turn_start() != ERROR_SUCCESS_0){
				start_success_ = false;
			}
			if(start_success_)
				plogic->change_to_do_random();
			else
				plogic->change_to_wait_start();
		}
		else{
			glb_log.write_log("warning, plogic is null in state_wait_start!");
		}
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

//自动出牌任务
class bot_autoplay_task : public task_info
{
public:
	player_ptr	pp;
	int			move_;
	bot_autoplay_task(io_service& ios) : task_info(ios)
	{

	}
	virtual	int	routine()
	{
		logic_ptr plogic = pp->the_game_.lock();
		if (plogic.get() && plogic->current_move_ == move_){
			plogic->auto_play();
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
			}

			if (plogic->the_banker_.get())	{
				plogic->the_banker_->not_set_bet_count_ = 0;
			}

			timer_.expires_from_now(boost::posix_time::millisec(5000));
			timer_.async_wait(boost::bind(&state_do_random::on_time_expired,
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

void state_do_random::on_time_expired( boost::system::error_code e )
{
	timer_.cancel(e);
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_vote_banker();
	}
}

int state_rest_end::enter()
{
	state_machine::enter();
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){

		plogic->balance_result();

		for (int i = 0; i < MAX_SEATS; i++){
			if (!plogic->is_playing_[i].get()) 
				continue;
			plogic->is_playing_[i]->em_open_deal_type_ = em_open_deal_null;
			plogic->is_playing_[i]->em_play_auto_status_ = em_play_auto_no;
		}
		plogic->the_banker_.reset();

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


ddz_logic::ddz_logic(int ismatch)
{
	is_waiting_config_ = false;
	id_ = get_id();
	strid_ = get_guid();
	turn_ = 0;
	shuffle_cards_ = ddz_card::generate();
	scene_set_ = NULL;
	run_state_ = 0;
	is_match_game_ = ismatch;
	current_order_ = 0;
}


int ddz_logic::get_banker_pos(player_ptr p1, player_ptr p2, player_ptr p3)
{
	if (the_banker_ == p1){
		return 0;
	}
	else if (the_banker_ == p2){
		return 1;
	}
	else 
		return 2;
}

void ddz_logic::broadcast_msg( msg_base<none_socket>& msg, bool include_observer)
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


std::vector<ddz_card> generate(std::string str_cards)
{
	std::vector<ddz_card> ret;
	std::vector<std::string> v;
	split_str(str_cards.c_str(), str_cards.length(), ",", v, true);
	for(unsigned i = 0; i < v.size(); i++)
	{
		ret.push_back(ddz_card::generate(v[i]));
	}
	return ret;
}

int ddz_logic::random_result()
{
	
	int cheat_win = the_service.cheat_win();
	bool is_all_player = std::all_of(is_playing_,is_playing_+MAX_SEATS,[](player_ptr p)->bool{
		return !p->is_bot_;
	});
	int bet_players = 0;
	/**/
REDO:
	int indx = 0;
	//一人发17张牌
 	ddz_card::reset_and_shuffle(shuffle_cards_);
	//#if defined(_DEBUG)
 //	shuffle_cards_.clear();
 //	auto v1 = generate("A,2,3,4,6,8,8,10,J,Q,K,A,2,3,4,3,4,");
 //	auto v2	= generate("A,2,3,7,5,6,7,9,9,10,J,Q,K,5,6,7,8,");
 //	auto v3	= generate("A,2,5,4,5,6,7,8,9,10,J,Q,K,9,10,J,Q,");
 //	auto v4	= generate("K,*,@,");
 //	shuffle_cards_.insert(shuffle_cards_.end(),v1.begin(),v1.end());
 //	shuffle_cards_.insert(shuffle_cards_.end(),v2.begin(),v2.end());
 //	shuffle_cards_.insert(shuffle_cards_.end(),v3.begin(),v3.end());
 //	shuffle_cards_.insert(shuffle_cards_.end(),v4.begin(),v4.end());
 //#endif

	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		player_ptr pp = is_playing_[i];
		pp->turn_cards_.clear();
		if (pp->turn_cards_.empty()){
			pp->turn_cards_.insert(pp->turn_cards_.begin(), shuffle_cards_.begin() + indx, shuffle_cards_.begin() + indx + 17);
			std::sort(pp->turn_cards_.begin(), pp->turn_cards_.end(), is_lower);//插入完成就进行排序
		}
		else if (pp->turn_cards_.size() != 17){
			glb_log.write_log("card dispatch is wrong, card size=%d", pp->turn_cards_.size());
		}
		
		//进行重新发牌的条件
		//1.必须有机器人参与
		//2.必须是系统必赢
		//3.并且该人必须是真实玩家（TODO: 可以考虑给机器人发好牌）
		if(!is_all_player && 1 == cheat_win && !pp->is_bot_){
			int es = evalue_score_to_be_banker(pp->turn_cards_);
			if(es >= 4){
				goto REDO;
			}
		}
		broadcast_random_result(pp, i);
		indx += 17;
		
		print_cards(pp->turn_cards_, pp == the_banker_);
	}
	
	
	//int try_times = 0;
	//bool is_try_success = false;
	//while (!is_all_player && !is_try_success && try_times++ < 1024){
	//	ddz_card::reset_and_shuffle(shuffle_cards_);
	//	int indx = 0;
	//	for (int i = 0; i < MAX_SEATS; i++){
	//		if (!is_playing_[i].get()){
	//			continue;
	//		}
	//		player_ptr _pp = is_playing_[i];
	//		if (_pp->turn_cards_.empty()){
	//				_pp->turn_cards_.insert(_pp->turn_cards_.begin(), shuffle_cards_.begin() + indx, shuffle_cards_.begin() + indx + 17);
	//		}else if (_pp->turn_cards_.size() != 17){
	//			glb_log.write_log("card dispatch is wrong, card size=%d", _pp->turn_cards_.size());
	//		}
	//		indx += 17;
	//		std::sort(_pp->turn_cards_.begin(), _pp->turn_cards_.end(), is_lower);

	//		if(_pp->is_bot_ && 1 == cheat_win){
	//			int sc = evalue_score_to_be_banker(_pp->turn_cards_);
	//			if(sc >= 4){
	//				is_try_success = true;
	//			}		
	//		}

	//	}
	//}













	/*
 	the_banker_ = is_playing_[0];
 
  if (is_playing_[0].get()){
 		is_playing_[0]->turn_cards_ = generate("10,");
 		print_cards(is_playing_[0]->turn_cards_, is_playing_[0] == the_banker_);
 		broadcast_random_result(is_playing_[0], 0);
 	}
 
 	if (is_playing_[1].get()){
 		is_playing_[1]->turn_cards_ = generate("3,5,");
 		print_cards(is_playing_[1]->turn_cards_, is_playing_[1] == the_banker_);
 		broadcast_random_result(is_playing_[1], 1);
 	}
 
 	if (is_playing_[2].get()){
 		is_playing_[2]->turn_cards_ = generate("K,K,7,7,");
 		print_cards(is_playing_[2]->turn_cards_, is_playing_[2] == the_banker_);
 		broadcast_random_result(is_playing_[2], 2);
 	}
 	shuffle_cards_.clear();
 	shuffle_cards_.insert(shuffle_cards_.end(), is_playing_[0]->turn_cards_.begin(), is_playing_[0]->turn_cards_.end());
 	shuffle_cards_.insert(shuffle_cards_.end(), is_playing_[1]->turn_cards_.begin(), is_playing_[1]->turn_cards_.end());
 	shuffle_cards_.insert(shuffle_cards_.end(), is_playing_[2]->turn_cards_.begin(), is_playing_[2]->turn_cards_.end());
 	vector<ddz_card> tv = generate("K,A,A,");
 	shuffle_cards_.insert(shuffle_cards_.end(), tv.begin(), tv.end());
 	current_order_ = 1;
	*/
	



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

//机器人加入任务
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
			}
		}
		return routine_ret_break;
	}
};

int ddz_logic::this_turn_start()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("this_turn_start");
	int ret = ERROR_SUCCESS_0;
	if (get_playing_count() == 3){
			turn_++;
			msg_cash_pool msg;
			msg.credits_ = (double)g_cash_pool;
			broadcast_msg(msg);
	}
	else{
		ret = SYS_ERR_CANT_FIND_CHARACTER;
	}
DEBUG_COUNT_PERFORMANCE_END()
	return ret;
}

int		ddz_logic::balance_banker_lose(player_ptr the_banker, player_ptr pp, double fac1)
{
	g_cash_pool += scene_set_->player_tax_;
	return ERROR_SUCCESS_0; 
}

int		ddz_logic::balance_banker_win(player_ptr the_banker, player_ptr pp)
{
	g_cash_pool += scene_set_->player_tax_;
	return ERROR_SUCCESS_0;
}

void ddz_logic::broadcast_match_result(player_ptr pp,  int pos)
{

}

void ddz_logic::save_match_result(player_ptr pp)
{
	
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


void ddz_logic::balance_result()
{
	__int64 win = scene_set_->gurantee_set_ * max_bet_;
	

	//庄家赢了
	if (current_csf_order_ == get_banker_pos(is_playing_[0], is_playing_[1], is_playing_[2])){
		if (is_playing_[0] ==  the_banker_){
			is_playing_[0]->guarantee_ += win * 2;
			if(!is_playing_[0]->is_bot_){
				the_service.add_stock(-win * 2);
			}

			msg_card_match_result msg;
			msg.pos_ = 0;
			msg.final_win_ = win * 2;
			broadcast_msg(msg);
		}
		else{
			is_playing_[0]->guarantee_ -= win;

			if(!is_playing_[0]->is_bot_){
				the_service.add_stock(win);
			}


			msg_card_match_result msg;
			msg.pos_ = 0;
			msg.final_win_ = -win;
			broadcast_msg(msg);
		}

		if (is_playing_[1] ==  the_banker_){
			is_playing_[1]->guarantee_ += win * 2;
			if(!is_playing_[1]->is_bot_){
				the_service.add_stock(-win * 2);
			}

			msg_card_match_result msg;
			msg.pos_ = 1;
			msg.final_win_ = win * 2;
			broadcast_msg(msg);
		}
		else{
			is_playing_[1]->guarantee_ -= win;

			if(!is_playing_[1]->is_bot_){
				the_service.add_stock(win);
			}

			msg_card_match_result msg;
			msg.pos_ = 1;
			msg.final_win_ = -win;
			broadcast_msg(msg);
		}
		if (is_playing_[2] ==  the_banker_){
			is_playing_[2]->guarantee_ += win * 2;
			if(!is_playing_[2]->is_bot_){
				the_service.add_stock(-win * 2);
			}
			msg_card_match_result msg;
			msg.pos_ = 2;
			msg.final_win_ = win * 2;
			broadcast_msg(msg);
		}
		else{
			is_playing_[2]->guarantee_ -= win;
			if(!is_playing_[2]->is_bot_){
				the_service.add_stock(win);
			}
			msg_card_match_result msg;
			msg.pos_ = 2;
			msg.final_win_ = -win;
			broadcast_msg(msg);
		}
	}
	//庄家输了
	else{
		if (is_playing_[0] ==  the_banker_){
			is_playing_[0]->guarantee_ -= win * 2;
			if(!is_playing_[0]->is_bot_){
				the_service.add_stock(win * 2);
			}
			msg_card_match_result msg;
			msg.pos_ = 0;
			msg.final_win_ = -win * 2;
			broadcast_msg(msg);
		}
		else{
			is_playing_[0]->guarantee_ += win;
			if(!is_playing_[0]->is_bot_){
				the_service.add_stock(-win);
			}
			msg_card_match_result msg;
			msg.pos_ = 0;
			msg.final_win_ = win;
			broadcast_msg(msg);
		}

		if (is_playing_[1] ==  the_banker_){
			is_playing_[1]->guarantee_ -= win * 2;
			if(!is_playing_[1]->is_bot_){
				the_service.add_stock(win * 2);
			}
			msg_card_match_result msg;
			msg.pos_ = 1;
			msg.final_win_ = -win * 2;
			broadcast_msg(msg);
		}
		else{
			is_playing_[1]->guarantee_ += win;
			if(!is_playing_[1]->is_bot_){
				the_service.add_stock(-win);
			}
			msg_card_match_result msg;
			msg.pos_ = 1;
			msg.final_win_ = win;
			broadcast_msg(msg);
		}
		if (is_playing_[2] ==  the_banker_){
			is_playing_[2]->guarantee_ -= win * 2;
			if(!is_playing_[2]->is_bot_){
				the_service.add_stock(win * 2);
			}
			msg_card_match_result msg;
			msg.pos_ = 2;
			msg.final_win_ = -win * 2;
			broadcast_msg(msg);
		}
		else{
			is_playing_[2]->guarantee_ = win;
			if(!is_playing_[2]->is_bot_){
				the_service.add_stock(-win);
			}
			msg_card_match_result msg;
			msg.pos_ = 2;
			msg.final_win_ = win;
			broadcast_msg(msg);
		}
	}
}

void ddz_logic::will_shut_down()
{
	auto itf = std::find(the_service.the_golden_games_.begin(),
		the_service.the_golden_games_.end(), shared_from_this());
	if (itf != the_service.the_golden_games_.end()){
		the_service.the_golden_games_.erase(itf);
	}
}

void ddz_logic::change_to_state( ms_ptr ms )
{
	if (current_state_.get()){
		current_state_->leave();
	}
	current_state_ = ms;
	current_state_->enter();
}

void			get_credits(msg_currency_change& msg, player_ptr pp, ddz_logic* logic)
{
	msg.credits_ = (double) the_service.cache_helper_.get_currency(pp->uid_);
}
extern int get_level(longlong& exp);
extern int glb_lvset[50];
class update_online_time : public task_info
{
public:
	boost::weak_ptr<ddz_player> the_pla_;
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
				msg.exp_ = static_cast<double>(exp);
				if (lv > 29) lv = 29;
				msg.exp_max_ = glb_lvset[lv - 1];
				pp->send_msg_to_client(msg);
				pp->lv_ = lv;
			}
			else{
				msg_levelup msg;
				msg.lv_ = lv;
				msg.exp_ = static_cast<double>(exp);
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

int ddz_logic::player_login( player_ptr pp, int seat)
{
	//坐位满了
	if (get_playing_count() == MAX_SEATS && seat == -1){
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
						msg2.credits_ = static_cast<double>(pp->free_gold_);
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
					msg2.credits_ = static_cast<double>(pp->free_gold_);
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
			ptask->schedule(5 * 60 * 1000 * rand_r(1, 4));
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
					msg.credits_ = static_cast<double>(is_playing_[i]->free_gold_);
				}
				else{
					get_credits(msg, is_playing_[i], this);
				}
				pp->send_msg_to_client(msg);

				msg.display_type_ = 2;
				if (is_match_game_){
					msg.credits_ = static_cast<double>(is_playing_[i]->free_guarantee_);
				}
				else{
					msg.credits_ = static_cast<double>(is_playing_[i]->guarantee_);
				}
				

				pp->send_msg_to_client(msg);
			}

			if (!is_playing_[i]->turn_cards_.empty()){
				msg_cards msg;
				msg.pos_ = i;
				for (unsigned int ii = 0; ii < is_playing_[i]->turn_cards_.size() && ii < 20; ii++)
				{
					msg.cards_[ii] = is_playing_[i]->turn_cards_[ii].card_id_;
				}
				pp->send_msg_to_client(msg);
			}

		}
	}

	//发送游戏状态机
	if (current_state_.get()){

		if(current_state_->state_id_ == GET_CLSID(state_do_random)){
			msg_cards_complete msg2;
			pp->send_msg_to_client(msg2);
		}

		if (current_state_->state_id_ == GET_CLSID(state_gaming)){
			{
				msg_extra_cards msg;
				msg.pos_ = current_order_;
				
				//msg.cards_[0] = shuffle_cards_[51].card_id_;
				//msg.cards_[1] = shuffle_cards_[52].card_id_;
				//msg.cards_[2] = shuffle_cards_[53].card_id_;
				unsigned size = shuffle_cards_.size();
				BOOST_ASSERT(size >= 3);
				msg.cards_[0] = shuffle_cards_[size-3].card_id_;
				msg.cards_[1] = shuffle_cards_[size-2].card_id_;
				msg.cards_[2] = shuffle_cards_[size-1].card_id_;

				pp->send_msg_to_client(msg);
			}

			int i = 0;
			auto it = play_records_.rbegin();
			while (it != play_records_.rend() && i < 2)
			{
				msg_cards_deliver msg;
				msg.pos_ = ((current_order_ - i - 1) + 3) % 3;
				build_card_played_msg(msg, *it);
				pp->send_msg_to_client(msg);
				i++;
				it++;
			}

			{
				msg_turn_to_play msg;
				msg.pos_ = current_order_;
				msg.timeleft_ = current_state_->get_time_left();
				pp->send_msg_to_client(msg);
			}

		}

		msg_state_change msg;
		msg.change_to_ = current_state_->state_id_;
		msg.time_left = current_state_->get_time_left();
		msg.time_total_ = current_state_->get_time_total();
		pp->send_msg_to_client(msg);
	}
	

	//向自己发送明牌消息
	for (int i = 0; i < MAX_SEATS; i++)
	{
		player_ptr _pp = is_playing_[i];
		if (NULL != _pp && em_open_deal_null != _pp->em_open_deal_type_)
		{
			msg_paly_cards_open_deal_type msp;
			msp.emOpenDealType_ = _pp->em_open_deal_type_;
			memcpy(msp.uid_, _pp->uid_.c_str(), max_guid);
			pp->send_msg_to_client(msp);
			break;//由于只有一个人可以明牌
		}
	}

	//向自己发送托管消息
	for (int i = 0; i < MAX_SEATS; i++)
	{
		player_ptr _pp = is_playing_[i];
		if (NULL != _pp )
		{
			msg_paly_auto_status mpa;
			mpa.em_play_auto_status = _pp->em_play_auto_status_;
			memcpy(mpa.uid_, _pp->uid_.c_str(), max_guid);
			pp->send_msg_to_client(mpa);
		}
	}

	return ERROR_SUCCESS_0;
}

void ddz_logic::leave_game( player_ptr pp, int pos, int why )
{
	if (is_match_game_){
		pp->free_gold_ += pp->free_guarantee_;
		pp->free_guarantee_ = 0;
	}

	msg_player_leave msg;
	msg.pos_ = pos;
	msg.why_ = why;
	broadcast_msg(msg);

	auto itf = std::find(observers_.begin(), observers_.end(), pp);
	if(itf != observers_.end()){
		observers_.erase(itf);
	}

	pp->the_game_.reset();
	pp->reset_temp_data(); //离开游戏后重置临时数据
	pp->guarantee_ = 0;
	pp->not_set_bet_count_ = 0;
	pp->pos_ = -1;
	pp->credits_ = 0;
	pp->em_open_deal_type_ = em_open_deal_null;
	pp->em_play_auto_status_ = em_play_auto_no;

	is_playing_[pos].reset();
	int c = get_playing_count(1);
	//如果没有真实玩家了.这个场消毁掉
	if (c == 0){
		auto itf = std::find(the_service.the_golden_games_.begin(), the_service.the_golden_games_.end(), shared_from_this());
		if (itf != the_service.the_golden_games_.end()){
			the_service.the_golden_games_.erase(itf);
		}
	}	
	
}


void ddz_logic::start_logic()
{
	turn_bank_order_ = 0;
	change_to_wait_start();
// 	{
// 		player_ptr pp = the_service.pop_onebot(shared_from_this());
// 		player_login(pp);
// 	}
// 
// 	{
// 		player_ptr pp = the_service.pop_onebot(shared_from_this());
// 		player_login(pp);
// 	}
// 
// 	{
// 		player_ptr pp = the_service.pop_onebot(shared_from_this());
// 		player_login(pp);
// 	}
	run_state_ = 1;
}

int ddz_logic::get_today_win(string uid, longlong& win )
{
	longlong n = g_todaywin[uid];
	win = n;
	return ERROR_SUCCESS_0;
}

int ddz_logic::can_continue_play( player_ptr pp )
{
	if (pp->is_connection_lost_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	if (pp->is_bot_ && !the_service.the_config_.enable_bot_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	if (the_service.the_config_.shut_down_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	return ERROR_SUCCESS_0;
}

//机器人叫牌任务
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
		int sc = evalue_score_to_be_banker(pp->turn_cards_);
		int agree = (sc >= 4);
		for (unsigned int i = 0 ; i < pgame->query_bankers_.size(); i++)
		{
			if (pgame->query_bankers_[i].uid_ == pp->uid_){
				pgame->query_bankers_[i].replied_ = pgame->max_bet_ + 1;
			}
		}
		msg_query_banker_rsp msg;
		msg.agree_ = agree;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		pgame->broadcast_msg(msg);
		return routine_ret_break;
	}
};


void ddz_logic::query_for_become_banker()
{
	int pos = current_askbank_order_ % 3;
	msg_query_to_become_banker msg;
	msg.pos_ = pos;
	broadcast_msg(msg);
	query_banker_info qi;
	qi.uid_ = is_playing_[pos]->uid_;
	query_bankers_.push_back(qi);

	if (is_playing_[pos]->is_bot_){
		banker_rsp_task* task = new banker_rsp_task(the_service.timer_sv_);
		task->pp = is_playing_[pos];
		task_ptr ptask(task);
		task->schedule(2000 + rand_r(2000));
	}
}

bool ddz_logic::is_all_query_replied()
{
	for (unsigned int i = 0; i < query_bankers_.size(); i++){
		if (query_bankers_[i].replied_ < 0)	
			return false;
	}
	return true;
}

//选取庄家(地主)
int ddz_logic::promote_banker( player_ptr p, int reason )
{
	the_banker_ = p;
	msg_promote_banker msg;
	COPY_STR(msg.uid_,p->uid_.c_str());
	msg.reason_ = reason;
	broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}

void ddz_logic::broadcast_random_result( player_ptr pp, int pos )
{
	msg_cards msg;
	msg.pos_ = pos;
	for (unsigned int i = 0; i < pp->turn_cards_.size() && i < 20; i++)
	{
		msg.cards_[i] = pp->turn_cards_[i].card_id_;
	}
	broadcast_msg(msg);
}

void ddz_logic::stop_logic()
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

//随机选择一个庄家
int ddz_logic::random_choose_banker( int reason )
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
	return ERROR_SUCCESS_0;
}

bool ddz_logic::this_turn_is_over()
{
	for(int i = 0 ;i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->turn_cards_.empty()) return true;
	}
	return false;
}

bool ddz_logic::is_all_bet_setted()
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

bool ddz_logic::is_ingame( player_ptr pp )
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

void ddz_logic::reset_to_not_banker()
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

int ddz_logic::this_turn_over()
{
	//庄家不能再做庄
	if (the_banker_.get()){
		reset_to_not_banker();
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

int ddz_logic::this_turn_init()
{
	query_bankers_.clear();
	will_join_.clear();
	the_banker_.reset();

	int plac = get_playing_count();
	int plab = get_playing_count(2);
	int plap = get_playing_count(1);
	if (plac < 3 && will_join_.empty() && plab < 2 && the_service.the_config_.enable_bot_){
		int i = 0;
		while (i < 2){
			bot_rejoin_game* ptask = new bot_rejoin_game(the_service.timer_sv_);
			task_ptr task(ptask);
			ptask->plogic = shared_from_this();
			ptask->schedule(5000 + rand_r(5000));
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


	card_played_ = 0;
	pass_count_ = 0;

	turn_bank_order_ = rand_r(2);
	current_askbank_order_ = turn_bank_order_;

	current_move_ = 0;
	
	play_records_.clear();
	current_order_ = -1;
	return ERROR_SUCCESS_0;
}

void ddz_logic::change_to_wait_start()
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

int ddz_logic::join_player(player_ptr pp)
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
		if (plogic->the_banker_.get()){
			plogic->max_bet_ = 1;
			plogic->change_to_gaming();
		}
		else{//没有地主
			plogic->max_bet_pos_ = plogic->turn_bank_order_;
			plogic->max_bet_ = 0;
			plogic->current_askbank_order_ = plogic->turn_bank_order_;
			plogic->query_for_become_banker();

			timer_.expires_from_now(boost::posix_time::millisec(500));
			timer_.async_wait(boost::bind(&state_vote_banker::on_update, 
				boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
				boost::asio::placeholders::error));
		}
	}
	return ERROR_SUCCESS_0;
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
		int is_timeout = get_time_left() < 3000;
		if (plogic->is_all_query_replied() || is_timeout){
			//重置下次超时计时
			enter_tick_ = GetTickCount();
			if (is_timeout){
				msg_query_banker_rsp msg_rsp;
				msg_rsp.agree_ = 0;//超时表示不叫
				COPY_STR(msg_rsp.uid_, plogic->query_bankers_[0].uid_.c_str());
				plogic->broadcast_msg(msg_rsp);
			}
			else{
				if (plogic->query_bankers_[0].replied_ > 0){
					plogic->max_bet_pos_ = plogic->current_askbank_order_ % 3;
					plogic->max_bet_ = plogic->query_bankers_[0].replied_;
				}
			}
			//所有人都问完了.
			if (plogic->max_bet_ >= 3 || (plogic->current_askbank_order_ >= plogic->turn_bank_order_ + 2)){
				if (plogic->max_bet_ == 0){
					plogic->max_bet_ = 1;
				}
				plogic->promote_banker(plogic->is_playing_[plogic->max_bet_pos_], 1);
				plogic->change_to_gaming();
			}
			else{
				plogic->current_askbank_order_++;

				plogic->query_bankers_.clear();
				plogic->query_for_become_banker();

				timer_.expires_from_now(boost::posix_time::millisec(500));
				timer_.async_wait(boost::bind(&state_vote_banker::on_update, 
					boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
					boost::asio::placeholders::error));
			}

		}
		else{
			timer_.expires_from_now(boost::posix_time::millisec(500));
			timer_.async_wait(boost::bind(&state_vote_banker::on_update, 
				boost::dynamic_pointer_cast<state_vote_banker>(shared_from_this()),
				boost::asio::placeholders::error));
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
			plogic->set_bet(pp, static_cast<longlong>(n[rand_r(3)] / 100.0 * c));
		}
		return routine_ret_break;
	}
};

void ddz_logic::turn_to_play(bool banker_first, bool is_lead)
{
	card_played_ = 0;
	if (banker_first){
		lead_card_ = 1;
		if (current_order_ == -1){
			for (int i = 0; i < MAX_SEATS; i++)
			{
				if (is_playing_[i] == the_banker_){
					current_order_ = i;
				}
			}
			if (current_order_ == -1){
				current_order_ = 0;
			}
			the_banker_->turn_cards_.insert(the_banker_->turn_cards_.end(), shuffle_cards_.end() - 3, shuffle_cards_.end());
			std::sort(the_banker_->turn_cards_.begin(), the_banker_->turn_cards_.end(), is_lower);
		}
	}
	else{
		lead_card_ = is_lead;
	}
	current_move_++;
}

player_ptr ddz_logic::get_next_enemy(int offset)
{
	int pos = (current_order_ + offset) % 3;	
	return is_playing_[pos];
}

void		build_card_played_msg(msg_cards_deliver& msg_dlv, csf_result& ret)
{
	int j = 0;
	if (ret.csf_val_ == csf_not_allowed){
		msg_dlv.count_ = 0;
	}
	msg_dlv.count_ = ret.main_cards_.size() + ret.attachments_.size();
	for (unsigned i = 0; i < ret.main_cards_.size(); i++)
	{
		msg_dlv.card_[j++] = ret.main_cards_[i].card_id_;
	}

	for (unsigned i = 0; i < ret.attachments_.size(); i++)
	{
		msg_dlv.card_[j++] = ret.attachments_[i].card_id_;
	}
	msg_dlv.mainc_ = ret.main_count_;
	msg_dlv.attach_c = ret.attachment_count_;
	msg_dlv.result_ = ret.csf_val_;
}

void		build_card_played_msg_tips(msg_cards_deliver_tips& msg_dlv, csf_result& ret)
{
	int j = 0;
	if (ret.csf_val_ == csf_not_allowed){
		msg_dlv.count_ = 0;
	}
	msg_dlv.count_ = ret.main_cards_.size() + ret.attachments_.size();
	for (unsigned i = 0; i < ret.main_cards_.size(); i++)
	{
		msg_dlv.card_[j++] = ret.main_cards_[i].card_id_;
	}

	for (unsigned i = 0; i < ret.attachments_.size(); i++)
	{
		msg_dlv.card_[j++] = ret.attachments_[i].card_id_;
	}
	msg_dlv.mainc_ = ret.main_count_;
	msg_dlv.attach_c = ret.attachment_count_;
	msg_dlv.result_ = ret.csf_val_;
}

//玩家出牌
void ddz_logic::play(player_ptr pp, csf_result& ret)
{
	play_records_.push_back(ret);
	if (ret.csf_val_ == csf_not_allowed){
		pass_count_++;

		msg_cards_deliver msg_dlv;
		msg_dlv.pos_ = current_order_;
		broadcast_msg(msg_dlv);

		std::string slog = "****[pass]*****\r\n";
		debug_log.write_log(slog.c_str());
	}
	else{

		pp->turn_cards_ = remove_cards(pp->turn_cards_, ret);
		pass_count_ = 0;
		current_csf_ = ret;
		current_csf_order_ = current_order_;

		msg_cards_deliver msg_dlv;
		msg_dlv.pos_ = current_order_;
		build_card_played_msg(msg_dlv, ret);
		broadcast_msg(msg_dlv);

		debug_log.write_log("****[%s]:[%s]***** left:%s\r\n",
			to_string(current_csf_.main_cards_).c_str(),
			to_string(current_csf_.attachments_).c_str(),
			to_string(pp->turn_cards_).c_str());
	}
	//下一个人
	current_order_ = (current_order_ + 1) % 3;
	card_played_ = true;
	pp->tips_results_.reset();
	pp->tips_offset_ = 0;
	
}

//自动打牌(托管)
void ddz_logic::auto_play()
{
	player_ptr pp = is_playing_[current_order_];
	player_ptr c1 = get_next_enemy(1);
	player_ptr c2 = get_next_enemy(2);
	int bankp = get_banker_pos(pp, c1, c2);
	csf_result ret;
	std::string order = boost::lexical_cast<std::string>(current_order_);
	if (lead_card_){
		debug_log.write_log((order + "p begin to lead cards =================================>").c_str());
		int r = lead_card(bankp, ret,	pp->turn_cards_, c1->turn_cards_, c2->turn_cards_);
		if (ret.csf_val_ != csf_not_allowed){
			play(pp, ret);
		}
		else{
			glb_log.write_log("warning, lead card failed, please contact to the develop department!");
			std::string s = "cards:" + to_string(pp->turn_cards_);
			glb_log.write_log(s.c_str());
		}
	}
	else{
		debug_log.write_log((order + "p begin to follow cards, last bigpos:%dp").c_str(), current_csf_order_);
		do 
		{
			int csf_order = remap_pos(current_csf_order_,
				is_playing_[0]->turn_cards_,
				is_playing_[1]->turn_cards_, 
				is_playing_[2]->turn_cards_,
				&pp->turn_cards_,
				&c1->turn_cards_, 
				&c2->turn_cards_);

			int order = remap_pos(current_order_,
				is_playing_[0]->turn_cards_,
				is_playing_[1]->turn_cards_, 
				is_playing_[2]->turn_cards_,
				&pp->turn_cards_,
				&c1->turn_cards_, 
				&c2->turn_cards_);

			bool should_check_pass = (bankp != csf_order && bankp != order);
			//如果我是闲家,并且上轮是闲家出的牌,则看是否需要pass牌
			if (should_check_pass){
				//看是否需要跳过
				if (should_pass_for_alliance(current_csf_)){
					if(!will_lose(bankp, csf_order, false, current_csf_, pp->turn_cards_, c1->turn_cards_, c2->turn_cards_)){
						std::string slog = "****[pass for alliance.]*****\r\n";
						debug_log.write_log(slog.c_str());
						play(pp, csf_result());
						break;
					}
				}
			}

			follow_card(bankp, csf_order, current_csf_, ret, pp->turn_cards_, c1->turn_cards_, c2->turn_cards_);
			if (ret.csf_val_ != csf_not_allowed){
				if (should_check_pass){
					//如果跟了大牌,就看是否不需要跟
					if (should_pass_for_alliance(ret)){
						if(!will_lose(bankp, csf_order, false, current_csf_, pp->turn_cards_, c1->turn_cards_, c2->turn_cards_)){
							std::string slog = "****[pass for alliance.]*****\r\n";
							debug_log.write_log(slog.c_str());
							play(pp, csf_result());
							break;
						}
						else{
							play(pp, ret);
						}
					}
					else{
							play(pp, ret);
					}
				}
				else{
					play(pp, ret);
				}
			}
			else{
				play(pp, csf_result());
			}
		} while (0);
	}
}


//提示出牌
void ddz_logic::play_cards_tips()
{
	player_ptr pp = is_playing_[current_order_];
	player_ptr c1 = get_next_enemy(1);
	player_ptr c2 = get_next_enemy(2);

	if(NULL == pp->tips_results_.get())
	{
		std::vector<ddz_card> pp_turn_cards = pp ->turn_cards_;
		std::vector<ddz_card> c1_turn_cards = c1 ->turn_cards_;
		std::vector<ddz_card> c2_turn_cards = c2 ->turn_cards_;

		boost::shared_ptr<std::vector<csf_result>> vec_all_tips(new std::vector<csf_result>());
		int bankp = get_banker_pos(pp, c1, c2);

		for (int i=0;i<20;i++)
		{
			csf_result ret;
			std::string order = boost::lexical_cast<std::string>(current_order_);
			if (lead_card_){
				debug_log.write_log((order + "p begin to lead cards =================================>").c_str());
				int r = lead_card(bankp, ret,	pp_turn_cards, c1_turn_cards, c2_turn_cards);
				if (ret.csf_val_ != csf_not_allowed){
					vec_all_tips->push_back(ret);
					pp_turn_cards = remove_cards(pp_turn_cards,ret);
					//play(pp, ret);
				}
				else
				{
					break;
					//glb_log.write_log("warning, lead card failed, please contact to the develop department!");
					//std::string s = "cards:" + to_string(pp->turn_cards_);
					//glb_log.write_log(s.c_str());
				}
			}
			else{
				debug_log.write_log((order + "p begin to follow cards, last bigpos:%dp").c_str(), current_csf_order_);
				do 
				{
					int csf_order = remap_pos(current_csf_order_,
						is_playing_[0]->turn_cards_,
						is_playing_[1]->turn_cards_, 
						is_playing_[2]->turn_cards_,
						&pp->turn_cards_,
						&c1->turn_cards_, 
						&c2->turn_cards_);

					int order = remap_pos(current_order_,
						is_playing_[0]->turn_cards_,
						is_playing_[1]->turn_cards_, 
						is_playing_[2]->turn_cards_,
						&pp->turn_cards_,
						&c1->turn_cards_, 
						&c2->turn_cards_);

					//如果我是闲家,并且上轮是闲家出的牌,则看是否需要pass牌
					if (bankp != csf_order && bankp != order){
						//看是否需要跳过
						if (should_pass_for_alliance(current_csf_)){
							if(!will_lose(bankp, csf_order, false, current_csf_, pp->turn_cards_, c1_turn_cards,c2_turn_cards)){
								std::string slog = "****[pass for alliance.]*****\r\n";
								debug_log.write_log(slog.c_str());
								//play(pp, csf_result());
								break;
							}
						}
					}

					follow_card(bankp, csf_order, current_csf_, ret, pp_turn_cards, c1->turn_cards_, c2->turn_cards_,true,true);
					if (ret.csf_val_ != csf_not_allowed){
						//play(pp, ret);
						vec_all_tips->push_back(ret);
						pp_turn_cards = remove_cards(pp_turn_cards,ret);
					}
					else{
						//play(pp, csf_result());
						break;
					}
				} while (0);
			}

		} // for (int i=0;i<20;i++)
		pp->tips_results_ = vec_all_tips;
	}
	
	if(NULL == pp->tips_results_.get())
	{
		glb_log.write_log("[error] ...");
		return;
	}
	msg_cards_deliver_tips mcd;
	mcd.pos_ = current_order_;

	if(pp->tips_results_->size() == 0)
	{
		glb_log.write_log("## pp->tipsResults->size() == 0");
		mcd.count_ = 0;
	}
	else
	{
		glb_log.write_log("pp->nTipsOffset_ = %d weight=%d",pp->tips_offset_,pp->tips_results_->at(pp->tips_offset_).weight_);
		build_card_played_msg_tips(mcd,pp->tips_results_->at(pp->tips_offset_));
		pp->tips_offset_++;
		pp->tips_offset_ %= pp->tips_results_->size();
	}
	pp->send_msg_to_client(mcd);

}

int state_gaming::enter()
{
	state_machine::enter();
	next_player(true);
	on_update( boost::system::error_code());
	return ERROR_SUCCESS_0;
}

state_gaming::state_gaming() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_gaming);
}

void state_gaming::on_update( boost::system::error_code e )
{
	if (e.value() != 0) return;
	
	bool isover = false;
	logic_ptr plogic = the_logic_.lock();

	//判断当前出牌者是否是托管状态，如果是托管状态，不用等待20秒出牌超时时间，直接出牌
	bool is_play_auto = false;
	if(plogic && plogic->is_playing_[plogic->current_order_]->em_play_auto_status_ == em_play_auto_yes){
		is_play_auto = true;
	}

	int time_left_to_paly = is_play_auto  ? 18000 : 2000;


	//玩家到时间了还没有出牌
	the_logic_.lock();
	if (get_time_left() < time_left_to_paly){
		//重置下次出牌计时
		enter_tick_ = GetTickCount();
		//系统帮他出牌
		isover = check_game();
	}
	else{
		
		//如果玩家已经出了牌
		if(plogic.get() && plogic->card_played_){
			//重置下次出牌计时
			enter_tick_ = GetTickCount();
			isover = check_game();
		}
		//如果玩家没出牌,继续等
	}
	if (!isover){
		timer_.expires_from_now(boost::posix_time::millisec(500));
		timer_.async_wait(boost::bind(&state_gaming::on_update, 
			boost::dynamic_pointer_cast<state_gaming>(shared_from_this()),
			boost::asio::placeholders::error));
	}
}

bool state_gaming::check_game()
{
	logic_ptr plogic = the_logic_.lock();
	if(!plogic.get()) return true;
	if (plogic->this_turn_is_over())	{
		plogic->change_to_end_rest();
		return true;
	}
	else{
		if (!plogic->card_played_){
			plogic->auto_play();
		}

		if (plogic->pass_count_ >= 2){
			next_player(false, true);
		}
		else{
			next_player();
		}
	}
	return false;
}

void state_gaming::next_player(bool bank_first, bool islead)
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->turn_to_play(bank_first, islead);

		msg_extra_cards msg;
		
		if (bank_first){
			msg.pos_ = plogic->current_order_;
			//msg.cards_[0] = plogic->shuffle_cards_[51].card_id_;
			//msg.cards_[1] = plogic->shuffle_cards_[52].card_id_;
			//msg.cards_[2] = plogic->shuffle_cards_[53].card_id_;
			
			unsigned size = plogic->shuffle_cards_.size();
			BOOST_ASSERT(size >= 3);
			msg.cards_[0] = plogic->shuffle_cards_[size-3].card_id_;
			msg.cards_[1] = plogic->shuffle_cards_[size-2].card_id_;
			msg.cards_[2] = plogic->shuffle_cards_[size-1].card_id_;
			plogic->broadcast_msg(msg);
		}

		msg_turn_to_play msg_toplay;
		msg_toplay.pos_ = plogic->current_order_;
		msg_toplay.timeleft_ = get_time_left();
		plogic->broadcast_msg(msg_toplay);

		player_ptr pp = plogic->is_playing_[plogic->current_order_];
		
		//真实玩家数目
		//int player_cout = std::count_if(plogic->is_playing_,plogic->is_playing_+MAX_SEATS,
		//	[](player_ptr pp)->bool{
		//		return !pp->is_bot_;
		//});

		if (pp->is_bot_ || pp->is_connection_lost_){
			//如果该玩家掉线并且不是机器人,则复制改玩家并将玩家设置为机器人
			//if(pp->is_connection_lost_ && !pp->is_bot_){
			//	player_ptr new_player = pp->clone();
			//	new_player->is_bot_ = true;
			//	plogic->is_playing_[plogic->current_order_] = new_player;
			//	auto itp = the_service.players_.find(pp->uid_);
			//	if(itp != the_service.players_.end()){
			//		the_service.players_.erase(itp);
			//	}
			//}
			bot_autoplay_task*ptask = new bot_autoplay_task(the_service.timer_sv_);
			task_ptr task(ptask);
			ptask->pp = pp;
			ptask->move_ = plogic->current_move_;
			
			//if(player_cout == 0){//如果没有真实玩家，玩快一点
			//	ptask->schedule(1000);
			//}
			//else{
				
			//}

			//如果是托管状态，自动出牌为1秒
			if(em_play_auto_yes ==  pp->em_play_auto_status_){
				ptask->schedule(1000);
			}else{
				ptask->schedule(rand_r(2000, 5000));
			}



		}
	}
}
