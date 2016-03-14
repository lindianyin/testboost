#ifndef net_h__
#define net_h__

#include <cstdlib>
#include <iostream>
#include <boost/aligned_storage.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <cstdint>

//增加对齐
#pragma pack(1)
struct CPack
{
	uint32_t nLen;
	uint8_t  aBody[0];
};
#pragma pack


#define CPACK_BODY_MAX_SIZE (1024 * 64)


class session;
typedef boost::shared_ptr<session> session_ptr;
using boost::asio::ip::tcp;

//方法以handle_开头的都是回调的的方法
//所有类似async_read_some / async_write_some 之类的方法都是不能确保缓冲区里面的数据都发送
class session
	: public boost::enable_shared_from_this<session>
{
public:
	session(boost::asio::io_service& io_service)
		: socket_(io_service)
	{

	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		boost::asio::async_read(socket_,boost::asio::buffer(head_),
			boost::bind(&session::handle_read_header,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
	

	//读数据包头回调（里面包含长度信息）
	void handle_read_header(const boost::system::error_code& error,size_t bytes_transferred)
	{
		if (!error)
		{
			if (bytes_transferred < sizeof(CPack));
			{
				return;
			}

			CPack *pCPack = (CPack*)head_.data();
			int nLen = pCPack->nLen;

			if (nLen > CPACK_BODY_MAX_SIZE)
			{
				return;
			}
			boost::shared_array<char> buff(new char[nLen]);
			//不能使用async_read_some 
			boost::asio::async_read(socket_,boost::asio::buffer(buff.get(),nLen),
				boost::bind(&session::handle_read_body,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,buff, nLen));
		}
		else
		{
			std::cout << error.message() << std::endl;
		}
	}

	//读取包体完成回调
	void handle_read_body(const boost::system::error_code& error,
		size_t bytes_transferred, boost::shared_array<char> data,int nLen)
	{
		if (!error)
		{
			boost::asio::async_read(socket_,boost::asio::buffer(head_),
				boost::bind(&session::handle_read_header,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			std::cout << error.message() << std::endl;
		}
	}

	//发送数据
	void do_write(const char * pBuff,const int nLen)
	{
		int _nLen = sizeof(CPack) + nLen;
		boost::shared_array<char> buff_(new char[_nLen]);
		CPack *pCPack = (CPack*)buff_.get();
		pCPack->nLen = nLen;
		memcpy((void*)pCPack->aBody, pBuff, nLen);

		boost::asio::async_write(socket_,boost::asio::buffer(buff_.get(), _nLen),
			boost::bind(&session::handle_write, shared_from_this(),boost::asio::placeholders::error, buff_));

	}

	//发送完成后回调
	void handle_write(const boost::system::error_code& error, boost::shared_array<char> buff){
		if (!error) 
		{

		}
		else
		{

		}
	}


	~session()
	{

	}

	void close_socket()
	{
		this->socket_.shutdown(boost::asio::socket_base::shutdown_both);
		this->socket_.close();
	}

public:

private:
	tcp::socket socket_;
	boost::array<char,4> head_;
};


class server
{
public:
	server(boost::asio::io_service& io_service, short port)
		: io_service_(io_service),
		acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
	{
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		session_ptr new_session(new session(io_service_));
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
			boost::asio::placeholders::error));
	}

	void handle_accept(session_ptr new_session,
		const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}

		new_session.reset(new session(io_service_));
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
			boost::asio::placeholders::error));
	}

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
};
#endif // net_h__