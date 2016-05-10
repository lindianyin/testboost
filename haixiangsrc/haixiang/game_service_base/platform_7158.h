#pragma once
#include "http_request.h"
#include "platform_gameid.h"
#include "game_ids.h"
#include "base64Con.h"

template<class real_t>
class http_request_with_log : public http_request<real_t>
{
protected:
	http_request_with_log(boost::asio::io_service& srv):http_request<real_t>(srv)
	{

	}
	virtual void	write_log(const char* log_fmt, ...)
	{
		va_list ap;
		va_start(ap, log_fmt);
		bool ret = glb_http_log.write_log(log_fmt, ap);
		va_end(ap);
	}
};

template<class service_t, class config_t>
class platform_broadcast_7158 : public http_request_with_log<platform_broadcast_7158<service_t, config_t>>
{
public:
	std::string idx_;
	std::string nickname_;
	std::string content_;
	config_t& platform_config_;
	platform_broadcast_7158(service_t& srv, config_t& cnf) : 
		http_request_with_log(srv.http_sv_), platform_config_(cnf)
	{

	}
	void do_broadcast()
	{
		static unsigned int gsn = 1;
		std::string str_sign = boost::lexical_cast<std::string>(gsn++);
		str_sign += idx_ + nickname_ + content_ + platform_config_.broadcast_skey_;

		std::string sign_key;
		unsigned char out_put[16];
		CMD5 md5;
		md5.MessageDigest((const unsigned char*)str_sign.c_str(), str_sign.length(), out_put);
		for (int i = 0; i < 16; i++)
		{
			char aa[4] = {0};
			sprintf_s(aa, 4, "%02x", out_put[i]);
			sign_key += aa;
		}
		std::string param;
		add_http_params(param, "sn", gsn - 1);
		add_http_params(param, "uidx", idx_);
		add_http_params(param, "nickname", nickname_);
		add_http_params(param, "text", content_);
		add_http_params(param, "sign", sign_key);
		add_http_params(param, "gameid", platform_config_.game_id_);

		std::string str_req = build_http_request(platform_config_.host_, 
			platform_config_.broadcast_param_ + param);

		request(str_req, platform_config_.host_, platform_config_.port_);
	}
	virtual	void	handle_http_body(std::string str_body){};
};

class platform_config_7158
{
public:
	std::string		fromid_;
	std::string		game_id_;//接入平台约定的游戏id
	std::string		host_;
	std::string		skey_;
	std::string		verify_param_;
	std::string		trade_param_;
	std::string		query_param_;
	std::string		port_;
	std::string		broadcast_param_;
	std::string		broadcast_skey_;
	float					rate_;	//金豆:7158

	platform_config_7158()
	{
		fromid_ = "1";
		game_id_ = GAME_ID_7158;

		port_ = "80";
		host_ = "api2.7158.com";
		skey_ = "E743C9007A97702A58F7529FFFA06A0BE39CE1C48579C37E0939796BB63358E16FEF493AAAEA0F78";
		verify_param_ = "/I_Verify.aspx?";
		trade_param_ = "/I_JB.aspx?";
		query_param_ = "/I_getCurJB.aspx?";
		broadcast_param_ = "/I_WinningInfo.aspx?";
		rate_ = 1.0;
		broadcast_skey_ = "8D8025CBE4ABFBB6FE3F1C3574DAAC0F9BDEC2D56ABC713BF52D5BBE5EB00D24E04B1D78AD4B37CA";
	}

	static platform_config_7158& get_instance()
	{
		static platform_config_7158	conf_7158_;
		return conf_7158_;
	}
};

template<class service_t, class config_t>
class user_7158_auto_trade : public http_request_with_log<user_7158_auto_trade<service_t, config_t>>
{
public:
	std::string	platform_id_;
	longlong		out_c, out_platform_;
	std::string	uid_;
	std::string platform_uid_;
	std::string orderkey_;
	std::string idx_;
	int					is_trade_in_;//1兑入,0兑出
	service_t&	the_service_;
	std::string	sn_;
	std::vector<std::string> v;
	boost::asio::deadline_timer time_out_;
	config_t&		platform_config_;
	user_7158_auto_trade(service_t& srv, config_t& cnf) :
		http_request_with_log<user_7158_auto_trade<service_t, config_t>>(srv.http_sv_),
		the_service_(srv), time_out_(srv.http_sv_), platform_config_(cnf)
	{
		out_c = 0;
		out_platform_ = 0;
		sn_ = get_guid();
		auto it = sn_.begin();
		while (it != sn_.end()){
			if (*it == '-'){
				it = sn_.erase(it);
			}
			else 
				it++;
		}
	}

