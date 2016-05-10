#pragma once
#include "net_service.h"
#include "error.h"
#include "boost/date_time.hpp"

class koko_player;
class koko_socket : public remote_socket_impl<koko_socket, koko_player>
{
public:
	koko_socket(net_server<koko_socket>& srv);
	atomic_value<std::string>		verify_code_, verify_code_backup_;		//ÑéÖ¤Âë±£´æ¡£
	std::string			mverify_code_, mverify_error_;
	boost::atomic_bool			is_login_, is_register_;
	boost::posix_time::ptime		mverify_time_;
	virtual void close();
};
typedef boost::shared_ptr<koko_socket> koko_socket_ptr;
typedef boost::weak_ptr<koko_socket> koko_socket_wptr;