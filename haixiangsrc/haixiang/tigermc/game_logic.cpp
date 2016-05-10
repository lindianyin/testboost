#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "db_delay_helper.h"
#include "schedule_task.h"
#include "boost/date_time/posix_time/conversion.hpp"
#include "platform_gameid.h"
extern log_file<cout_output> glb_log;

using namespace boost;
using namespace boost::posix_time;
using namespace boost::gregorian;

longlong	 g_cash_pool = 0;
std::map<string, longlong>	g_todaywin;
std::map<string, longlong>	g_debug_performance_total;	//每个函数执行时间累计
std::map<string, longlong>	g_debug_performance_count;
std::map<string, longlong>	g_debug_performance_avg;

static int get_id()
{
	static int gid = 1;
	return gid++;
}

tiger_logic::tiger_logic(int is_match)
{
	is_waiting_config_ = false;
	id_ = get_id();
	strid_ = get_guid();
	scene_set_ = NULL;
}


extern int get_today_win(std::string uid, longlong& win );

int tiger_logic::set_bet( player_ptr pp, int pid, longlong count )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}

	if (count < 0){
		return GAME_ERR_NOT_ENOUGH_CURRENCY;
	}

	longlong out_count = 0;
	get_today_win(pp->uid_, out_count);
	if ((out_count < 0 && abs(out_count) > scene_set_->max_lose_)){
		return GAME_ERR_LOSE_CAP_EXCEED;
	}
	else if ((out_count > 0 && abs(out_count) > scene_set_->max_win_)){
		return GAME_ERR_WIN_CAP_EXCEED;
	}

	pp->bets_[pid] += count;
	
	msg_player_setbet msg_set;
	COPY_STR(msg_set.uid_, pp->uid_.c_str());
	msg_set.pid = pid;
	msg_set.max_setted_ = (double)count;
	pp->send_msg_to_client(msg_set);

	return ERROR_SUCCESS_0;
}

void tiger_logic::save_match_result(player_ptr pp)
{

}

void tiger_logic::will_shut_down()
{
	while(!is_playing_.empty())
	{
		if (!is_playing_.front().get()){
			is_playing_.erase(is_playing_.begin());
		}
		else
			leave_game(is_playing_.front(), is_playing_.front()->pos_);
	}

	auto itf = std::find(the_service.the_golden_games_.begin(),
		the_service.the_golden_games_.end(), shared_from_this());
	if (itf != the_service.the_golden_games_.end()){
		the_service.the_golden_games_.erase(itf);
	}
}

int tiger_logic::leave_game(player_ptr pp, int pos, int why)
{
	pp->the_game_.reset();
	pp->reset_temp_data();
	if (!is_playing_.empty()){
		is_playing_.erase(std::find(is_playing_.begin(), is_playing_.end(), pp));
	}
	return ERROR_SUCCESS_0;
}

extern int get_level(longlong& exp);
class update_online_time : public task_info
{
public:
	boost::weak_ptr<tiger_player> the_pla_;
	update_online_time(io_service& ios) : task_info(ios)
	{

	}
	int		 routine()
	{
		DEBUG_COUNT_PERFORMANCE_BEGIN("update_online_time");
		int ret = routine_ret_continue;
		player_ptr pp = the_pla_.lock();
		if (pp.get() && !pp->is_connection_lost_){

			longlong exp = the_service.cache_helper_.add_var(pp->uid_ + KEY_ONLINE_TIME, the_service.the_config_.game_id, 10);
			exp = the_service.cache_helper_.add_var(pp->uid_ + KEY_EXP, the_service.the_config_.game_id, 10);

			msg_exp msg_ret;
			
			int lv = get_level(exp);
			msg_ret.lv_ = lv;
			msg_ret.exp_ = (double)exp;
			extern int glb_lvset[50];
			msg_ret.exp_max_ = glb_lvset[lv > 29 ? 29 : lv];
			pp->lv_ = lv;
  			pp->send_msg_to_client(msg_ret);


			//int lv = get_level(exp);
// 			if (pp->lv_ != lv){
// 				msg_levelup msg;
// 				msg.lv_ = lv;
// 				pp->send_msg_to_client(msg);
// 				pp->lv_ = lv;
// 			}
		
		}
		else{
			ret = routine_ret_break;
		}
		DEBUG_COUNT_PERFORMANCE_END();
		return ret;
	}
};


int tiger_logic::player_login( player_ptr pp, int seat)
{
	//坐位满了
	if (is_playing_.size() >= MAX_SEATS){
		return -1;
	}
	else{
		msg_common_reply<none_socket> rpl;
		rpl.err_ = GAME_ERR_ENTER_GAME_SUCCESS;
		pp->send_msg_to_client(rpl);

		pp->the_game_ = shared_from_this();
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

		{	//广播玩家坐下位置
			msg_player_seat msg;
			msg.pos_ = 0;
			COPY_STR(msg.uname_, pp->name_.c_str());
			COPY_STR(msg.uid_, pp->uid_.c_str());
			COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
			pp->send_msg_to_client(msg);
		}

		{
			msg_deposit_change2 msg;
			msg.pos_ = 0;
			msg.credits_ = the_service.cache_helper_.get_currency(pp->uid_);
			pp->send_msg_to_client(msg);
		}

		update_online_time* ptask = new update_online_time(the_service.timer_sv_);
		task_ptr task(ptask);
		ptask->the_pla_ = pp;
		ptask->schedule(60000);//60s，每一分钟调用一次

		return ERROR_SUCCESS_0;
	}
}

static int get_today_win(std::string uid, longlong& win )
{
	longlong n = g_todaywin[uid];
	win = n;
	return ERROR_SUCCESS_0;
}
