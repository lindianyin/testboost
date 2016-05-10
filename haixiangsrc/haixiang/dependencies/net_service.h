/************************************************************************/
/* 这个模块的所有asio::io_service操作需要在主线程中完成,否则会发生多线程同步问题
/* 除了add_to_send_queue这个函数可以多线程调用。
/************************************************************************/

#pragma once
#include "boost/smart_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio.hpp"
#include "boost/thread.hpp"
#include "boost/atomic.hpp"
#include "boost/system/error_code.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include <vector>
#include "msg.h"

using namespace boost;
using namespace boost::asio;

typedef	boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work>	work_ptr;
typedef boost::shared_ptr<boost::thread>	thread_ptr;
typedef boost::lock_guard<boost::mutex>	write_lock;

enum{
	keep_all,
	keep_new,
	keep_none,
};

struct send_helper
{
	boost::atomic_bool					is_sending_;
	stream_buffer								sending_buffer_;
	std::list<stream_buffer>		send_vct;//需要进行线程同步
	boost::mutex								send_vct_mut;
};

template<class inherit>
struct basic_socket_impl : boost::enable_shared_from_this<inherit>
{
protected:
	boost::atomic_bool close_when_idle_;	//闲时关闭连接
	send_helper				send_helper_;
	stream_buffer			reading_buffer_;		//正在接收的缓冲区
	boost::asio::io_service&		ios_;
	boost::asio::deadline_timer check_data_recv_;
public:
	boost::atomic_bool is_closed_;
	ip::tcp::socket s;		//要保证同时一间不存在两个线程同时操作socket

	basic_socket_impl(boost::asio::io_service& ios) :	
		ios_(ios), 
		s(ios),
		reading_buffer_(boost::shared_array<char>(new char[4096]), 0, 4096),
		check_data_recv_(ios)
	{
		send_helper_.is_sending_ = false;
		close_when_idle_ = false;
		is_closed_ = false;
	}

	//必须在主线程中调用
	virtual void	close()
	{
		if (!is_closed_){	
			is_closed_ = true;
			boost::system::error_code ec;
			s.shutdown(s.shutdown_both, ec);
			s.close(ec);
			check_data_recv_.cancel(ec);
		}
	}
	//可在任意线程调用
	int	add_to_send_queue(stream_buffer dat, bool close_this = false)
	{
		if (is_closed_) return 0;

		write_lock l(send_helper_.send_vct_mut);
		send_helper_.send_vct.push_back(dat);
		if (close_this){
			close_when_idle_ = true;
		}
		//交给主线程发送
		ios_.post(boost::bind(&basic_socket_impl<inherit>::send_new_buffer, shared_from_this()));
		return send_helper_.send_vct.size();
	}

