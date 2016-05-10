#pragma once
#include "boost/smart_ptr.hpp"
#include "boost/asio.hpp"
#include "boost/date_time.hpp"

namespace HttpUtility
{  
	typedef unsigned char BYTE;  

	inline BYTE toHex(const BYTE &x)  
	{  
		return x > 9 ? x -10 + 'A': x + '0';  
	}  

	inline BYTE fromHex(const BYTE &x)  
	{  
		return isdigit(x) ? x-'0' : x-'A'+10;  
	}  

	inline std::string URLEncode(const std::string &sIn)  
	{  
		std::string sOut;  
		for( size_t ix = 0; ix < sIn.size(); ix++ )  
		{        
			BYTE buf[4];  
			memset( buf, 0, 4 );  
			if( isalnum( (BYTE)sIn[ix] ) )  
			{        
				buf[0] = sIn[ix];  
			}  
			else  
			{  
				buf[0] = '%';  
				buf[1] = toHex( (BYTE)sIn[ix] >> 4 );  
				buf[2] = toHex( (BYTE)sIn[ix] % 16);  
			}  
			sOut += (char *)buf;  
		}  
		return sOut;  
	};  

	inline std::string URLDecode(const std::string &sIn)  
	{  
		std::string sOut;  
		for( size_t ix = 0; ix < sIn.size(); ix++ )  
		{  
			BYTE ch = 0;  
			if(sIn[ix]=='%')  
			{  
				ch = (fromHex(sIn[ix+1])<<4);  
				ch |= fromHex(sIn[ix+2]);  
				ix += 2;  
			}  
			else if(sIn[ix] == '+')  
			{  
				ch = ' ';  
			}  
			else  
			{  
				ch = sIn[ix];  
			}  
			sOut += (char)ch;  
		}  
		return sOut;  
	}  
}

enum request_failed
{
	connection_fail,
	connection_break,
	http_response_error_code,
	http_business_error,
	http_business_repeat,
	http_timeout,
	http_error_exbegin = 100,
};

enum
{
	ret_head_report_error = http_error_exbegin,
	ret_content_length_error,
	ret_success,
	ret_continue_recv,
};

static std::string build_http_request(std::string host, std::string param)
{
	std::string ret = "GET %s HTTP/1.1\r\n";
	char c[1024] = {0};
	sprintf(c, ret.c_str(), param.c_str());
	ret = c;
	ret += "Connection:Keep-Alive\r\n";
	ret += "Host:" + host + "\r\n\r\n";
	return ret;
}

static void add_http_params(std::string& param, std::string k, __int64 v)
{
	if (param.empty()){
		param = k + "=" + boost::lexical_cast<std::string>(v);
	}
	else 
		param += "&" + k + "=" + boost::lexical_cast<std::string>(v);
}

static void add_http_params(std::string& param, std::string k, std::string v)
{
	v = HttpUtility::URLEncode(v);
	if (param.empty()){
		param = k + "=" + v;
	}
	else 
		param += "&" + k + "=" + v;
}

static int  http_request_parse(std::string recved, std::string& request)
{
	int	 ret = ret_continue_recv;
	do 
	{
		const char* str_body = strstr((char*)recved.c_str(), "\r\n\r\n");
		if (!str_body){
			ret = ret_continue_recv;
			break;
		}
		str_body += 4;
		char* cstart = (char*)recved.c_str();
		char* get = strstr(cstart, "GET ");
		char* post = strstr(cstart, "POST ");
		//如果是GET方式
		if (get == cstart){
			char* end = strstr(get, " HTTP/1.1");
			if (end){
				ret = ret_success;
				request.insert(request.end(), get + strlen("GET "), end);
			}
			else{
				ret = ret_content_length_error;
			}
			break;
		}
		//如果是POST方式
		else{
			bool chunked = false;
			const char* np = strstr((char*)recved.c_str(), "Content-Length:");
			if (!np){
				np = strstr((char*)recved.c_str(), "Transfer-Encoding:");
				if (!np){
					ret = ret_content_length_error;
					break;
				}
				else{
					np = strstr(np + strlen("Transfer-Encoding:"), "chunked");
					if (np){
						chunked = true;
					}
					else{
						ret = ret_content_length_error;
						break;
					}
				}
			}

			unsigned int	content_len = 0;
			if (chunked){
				const char* end = strstr(str_body, "\r\n\r\n");
				if (end){
					content_len = end - str_body;
				}
				//如果找不到"\r\n\r\n,表明块没有结束，继续收"
				else{
					content_len = 0xFFFFFFFF;
				}
			}
			else{
				const char* ls = np + strlen("Content-Length:");
				const char* le = strstr(ls, "\r\n");
				if (!le || le <= ls){
					ret = ret_content_length_error;
					break;
				}
				std::string slen;
				slen.insert(slen.end(), ls, le);
				sscanf(slen.c_str(), "%ud", &content_len);
			}

			if (int(recved.size() - (str_body - recved.c_str())) >= content_len){
				ret = ret_success;
				request.insert(request.end(), str_body, str_body + content_len);
			}
			else{
				ret = ret_continue_recv;
			}
		}
	} while (0);
	return ret;
}


