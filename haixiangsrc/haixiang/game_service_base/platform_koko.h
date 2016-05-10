#pragma once
#include "net_service.h"
#include "msg_client_common.h"
#include "utility.h"

extern std::string	md5_hash(std::string sign_pat);

template<class service_t>
class user_koko_auto_trade_in : public boost::enable_shared_from_this<user_koko_auto_trade_in<service_t>>
{
public:
	std::string					uid_;
	std::string					platform_uid_;
	user_koko_auto_trade_in(service_t& srv) :
		the_service(srv),
		check_result_(srv.timer_sv_)
	{
		response_ = 0;
	}

	int		do_trade(){
		sn_ = get_guid();
		int ret = on_trade_begin();
		if (ret == ERROR_SUCCESS_0){
			ret = send_request();
			if (ret == ERROR_SUCCESS_0)	{
				check_result_.expires_from_now(boost::posix_time::seconds(5));
				check_result_.async_wait(boost::bind(&user_koko_auto_trade_in::on_check_result,
					shared_from_this(), 
					boost::asio::placeholders::error));
				the_service.koko_trades_.insert(std::make_pair(sn_, shared_from_this()));
			}
		}
		on_trade_sended(ret);
		return ret;
	}
	//收到回复
	virtual int		handle_response(msg_koko_trade_inout_ret& msg)
	{
		if (response_) return ERROR_SUCCESS_0;

		response_ = true;
		Database& db = *the_service.gamedb_;	
		BEGIN_UPDATE_TABLE("trade_inout");
		if (msg.result_ == 0 || msg.result_ == 1){
			__int64 out_count = 0;
			int ret = the_service.cache_helper_.give_currency(uid_, msg.count_, out_count, false);
			if (ret == ERROR_SUCCESS_0){
				SET_FIELD_VALUE("state", 1003);
				player_ptr pp = the_service.get_player(uid_);
				if (pp.get()) {
					msg_currency_change msg;
					msg.credits_ = (double) out_count;
					msg.why_ = msg_currency_change::why_trade_complete;
					auto psock = pp->from_socket_.lock();
					if (psock.get()){
						send_msg(psock, msg);
					}
				}
			}
			else{
				SET_FIELD_VALUE("state", 1005);
			}
			SET_FIELD_VALUE("count", msg.count_);
		}
		else{
			SET_FIELD_VALUE("state", 1004);
		}
		SET_FINAL_VALUE("return_code", msg.result_);
		WITH_END_CONDITION("where ordernum='" + sn_ + "'");
		EXECUTE_IMMEDIATE();
		the_service.koko_trades_.erase(sn_);
		check_result_.cancel(boost::system::error_code());
		return ERROR_SUCCESS_0;
	}
protected:
	service_t&					the_service;
	std::string					sn_;
	boost::asio::deadline_timer		check_result_;
	bool			response_;


	void			on_check_result(boost::system::error_code ec)
	{
		if (ec.value() == 0){
			if (!response_){

				int ret = send_request();

				Database& db = *the_service.gamedb_;	
				BEGIN_UPDATE_TABLE("trade_inout");
				if (ret == ERROR_SUCCESS_0)	{
					SET_FINAL_VALUE("state", 1001);
				}
				else{
					SET_FINAL_VALUE("state", 1002);
				}
				WITH_END_CONDITION("where ordernum='" + sn_ + "'");
				EXECUTE_IMMEDIATE();

				check_result_.expires_from_now(boost::posix_time::seconds(5));
				check_result_.async_wait(
					boost::bind(&user_koko_auto_trade_in::on_check_result,
					shared_from_this(), 
					boost::asio::placeholders::error));
			}
		}
	}

	virtual int		on_trade_begin()
	{
		Database& db = *the_service.gamedb_;
		BEGIN_INSERT_TABLE("trade_inout");
		SET_FIELD_VALUE("uid", uid_);
		SET_FIELD_VALUE("dir", 1);				//0  out, 1 in
		SET_FINAL_VALUE("ordernum", sn_);
		EXECUTE_IMMEDIATE();
		return ERROR_SUCCESS_0;
	}

	virtual int		on_trade_sended(int ret)
	{
		Database& db = *the_service.gamedb_;
		BEGIN_UPDATE_TABLE("trade_inout");
		if (ret == ERROR_SUCCESS_0)	{
			SET_FINAL_VALUE("state", 1000);	
		}
		else{
			SET_FINAL_VALUE("state", 999);
		}
		WITH_END_CONDITION("where ordernum='" + sn_ + "'");
		EXECUTE_IMMEDIATE();
		return ERROR_SUCCESS_0;
	}

	virtual int		send_request()
	{
		msg_koko_trade_inout msg;
		msg.count_ = 0;
		msg.dir_ = 1;
		msg.time_stamp_ = ::time(nullptr);
		COPY_STR(msg.sn_, sn_.c_str());
		COPY_STR(msg.uid_, platform_uid_.c_str());
		build_sign(msg);
		return send_msg(the_service.world_connector_, msg, false, true);
	}

