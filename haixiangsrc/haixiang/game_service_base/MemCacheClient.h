#pragma once
#include "boost/date_time.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/write.hpp"
#include "boost/asio/read.hpp"
#include "boost/thread/thread.hpp"
#include "boost/bind.hpp"
#include "boost/atomic.hpp"

#include "net_service.h"
#include "msg.h"
#include "utility.h"
#include <map>
#include <string>
#include "log.h"
#include "game_ids.h"
#include "error_define.h"
#include "msg_cache.h"

using namespace boost;
using namespace std;

typedef long long longlong;
extern log_file<cout_output> glb_log;	

class mem_connector : public native_socket<mem_connector>
{
public:
	mem_connector(boost::asio::io_service& ios) : native_socket(ios)
	{

	}
	void close()
	{
		native_socket<mem_connector>::close();
	}
};

typedef boost::shared_ptr<mem_connector> mem_connector_ptr;
typedef boost::shared_ptr<msg_base<mem_connector_ptr>> msg_ptr;

class mem_cache_client 
{
public:
	boost::asio::io_service ios_;
	mem_connector_ptr	s_,	async_s_;				//同步请求socket,异步请求socket
	boost::asio::deadline_timer timer_;
	std::string		ip_;
	unsigned short port_;

	boost::atomic_int	sn_gen_;
	explicit mem_cache_client(int gameid): timer_(ios_)
	{
		game_id_ = gameid;
		sn_gen_ = 1;
	}

	int				reconnect(int switch_connection = 0)
	{
		boost::asio::ip::address addr;
		boost::system::error_code ec;
		//主cache
		if (ip_ != ""){
			s_.reset(new mem_connector(ios_));
			int r = s_->connect(ip_, port_);
			if (r == 0){
				msg_cache_updator_register<mem_connector_ptr > msg;
				stream_buffer sd = build_send_stream(&msg);
				boost::asio::write(s_->s, asio::buffer(sd.data(), sd.data_left()), ec);
				if (ec.value() != 0)	{return -2;}
			}

			async_s_.reset(new mem_connector(ios_));
			r = async_s_->connect(ip_, port_);
			if (r == 0) {
				msg_cache_updator_register<mem_connector_ptr > msg;
				stream_buffer sd = build_send_stream(&msg);
				boost::asio::write(async_s_->s, asio::buffer(sd.data(), sd.data_left()), ec);
				if (ec.value() != 0) { return -2; }
			}
			return r;
		}
		return 0;
	}
	void		keep_alive()
	{
		boost::system::error_code ec;
		if (!s_.get()) return; 
		int r = reconnect(0);
		if (r != 0)	{
			glb_log.write_log("!!!!!!all cache is down!!!!!");
		}
	}

	static boost::shared_ptr<msg_base<mem_connector_ptr>> create_msg(unsigned short cmd)
	{
		if (cmd == GET_CLSID(msg_cache_cmd_ret)){
			return boost::shared_ptr<msg_base<mem_connector_ptr>>(new msg_cache_cmd_ret<mem_connector_ptr>());
		}
		else if (cmd == GET_CLSID(msg_cache_kick_account)){
			return boost::shared_ptr<msg_base<mem_connector_ptr>>(new msg_cache_kick_account<mem_connector_ptr>());
		}
		else if (cmd == GET_CLSID(msg_cache_ping)){
			return boost::shared_ptr<msg_base<mem_connector_ptr>>(new msg_cache_ping<mem_connector_ptr>());
		}
		return boost::shared_ptr<msg_base<mem_connector_ptr>>();
	}

	template<class val_t, class ret_t>
	int				send_cache_cmd(std::string key, std::string cmd, val_t val, ret_t& ret)
	{
		msg_cache_cmd_ret<mem_connector_ptr> msg_ret;
		COPY_STR(msg_ret.key_, key.c_str());
		int r = do_send_cache_cmd(key, cmd, val, &msg_ret);
		if (r == ERROR_SUCCESS_0)	{
			pickup_value(msg_ret, ret);
			r = msg_ret.return_code_;
		}
		return r;
	}

	//如果返回0表示失败，否则返回的是订单号
	template<class val_t>
	int				async_send_cache_cmd(std::string key, std::string cmd, val_t val, int* out_sn = nullptr)
	{
		return do_send_cache_cmd(key, cmd, val, nullptr, 1000, out_sn);
	}

	void			close()
	{
		boost::system::error_code ec;
		timer_.cancel(ec);
		ios_.stop();
		if(s_.get())
			s_->close();
	}

