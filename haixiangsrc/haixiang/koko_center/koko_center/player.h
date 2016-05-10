#pragma once
#include <boost/smart_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/atomic.hpp>

#include <list>

class koko_socket;
class koko_player : public boost::enable_shared_from_this<koko_player>
{
public:
	std::string			uid_;
	int							vip_level_;
	__int64					gold_, gold_game_, gold_free_;
	__int64					iid_;
	std::vector<char>			head_ico_;
	std::string			nickname_;
	std::string			token_;
	__int64					seq_;
	int							gender_, age_, level_, mobile_v_, email_v_, byear_, bmonth_, bday_;
	int							region1_, region2_, region3_;
	std::string			idnumber_, mobile_, email_, address_;
	boost::weak_ptr<koko_socket> from_socket_;
	std::list<int>	in_channels_;
	boost::atomic_int		is_connection_lost_; //标记用户已掉线，如果5秒内不重连，会走掉线流程
	boost::atomic<boost::posix_time::ptime> connect_lost_tick_; 
	
	std::vector<char>	screen_shoot_;		//主播截图
	std::vector<int>	host_rooms_;
	koko_player()
	{
		is_connection_lost_ = 0;
		gold_ = 0;
		iid_ = 0;
		vip_level_ = 0;
	}
	void						connection_lost();
};

typedef boost::shared_ptr<koko_player> player_ptr;