	void	reset()
	{
		http_request::reset();
		v.clear();
	}

	void	on_time_out(boost::system::error_code ec)
	{
		if (ec.value() == 0){
			record_result(10, sn_, "time out");
			on_request_failed(http_timeout, "time out");
		}
	}

	int	do_trade()
	{
		if (is_trade_in_ == 0){
			longlong cap = the_service.cache_helper_.get_var<longlong>(uid_ + KEY_CUR_TRADE_CAP, -1);
			longlong has_money = 0;
			int r = the_service_.cache_helper_.apply_cost(uid_, 0, has_money, true, false);
			if (r != ERROR_SUCCESS_0){
				write_log("%s user tradeout failed on cache error!", platform_id_.c_str(), out_c);
				return -1;
			}
			out_c = has_money - cap;
			//如果不够兑换
			if (out_c <= 0) {
				//把扣的钱还给cache
				r = the_service_.cache_helper_.give_currency(uid_, has_money, has_money, false);
				if (r != ERROR_SUCCESS_0){
					write_log("%s user tradeout failed on cache error!", platform_id_.c_str(), out_c);
					return -2;
				}
				else{
					return 0;
				}
			}
			//钱还给cache
			r = the_service_.cache_helper_.give_currency(uid_, cap, cap, false);
			if (r != ERROR_SUCCESS_0){
				write_log("%s user tradeout failed on cache error!", platform_id_.c_str(), out_c);
				return -3;
			}
			write_log("%s user tradeout begin! count = %d", platform_id_.c_str(), out_c);
			out_platform_ = (longlong)(out_c / platform_config_.rate_);
		}

		record_result(0, sn_, "");
		time_out_.expires_from_now(boost::posix_time::milliseconds(5000));
		time_out_.async_wait(boost::bind(&user_7158_auto_trade<service_t, config_t>::on_time_out,
			shared_from_this(), boost::asio::placeholders::error));

		std::string param,		str_sign;
		add_http_params(param, "gameID", platform_config_.game_id_);
		str_sign += platform_config_.game_id_;
		if (is_trade_in_){
			add_http_params(param, "type", 2);
			str_sign += "2";
		}
		else{
			add_http_params(param, "type", 1);
			str_sign += "1";
		}
		add_http_params(param, "IDX", idx_);
		str_sign += idx_;
		add_http_params(param, "UID", platform_uid_);
		str_sign += platform_uid_;

		if (platform_id_ == "7158"){
			if (is_trade_in_){
				add_http_params(param, "orderNum", "");
				add_http_params(param, "decMoney", out_c);
				str_sign += boost::lexical_cast<std::string>(out_c);
				add_http_params(param, "addMoney", 0);
				str_sign += "0";
			}
			else{
				add_http_params(param, "orderNum", orderkey_);
				str_sign += orderkey_;
				add_http_params(param, "decMoney", 0);
				str_sign += "0";
				add_http_params(param, "addMoney", out_platform_);
				str_sign += boost::lexical_cast<std::string>(out_platform_);
			}
		}
		else if (platform_id_ == "t58"){
			if (is_trade_in_){
				add_http_params(param, "decMoney", 0);
				str_sign += "0";
			}
			else{
				add_http_params(param, "decMoney", out_platform_);
				str_sign += boost::lexical_cast<std::string>(out_platform_);
			}	
			str_sign += sn_; //跳舞吧验证需要sn,7158不需要
		}

		add_http_params(param, "sn", sn_);
		str_sign += platform_config_.skey_;

		unsigned char out_put[16];
		CMD5 md5;
		md5.MessageDigest((const unsigned char*)str_sign.c_str(), str_sign.length(), out_put);
		std::string sign_key;
		for (int i = 0; i < 16; i++)
		{
			char aa[4] = {0};
			sprintf_s(aa, 4, "%02x", out_put[i]);
			sign_key += aa;
		}
		add_http_params(param, "sign", sign_key);

		std::string str_req = build_http_request(platform_config_.host_, 
			platform_config_.trade_param_ + param);
		request(str_req, 
			platform_config_.host_,
			platform_config_.port_);
		return 0;
	}
protected:
	void trim(std::string& s)
	{
		auto it = s.begin();
		while (it != s.end()){
			if (*it == ' ' || *it == 0){
				it = s.erase(it);
			}
			else
				it++;
		}
	}
	void		record_result(int nstate, std::string ordernum, std::string ret_code)
	{
		BEGIN_INSERT_TABLE("trade_inout");
		SET_FIELD_VALUE("uid", uid_);
		SET_FIELD_VALUE("dir", is_trade_in_);

		if (is_trade_in_){
			SET_FIELD_VALUE("count", out_c);
		}
		else{
			SET_FIELD_VALUE("count", out_platform_);
		}

		SET_FIELD_VALUE("state", nstate);		//3的话表示兑出失败，钱还回cache成功
		SET_FIELD_VALUE("ordernum", ordernum);
		SET_FINAL_VALUE("return_code", ret_code);
		EXECUTE_NOREPLACE_DELAYED("", the_service_.delay_db_game_);
	}