	int	get_cache_result(
		std::vector<msg_ptr>&	msg_lst,
		mem_connector_ptr ref_socket,
		unsigned int time_out = 1000)
	{
			pickup_msg(ref_socket, msg_lst);
			return 0;
	}
	

	int				get_cache_result(
		msg_cache_cmd_ret<mem_connector_ptr>& msg_ret,
		mem_connector_ptr ref_socket,
		std::string sequence,
		unsigned int time_out = 1000)
	{
		int ret = -3;
		boost::posix_time::ptime p1 = boost::posix_time::microsec_clock::local_time();
		while(true)
		{
			ios_.reset();
			ios_.poll();

			if (ref_socket->is_closed_){
				return -2;
			}
			std::vector<msg_ptr>	msg_lst;
			pickup_msg(ref_socket, msg_lst);

			auto itm = msg_lst.begin();
			while (itm != msg_lst.end())
			{
				msg_ptr pmsg = *itm; itm++;
				if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_cmd_ret))	{
					msg_cache_cmd_ret<mem_connector_ptr>* cache_ret = (msg_cache_cmd_ret<mem_connector_ptr>*)pmsg.get();
					if (cache_ret->sequence_ == sequence){
						msg_ret = *cache_ret;
						ret = 0;
						break;
					}
				}
			}
			if (ret == 0){
				break;
			}
			if((boost::posix_time::microsec_clock::local_time() - p1).total_milliseconds() > time_out){
				break;
			}
		}
		return ret;
	}

	void				pickup_msg(mem_connector_ptr ref_socket, std::vector<msg_ptr>& msg_lst)
	{
		auto msg = pickup_msg_from_socket(ref_socket, mem_cache_client::create_msg);
		while (msg.get())
		{
			msg_lst.push_back(boost::dynamic_pointer_cast<msg_base<mem_connector_ptr>>(msg));
			msg = pickup_msg_from_socket(ref_socket, mem_cache_client::create_msg);
		}
	}
protected:
	char				wb_[512];
	char				rb_[512];
	int					game_id_;
	void				send_notify(boost::system::error_code ec, unsigned int bytes_transferred)
	{
		if (ec.value() == 0) {

		}
	}


	void			pickup_value(msg_cache_cmd_ret<mem_connector_ptr>&msg, std::string& ret)
	{
		if (msg.return_code_ == 0) {
			ret = msg.str_value_;
		}
	}

	void			pickup_value(msg_cache_cmd_ret<mem_connector_ptr>&msg, longlong& ret)
	{
		if (msg.return_code_ == 0) {
			ret = msg.value_;
		}
	}

	void			set_value(msg_cache_cmd<mem_connector_ptr>&msg, std::string val)
	{
		COPY_STR(msg.str_value_, val.c_str());
		msg.val_type_ = 1;
	}

	void			set_value(msg_cache_cmd<mem_connector_ptr>&msg, longlong val)
	{
		msg.value_ = val;
		msg.val_type_ = 0;
	}

	template<class val_t>
	int				do_send_cache_cmd(
		std::string key,
		std::string cmd,
		val_t val,
		msg_cache_cmd_ret<mem_connector_ptr>* msg_ret,
		int time_out = 1000,
		int* out_seq = nullptr)
	{
		boost::system::error_code ec;

		msg_cache_cmd<mem_connector_ptr> msg;
		sn_gen_++;
		if (sn_gen_ == 0) {
			sn_gen_ = 1;
		}
		std::string seq = boost::lexical_cast<std::string>(sn_gen_.load());//get_guid();
		COPY_STR(msg.sequence_, seq.c_str());
		COPY_STR(msg.key_, key.c_str());
		COPY_STR(msg.cache_cmd, cmd.c_str());

		set_value(msg, val);

		msg.game_id_ = game_id_;
		char* w = wb_;
		msg.write(w, 512);
		stream_buffer sd = build_send_stream(&msg);
		if (msg_ret) {
			boost::asio::write(s_->s, asio::buffer(sd.data(), sd.data_left()), ec);
			if (ec.value() != 0) { return -2; }
			int r = get_cache_result(*msg_ret, s_, msg.sequence_, time_out);
			return r;
		}
		else {
			boost::asio::async_write(async_s_->s,
				asio::buffer(sd.data(), sd.data_left()),
				boost::bind(&mem_cache_client::send_notify,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			if (out_seq) {
				*out_seq = sn_gen_.load();
			}
		}
		return 0;
	}
};
