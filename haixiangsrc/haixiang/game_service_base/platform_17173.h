#pragma once
#include <string>
#include "http_request.h"
#include "platform_gameid.h"
#include "game_ids.h"
using namespace std;
class platform_config_173
{
public:
	std::string		game_id_;//接入平台约定的游戏id
	std::string		host_;
	std::string		skey_;
	std::string		port_;
	std::string		trade_param_;
	std::string     appid_;   //173指定要用
	std::string     broadcast_appid_;
	std::string		broadcast_param_;
	std::string     broadcast_key_;

	platform_config_173()
	{
		game_id_ = GAME_ID_173;
		port_ = "80";
		host_ = "v.17173.com";
		skey_ = "C2546726272942A294CEDE3E19258C6C";
		trade_param_ = "/show/open/t_one.action?";
		broadcast_param_ = "/show/open/t_award.action?";
		appid_       ="600014";
		broadcast_appid_="600014";
		broadcast_key_  ="C2546726272942A294CEDE3E19258C6C";
	}

	static platform_config_173& get_instance()
	{
		static platform_config_173	conf_173_;
		return conf_173_;
	}
};

template<class service_t, class config_t>
class platform_broadcast_17173 :  public  http_request< platform_broadcast_17173 < service_t , config_t > >
{
public:
	platform_broadcast_17173(service_t& srv , config_t & cnf ): http_request< platform_broadcast_17173<service_t, config_t > > (srv.http_sv_),
		time_out_(srv.http_sv_), platform_config_(cnf)
	{

	};

	~platform_broadcast_17173() {

	};

	boost::asio::deadline_timer time_out_;
	string          uid_;     //用户ID
	string          rel_uid_; //真实ID

	string         award_;   //广播消息内容
	string         award_utf8;// utf8的编码

	config_t&		platform_config_;


	int send_request()
	{
		std::string param,		str_sign;
		//appid
		add_http_params(param, "appId", platform_config_.broadcast_appid_);
		str_sign = "appId="+platform_config_.broadcast_appid_;
		//award
		add_http_params(param, "award", award_utf8);
		str_sign += "award="+HttpUtility::URLEncode(award_utf8);

		////gameid
		add_http_params(param, "gid", platform_config_.game_id_);
		str_sign += "gid="+platform_config_.game_id_;
		//time
		time_t tmp;
		time(&tmp);
		add_http_params(param, "time", tmp);
		str_sign += "time="+boost::lexical_cast<std::string>(tmp);
		//uid
		add_http_params(param, "uid",rel_uid_ );
		str_sign += "uid="+rel_uid_;

		//
		str_sign += platform_config_.broadcast_key_;

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
		add_http_params(param, "sig", sign_key);

		std::string str_req = build_http_request(platform_config_.host_, 
			platform_config_.broadcast_param_ + param);

		request(str_req, 
			platform_config_.host_,
			platform_config_.port_);

		return ERROR_SUCCESS_0;
	}

protected:

	void handle_http_body(std::string str_body)
	{
		/*{"code":"000000","msg":"ok","obj":""}
		code:100003 消费失败
		code:100004 余额不足
		code:900002 请求已被处理
		code:900002 请求已被处理
		code:000000 成功
		*/
	}

	void on_request_failed(int reason, std::string err_msg)
	{

	}
};