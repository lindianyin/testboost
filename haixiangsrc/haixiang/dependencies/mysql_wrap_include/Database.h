/**
 **	Database.h
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

#ifndef _DATABASE_H
#define _DATABASE_H

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <string>
#include <list>
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#include <mysql.h>
#else
#include <stdint.h>
#include "mysql/mysql.h"
#endif
#include "boost/thread/tss.hpp"

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif

#define BEGIN_UPDATE_TABLE(table)\
	std::string str_field;\
	str_field = "update `" + lex_cast_to_str(table) + "` set "	

#define BEGIN_INSERT_TABLE(table)\
	std::string str_field;\
	str_field = "insert into `" + lex_cast_to_str(table) + "` set "

#define BEGIN_REPLACE_TABLE(table)\
	std::string str_field;\
	str_field = "replace into `" + lex_cast_to_str(table) + "` set "

#define BEGIN_DELETE_TABLE(table)\
	std::string str_field;\
	bool bimmediate = false;\
	str_field = "delete from `" + lex_cast_to_str(table) + "`"

#define SET_FIELD_VALUE(f, v)\
	str_field += "`" + lex_cast_to_str(f) + "` = '" + lex_cast_to_str(v) + "',"

#define ADD_FIELD_VALUE(f, v)\
	str_field += "`"+lex_cast_to_str(f) + "` = " + "`"+lex_cast_to_str(f) + "` + " + lex_cast_to_str(v) + ","

#define SET_FINAL_VALUE(f, v)\
	str_field += "`"+lex_cast_to_str(f) + "` = '" + lex_cast_to_str(v) + "'"

#define ADD_FINAL_VALUE(f, v)\
	str_field += "`"+lex_cast_to_str(f) + "` = " + "`"+lex_cast_to_str(f) + "` + " + lex_cast_to_str(v)

#define WITH_END_CONDITION(cndt)\
	str_field += " " + lex_cast_to_str(cndt);\

#define EXECUTE_REPLACE_DELAYED(ki, op, db_helper)\
	bool sql_exe_success = true;\
	db_helper.push_sql_exe(ki, op, str_field, true);

#define EXECUTE_NOREPLACE_DELAYED(ki, db_helper)\
	bool sql_exe_success = true;\
	db_helper.push_sql_exe(ki, -1, str_field, false);

#define EXECUTE_IMMEDIATE() Query q(db); bool sql_exe_success = q.execute(str_field)

class Query;

class Database
{
public:
	struct OPENDB 
	{
		MYSQL mysql;
	};
	boost::thread_specific_ptr<OPENDB>	odb;
	//boost::shared_ptr<OPENDB>	odb;
	/** Connect to a MySQL server */
	Database(const std::string& host,
		const std::string& user,
		const std::string& password = "",
		const std::string& database = "",
		unsigned long option = 0,
		unsigned int port = 3306);

	void					keep_alive();
	OPENDB*				grabdb();
	// utility
	std::string		safestr(const std::string& );
	std::string		unsafestr(const std::string& );
	std::string		xmlsafestr(const std::string& );

	int64_t				a2bigint(const std::string& );
	uint64_t			a2ubigint(const std::string& );
	virtual void	on_connection_complete(Database::OPENDB* odb) {};

private:
	Database(const Database& ){}
	Database& operator=(const Database& ) { return *this; }
	//
	std::string		host;
	std::string		user;
	std::string		password;
	std::string		database;

	unsigned long	m_option;
	unsigned int	m_port;
};

#ifdef MYSQLW_NAMESPACE
} //namespace MYSQLW_NAMESPACE {
#endif


#endif // _DATABASE_H
