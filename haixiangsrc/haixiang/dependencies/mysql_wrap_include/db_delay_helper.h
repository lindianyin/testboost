#pragma once
#include "Database.h"
#include "boost/shared_ptr.hpp"
#include "safe_list.h"
#include <string>

class Database;
typedef boost::shared_ptr<Database>	db_ptr;
using namespace std;

template<class key_t, class opcode_t>
struct delay_info
{
	key_t		id_;
	opcode_t	op_code_;
	std::string		sql_content_;
};


template<class key_t, class opcode_t>
struct db_delay_helper
{
	typedef boost::lock_guard<boost::mutex> write_lock;
	typedef delay_info<key_t, opcode_t>		delay_t;
	typedef safe_list<delay_t>				delay_list_t;
	typedef	void (*execute_callback)(key_t key, bool exe_result);
	bool			pop_and_execute(execute_callback cb = nullptr);

	void			push_sql_exe(key_t k, opcode_t o, string sql, bool replace_previous, bool must_be_success = false);

	bool			all_sql_done(key_t k);
	void			set_db_ptr(db_ptr db);
	db_ptr		get_db_ptr() {return db_;}
private:
	delay_list_t sql_exe_lst_;
	db_ptr db_;
};

template<class key_t, class opcode_t>
bool db_delay_helper<key_t, opcode_t>::pop_and_execute(
	typename db_delay_helper<key_t, opcode_t>::execute_callback cb)
{
	if (!db_.get()) return false;
	typename delay_list_t::this_type copy_l;
	{
		write_lock l(sql_exe_lst_.val_lst_mutex_);
		copy_l = sql_exe_lst_.lst_;
		sql_exe_lst_.lst_.clear();
	}
	if (copy_l.size() > 100){
		glb_log.write_log("db is busy, the pending sql execution is %d.", copy_l.size());
	}
	auto it_copy = copy_l.begin();
	while (it_copy != copy_l.end()){
		delay_t dt = *it_copy; it_copy++;
		Database::OPENDB* db_cnn = db_->grabdb();
		if(db_cnn){
			if (!dt.sql_content_.empty() && mysql_query(&db_cnn->mysql, dt.sql_content_.c_str()) != 0)	{
				glb_log.write_log("sql execute err : %s \r\n, SQL = %s",
					mysql_error(&db_cnn->mysql),
					dt.sql_content_.c_str());

				if (cb){
					cb(dt.id_, false);
				}
			}
			else if(!dt.sql_content_.empty()){
				if (cb){
					cb(dt.id_, true);
				}
			}
		}
	}
	return !copy_l.empty();
}

template<class key_t, class opcode_t>
void db_delay_helper<key_t, opcode_t>::push_sql_exe( key_t k, opcode_t o, string sql, bool replace_previous, bool must_be_success)
{
	if (replace_previous){
		write_lock l(sql_exe_lst_.val_lst_mutex_);
		typename delay_list_t::iterator it = sql_exe_lst_.lst_.begin();
		while (it != sql_exe_lst_.lst_.end()){
			delay_t& tmp_d = *it;
			if(tmp_d.id_ == k && tmp_d.op_code_ == o)
				it = sql_exe_lst_.lst_.erase(it);
			else
				it++;
		}
	}

	delay_t e;
	e.id_ = k;
	e.op_code_ = o;
	e.sql_content_ = sql;
	sql_exe_lst_.push_back(e);
}


template<class key_t, class opcode_t>
bool db_delay_helper<key_t, opcode_t>::all_sql_done( key_t k )
{
	write_lock l(sql_exe_lst_.val_lst_mutex_);
	typename delay_list_t::iterator it = sql_exe_lst_.begin();
	while (it != sql_exe_lst_.end()){
		delay_t& tmp_d = *it;
		if(tmp_d.id_ == k)
			return false;
		else
			it++;
	}
	return true;
}

template<class key_t, class opcode_t>
void db_delay_helper<key_t, opcode_t>::set_db_ptr( db_ptr db )
{
	db_ = db;
}
