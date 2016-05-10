#pragma once
#include "simple_xml_parse.h"

#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)

#define WRITE_XML_VALUE(p, n)\
	doc.add_node(p, #n, n)

using namespace nsp_simple_xml;

enum 
{
	server_role_account = 1,	//账号服务器
	server_role_channel = 2,	//频道服务器
	server_role_broadcast = 4,//喇叭服务器
	server_role_match = 8,		//比赛服务器
};

class service_config
{
public:
	std::string					accdb_host_;
	std::string					accdb_user_;
	std::string					accdb_pwd_;
	short								accdb_port_;
	std::string					accdb_name_;

	unsigned short			client_port_;
	std::string					signkey_;
	std::string					token_generate_key_;
	std::string					shortmsg_host_;	//短信验证码服务器地址
	std::string					shortmsg_port_;
	service_config()
	{
		restore_default();
	}
	virtual ~service_config()
	{

	}
	virtual int				save_default()
	{
		return 0;
	}

	void			restore_default()
	{
		accdb_host_ = "192.168.17.230";
		accdb_user_ = "root";
		accdb_pwd_ = "123456";
		accdb_port_ = 3306;
		accdb_name_ = "account_server";
		client_port_ = 9999;
		signkey_ = "{51B539D8-0D9A-4E35-940E-22C6EBFA86A8}";
		token_generate_key_ = "{327BE8D2-4A23-428F-B006-5A755288D5AB}";
	}

	int				load_from_file()
	{
		xml_doc doc;
		if (!doc.parse_from_file("memcache.xml")){
			restore_default();
			save_default();
		}
		else{
			READ_XML_VALUE("config/", client_port_);
			READ_XML_VALUE("config/", accdb_port_	);
			READ_XML_VALUE("config/", accdb_host_	);
			READ_XML_VALUE("config/", accdb_user_	);
			READ_XML_VALUE("config/", accdb_pwd_	);
			READ_XML_VALUE("config/", accdb_name_	);
			READ_XML_VALUE("config/", signkey_	);
			READ_XML_VALUE("config/", token_generate_key_);
			READ_XML_VALUE("config/", shortmsg_host_);
			READ_XML_VALUE("config/", shortmsg_port_);
		}
		return 0;
	}
};
