#pragma once
#include "Database.h"

class utf8_data_base : public Database
{
public:
	
	utf8_data_base(const std::string& host,
		const std::string& user,
		const std::string& password = "",
		const std::string& database = "",
		unsigned long option = 0,
		unsigned int port = 3306
		) : Database(host, user, password, database, option, port)
	{}
	virtual void on_connection_complete(Database::OPENDB* odb)
	{
		int r = mysql_set_server_option(&odb->mysql, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	}
};
