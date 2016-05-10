#pragma once
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include "boost/enable_shared_from_this.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/date_time/posix_time/conversion.hpp"
#include <unordered_map>

using namespace boost;
using namespace boost::posix_time;
using namespace boost::gregorian;

struct date_info
{
	int			d,	h,	m,	s;
	date_info()
	{
		d = h = m = s = 0;
	}
};

class task_info;
typedef boost::weak_ptr<task_info> task_wptr;
class task_info : public boost::enable_shared_from_this<task_info>
{
public:
	enum{
		routine_ret_continue,
		routine_ret_break,
	};
	static	std::unordered_map<unsigned int, task_wptr>	 glb_running_task_;
	asio::deadline_timer timer_;
	task_info(asio::io_service& ios):timer_(ios)
	{
		static unsigned int idgen = 1;
		delay_ = 0;
		iid_ = idgen++;
	}

	virtual ~task_info()
	{
		cancel();
	}

	void		schedule(unsigned int delay)
	{
		delay_ = delay;
		timer_.expires_from_now(boost::posix_time::milliseconds(delay_));
		timer_.async_wait(boost::bind(&task_info::handler, shared_from_this(), boost::asio::placeholders::error));
		glb_running_task_.insert(std::make_pair(iid_, shared_from_this()));
	}
	void		schedule_at_next_days(date_info& d)
	{
		dat_ = d;
		date dat = date_time::day_clock<date>::local_day();
		ptime pt(dat, time_duration(dat_.h - 8, dat_.m, dat_.s)); //中国时区+8
		pt += days(dat_.d);
		timer_.expires_at(pt);
		timer_.async_wait(boost::bind(&task_info::handler, shared_from_this(), boost::asio::placeholders::error));
		glb_running_task_.insert(std::make_pair(iid_, shared_from_this()));
	}
	unsigned int	time_left()
	{
		auto left = timer_.expires_from_now();
		return left.total_seconds();
	}

	void		cancel()
	{
		boost::system::error_code ec;
		timer_.cancel(ec);
		auto it = glb_running_task_.find(iid_);
		if (it != glb_running_task_.end()){
			glb_running_task_.erase(it);
		}
	}

protected:
	int delay_;
	date_info dat_;

	void		handler(boost::system::error_code ec)
	{
		if (ec.value() == 0){
			if(routine() == routine_ret_break){
				cancel();
			}
			else{
				if (delay_ > 0){
					schedule(delay_);
				}
				else{
					schedule_at_next_days(dat_);
				}
			}
		}
		else{
			cancel();
		}
	}

	virtual int routine()
	{
		return routine_ret_break;
	}

private:
	unsigned	int iid_;
};

typedef boost::shared_ptr<task_info> task_ptr;
