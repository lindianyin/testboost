#pragma once
#include <string>
#include <vector>
#include <map>
#include "simple_xml_parse.h"

using namespace nsp_simple_xml;

#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)

#define WRITE_XML_VALUE(p, n)\
	doc.add_node(p, #n, n)

struct online_config
{
	int time_;
	int prize_;
};

class service_config_base
{
public:
	int									game_id;						//游戏ID			
	std::string					game_name;					//游戏名称
	std::string					accdb_host_;
	std::string					accdb_user_;
	std::string					accdb_pwd_;
	short								accdb_port_;
	std::string					accdb_name_;

	std::string					db_host_;
	std::string					db_user_;
	std::string					db_pwd_;
	short								db_port_;
	std::string					db_name_;

	std::string					php_sign_key_;
	unsigned short			client_port_;
	std::string					cache_ip_;
	unsigned short			cache_port_;

	std::string					world_ip_;
	unsigned short			world_port_;
	int									world_id_;
	std::string					world_key_;

	int									wait_reconnect;

	int									shut_down_;			//是否处于关闭状态

	std::vector<online_config>	online_prize_;

	std::map<std::string, std::string> broadcast_telfee;

	__int64		max_trade_cap_;
	int				trade_tax_;
	__int64		alert_email_set_;



	int				register_account_reward_;
	int				register_fee_;

	__int64	stock_lowcap_,
		stock_lowcap_max_,
		stock_upcap_start_,
		stock_upcap_max_,
		stock_decay_;
	std::map<std::string, float> personal_rate_control_;
	service_config_base()
	{
		restore_default();
		register_account_reward_ = 0;
		alert_email_set_ = 999999999;
		register_fee_ = 0;
		world_key_ = "{327BE8D2-4A23-428F-B006-5A755288D5AB}";
	}

	virtual ~service_config_base()
	{

	}

	int save_default()
	{
		return 0;
	}
	virtual int	load_from_file(std::string path)
	{
		xml_doc doc;
		if (!doc.parse_from_file(path.c_str())) {
			restore_default();
			save_default();
		}
		else {
			READ_XML_VALUE("config/", db_port_);
			READ_XML_VALUE("config/", db_host_);
			READ_XML_VALUE("config/", db_user_);
			READ_XML_VALUE("config/", db_pwd_);
			READ_XML_VALUE("config/", db_name_);
			READ_XML_VALUE("config/", client_port_);
			READ_XML_VALUE("config/", php_sign_key_);
			READ_XML_VALUE("config/", accdb_port_);
			READ_XML_VALUE("config/", accdb_host_);
			READ_XML_VALUE("config/", accdb_user_);
			READ_XML_VALUE("config/", accdb_pwd_);
			READ_XML_VALUE("config/", accdb_name_);
			READ_XML_VALUE("config/", cache_ip_);
			READ_XML_VALUE("config/", cache_port_);
			READ_XML_VALUE("config/", world_ip_);
			READ_XML_VALUE("config/", world_port_);
			READ_XML_VALUE("config/", world_id_);
			READ_XML_VALUE("config/", world_key_);
			READ_XML_VALUE("config/", alert_email_set_);
			READ_XML_VALUE("config/", register_fee_);
		}
		return ERROR_SUCCESS_0;
	}
	void restore_default()
	{
		db_host_ = "192.168.17.224";
		db_user_ = "root";
		db_pwd_ = "123456";
		db_port_ = 3306;
		db_name_ = "Competition";

		accdb_host_ = "192.168.17.224";
		accdb_user_ = "root";
		accdb_pwd_ = "123456";
		accdb_port_ = 3306;
		accdb_name_ = "account_server";

		cache_ip_ = "192.168.17.238";
		cache_port_ = 9999;

		client_port_ = 15000;
		php_sign_key_ = "z5u%wi31^_#h=284u%keg+ovc+j6!69wpwqe#u9*st5os5$=j2";
		alert_email_set_ = -10000000;

		wait_reconnect = 30000;
	}
	virtual void	refresh() = 0;
};
