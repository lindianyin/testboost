#pragma once
#include <list>
#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"

template<class value_t>
struct safe_list 
{
	typedef boost::lock_guard<boost::mutex> write_lock;
	typedef	typename std::list<value_t> this_type;
	typedef typename this_type::iterator iterator;
	typedef typename this_type::const_iterator const_iterator;

	boost::mutex	val_lst_mutex_;
	void		push_back(const value_t& v)
	{
		write_lock l(val_lst_mutex_);
		lst_.push_back(v);
	}
	bool		pop_front(value_t& ret)
	{
		write_lock l(val_lst_mutex_);
		if(!lst_.empty()){
			ret = lst_.front();
			lst_.pop_front();
			return true;
		}
		return false;
	}
	int		size()
	{
		write_lock l(val_lst_mutex_);
		return lst_.size();
	}
	this_type lst_;
};