	void			build_sign(msg_koko_trade_inout& msg)
	{
		std::string pat = msg.uid_;
		pat += boost::lexical_cast<std::string>(msg.dir_);
		pat += boost::lexical_cast<std::string>(msg.count_);
		pat += boost::lexical_cast<std::string>(msg.time_stamp_);
		pat += msg.sn_;
		pat += the_service.the_config_.world_key_;
		std::string si = md5_hash(pat);
		COPY_STR(msg.sign_, si.c_str());
	}
};

template<class service_t>
class  user_koko_auto_trade_out : public user_koko_auto_trade_in<service_t>
{
public:
	user_koko_auto_trade_out(service_t& srv):
		user_koko_auto_trade_in(srv)
	{
		count_ = 0;
	}
protected:
	//先从cache里扣钱,
	virtual int		on_trade_begin()
	{
		user_koko_auto_trade_in::on_trade_begin();
		__int64 outc = 0;
		int ret = the_service.cache_helper_.apply_cost(uid_, 0, outc, true, false);
		if (ret == ERROR_SUCCESS_0){
			count_ = outc;
		}
		return ret;
	}

	virtual int		send_request()
	{
		msg_koko_trade_inout msg;
		COPY_STR(msg.uid_, platform_uid_.c_str());
		msg.count_ = count_;
		msg.dir_ = 2;
		msg.time_stamp_ = ::time(nullptr);
		COPY_STR(msg.sn_, sn_.c_str());
		COPY_STR(msg.uid_, platform_uid_.c_str());
		build_sign(msg);
		return send_msg(the_service.world_connector_, msg, false, true);
	}

	//如果请求koko平台失败,则返钱
	virtual int		on_trade_sended(int ret)
	{
		user_koko_auto_trade_in::on_trade_sended(ret);
		if (ret != ERROR_SUCCESS_0 && count_ > 0){

			Database& db = *the_service.gamedb_;	
			BEGIN_UPDATE_TABLE("trade_inout");
			__int64 outc = 0;
			ret = the_service.cache_helper_.give_currency(uid_, count_, outc, false);
			if (ret == ERROR_SUCCESS_0){
				SET_FIELD_VALUE("count", count_);
				SET_FINAL_VALUE("state", 1006);
				count_ = 0;
			}
			else{
				SET_FINAL_VALUE("state", 1007);
			}
			WITH_END_CONDITION("where ordernum='" + sn_ + "'");
			EXECUTE_IMMEDIATE();
		}
		return ret;
	}

	virtual int		handle_response(msg_koko_trade_inout_ret& msg)
	{
		if (response_) return ERROR_SUCCESS_0;

		response_ = true;
		Database& db = *the_service.gamedb_;	
		BEGIN_UPDATE_TABLE("trade_inout");
		if (msg.result_ == 0 || msg.result_ == 1){
			SET_FIELD_VALUE("count", count_);
			SET_FIELD_VALUE("state", 1003);
			count_ = 0;
		}
		else{
			__int64 outc = 0;
			int ret = the_service.cache_helper_.give_currency(uid_, count_, outc, false);
			if (ret == ERROR_SUCCESS_0){
				SET_FIELD_VALUE("state", 1006);
				count_ = 0;
			}
			else{
				SET_FIELD_VALUE("state", 1007);
			}
		}
		SET_FINAL_VALUE("return_code", msg.result_);
		WITH_END_CONDITION("where ordernum='" + sn_ + "'");
		EXECUTE_IMMEDIATE();
		the_service.koko_trades_.erase(sn_);
		check_result_.cancel(boost::system::error_code());
		return ERROR_SUCCESS_0;
	}
	__int64			count_;
};

template<class service_t>
class platform_koko
{
public:
	service_t&		the_service;

	platform_koko(service_t& srv):the_service(srv)
	{

	}

	template<class player_t>
	void		auto_trade_in(player_t p)
	{
		user_koko_auto_trade_in<service_t>* ptrade = new user_koko_auto_trade_in<service_t>(the_service);
		boost::shared_ptr<user_koko_auto_trade_in<service_t>> pt(ptrade);
		ptrade->platform_uid_ = p->platform_uid_;
		ptrade->uid_ = p->uid_;
		ptrade->do_trade();
	}

	template<class player_t>
	void		auto_trade_out(player_t p)
	{
		user_koko_auto_trade_out<service_t>* ptrade = new user_koko_auto_trade_out<service_t>(the_service);
		boost::shared_ptr<user_koko_auto_trade_out<service_t>> pt(ptrade);
		ptrade->platform_uid_ = p->platform_uid_;
		ptrade->uid_ = p->uid_;
		ptrade->do_trade();
	}
};