#pragma once
#include <string>
#include "platform_7158.h"

class platform_config_t5b
{
public:
	std::string		fromid_;
	std::string		game_id_;//接入平台约定的游戏id
	std::string		host_;
	std::string		skey_;
	std::string		verify_param_;
	std::string		trade_param_;
	std::string		port_;
	int						rate_;	//金币:跳舞吧

	platform_config_t5b()
	{
		fromid_ = "2";
		game_id_ = "9000";
		port_ = "80";
		host_ = "games.tiao58.com";
		skey_ = "B7AD0EAA3BBC401795CE6B6F039E4484";
		verify_param_ = "/FishGame/gameLogin.aspx?";
		trade_param_ = "/FishGame/gamechangecoin.aspx?";
		rate_ = 10;
	}

	static platform_config_t5b& get_instance()
	{
		static platform_config_t5b	conf_t58_;
		return conf_t58_;
	}
};

template<class service_t>
class platform_t58
{
public:
	service_t&		the_service;

	platform_t58(service_t& srv):the_service(srv)
	{

	}
	template<class player_t>
	void		auto_trade_in(player_t p)
	{
		user_7158_auto_trade<service_t, platform_config_t5b>* tradeout = 
			new user_7158_auto_trade<service_t, platform_config_t5b>(the_service, platform_config_t5b::get_instance());
		boost::shared_ptr<user_7158_auto_trade<service_t, platform_config_t5b>> ptrade(tradeout);
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
		user_7158_auto_trade<service_t, platform_config_t5b>* tradeout = 
			new user_7158_auto_trade<service_t, platform_config_t5b>(the_service, platform_config_t5b::get_instance());
		boost::shared_ptr<user_7158_auto_trade<service_t, platform_config_t5b>> ptrade(tradeout);
		tradeout->uid_ = p->uid_;
		tradeout->platform_uid_ = p->platform_uid_;
		tradeout->orderkey_ = p->order_key_;
		tradeout->idx_ = p->uidx_;
		tradeout->is_trade_in_ = 0;
		tradeout->platform_id_ = p->platform_id_;
		tradeout->do_trade();
	}
};