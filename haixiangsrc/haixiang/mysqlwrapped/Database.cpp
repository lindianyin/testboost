/**
 **	Database.cpp
 **
 **	Published / author: 2001-02-15 / grymse@alhem.net
 **/

/*
Copyright (C) 2001  Anders Hedstrom

This program is made available under the terms of the GNU GPL.

If you would like to use this program in a closed-source application,
a separate license agreement is available. For information about 
the closed-source license agreement for this program, please
visit http://www.alhem.net/sqlwrapped/license.html and/or
email license@alhem.net.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#ifdef _WIN32
#pragma warning(disable:4786)
#endif

#include <string>
#include <map>
#include "log.h"

#include <list>

#ifdef WIN32
#include <config-win.h>
#include <mysql.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql/mysql.h>
#include <stdarg.h>
#endif
#include "Database.h"

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif

extern log_file<cout_output> glb_log;
Database::Database(const std::string& h,const std::string& u,const std::string& p,const std::string& d,unsigned long option,unsigned int port)
:host(h)
,user(u)
,password(p)
,database(d)
,m_option(option)
,m_port(port)
{

}

Database::OPENDB* Database::grabdb()
{
	if (!odb.get())									
	{
		odb.reset(new OPENDB);
		if (!odb.get())									{	return NULL; }
		if (!mysql_init(&odb -> mysql))	{	odb.reset();	return NULL; }
		int opt = 5;
		mysql_options(&odb -> mysql, MYSQL_SET_CHARSET_NAME, "utf8");
		mysql_options(&odb -> mysql, MYSQL_OPT_WRITE_TIMEOUT, &opt);
		mysql_options(&odb -> mysql, MYSQL_OPT_READ_TIMEOUT, &opt);

		if (!mysql_real_connect(&odb -> mysql,		host.c_str(),	user.c_str(),	password.c_str(),
														database.c_str(),	m_port,	NULL,	m_option))	{
			const char * err_str = mysql_error(&odb->mysql);
			glb_log.write_log("connect to db failed, host = %s, database = %s, port = %d, err =\r\n %s",
				host.c_str(), database.c_str(), m_port, err_str);
			odb.reset();
			return NULL;
		}
		on_connection_complete(odb.get());
	}
	else{
		if (mysql_ping(&odb -> mysql) != 0){
			mysql_close(&odb->mysql);
			odb.reset();
			grabdb();
		}
	}
	return odb.get();
}

void Database::keep_alive()
{
	if (odb.get()){
		int ping_result = mysql_ping(&odb -> mysql);
		if (ping_result)
		{
			glb_log.write_log("ping db err, ret = %d, %s", ping_result, mysql_error(&odb -> mysql));
			mysql_close(&odb->mysql);
			odb.reset();
		}
	}
}

std::string Database::safestr(const std::string& str)
{
	std::string str2;
	for (size_t i = 0; i < str.size(); i++)
	{
		switch (str[i])
		{
		case '\'':
		case '\\':
		case 34:
			str2 += '\\';
		default:
			str2 += str[i];
		}
	}
	return str2;
}


std::string Database::unsafestr(const std::string& str)
{
	std::string str2;
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] == '\\')
		{
			i++;
		}
		if (i < str.size())
		{
			str2 += str[i];
		}
	}
	return str2;
}


std::string Database::xmlsafestr(const std::string& str)
{
	std::string str2;
	for (size_t i = 0; i < str.size(); i++)
	{
		switch (str[i])
		{
		case '&':
			str2 += "&amp;";
			break;
		case '<':
			str2 += "&lt;";
			break;
		case '>':
			str2 += "&gt;";
			break;
		case '"':
			str2 += "&quot;";
			break;
		case '\'':
			str2 += "&apos;";
			break;
		default:
			str2 += str[i];
		}
	}
	return str2;
}


int64_t Database::a2bigint(const std::string& str)
{
	int64_t val = 0;
	bool sign = false;
	size_t i = 0;
	if (str[i] == '-')
	{
		sign = true;
		i++;
	}
	for (; i < str.size(); i++)
	{
		val = val * 10 + (str[i] - 48);
	}
	return sign ? -val : val;
}


uint64_t Database::a2ubigint(const std::string& str)
{
	uint64_t val = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		val = val * 10 + (str[i] - 48);
	}
	return val;
}

#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

