#pragma once

#include "boost/date_time.hpp"
#include "boost/date_time/posix_time/time_formatters.hpp"
#include "boost/thread.hpp"
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <iostream>
#include <list>
#include <stdarg.h>
#include <string>
template<class out_put_t>
class log_file
{
public:
	out_put_t output_;

	log_file()
	{
		quit_ = false;
		trd = new boost::thread(boost::bind(&log_file<out_put_t>::log_write_thread, this));
	}
	~log_file()
	{
		stop_log();
		delete trd;
	}
	void	stop_log()
	{
		quit_ = true;
		trd->join();
	}

	bool	write_log(bool auto_break_line, const char* log_fmt, va_list ap)
	{
		char buff[2048];
		boost::posix_time::ptime p1 = boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time();
		std::string s = boost::posix_time::to_simple_string(p1);
		std::string sfmt;

		if (auto_break_line){
			sfmt = "[" + s + "]:" + log_fmt;
			sfmt += "\r\n";
		}
		else{
			sfmt = log_fmt;
		}

		vsprintf_s(buff, 2048, sfmt.c_str(), ap);
		logs_.push_back(std::string(buff));
		return true;
	}

	bool	write_log(const char* log_fmt, ...)
	{
		boost::unique_lock<boost::mutex> l(mtu_);
		va_list ap;
		va_start(ap, log_fmt);
		bool ret = write_log(true, log_fmt, ap);
		va_end(ap);
		return ret;
	}

	bool	write_log_no_break(const char* log_fmt, ...)
	{
		boost::unique_lock<boost::mutex> l(mtu_);
		va_list ap;
		va_start(ap, log_fmt);
		bool ret = write_log(false, log_fmt, ap);
		va_end(ap);

		return ret;
	}

private:
	boost::mutex mtu_;
	boost::thread* trd;
	bool	quit_;
	std::list<std::string> logs_;
	void	log_write_thread()
	{
		std::list<std::string> logback;
		while(!quit_)
		{
			{
				boost::unique_lock<boost::mutex> l(mtu_);
				logback = logs_;
				logs_.clear();
				while(!logback.empty())
				{
					output_.output((char*)logback.front().c_str(), logback.front().length());
					logback.pop_front();
				}
			}
			boost::this_thread::sleep(boost::posix_time::millisec(500));
		}
	}
};

struct file_output
{
	std::string fname_;
	void output(char* buff,  unsigned int len)
	{
		FILE* fp = NULL;
		int err = fopen_s(&fp, fname_.c_str(), "a");
		if(fp){
			fwrite(buff, 1, len, fp);
			fclose(fp);
		}
	}
};

struct cout_output
{
	void output(char* buff,  unsigned int len)
	{
		std::cout<<buff;
	}
};

struct cout_and_file_output
{
	std::string fname_;
	void output(char* buff,  unsigned int len)
	{
		FILE* fp = NULL;
		fopen_s(&fp, fname_.c_str(), "a");
		if(fp){
			fwrite(buff, 1, len, fp);
			fclose(fp);
		}
		std::cout<<buff;
	}
};

//#ifdef _X86_

struct debug_output
{
public:
	void output(char* buff,  unsigned int len)
	{
		//::OutputDebugStringA(buff);
	}
};
//#endif