	//必须主线程调用
	int	pickup_data(void* out, unsigned int len, bool remove = false, bool all = false)
	{
		if (is_closed_) return 0;

		int rl = 0;
		if (all){
			rl = std::min<unsigned int>(len, reading_buffer_.data_left());
		}
		else{
			if (reading_buffer_.data_left() < len ) return false;	
			rl = len;
		}

		memcpy(out, reading_buffer_.data(), rl);
		if (remove){
			reading_buffer_.use_data(rl);
		}

		return rl;
	}

protected:
	void	on_data_sended_notify(const boost::system::error_code& ec, size_t byte_transfered)
	{
		if (ec.value() != 0){ send_helper_.is_sending_ = false;	return;	}

		send_helper_.sending_buffer_.use_data(byte_transfered);
		send_helper_.is_sending_ = false;
		if (send_helper_.sending_buffer_.is_complete()){
			if (close_when_idle_){
				ios_.post(boost::bind(&basic_socket_impl<inherit>::close, shared_from_this()));
			}
			else{
				send_new_buffer();
			}
		}
		//没发完，继续发剩下的
		else{
			s.async_send(boost::asio::buffer(send_helper_.sending_buffer_.data(), send_helper_.sending_buffer_.data_left()), 
				boost::bind(&basic_socket_impl<inherit>::on_data_sended_notify,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	}

	void  on_recv_check(const boost::system::error_code& ec)
	{
		on_recv_notify(boost::system::error_code(), 0);
	}

	void	on_recv_notify(const boost::system::error_code& ec, size_t byte_transfered)
	{
		if (ec.value() != 0){
			if (!is_closed_){
				close();
			}
		}
		else{
			reading_buffer_.use_buffer(byte_transfered);
			if (reading_buffer_.buffer_left() < 128){
				reading_buffer_.remove_used();
			}

			if (reading_buffer_.buffer_left() > 0){
				s.async_receive(
					boost::asio::buffer(reading_buffer_.buffer(), reading_buffer_.buffer_left()),
					boost::bind(&basic_socket_impl<inherit>::on_recv_notify,
					shared_from_this(), 
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
			}
			else{
				//缓冲区满了,100ms后再读数据，让目前的数据有时间处理
				check_data_recv_.expires_from_now(boost::posix_time::millisec(100));
				check_data_recv_.async_wait(
					boost::bind(&basic_socket_impl<inherit>::on_recv_check,
					shared_from_this(),
					boost::asio::placeholders::error));
			}
		}
	}

	bool	send_new_buffer()
	{
		if(send_helper_.is_sending_) return true;
		{
			write_lock l(send_helper_.send_vct_mut);
			if (!send_helper_.send_vct.empty()){
				send_helper_.sending_buffer_ = send_helper_.send_vct.front();
				send_helper_.send_vct.pop_front();
			}
		}
		if (!send_helper_.sending_buffer_.is_complete()){
			send_helper_.is_sending_ = true;
			s.async_send(boost::asio::buffer(send_helper_.sending_buffer_.data(), send_helper_.sending_buffer_.data_left()),
				boost::bind(&basic_socket_impl<inherit>::on_data_sended_notify, 
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
			return true;
		}
		else{
			send_helper_.is_sending_ = false;
		}
		return false;
	}
};

template<class remote_t> struct net_server;

template<class inherit, class socket_service_t>
struct remote_socket_impl : public basic_socket_impl<inherit>
{
	friend struct net_server<inherit>;
	typedef socket_service_t client_t;

	boost::weak_ptr<client_t>	the_client_;
	boost::posix_time::ptime	security_check_;
	boost::asio::deadline_timer check_athorize_timer_;
	net_server<inherit>&		net_server_;
	bool						is_authorized_;

	std::string			device_id_;
	int							priv_flag_;
	int							debug_data1_;
	int							debug_data2_;

	remote_socket_impl(net_server<inherit>& srv) : 
		basic_socket_impl<inherit>(srv.ios_),
		net_server_(srv),
		check_athorize_timer_(srv.ios_)
	{
		priv_flag_ = 0;
		debug_data1_ = debug_data2_ = 0;
		is_authorized_ = false;
		authrize_check_time_ = 5000;
	}
	void	close()
	{
		basic_socket_impl<inherit>::close();

		boost::system::error_code ec;
		check_athorize_timer_.cancel(ec);

		//在客户端列表中找到对于的socket并删除
		auto itf = std::find(net_server_.remotes_.begin(), net_server_.remotes_.end(), shared_from_this());
		if(itf != net_server_.remotes_.end())
			net_server_.remotes_.erase(itf);
	}
	std::string		remote_ip()
	{
		boost::system::error_code ec;
		auto edp = s.remote_endpoint(ec);
		return edp.address().to_string(ec);
	}

	bool	is_authorized()
	{
		return is_authorized_;
	}

	void	set_authorized()
	{
		is_authorized_ = true;
	}

	void	set_authorize_check(int ms)
	{
		authrize_check_time_ = ms;
	}

protected:
	int		authrize_check_time_;
	void	start_recv()
	{
		s.async_receive(
			boost::asio::buffer(reading_buffer_.buffer(), reading_buffer_.buffer_left()),
			boost::bind(&remote_socket_impl<inherit, socket_service_t>::on_recv_notify,
			shared_from_this(), 
			asio::placeholders::error,
			asio::placeholders::bytes_transferred));

 		check_athorize_timer_.expires_from_now(boost::posix_time::millisec(authrize_check_time_));
 		check_athorize_timer_.async_wait(
			boost::bind(&remote_socket_impl<inherit, socket_service_t>::on_check_athorize,
			shared_from_this(),
			boost::asio::placeholders::error));
	}

	void	on_check_athorize(boost::system::error_code ec)
	{
		if (!is_authorized()){
			close();
		}
	}
};

template<class socket_service_t>
class remote_socket : public remote_socket_impl<remote_socket<socket_service_t>, socket_service_t>
{
public:
	remote_socket(net_server<remote_socket<socket_service_t>>& srv):
		remote_socket_impl<remote_socket<socket_service_t>, socket_service_t>(srv)
	{
		set_authorize_check(10000);
	}
};

template<class remote_t>
struct net_server
{
	typedef boost::shared_ptr<remote_t> remote_socket_ptr;
	friend struct remote_socket_impl<remote_t, typename remote_t::client_t>;
protected:
	std::vector<acceptor_ptr>	  acceptors_;
	std::vector<unsigned short>	ports_;
	bool												stoped_;
	std::vector<remote_socket_ptr>	remotes_; //Socket客户端

public:
	io_service			ios_;
	explicit net_server(int thread_count)
	{
	}

	void	add_acceptor( unsigned short port )
	{
		ports_.push_back(port);
	}

	bool	run()
	{
		boost::system::error_code ec;
		stoped_ = false;
		ios_.reset();
		for (size_t i = 0 ; i < ports_.size(); i++)
		{
			acceptors_.push_back(acceptor_ptr(new ip::tcp::acceptor(ios_)));
		}

		for (size_t i = 0 ; i < acceptors_.size(); i++)
		{
			acceptor_ptr pacc = acceptors_[i];

			pacc->open(ip::tcp::v4(), ec);	
			if(ec) return false;

			socket_base::reuse_address opt(true);
			pacc->set_option(opt, ec);
			if(ec) return false;

			ip::tcp::endpoint endpt(ip::tcp::v4(), ports_[i]);
			pacc->bind(endpt, ec);
			if(ec) return false;

			pacc->listen(socket_base::max_connections, ec);
			if(ec) return false;

			accept_some(pacc, 2);
		}
		return true;
	}

	bool	stop()
	{
		stoped_ = true;

		std::vector<remote_socket_ptr> cpl = remotes_;
		for (size_t i = 0; i < cpl.size(); i++)
		{
			cpl[i]->close();
		}
		remotes_.clear();

		boost::system::error_code ec;
		for (size_t i = 0 ; i < acceptors_.size(); i++)
		{
			acceptor_ptr pacc = acceptors_[i];
			pacc->close(ec);
		}
		acceptors_.clear();

		ios_.reset();
		ios_.poll(ec);

		ios_.stop();
		return true;
	}

	void accept_once( acceptor_ptr acc )
	{
		if (!acc.get()) return;

		remote_socket_ptr remote(new remote_t(*this));

		acc->async_accept(remote->s, 
			boost::bind(&net_server::on_accept_complete, this, 
			asio::placeholders::error,
			acc,
			remote));
	}

	void	accept_some( acceptor_ptr acc, int count )
	{
		for (int i = 0; i < count; i++)
		{
			accept_once(acc);
		}
	}

	std::vector<remote_socket_ptr>		get_remotes()
	{
		std::vector<remote_socket_ptr>	ret;
		{
			ret = remotes_;
		}
		return ret;
	}

protected:
	void			on_accept_complete(const boost::system::error_code& ec, acceptor_ptr pacc_, remote_socket_ptr remote_)
	{
		if (ec.value() == 0){
			remote_->security_check_ = boost::posix_time::microsec_clock::local_time();
			remote_->start_recv();
			remotes_.push_back(remote_);

			accept_once(pacc_);
		}
		else if(!stoped_){
			accept_once(pacc_);
		}
	}
};

template<class inherit>
class native_socket : public basic_socket_impl<inherit>
{
public:
	bool	is_connected_;
	native_socket(boost::asio::io_service& ios):basic_socket_impl<inherit>(ios)
	{
		is_connected_ = false;
	}

	int connect( std::string ip, int port, unsigned int time_out = 100 )
	{
		is_connected_ = false;
		boost::system::error_code ec;
		asio::ip::address addr = asio::ip::address::from_string(ip, ec);
		if(ec){
			return -1;
		}
		asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
		endpoint.address(addr);
		return connect(endpoint, time_out);
	}


	int connect( const boost::asio::ip::tcp::endpoint & peer_endpoint, unsigned int time_out = 100 )
	{
		is_connected_ = false;
		s.async_connect(peer_endpoint, boost::bind(&native_socket<inherit>::connect_handler,
			shared_from_this(),
			boost::asio::placeholders::error));

		boost::posix_time::ptime p1 = boost::posix_time::microsec_clock::local_time();
		while((boost::posix_time::microsec_clock::local_time() - p1).total_milliseconds() < time_out)
		{
			s.get_io_service().reset();
			s.get_io_service().poll();
			if (is_connected_){
				break;
			}
		}

		if(!is_connected_ ){
			return -1;
		}
		return 0;
	}

	void close()
	{
		basic_socket_impl<inherit>::close();
		is_connected_ = false;
	}

	virtual void on_connection_notify()
	{

	}

protected:
	void connect_handler(const boost::system::error_code& error)
	{
		if (!error){
			on_connection_notify();
			is_connected_ = true;
			s.async_receive(boost::asio::buffer(reading_buffer_.buffer(), reading_buffer_.buffer_left()), 
				boost::bind(&native_socket<inherit>::on_recv_notify, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	}
};

class	 socket_service_default {};
