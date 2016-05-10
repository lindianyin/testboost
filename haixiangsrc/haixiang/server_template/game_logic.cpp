#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "db_delay_helper.h"
#include "schedule_task.h"
#include "boost/date_time/posix_time/conversion.hpp"

extern log_file<cout_output> glb_log;

using namespace boost;
using namespace boost::posix_time;
using namespace boost::gregorian;


longlong	 total_win_ = 0;
std::map<string, longlong>	g_todaywin;
std::map<string, longlong>	g_debug_performance_total;	//每个函数执行时间累计
std::map<string, longlong>	g_debug_performance_count;
std::map<string, longlong>	g_debug_performance_avg;

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


fishing_logic::fishing_logic()
{
	is_waiting_config_ = false;
	id_ = get_id();
	strid_ = get_guid();
	turn_ = 0;
	scene_set_ = NULL;
}

void fishing_logic::broadcast_msg( msg_base<none_socket>& msg, bool include_observer)
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		is_playing_[i]->send_msg_to_client(msg);
	}
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
			}
		}
		return routine_ret_break;
	}
};

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

void fishing_logic::will_shut_down()
{
	auto itf = std::find(the_service.the_games_.begin(),
		the_service.the_games_.end(), shared_from_this());
	if (itf != the_service.the_games_.end()){
		the_service.the_games_.erase(itf);
	}
}

void			get_credits(msg_currency_change& msg, player_ptr pp, fishing_logic* logic)
{
	msg.credits_ = (double) the_service.get_currency(pp->uid_);
}
extern int get_level(longlong exp);
class update_online_time : public task_info
{
public:
	boost::weak_ptr<fish_player> the_pla_;
	update_online_time(io_service& ios) : task_info(ios)
	{

	}
	int		 routine()
	{
		DEBUG_COUNT_PERFORMANCE_BEGIN("update_online_time");
		int ret = routine_ret_continue;
		player_ptr pp = the_pla_.lock();
		if (pp.get() && !pp->is_connection_lost_){
			longlong exp = the_service.add_var(pp->uid_ + KEY_ONLINE_TIME, IPHONE_NIUINIU, 10);
			int lv = get_level(exp);
			if (pp->lv_ != lv){
				msg_levelup msg;
				msg.lv_ = lv;
				msg.exp_ = 0;
				pp->send_msg_to_client(msg);
				pp->lv_ = lv;
			}
			else{
				msg_levelup msg;
				msg.lv_ = 0;
				msg.exp_ = exp;
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


int fishing_logic::player_login( player_ptr pp, int seat)
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

			{
				msg_deposit_change2 msg;
				msg.pos_ = i;
				get_credits(msg, is_playing_[i], this);
				pp->send_msg_to_client(msg);
			}

		}
	}

	return ERROR_SUCCESS_0;
}

void fishing_logic::leave_game( player_ptr pp, int pos, int why )
{
	msg_player_leave msg;
	msg.pos_ = pos;
	msg.why_ = why;
	broadcast_msg(msg);

	pp->the_game_.reset();

	is_playing_[pos].reset();
	pp->reset_temp_data();
}

void fishing_logic::start_logic()
{
	player_ptr pp = the_service.pop_onebot(shared_from_this());
	if (pp.get()){
		player_login(pp);
	}

	pp = the_service.pop_onebot(shared_from_this());
	if (pp.get()){
		player_login(pp);
	}
}

int fishing_logic::get_today_win(string uid, longlong& win )
{
	longlong n = g_todaywin[uid];
	win = n;
	return ERROR_SUCCESS_0;
}

void fishing_logic::stop_logic()
{

}

int fishing_logic::get_playing_count( int get_type /*= 0*/ )
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

bool fishing_logic::is_ingame( player_ptr pp )
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