	void		trade_inout_failed(int reason, std::string response)
	{
		repeat_count_ = 0x7FFFFFFF;
		std::string ret_code = response;
		//兑出失败
		if (!is_trade_in_){
			longlong out = 0;
			int r = the_service_.cache_helper_.give_currency(uid_, out_c, out, true);
			out_c = 0;
			if (r != ERROR_SUCCESS_0){
				record_result(21,   sn_, ret_code);
				write_log("%s user trade out failed and cache return %d", platform_id_.c_str(), r);
			}
			else{
				record_result(20,   sn_, ret_code);
				write_log("%s user trade out failed and cache return 0", platform_id_.c_str());
			}
		}
		//兑入失败
		else{
			record_result(3, sn_, ret_code);
			write_log("%s user trade_inout failed! return code = %s, reason = %d", platform_id_.c_str(),  ret_code.c_str(), reason);
		}
		boost::system::error_code ec;
		time_out_.cancel(ec);
	}

	virtual	void	handle_http_body(std::string str_body)
	{
		split_str(str_body.c_str(), str_body.length(), "|", v, false);
		if (v.size() > 0){
			//成功
			if (v[0].size() > 0 && (v[0]== "0" || v[0] == "10") && v.size() == 3){
				trim(v[0]);	trim(v[1]); trim(v[2]);
				//如果是兑入成功
				if (is_trade_in_ == 1){
					try{
						out_c = boost::lexical_cast<longlong>(v[2]);
					}
					catch(...){
						write_log("boost::lexical_cast<longlong>(v[2]) error v[2] = %s", v[2].c_str());
						out_c = 0;
					}

					int r = 0;
					if (out_c > 0){
						longlong out = 0;
						longlong c = (longlong )(out_c * platform_config_.rate_);
						r = the_service_.cache_helper_.give_currency(uid_, c, out, true);
					}

					//如果cache也成功
					if (r == ERROR_SUCCESS_0){
						record_result(2,  sn_, v[0]);
					}
					//如果cache不成功
					else{
						record_result(21, sn_, v[0]);
					}
				}
				//如果是兑出成功，从cache里扣钱。
				else{
					record_result(2,   sn_, v[0]);
					write_log("%s user trade out success and cache return 0", platform_id_.c_str());
				}
				boost::system::error_code ec;
				time_out_.cancel(ec);
			}
			else{
				on_request_failed(http_business_error, str_body);
			}
		}
		else{
			on_request_failed(http_business_error, str_body);
		}
	}
	//进行重试
	void					retry()
	{
		boost::system::error_code ec;
		time_out_.cancel(ec);
		time_out_.get_io_service().reset();
		time_out_.get_io_service().poll(ec);

		reset();

		repeat_count_++;
		int wait_tm = 500 * repeat_count_;
		if (wait_tm > 5000) wait_tm = 5000;

		//重试间隔越来越长;
		time_out_.expires_from_now(boost::posix_time::milliseconds(500 * repeat_count_));
		time_out_.async_wait(boost::bind(&user_7158_auto_trade<service_t, config_t>::do_retry,
			shared_from_this(), boost::asio::placeholders::error));
	}