static int	http_response_parse(std::string recved, std::string& http_head, std::string& http_body)
{
	int	 ret = ret_continue_recv;
	do 
	{
		char* first_line = strstr((char*)recved.c_str(), "\r\n");
		if (!first_line){
			ret = ret_continue_recv;
			break;
		}

		if (!strstr(recved.c_str(), "HTTP/1.1 200 OK\r\n")){
			ret = http_business_error;
			break;
		}

		const char* str_body = strstr((char*)recved.c_str(), "\r\n\r\n");
		if (!str_body){
			ret = ret_continue_recv;
			break;
		}
		str_body += 4;

		bool chunked = false;
		const char* np = strstr((char*)recved.c_str(), "Content-Length:");
		if (!np){
			np = strstr((char*)recved.c_str(), "Transfer-Encoding:");
			if (!np){
				ret = ret_content_length_error;
				break;
			}
			else{
				np = strstr(np + strlen("Transfer-Encoding:"), "chunked");
				if (np){
					chunked = true;
				}
				else{
					ret = ret_content_length_error;
					break;
				}
			}
		}

		unsigned int	content_len = 0;
		if (chunked){
			const char* end = strstr(str_body, "\r\n\r\n");
			if (end){
				content_len = end - str_body;
			}
			//如果找不到"\r\n\r\n,表明块没有结束，继续收"
			else{
				content_len = 0xFFFFFFFF;
			}
		}
		else{
			const char* ls = np + strlen("Content-Length:");
			const char* le = strstr(ls, "\r\n");
			if (!le || le <= ls){
				ret = ret_content_length_error;
				break;
			}
			std::string slen;
			slen.insert(slen.end(), ls, le);
			sscanf(slen.c_str(), "%ud", &content_len);
		}

		if (int(recved.size() - (str_body - recved.c_str())) >= content_len){
			ret = ret_success;
			http_head.insert(http_head.end(), (char*)recved.c_str(), str_body);
			http_body.insert(http_body.end(), str_body, str_body + content_len);
		}
		else{
			ret = ret_continue_recv;
		}
	} while (0);
	return ret;
}

//这个函数不支持多线程
template<class remote_t>
int			pickup_http_request_from_socket(remote_t psock, std::string& request)
{
	static char data[1024];
	if(psock->pickup_data(data, 1024, false, true)){
		int ret = http_request_parse(data, request);
		if (ret == ret_success ||
			ret == ret_continue_recv){
			return 0;
		}
	}
	return -1;
}

template<class http_request_t>
class http_request_handler
{
public:
	boost::shared_ptr<http_request_t> the_http_;
	int		repeat_;
	http_request_handler()
	{
		repeat_ = 0;
	}
	static boost::shared_ptr<http_request_handler<http_request_t>> build_handler(boost::shared_ptr<http_request_t> the_request)
	{
		boost::shared_ptr<http_request_handler<http_request_t>> http_handler(new http_request_handler<http_request_t>());
		http_handler->the_http_ = the_request;
		http_handler->repeat_ = the_request->repeat_count_;
		return http_handler;
	}
	void		on_connect_notify(const boost::system::error_code& ec)
	{
		//如果repeat_ != the_request->repeat_count_, 说明这个回调已经迟了，忽略这次回调
		if (repeat_ == the_http_->repeat_count_){
			the_http_->on_connect_notify(ec);
		}
	}
	void	on_sended_notify(const boost::system::error_code& ec, size_t byte_transfered)
	{
		//如果repeat_ != the_request->repeat_count_, 说明这个回调已经迟了，忽略这次回调
		if (repeat_ == the_http_->repeat_count_){
			the_http_->on_sended_notify(ec, byte_transfered);
		}
	}
	void	on_recv_notify(const boost::system::error_code& ec, size_t byte_transfered)
	{
		//如果repeat_ != the_request->repeat_count_, 说明这个回调已经迟了，忽略这次回调
		if (repeat_ == the_http_->repeat_count_){
			the_http_->on_recv_notify(ec, byte_transfered);
		}
	}
};

template<class inherit>
class http_request : public boost::enable_shared_from_this<inherit>
{
public:

	boost::asio::io_service& ios_;
	boost::asio::ip::tcp::socket s_;
	char					recv_buffer_[1024];
	unsigned int	recv_len_;
	std::string		str_request_;
	unsigned int	str_request_sended_;
	int						repeat_count_;
	boost::posix_time::ptime pstart_;
	int						dur_;
	std::string		recved;
	http_request(boost::asio::io_service& ios) : ios_(ios), s_(ios)
	{
		memset(recv_buffer_, 0, 1024);
		recv_len_ = 0;

		repeat_count_ = 0;
		pstart_ = boost::posix_time::microsec_clock::local_time();
		dur_ = -1;
	}

	virtual void	reset()
	{
		memset(recv_buffer_, 0, 1024);
		recv_len_ = 0;
		recved.clear();

		boost::system::error_code ec;
		s_.shutdown(socket_base::shutdown_both, ec);
		s_.close(ec);
		s_.get_io_service().reset();
		s_.get_io_service().poll(ec);
	}

	void		request(std::string str_request, std::string host, std::string port)
	{
		write_log("http_request request");
		str_request_ = str_request;
		str_request_sended_ = 0;

		boost::system::error_code ec;

		ip::tcp::resolver resolv(ios_);
		ip::tcp::resolver::query q(host, port);
		boost::asio::ip::tcp::resolver::iterator edp_it = resolv.resolve(q, ec);

		if(ec.value()!= 0){
			on_request_failed(connection_fail, ec.message());
		}
		else{
			auto http_handler = http_request_handler<inherit>::build_handler(shared_from_this());
			s_.async_connect(*edp_it,
				boost::bind(&http_request_handler<inherit>::on_connect_notify,
				http_handler,
				boost::asio::placeholders::error));
		}
	}

	void		on_connect_notify(const boost::system::error_code& ec)
	{
		if (ec.value() == 0){
			write_log("http_request connected");
			auto http_handler = http_request_handler<inherit>::build_handler(shared_from_this());
			s_.async_send(asio::buffer(str_request_.c_str(), str_request_.size()), 
				boost::bind(&http_request_handler<inherit>::on_sended_notify,
				http_handler,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
		else{
			write_log("http_request connect failed!");
			on_request_failed(connection_fail, ec.message());
		}
	}

	void	on_sended_notify(const boost::system::error_code& ec, size_t byte_transfered)
	{
		if (ec.value() == 0){
			if (byte_transfered + str_request_sended_ < str_request_.size()){
				str_request_sended_ += byte_transfered;

				auto http_handler = http_request_handler<inherit>::build_handler(shared_from_this());

				s_.async_send(asio::buffer(str_request_.c_str() + str_request_sended_, str_request_.size() - str_request_sended_), 
					boost::bind(&http_request_handler<inherit>::on_sended_notify,
					http_handler,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			}
			else{
				std::string s = "http_request sended success, request data:" + str_request_;
				for (int i = 0; i < (int)s.size(); i++)
				{
					if (s[i] == '%'){
						s[i]= '*';
					}
				}
				write_log(s.c_str());

				auto http_handler = http_request_handler<inherit>::build_handler(shared_from_this());

				s_.async_receive(asio::buffer(recv_buffer_, 1024), 
					boost::bind(&http_request_handler<inherit>::on_recv_notify,
					http_handler,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			}
		}
		else{
			write_log("http_request sended failed!");
			on_request_failed(connection_fail, ec.message());
		}
	}

	void	continue_recv()
	{
		auto http_handler = http_request_handler<inherit>::build_handler(shared_from_this());
		s_.async_receive(asio::buffer(recv_buffer_, 1024), 
			boost::bind(&http_request_handler<inherit>::on_recv_notify,
			http_handler,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}

	void	on_recv_notify(const boost::system::error_code& ec, size_t byte_transfered)
	{
		dur_ = (int)(boost::posix_time::microsec_clock::local_time() - pstart_).total_milliseconds();
		if (ec.value() == 0){
			recved.insert(recved.end(), recv_buffer_, recv_buffer_ + byte_transfered);
			write_log("http_request recv data: %s", recved.c_str());
			std::string http_head, http_body;

			int ret = http_response_parse(recved, http_head, http_body);

			if (ret == ret_success){
				handle_http_body(http_body);
			}
			else if (ret == ret_continue_recv){
				continue_recv();
			}
			else if (
				ret == ret_head_report_error ||
				ret == ret_content_length_error){
					on_request_failed(http_response_error_code, ec.message());
			}
		}
		else{
			on_request_failed(connection_break, ec.message());
		}
	}

	virtual void	on_request_failed(int reason, std::string err_msg)
	{
		write_log("http_request failed, reason = %d, info = %s", reason, err_msg.c_str());
	}
	virtual void	write_log(const char* log_fmt, ...){};
	virtual	void	handle_http_body(std::string str_body) = 0;
};
