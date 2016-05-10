#pragma once
#include "msg.h"
#include "boost/shared_ptr.hpp"
using namespace boost;

template<class remote_t>
class msg_from_client : public msg_base<remote_t>
{
public:
	unsigned char    sign_[32];

	int					read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data<unsigned char>(sign_, 32, data_s);
		return 0;
	}

	int					write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<unsigned char>(sign_, 32, data_s);
		return 0;
	}
  virtual ~msg_from_client() {}
};

//所有登录后发的数据包都需要从此继承

template<class remote_t>
class msg_authrized : public  msg_from_client<remote_t>
{
public:

};