	void					do_retry(boost::system::error_code ec)
	{
		if (ec.value() == 0){
			request(str_request_, platform_config_.host_,platform_config_.port_);
			time_out_.expires_from_now(boost::posix_time::milliseconds(5000));
			time_out_.async_wait(boost::bind(&user_7158_auto_trade<service_t, config_t>::on_time_out,
				shared_from_this(), boost::asio::placeholders::error));
		}
	}

	virtual void	on_request_failed(int reason, std::string err_msg) override
	{
		http_request::on_request_failed(reason, err_msg);

		//由于超时处理可能会导致跟远端状态不一致，这边的处理是会一直重试下去，直到有对面回复。
		//http_business_error不需要重试，因为对面端已经回复结果了。
		if (reason == connection_break ||	reason == connection_fail ||	reason == http_response_error_code
			|| reason == http_timeout){
				write_log("%s user trade_inout retry! repeat = %d, tradein = %d, reason = %d", platform_id_.c_str(), repeat_count_, is_trade_in_, reason);
				retry();
		}
		else{
			write_log("trade out failed on reason:%d, repeat_count = %d!", reason, repeat_count_);
			trade_inout_failed(reason, err_msg);
		}
	}
};

template<class service_t, class config_t>
class user_7158_verify : public http_request_with_log<user_7158_verify<service_t, config_t>>
{
public:
	std::string	fromid, uidx, uid, pwd, uip, nickname, head_pic, platform_id_;
	typename service_t::remote_socket_ptr from_sock_;
	config_t& platform_config_;
	std::string reslt;

	service_t& the_service;

