#pragma once
#include "boost/shared_ptr.hpp"
#include "schedule_task.h"
#include "memcache_defines.h"

template<class inherit, class socket_t, class logic_t>
class player_base : public boost::enable_shared_from_this<inherit>
{
public:
	std::string		uid_;
	std::string		uidx_;
	std::string		name_;
	std::string		head_ico_;
	int						sex_;
	__int64				iid_;
	int						is_bot_;
	int						lv_;
	int						pos_;
	bool					is_connection_lost_;//连接是否断掉
	__int64				credits_;
	__int64				free_coins_;
	std::string		order_key_;
	int						online_prize_avilable_;
	int						online_prize_;
	task_ptr			online_counter_;
	std::string		platform_uid_;
	std::string		platform_id_;
	std::string		device_id_;
	//游戏临时数据
	boost::weak_ptr<logic_t>		the_game_;
	boost::weak_ptr<socket_t>		from_socket_;
	boost::posix_time::ptime		conn_lost_tick_;//连接断掉的时间
	int send_msg_to_client( msg_base<none_socket>& msg )
	{
		auto psock = from_socket_.lock();
		if(psock.get()){
			send_msg(psock, msg);
			return 0;
		}
		else
			return -1;
	}

	int		sync_account(__int64 out_count)
	{
		msg_currency_change msg;
		msg.credits_ = (double) out_count;
		msg.why_ = msg_currency_change::why_sync;

		boost::shared_ptr<socket_t> psock = from_socket_.lock();
		if (psock.get()){
			send_msg(psock, msg);
			return 0;
		}
		else
			return 11;
	}

	//设置最后在线时间,用于解决平台用户同时玩多个游戏时,从某个游戏退出会导致货币被兑回平台.
	//现在用户退出游戏后,不会立即兑出货币,而是定时检查用户是不是已经没有在玩任何游戏了.
	class set_last_online_time : public task_info
	{
	public:
		boost::weak_ptr<inherit> the_pla_;
		int online_lv_;
		set_last_online_time(io_service& ios) : task_info(ios)
		{

		}
		int		 routine()
		{
			int ret = routine_ret_continue;
			player_ptr pp = the_pla_.lock();
			if (pp.get() && !pp->is_connection_lost_){
				the_service.cache_helper_.async_set_var(pp->uid_ + KEY_ONLINE_LAST, -1, ::time(nullptr));
			}
			else{
				ret = routine_ret_break;
			}
			return ret;
		}
	};

	class update_online_prize : public task_info
	{
	public:
		boost::weak_ptr<inherit> the_pla_;
		int online_lv_;
		update_online_prize(io_service& ios) : task_info(ios)
		{

		}
		int		 routine()
		{
			int ret = routine_ret_continue;
			player_ptr pp = the_pla_.lock();
			if (pp.get() && !pp->is_connection_lost_){
				pp->online_prize_avilable_ = 1;
				ret = routine_ret_break;//fix bug for online reward by liufapu
			}
			return ret;
		}
	};

	void counter_online()
	{
		{
			online_prize_avilable_ = 0;
			update_online_prize* ptask = new update_online_prize(the_service.timer_sv_);
			online_counter_.reset(ptask);
			ptask->the_pla_ = shared_from_this();
			extern std::map<std::string, int> g_online_getted_;
			auto itg = g_online_getted_.find(uid_);
			if (itg != g_online_getted_.end()){
				online_prize_ = itg->second + 1;
			}
			else{
				online_prize_ = 0;
			}

			if (online_prize_ >= (int)the_service.the_config_.online_prize_.size() - 1)
				return;

			ptask->schedule((unsigned int)boost::posix_time::minutes(the_service.the_config_.online_prize_[online_prize_].time_).total_milliseconds());

		}
	}

	void update_last_online()
	{
		set_last_online_time* ptask = new set_last_online_time(the_service.timer_sv_);
		task_ptr task(ptask);
		ptask->the_pla_ = shared_from_this();
		ptask->schedule((unsigned int)boost::posix_time::seconds(1).total_milliseconds());
	}
};