	user_7158_verify(service_t& srv, config_t& cnf) : http_request_with_log<user_7158_verify<service_t, config_t>>(srv.http_sv_),
		platform_config_(cnf),
		the_service(srv)
	{
		uip = "";
		repeat_count_ = 0;
	}
	void reset()
	{
		reslt.clear();
		http_request::reset();
	}
	int			do_verify()
	{
		std::string str_sign;
		if (platform_id_ == "7158"){
			str_sign = fromid + 
				platform_config_.game_id_ +
				uidx + uid + pwd + uip + platform_config_.skey_;
		}
		else if(platform_id_ == "t58"){
			str_sign = fromid + platform_config_.game_id_ +
				uidx + uid + pwd + platform_config_.skey_;
		}

		unsigned char out_put[16];
		CMD5 md5;
		md5.MessageDigest((const unsigned char*)str_sign.c_str(), str_sign.length(), out_put);
		std::string sign_key;
		for (int i = 0; i < 16; i++)
		{
			char aa[4] = {0};
			sprintf_s(aa, 4, "%02x", out_put[i]);
			sign_key += aa;
		}
		std::string params;

		add_http_params(params, "FromID", platform_config_.fromid_);
		add_http_params(params, "gameID", platform_config_.game_id_);
		add_http_params(params, "uidx", uidx);
		add_http_params(params, "user", uid);
		add_http_params(params, "pw", pwd);

		if (platform_id_ == "7158"){
			add_http_params(params, "ip", uip);
			add_http_params(params, "nick", "");
		}
		else if(platform_id_ == "t58"){

		}
		add_http_params(params, "sign", sign_key);
		std::string str_req = build_http_request(	platform_config_.host_, 
			platform_config_.verify_param_ + params);
		request(str_req, platform_config_.host_,
			platform_config_.port_);
		return 0;
	}

protected:
	virtual	void	handle_http_body(std::string str_body)
	{
		dur_ = (int)(boost::posix_time::microsec_clock::local_time() - pstart_).total_milliseconds();
		write_log("%s user login http body getted ,uid = %s, cost = %dms, rsp:%s",
			platform_id_.c_str(),
			uid.c_str(),
			dur_, 
			str_body.c_str());
		std::vector<std::string> v;
		split_str(str_body.c_str(), str_body.length(), "&", v, false);
		if (!v.empty()){
			for (int i = 0 ; i < (int)v.size(); i++){
				std::string s = v[i];
				string::size_type np = s.find("result=");
				if (np != std::string::npos){
					reslt = s.data() + np + strlen("result=");
					continue;
				}

				string::size_type nick = s.find("nickName=");
				if(nick != std::string::npos)
				{
					std::string base64_nickName = s.data() + nick + strlen("nickName=");
					nickname = Base64Con::base64_decode(base64_nickName);
					continue;
				}
			}

			//成功
			if (reslt == "0000"){
				std::string uname = the_service.cache_helper_.get_var<std::string>(platform_id_ + uid + KEY_USER_UNAME, -1);
				std::string headpic = the_service.cache_helper_.get_var<std::string>(platform_id_ + uid + KEY_USER_HEADPIC, the_service.the_config_.game_id);
				if (uname == ""){
					the_service.cache_helper_.set_var(platform_id_ + std::string(uid) + KEY_USER_UNAME, -1, nickname);
					uname = nickname;
				}
				if (headpic == ""){
					
					the_service.cache_helper_.set_var(platform_id_ + std::string(uid) + KEY_USER_HEADPIC, the_service.the_config_.game_id, head_pic);
					headpic = head_pic;
				}
				if (!from_sock_->is_closed_){
					user_login(from_sock_, uid, nickname, headpic, uidx, platform_id_);
				}
			}
			else{
				on_request_failed(http_business_error, str_body);
			}
		}
		else{
			on_request_failed(http_business_error, str_body);
		}
	}
	virtual void	on_request_failed(int reason, std::string err_msg) override
	{
		dur_ = (int)(boost::posix_time::microsec_clock::local_time() - pstart_).total_milliseconds();
		http_request::on_request_failed(reason, err_msg);
		//重试一遍
		if (repeat_count_ < 1){
			write_log("%s user login retry! cost = %dms, error = %s",
				platform_id_.c_str(),
				dur_,
				err_msg.c_str());

			reset();
			repeat_count_++;

			request(str_request_,  platform_config_.host_,
				platform_config_.port_);
		}
		else{
			if (from_sock_.get()){
				from_sock_->close();
			}
			write_log("%s user login failed ,uid = %s, cost = %dms, repeat_count :%d, reason = %d, err = %s, close the client connection!",
				platform_id_.c_str(),
				uid.c_str(),
				dur_,
				repeat_count_,
				reason,
				err_msg.c_str());
		}
	}
};

template<class service_t>
class platform_7158
{
public:
	service_t&		the_service;

	platform_7158(service_t& srv):the_service(srv)
	{

	}
	template<class player_t>
	void		auto_trade_in(player_t p)
	{
		user_7158_auto_trade<service_t, platform_config_7158>* tradeout = 
			new user_7158_auto_trade<service_t, platform_config_7158>(the_service, platform_config_7158::get_instance());
		boost::shared_ptr<user_7158_auto_trade<service_t, platform_config_7158>> ptrade(tradeout);
		tradeout->uid_ = p->uid_;
		tradeout->platform_uid_ = p->platform_uid_;
		tradeout->orderkey_ = p->order_key_;
		tradeout->idx_ = p->uidx_;
		tradeout->is_trade_in_ = 1;
		tradeout->platform_id_ = p->platform_id_;
		tradeout->do_trade();
	}

	template<class player_t>
	void		auto_trade_out(player_t p)
	{
		user_7158_auto_trade<service_t, platform_config_7158>* tradeout = 
			new user_7158_auto_trade<service_t, platform_config_7158>(the_service, platform_config_7158::get_instance());
		boost::shared_ptr<user_7158_auto_trade<service_t, platform_config_7158>> ptrade(tradeout);
		tradeout->uid_ = p->uid_;
		tradeout->platform_uid_ = p->platform_uid_;
		tradeout->orderkey_ = p->order_key_;
		tradeout->idx_ = p->uidx_;
		tradeout->is_trade_in_ = 0;
		tradeout->platform_id_ = p->platform_id_;
		tradeout->do_trade();
	}
};