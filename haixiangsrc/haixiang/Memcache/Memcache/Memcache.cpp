/************************************************************************/
/* 于2015-3-19号重新设计
/* 由于在数据量大时，内存使用太高，需要做成缓存形式，为缓存做过期处理
/* 纯KEY-VALUE的形式会带来一些不方便，特别是数据间是有某些关系的时候，比如根据id查用户名，等等
/* 借鉴关系形数据库的设计方法，重新设计
/* 表结构可以自定义
/************************************************************************/
#include "hash_map.hpp"
#include <unordered_map>
#include "log.h"
#include "Database.h"
#include "Query.h"
#include "net_service.h"
#include "utility.h"
#include "utf8_db.h"
#include "simple_xml_parse.h"
#include "db_delay_helper.h"
#include "boost/thread.hpp"
#include "../game_service_base/game_ids.h"
#include "msg.h"
#include <map>
#include "schedule_task.h"
#include "boost/atomic.hpp"
#include "msg_cache.h"

#define  FLOAT_PRECISION 0.00001
using namespace nsp_simple_xml;


#define READ_XML_VALUE(p, n)\
	n = doc.get_value(p#n, n)

#define WRITE_XML_VALUE(p, n)\
	doc.add_node(p, #n, n)

struct none_logic{};

std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;
typedef __int64 longlong;
vector<std::string> g_bot_names;
log_file<cout_output> glb_log;
boost::atomic_uint	msg_recved(0);
boost::atomic_uint	msg_handled(0);
boost::atomic_uint	unsended_data(0);
boost::asio::io_service timer_srv;

struct gold_rank
{
	std::string		uid_;
	std::string		uname_;
	longlong	value_;
};

struct unique_check
{
	std::string check_for_;
	std::map<std::string, std::string> unique_values_;
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
	unsigned short			sync_local_port_;	//同步本地侦听端口

	std::string					sync_remote_ip_;		//同步ip
	unsigned short			sync_remote_port_;	//同步本地侦听端口
	int									is_slave_;	//是否为备用cache
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
		sync_local_port_ = 8000;
		is_slave_ = 0;
		sync_remote_ip_ = "192.168.17.238";
		sync_remote_port_ = 8000;
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
			READ_XML_VALUE("config/", sync_local_port_	);
			READ_XML_VALUE("config/", is_slave_	);
			READ_XML_VALUE("config/", sync_remote_ip_	);
			READ_XML_VALUE("config/", sync_remote_port_	);
		}
		return 0;
	}
};


class mem_cache_socket : public remote_socket<none_logic>
{
public:
	mem_cache_socket(boost::asio::io_service& ios) : remote_socket<none_logic>(ios)
	{
	}
};

typedef boost::shared_ptr<mem_cache_socket> remote_socket_ptr;
net_server<mem_cache_socket> the_net(4);
service_config the_config;

struct data_per_row;
typedef boost::shared_ptr<data_per_row> cache_data_ptr;

std::unordered_map<std::string, cache_data_ptr>	the_cache_, will_persistent_;
boost::mutex			the_cache_mut_, will_persistent_mut_;

std::unordered_map<std::string, std::string> acc_maps_uid;
std::unordered_map<longlong, std::string> iid_maps_uid;

boost::mutex			common_mut_;



bool glb_exit = false;

vector<gold_rank> glb_goldrank_;
boost::mutex			glb_goldrank_mut_;

bool glb_is_syncing = false;

std::list<msg_base<remote_socket_ptr>::msg_ptr>	glb_msg_list_;
std::list<cache_sync_operation<remote_socket_ptr>>	glb_pending_sync_;
std::unordered_map<std::string, remote_socket_ptr> glb_user_game_map_;
boost::mutex			glb_user_game_map_mut_;

struct handled_cache
{
	std::string		sequence_;
	boost::posix_time::ptime pt_;
	int			result_;
};
template<> std::size_t calculate_hash_value<std::string>(std::string key);
std::unordered_map<std::string, handled_cache> glb_handled_sequence_;
void sql_trim(std::string& str)
{
	for (unsigned int i = 0; i < str.size(); i++)
	{
		if (str[i] == '\''){
			str[i] = ' ';
		}
	}
}

std::string trim_to_uid(std::string key)
{
	auto n = key.find_last_of('_'); //从6开始是为了跳过ipbot_test中的下划线
	if (n == std::string::npos){
		return "";
	}
	else{
		key.erase(key.begin() + n, key.end());
		return key;
	}
}

std::string trim_to_subkey(std::string key)
{
	auto n = key.find_last_of('_');
	if (n == std::string::npos){
		return "";
	}
	else{
		key.erase(key.begin(), key.begin() + n);
		return key;
	}
}

bool is_account_info_key(std::string& subkey)
{
	if (subkey == KEY_CUR_GOLD ||
		subkey == KEY_USER_ACCNAME ||
		subkey == KEY_USER_ACCPWD || 
		subkey == KEY_USER_UNAME || 
		subkey == KEY_USER_IID)
	{
		return true;
	}
	return false;
}

struct data_per_row : boost::enable_shared_from_this<data_per_row>
{
	std::unordered_map<std::string, variant>	column_and_value_;
	data_per_row()
	{
	}

	double		set_double(double val)
	{
		double ret = 0;
		bool need_persistent = false;
		{
			boost::lock_guard<boost::mutex> lk(mut_);
			if (value_type_ == 2){
			}
			value_type_ = 0;
			if (abs(value_ - val) > FLOAT_PRECISION){
				value_ = val;
				need_persistent = true;
			}

			ret = value_;
		}
		if(need_persistent){
			boost::lock_guard<boost::mutex> lk(will_persistent_mut_);
			will_persistent_.insert(std::make_pair(key_, shared_from_this()));
		}
		return ret;
	}

	std::string		set_str(const char* str)
	{
		std::string	ret;
		bool need_persistent = false;
		{
			boost::lock_guard<boost::mutex> lk(mut_);
			value_type_ = 1;
			if (_stricmp(str_value_, str) != 0){
				COPY_STR(str_value_, str);
				need_persistent = true;
			}
			ret = str_value_;
		}

		if (need_persistent){
			boost::lock_guard<boost::mutex> lk(will_persistent_mut_);
			will_persistent_.insert(std::make_pair(key_, shared_from_this()));
		}
		return ret;
	}

	double	add_double(double val)
	{
		double ret = 0;
		{
			boost::lock_guard<boost::mutex> lk(mut_);
			value_type_ = 0;
			value_ += val;
			ret = value_;
		}
		if (abs(val) > FLOAT_PRECISION){
			boost::lock_guard<boost::mutex> lk(will_persistent_mut_);
			will_persistent_.insert(std::make_pair(key_, shared_from_this()));
		}
		return ret;
	}

	double	take(double val, int& ret)
	{
		double dret = 0;
		ret = 0;
		{
		boost::lock_guard<boost::mutex> lk(mut_);
			if (value_type_ != 0){
				ret = -1;
				return 0;
			}

			if (value_ < val){
				ret = -1;
				return 0;
			}
			value_ -= val;
			dret = value_;
		}

		if(abs(val) > FLOAT_PRECISION){
			boost::lock_guard<boost::mutex> lk(will_persistent_mut_);
			will_persistent_.insert(std::make_pair(key_, shared_from_this()));
		}

		return dret;
	}

	double	take_all(int& ret)
	{
		double dret = 0;
		ret = 0;
		{
			boost::lock_guard<boost::mutex> lk(mut_);
			if (value_type_ != 0){
				ret = -1;
				return 0;
			}
			dret = value_;
			value_ = 0;
		}

		if(abs(dret) > FLOAT_PRECISION)	{
			boost::lock_guard<boost::mutex> lk(will_persistent_mut_);
			will_persistent_.insert(std::make_pair(key_, shared_from_this()));
		}

		return dret;
	}

	void	get_data(char& tp, char* ret, unsigned int len)
	{
		boost::lock_guard<boost::mutex> lk(mut_);
		tp = value_type_;
		if (value_type_ == 0){
			*(double*)ret = value_;
		}
		else{
			strncpy_s(ret, len, str_value_, _TRUNCATE);
		}
	}
	double get_value()
	{
		boost::lock_guard<boost::mutex> lk(mut_);
		return value_;
	}

	std::string get_str_value()
	{
		boost::lock_guard<boost::mutex> lk(mut_);
		return str_value_;
	}

	//保存到数据库
	void		persistent(db_delay_helper<std::string, int>& dbhelper)
	{
		std::string subkey = trim_to_subkey(key_); std::string uid = trim_to_uid(key_);
		boost::lock_guard<boost::mutex> lk(mut_);
		if (value_type_ == 1){
			std::string ss = str_value_;
			sql_trim(ss);
			if (is_account_info_key(subkey)){
				std::string sql = "call save_account_info('" + uid + "', '" + subkey + "', 0, '" + ss +"');";
				dbhelper.push_sql_exe(key_, 1, sql, false);
			}
			else{
				std::string sql = "call save_account_info('" + key_ + "','', 0, '" + ss + "');";
				dbhelper.push_sql_exe(key_, 1, sql, false);
			}
		}
		else if (value_type_ == 0)	{
			if (is_account_info_key(subkey)){
				std::string sql = "call save_account_info('" + uid + "','" + subkey + "', " + lex_cast_to_str(value_) + ", '');";
				dbhelper.push_sql_exe(key_, 2, sql, false);
			}
			else{
				std::string sql = "call save_account_info('" + key_ + "','', " + lex_cast_to_str(value_) + ", '');";
				dbhelper.push_sql_exe(key_, 2, sql, false);
			}
		}
	}
private:
	boost::mutex mut_;		//键级锁
	char	value_type_;		//0数字，1字符，2用户信息
	union
	{
		double  value_;
		char	 str_value_[max_guid];
	};
};

cache_data_ptr get_cache_unit(std::string key)
{
	auto it = the_cache_.find(key);
	if (it != the_cache_.end()){
		return it->second;
	}
	return cache_data_ptr();
}

void				add_new_cache_unit(std::string key, cache_data_ptr pdata)
{
	the_cache_.insert(std::make_pair(key, pdata));
}

int bigger_gold_rank(gold_rank& r1, gold_rank& r2)
{
	return r1.value_ > r2.value_;
}

void	pickup_modify()
{
	std::list<std::pair<std::string, cache_data_ptr>> copy_l;
	{
		boost::lock_guard<boost::mutex> lk(will_persistent_mut_);
		if (!the_config.is_slave_){
			copy_l.insert(copy_l.begin(), will_persistent_.begin(), will_persistent_.end());
		}
		will_persistent_.clear();
	}

	auto itc = copy_l.begin();
	while (itc != copy_l.end())
	{
		cache_data_ptr pcache = itc->second; itc++;
		pcache->persistent(db_delay_helper_[calculate_hash_value<std::string>(pcache->key_) % db_hash_count]);
	}
}

int cache_persistent_thread_fun(db_delay_helper<std::string, int>* db_delay)
{
	unsigned int idle = 0;
	while(!glb_exit)
	{
		if(!db_delay->pop_and_execute())
			idle++;
		else{
			idle = 0;
		}
		boost::posix_time::milliseconds ms(50);
		boost::this_thread::sleep(ms);
	}
	return 0;
};

template<class remote_t>
void		bind_iid(std::string key, longlong iid, msg_cache_cmd_ret<remote_t>& msg)
{
	cache_data_ptr idd_gen = get_cache_unit(KEY_GENUID);
	if (!idd_gen.get())	{
		idd_gen.reset(new data_per_row);
		idd_gen->key_ = KEY_GENUID;
		idd_gen->set_double(2000000);
		add_new_cache_unit(idd_gen->key_, idd_gen);
	}
	msg.val_type_ = 0;

	cache_data_ptr pdat = get_cache_unit(key);
	if (pdat.get() && (int)pdat->get_value() > 0){
		msg.value_ = pdat->get_value();
		msg.return_code_ = CACHE_RET_UNIQUE_CHECK_FAIL;
	}
	else {
		 if (!pdat.get()){
			 pdat.reset(new data_per_row);
			 pdat->key_ = key;
		 }
		 if (iid == 0){
			 longlong iid_test = (longlong)ceil(idd_gen->add_double(1.0));
			 boost::lock_guard<boost::mutex> lk(common_mut_);
			 auto itiid = iid_maps_uid.find(iid_test);
			//如果生成的iid已存在，则继续生成
			 while (itiid != iid_maps_uid.end() && itiid->first > 0){
				 iid_test = (longlong)ceil(idd_gen->add_double(1.0));
				 itiid = iid_maps_uid.find(iid_test);
			 }
			 msg.value_ = pdat->set_double((double)iid_test);
		 }
		 else{
			 if (iid > 0){
				 boost::lock_guard<boost::mutex> lk(common_mut_);
				 if (iid_maps_uid.find(iid) != iid_maps_uid.end()){
					 msg.return_code_ = CACHE_RET_UNIQUE_CHECK_FAIL;
				 }
			 }
			 msg.value_ = pdat->set_double((double)iid);
		 }

		 add_new_cache_unit(pdat->key_, pdat);
		 {
			 boost::lock_guard<boost::mutex> lk(common_mut_);
			 iid_maps_uid.insert(std::make_pair((longlong)msg.value_, trim_to_uid(key)));
		 }
		 msg.return_code_ = CACHE_RET_SUCCESS;
	}
}

template<class remote_t>
void	user_login(std::string uid, remote_t remote)
{

}

template<> 
void	user_login<remote_socket_ptr>(std::string uid, remote_socket_ptr remote)
{
	lock_guard<boost::mutex> lk(glb_user_game_map_mut_);
	//匹配成功
	auto itcnn = glb_user_game_map_.find(uid);
	if (itcnn != glb_user_game_map_.end()){
		if (remote != itcnn->second){
			msg_cache_kick_account<remote_socket_ptr> msg;
			COPY_STR(msg.uid_, uid.c_str());
			remote_socket_ptr old_sock = itcnn->second;
			//send_msg(old_sock, msg);
		}
	}
	glb_user_game_map_.insert(std::make_pair(uid, remote));
}

template<class remote_t>
void	user_logout(std::string uid, remote_t remote)
{

}

template<>
void	user_logout<remote_socket_ptr>(std::string uid, remote_socket_ptr remote)
{
	lock_guard<boost::mutex> lk(glb_user_game_map_mut_);
	auto itw = glb_user_game_map_.find(uid);
	if (itw != glb_user_game_map_.end()){
		if (itw->second == remote){
			glb_user_game_map_.erase(uid);
		}
	}
}

template<class remote_t>
int				handle_msg_cache_cmd(msg_cache_cmd<remote_t>* cmd_msg, remote_t psock)
{
	msg_cache_cmd_ret<remote_t> msg_ret;
	COPY_STR(msg_ret.key_, cmd_msg->key_);
	COPY_STR(msg_ret.sequence_, cmd_msg->sequence_);
	msg_ret.val_type_ = cmd_msg->val_type_;
	msg_ret.return_code_ = CACHE_RET_SUCCESS;
	handled_cache hc;
	hc.pt_ = boost::posix_time::second_clock::local_time();
	hc.sequence_ = cmd_msg->sequence_; 

	std::string cmd = cmd_msg->cache_cmd;
	int is_add = 0;
	//增加
	if (cmd == CMD_ADD){
		if (cmd_msg->val_type_ != 0)	{
			msg_ret.return_code_ = CACHE_RET_WRONG_DAT_TYPE;
		}
		else{
			cache_data_ptr pdat = get_cache_unit(cmd_msg->key_);
			if (pdat.get()){
				msg_ret.value_ = pdat->add_double(cmd_msg->value_);
			}
			else{
				cache_data_ptr dat(new data_per_row);
				dat->key_ = cmd_msg->key_;
				if (_stricmp(cmd_msg->key_, KEY_GENUID) == 0){
					dat->set_double(10000);
				}
				else
					dat->set_double(cmd_msg->value_);

				add_new_cache_unit(dat->key_, dat);
				msg_ret.value_ = cmd_msg->value_;
			}

			is_add = 2;

			if (strstr(cmd_msg->key_, KEY_CUR_GOLD)){
				boost::lock_guard<boost::mutex> lk(glb_goldrank_mut_);
				string uid;
				uid.append(cmd_msg->key_, cmd_msg->key_ + strlen(cmd_msg->key_) - strlen(KEY_CUR_GOLD));
				bool finded = false;
				for (unsigned int i = 0 ; i < glb_goldrank_.size(); i++)
				{
					if (glb_goldrank_[i].uid_ == uid){
						glb_goldrank_[i].value_ = (longlong)msg_ret.value_;
						finded = true;
						break;
					}
				}
				if (!finded){
					gold_rank r;
					r.uid_ = uid;
					r.value_ = (longlong)msg_ret.value_;
					if (strstr(msg_ret.key_, "ipbot_test") && !g_bot_names.empty()){
						r.uname_ = g_bot_names[rand_r(g_bot_names.size() - 1)];
					}
					if (glb_goldrank_.size() < 10){
						glb_goldrank_.push_back(r);
						std::sort(glb_goldrank_.begin(), glb_goldrank_.end(), bigger_gold_rank);
					}
					else{
						if (msg_ret.value_ > glb_goldrank_[glb_goldrank_.size() - 1].value_){
							glb_goldrank_.pop_back();
							glb_goldrank_.push_back(r);
							std::sort(glb_goldrank_.begin(), glb_goldrank_.end(), bigger_gold_rank);
						}
					}
				}
				else
					std::sort(glb_goldrank_.begin(), glb_goldrank_.end(), bigger_gold_rank);
			}
		}
	}
	else if (cmd == CMD_SET){
		if (msg_ret.return_code_ == CACHE_RET_SUCCESS){
			cache_data_ptr pdat = get_cache_unit(cmd_msg->key_);
			if (cmd_msg->val_type_ == 1){
				if (pdat.get()){
					pdat->set_str(cmd_msg->str_value_);
				}
				else{
					cache_data_ptr dat(new data_per_row);
					dat->key_ = cmd_msg->key_;//key 一定要在value前设置
					dat->set_str(cmd_msg->str_value_);
					add_new_cache_unit(dat->key_, dat);
				}
				COPY_STR(msg_ret.str_value_, cmd_msg->str_value_);
				is_add = 10;
			}
			else if(cmd_msg->val_type_ == 0){
				if (pdat.get()){
					msg_ret.value_ = pdat->set_double(cmd_msg->value_);
				}
				else{
					cache_data_ptr dat(new data_per_row);
					dat->key_ = cmd_msg->key_;
					dat->set_double(cmd_msg->value_);
					add_new_cache_unit(dat->key_, dat);

					msg_ret.value_ = cmd_msg->value_;
				}
				is_add = 4;
			}
			else {
				msg_ret.return_code_ = CACHE_RET_WRONG_DAT_TYPE;
			}
		}
	}
	//试着减少,值不能<0
	else if(cmd == CMD_COST){
		if (cmd_msg->val_type_ != 0)	{
			msg_ret.return_code_ = CACHE_RET_WRONG_DAT_TYPE;
		}
		else{
			cache_data_ptr pdat = get_cache_unit(cmd_msg->key_);
			if (pdat.get()){
				int ret = 0;
				double v = pdat->take(cmd_msg->value_, ret);
				if (ret != 0 )	{
					msg_ret.return_code_ = CACHE_RET_LESS_THAN_COST;
				}
				else{
					msg_ret.value_ = v;
					is_add = 1;
				}
			}
			else{
				msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
			}
		}
	}
	//扣除所有
	else if(cmd == CMD_COST_ALL ){
		if (cmd_msg->val_type_ != 0)	{
			msg_ret.return_code_ = CACHE_RET_WRONG_DAT_TYPE;
		}
		else{
			cache_data_ptr pdat = get_cache_unit(cmd_msg->key_);
			if (pdat.get()){
				int ret = 0;
				double v = pdat->take_all(ret);
				if (ret != 0)	{
					msg_ret.return_code_ = CACHE_RET_LESS_THAN_COST;
				}
				else{
					msg_ret.value_ = v;
					is_add = 1;
				}
			}
			else{
				msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
			}
		}
	}
	else if (cmd == CMD_BIND){
		std::string s = cmd_msg->key_; s += KEY_USER_IID;
		bind_iid(s, (longlong)cmd_msg->value_, msg_ret);
		if (msg_ret.return_code_ == CACHE_RET_SUCCESS){
			is_add = 5;
		}
	}
	//查询
	else if (cmd == CMD_GET){
		if (_stricmp(cmd_msg->key_, KEY_GETRANK) == 0){
			msg_ret.val_type_ = 1;
			boost::lock_guard<boost::mutex> lk(glb_goldrank_mut_);
			if (glb_goldrank_.empty()){
				msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
			}
			for (unsigned int i = 0 ; i < glb_goldrank_.size(); i++)
			{
				msg_cache_cmd_ret<remote_t> msg_r;
				msg_r.return_code_ = 0;
				COPY_STR(msg_r.key_, cmd_msg->key_);
				msg_r.val_type_ = 1;
				//如果是机器人
				if (strstr(glb_goldrank_[i].uid_.c_str(), "ipbot_test")){
					msg_r.str_value_[0] = 1;
					strncpy_s(msg_r.str_value_ + 1, 44, glb_goldrank_[i].uname_.c_str(), 44);
					strncpy_s(msg_r.str_value_ + 45, 19, lex_cast_to_str( glb_goldrank_[i].value_).c_str(), 19);
				}
				else{
					msg_r.str_value_[0] = 0;
					cache_data_ptr pdat_name = get_cache_unit(glb_goldrank_[i].uid_ + KEY_USER_UNAME);
					if (pdat_name.get()){
						if (pdat_name->get_str_value() == ""){
							pdat_name->set_str("guest");
						}
						strncpy_s(msg_r.str_value_ + 1, 44,  pdat_name->get_str_value().c_str(), 44);
					}
					strncpy_s(msg_r.str_value_ + 45, 19, lex_cast_to_str( glb_goldrank_[i].value_).c_str(), 19);
				}
				send_msg(psock, msg_r);
			}
		}
		else{
			cache_data_ptr pdat = get_cache_unit(cmd_msg->key_);
			if (!pdat.get()){
				msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
			}
			else{
				pdat->get_data(msg_ret.val_type_, msg_ret.str_value_, sizeof(msg_ret.str_value_));
			}
		}
	}
	else if (cmd == ACC_REG){
		msg_ret.val_type_ = 0;
		int r = CACHE_RET_SUCCESS;
		{
			boost::lock_guard<boost::mutex> lk(common_mut_);
			if(acc_maps_uid.find(cmd_msg->str_value_) != acc_maps_uid.end()){
				r = CACHE_RET_UNIQUE_CHECK_FAIL;
			}
		}
		if (r != CACHE_RET_SUCCESS){
			msg_ret.return_code_ = r;
		}
		else{
			cache_data_ptr pcache = get_cache_unit(std::string(cmd_msg->key_) + KEY_USER_ACCNAME);
			if (!pcache.get()){
				{
					boost::lock_guard<boost::mutex> lk(common_mut_);
					acc_maps_uid.insert(std::make_pair<std::string, std::string>(cmd_msg->str_value_, cmd_msg->key_));
				}
				pcache.reset(new data_per_row);
				pcache->key_ = std::string(cmd_msg->key_) + KEY_USER_ACCNAME;
				pcache->set_str(cmd_msg->str_value_);
				add_new_cache_unit(pcache->key_, pcache);

				bind_iid(std::string(cmd_msg->key_) + KEY_USER_IID, 0, msg_ret);
				msg_ret.return_code_ = CACHE_RET_SUCCESS;
				is_add = 11;
			}
			else{
				msg_ret.return_code_ = CACHE_RET_DATA_EXISTS;
			}
		}
	}
	else if (cmd == UID_LOGIN){
		msg_ret.return_code_ = CACHE_RET_SUCCESS;
		msg_ret.val_type_ = 0;
		user_login(cmd_msg->key_, psock);
	}
	else if (cmd == UID_LOGOUT){
		msg_ret.return_code_ = CACHE_RET_SUCCESS;
		msg_ret.val_type_ = 0;
		user_logout(cmd_msg->key_, psock);
	}
	else if (cmd == ACC_LOGIN){

		msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
		msg_ret.val_type_ = 1;

		boost::lock_guard<boost::mutex> lk(common_mut_);
		auto itf = acc_maps_uid.find(cmd_msg->key_);
		if (itf != acc_maps_uid.end()){
			std::string uid = itf->second;
			cache_data_ptr pcache = get_cache_unit(uid + KEY_USER_ACCPWD);
			if (pcache.get()){
				char tp; char val[max_guid] = {0};
				pcache->get_data(tp, val, max_guid);
				//密码匹配
				if (_stricmp(val, cmd_msg->str_value_) == 0){
					COPY_STR(msg_ret.str_value_, uid.c_str());
					msg_ret.return_code_ = CACHE_RET_SUCCESS;
				}
				else
				{
					msg_ret.return_code_ = CACHE_RET_NOT_MATCH;
				}
			}
		}
	}
	else if (cmd == IID_LOGIN){
		msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
		msg_ret.val_type_ = 1;
		boost::lock_guard<boost::mutex> lk(common_mut_);
		auto itf = iid_maps_uid.find((longlong)cmd_msg->value_);
		if (itf != iid_maps_uid.end()){
			std::string uid = itf->second;
			cache_data_ptr pcache = get_cache_unit(uid + KEY_USER_ACCPWD);
			if (pcache.get()){
				char tp; char val[max_guid] = {0};
				pcache->get_data(tp, val, max_guid);
				//密码匹配,这个命令密码放在key段。省事。
				if (_stricmp(val, cmd_msg->key_) == 0){
					COPY_STR(msg_ret.str_value_, uid.c_str());
					msg_ret.return_code_ = CACHE_RET_SUCCESS;
					user_login(uid, psock);
				}
				else
				{
					msg_ret.return_code_ = CACHE_RET_NOT_MATCH;
				}
			}
		}
	}
	else if (cmd == ACC_UID){
		msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
		msg_ret.val_type_ = 1;
		boost::lock_guard<boost::mutex> lk(common_mut_);
		auto itf = acc_maps_uid.find(cmd_msg->key_);
		if (itf != acc_maps_uid.end()){
			msg_ret.return_code_ = CACHE_RET_SUCCESS;
			COPY_STR(msg_ret.str_value_, itf->second.c_str());
		}
	}
	else if (cmd == IID_UID){
		msg_ret.return_code_ = CACHE_RET_NO_REQ_DATA;
		msg_ret.val_type_ = 1;
		boost::lock_guard<boost::mutex> lk(common_mut_);
		auto itf = iid_maps_uid.find((longlong)cmd_msg->value_);
		if (itf != iid_maps_uid.end()){
			msg_ret.return_code_ = CACHE_RET_SUCCESS;
			COPY_STR(msg_ret.str_value_, itf->second.c_str());
		}
	}
	else{
		msg_ret.return_code_ = CACHE_RET_UNKNOWN_CMD;
	}
	//保存结果
	if (msg_ret.return_code_ == CACHE_RET_SUCCESS && is_add > 0){

	}

	if (cmd_msg->head_.cmd_ == GET_CLSID(msg_cache_cmd)){
		send_msg(psock, msg_ret);
		//同步到另一台服务器
		if (!glb_is_syncing && is_add > 0){
			cache_sync_operation<remote_socket_ptr> op;
			op.sync_time_ = boost::posix_time::microsec_clock::local_time();
			msg_cache_cmd_sync<remote_socket_ptr>* psync = new msg_cache_cmd_sync<remote_socket_ptr>();
			COPY_STR(psync->cache_cmd, cmd_msg->cache_cmd);
			COPY_STR(psync->key_, cmd_msg->key_);
			memcpy(psync->str_value_, cmd_msg->str_value_, max_guid);
			psync->val_type_ = cmd_msg->val_type_;
			psync->game_id_ = cmd_msg->game_id_;
			op.the_msg_ = boost::shared_ptr<msg_cache_cmd<remote_socket_ptr>>(psync);
			glb_pending_sync_.push_back(op);

			hc.result_ = msg_ret.return_code_;
			auto sdat = std::make_pair(cmd_msg->sequence_, hc);
			glb_handled_sequence_.insert(sdat);
		}
	}
	//如果是同步过来的数据，回复同步结果
	else if (cmd_msg->head_.cmd_ == GET_CLSID(msg_cache_cmd_sync)){
		send_msg(psock, msg_ret);
	}
	return 0;
}

int handle_cache_msgs()
{
	auto it_cache = glb_msg_list_.begin();
	while (it_cache != glb_msg_list_.end())
	{
		auto cmd = *it_cache;	it_cache++;
		if (cmd->head_.cmd_ == GET_CLSID(msg_cache_cmd) || cmd->head_.cmd_ == GET_CLSID(msg_cache_cmd_sync)){
			//如果这个消处已处理，则回复已处理
			msg_cache_cmd<remote_socket_ptr> * cmd_msg = (msg_cache_cmd<remote_socket_ptr> *) cmd.get();
			auto seq_it = glb_handled_sequence_.find(cmd_msg->sequence_);
			if (seq_it != glb_handled_sequence_.end()){
				msg_cache_cmd_ret<remote_socket_ptr> msg;
				COPY_STR(msg.key_, cmd_msg->key_);
				COPY_STR(msg.sequence_, cmd_msg->sequence_);
				msg.val_type_ = cmd_msg->val_type_;
				msg.value_ = seq_it->second.result_;
				msg.return_code_ = CACHE_RET_SEQUENCE_LESS;
				send_msg(cmd_msg->from_sock_, msg);
			}
			else{
				int r = handle_msg_cache_cmd(cmd_msg, cmd_msg->from_sock_);
			}
		}
		else if (cmd->head_.cmd_ == GET_CLSID(msg_cache_updator_register)){
			msg_cache_updator_register<remote_socket_ptr> * cmd_msg = (msg_cache_updator_register<remote_socket_ptr> *) cmd.get();
			if(_stricmp(cmd_msg->key_, "FE77AC20-5911-4CCB-8369-27449BB6D806") == 0 ){
				cmd_msg->from_sock_->set_authorized();
			}
		}
		msg_handled++;
	}
	glb_msg_list_.clear();
	return 0;
}

template<class remote_t>
boost::shared_ptr<msg_base<remote_t>> create_msg(unsigned short cmd)
{
	typedef boost::shared_ptr<msg_base<remote_t>> msg_ptr;

	DECLARE_MSG_CREATE()
	case GET_CLSID(msg_cache_cmd):
		ret_ptr.reset(new msg_cache_cmd<remote_t>());
		break;
	case GET_CLSID(msg_cache_cmd_ret):
		ret_ptr.reset(new msg_cache_cmd_ret<remote_t>());
		break;
	case GET_CLSID(msg_cache_data_begin):
		ret_ptr.reset(new msg_cache_data_begin<remote_t>());
		break;
	case GET_CLSID(msg_cache_data_end):
		ret_ptr.reset(new msg_cache_data_end<remote_t>());
		break;
	case GET_CLSID(msg_cache_sync_complete):
		ret_ptr.reset(new msg_cache_sync_complete<remote_t>());
		break;
	case GET_CLSID(msg_cache_sync_request):
		ret_ptr.reset(new msg_cache_sync_request<remote_t>());
		break;
	case GET_CLSID(msg_cache_cmd_sync):
		ret_ptr.reset(new msg_cache_cmd_sync<remote_t>());
		break;
	case GET_CLSID(msg_cache_cmd_sync2):
		ret_ptr.reset(new msg_cache_cmd_sync2<remote_t>());
		break;
	case GET_CLSID(msg_cache_reset_sequence):
		ret_ptr.reset(new msg_cache_reset_sequence<remote_t>());
		break;
	case GET_CLSID(msg_cache_updator_register):
		ret_ptr.reset(new msg_cache_updator_register<remote_t>());
		break;
	case GET_CLSID(msg_cache_ping):
		ret_ptr.reset(new msg_cache_ping<remote_t>());
		break;
	END_MSG_CREATE()
		return  ret_ptr;
}

const longlong IID_GEN_START = 20000000;

class tast_on_5sec : public task_info
{
public:
	tast_on_5sec(boost::asio::io_service& ios):task_info(ios)
	{

	}
	int		routine();
};

int			load_bot_names()
{
	FILE* fp = NULL;
	fopen_s(&fp, "bot_names.txt", "r");
	if(!fp){
		glb_log.write_log("bot_names.txt does not exists!");
		return -1;
	}
	vector<char> data_read;
	char c[128];
	while (!feof(fp))
	{
		int s = fread(c, sizeof(char), 128, fp);
		for (int i = 0 ; i < s; i++)
		{
			data_read.push_back(c[i]);
		}
	}
	fclose(fp);

	split_str(data_read.data() + 3, data_read.size(), "\n", g_bot_names, true);
	if (g_bot_names.empty()){
		glb_log.write_log("bot names not valid!");
		return -1; 
	}
	return 0;
}

int			load_cache_dataes()
{
	std::string sql = "select uid, value_type, credits, str_value, optype from `account`";
	Query q(*db_);
	if(!q.get_result(sql)){
		glb_log.write_log("read cache from db error!!!");
		return -1;
	}

	while(q.fetch_row() && !glb_exit)
	{
		std::string key = q.getstr();
		int value_type_ = (char)q.getval();
		longlong v = q.getbigint();
		std::string s = q.getstr();
		int opt = q.getval();

		std::string subkey = trim_to_subkey(key); std::string uid = trim_to_uid(key);
		cache_data_ptr dat(new data_per_row);
		dat->key_ = key;

		if (value_type_ == 0){
			dat->set_double((double)v);
			if (strstr(key.c_str(), KEY_GENUID))	{
				if (v < IID_GEN_START){
					dat->set_double((longlong)IID_GEN_START);
				}
			}
		}
		else if(value_type_ == 1){
			dat->set_str(s.c_str());
		}
		else{
			glb_log.write_log("load cache failed, the data type incorrect, key = %s!", key.c_str());
			return -1;
		}
		add_new_cache_unit(dat->key_, dat);
		if (the_cache_.size() % 1000 == 0){
			glb_log.write_log("caches loaded from database: %d caches", the_cache_.size());
		}
	}
	q.free_result();
	glb_log.write_log("caches loaded from database: %d caches", the_cache_.size());

	sql = "select `uid`, `iid`, `uname`, `pwd`, `gold`, `nickname` from account_summary";
	if(!q.get_result(sql)){
		glb_log.write_log("read cache from db error!!!");
		return -1;
	}
	int nc = 0;
	while(q.fetch_row() && !glb_exit)
	{
		nc++;
		std::string uid = q.getstr();
		longlong iid = q.getbigint();
		std::string accname = q.getstr();
		std::string pwd = q.getstr();
		longlong gold = q.getbigint();
		std::string nickname = q.getstr();

		cache_data_ptr dat;

		dat.reset(new data_per_row);
		dat->key_ = uid + KEY_CUR_GOLD;
		dat->set_double((double)gold);
		add_new_cache_unit(dat->key_, dat);

		gold_rank r;
		r.uid_ = uid;
		r.value_ = gold;
		if (strstr(r.uid_.c_str(), "ipbot_test")){
			r.uname_ = g_bot_names[rand_r(g_bot_names.size() - 1)];
		}
		glb_goldrank_.push_back(r);

		if (iid > 0){
			dat.reset(new data_per_row);
			dat->key_ = uid + KEY_USER_IID;
			dat->set_double((double)iid);
			add_new_cache_unit(dat->key_, dat);

			iid_maps_uid.insert(std::make_pair(iid, uid));
		}

		if(accname != "")	{
			dat.reset(new data_per_row);
			dat->key_ = uid + KEY_USER_ACCNAME;
			dat->set_str(accname.c_str());
			add_new_cache_unit(dat->key_, dat);
			acc_maps_uid.insert(std::make_pair(accname, uid));
		}

		dat.reset(new data_per_row);
		dat->key_ = uid + KEY_USER_ACCPWD;
		dat->set_str(pwd.c_str());
		add_new_cache_unit(dat->key_, dat);

		dat.reset(new data_per_row);
		dat->key_ = uid + KEY_USER_UNAME;
		dat->set_str(nickname.c_str());
		add_new_cache_unit(dat->key_, dat);
		if (nc % 100 == 0){
			glb_log.write_log("caches loaded from database: %d caches", the_cache_.size());
		}
	}
	return 0;
}

int		start_network()
{
	the_sync_net.add_acceptor(the_config.sync_local_port_);
	if(!the_sync_net.run()){
		glb_log.write_log("!!!!!the_sync_net start failed!!!!");
		return -1;
	}

	the_net.add_acceptor(the_config.client_port_);

	if(the_net.run()){
		glb_log.write_log("net service start success!");
	}
	else{
		glb_log.write_log("!!!!!net service start failed!!!!");
		return -1;
	}
	return 0;
}

int		sync_from_other_cache(boost::asio::io_service& upd_ios, boost::shared_ptr<upd_socket> upd_sock)
{
	bool slave_is_available = false;
	//主cache启动，从备用cache那同步数据
	if(upd_sock->connect(the_config.sync_remote_ip_, the_config.sync_remote_port_, 1000) == 0){
	
		msg_cache_updator_register<remote_socket_ptr> msg_reg;
		send_msg(upd_sock, msg_reg);

		slave_is_available = true;
		{
			msg_cache_sync_request<boost::shared_ptr<upd_socket>> msg;
			send_msg(upd_sock, msg);
		}
		unsigned int nsync = 0;
		while(!glb_exit)
		{
			upd_ios.reset();
			upd_ios.poll();
			if (upd_sock->is_closed_){
				break;
			}
			boost::shared_ptr<msg_base<boost::shared_ptr<upd_socket>>>
				sync_msg = pickup_msg_from_socket(upd_sock, create_msg<boost::shared_ptr<upd_socket>>);
			bool	br = false;
			while (sync_msg.get()){
				if (sync_msg->head_.cmd_ == GET_CLSID(msg_cache_cmd_sync2)){
					nsync++;
					handle_msg_cache_cmd<boost::shared_ptr<upd_socket>>((msg_cache_cmd<boost::shared_ptr<upd_socket>> *)sync_msg.get(), upd_sock);
					if (nsync % 1000 == 0){
						glb_log.write_log("is synchronizing caches : %d", nsync);
					}
				}
				else if (sync_msg->head_.cmd_ ==  GET_CLSID(msg_cache_data_end)){
					br = true;
					break;
				}
				sync_msg = pickup_msg_from_socket(upd_sock, create_msg<boost::shared_ptr<upd_socket>>);
			}
			if (br)
				break;
		}
	}
	else{
		glb_log.write_log("!!!!!upd_sock->connect failed!!!! ip = %s, port = %d", 
			the_config.sync_remote_ip_.c_str(),
			 the_config.sync_remote_port_);
	}

	//启动网络模块
	if (start_network() != 0){
		return -1;
	}

	//直到本cache各模块全部准备好，才告诉从cache重启客户端服务
	if (slave_is_available){
		msg_cache_sync_complete<boost::shared_ptr<upd_socket>> msg_cmpl;
		send_msg(upd_sock, msg_cmpl);

		while(!glb_exit)
		{
			upd_ios.reset();
			upd_ios.poll();
			if (upd_sock->is_closed_){
				break;
			}
			boost::shared_ptr<msg_base<boost::shared_ptr<upd_socket>>>
				sync_msg = pickup_msg_from_socket(upd_sock, create_msg<boost::shared_ptr<upd_socket>>);
			if (sync_msg.get()){
				if(sync_msg->head_.cmd_ == GET_CLSID(msg_cache_sync_complete)){
					break;
				}
			}
		}
	}
	return 0;
}

void	check_for_sync()
{
	std::vector<remote_socket_ptr> remotes = the_sync_net.get_remotes();

	if (remotes.empty())	{	
		glb_is_syncing = false;
		return;
	}

	for (unsigned int i = 0 ;i < remotes.size(); i++)
	{
		remote_socket_ptr psock = remotes[i];
		boost::shared_ptr<msg_base<remote_socket_ptr>> pmsg = pickup_msg_from_socket(psock, create_msg<remote_socket_ptr>);
		while (pmsg.get())
		{
			//协同cache发出同步命令
			if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_sync_request)){
				if(glb_is_syncing) break;
				glb_is_syncing = true;
				glb_pending_sync_.clear();
				msg_cache_data_begin<remote_socket_ptr> msg_begin;
				send_msg(psock, msg_begin);

				auto itc = the_cache_.begin();
				while (itc != the_cache_.end())
				{
					cache_data_ptr pcache = itc->second; itc++;
					msg_cache_cmd_sync2<remote_socket_ptr> msg_cmd;
					COPY_STR(msg_cmd.cache_cmd, "CMD_SET");
					COPY_STR(msg_cmd.key_, pcache->key_.c_str());
					pcache->get_data(msg_cmd.val_type_, msg_cmd.str_value_, sizeof(msg_cmd.str_value_));
					send_msg(psock, msg_cmd);
				}
				msg_cache_data_end<remote_socket_ptr> msg_end;
				send_msg(psock, msg_end);
				glb_log.write_log("all sync sended!");
			}
			else if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_sync_complete)){
				glb_is_syncing = false;
				glb_pending_sync_.clear();
				upd_sock_.reset(new upd_socket(upd_ios));
				int r = upd_sock_->connect(the_config.sync_remote_ip_, the_config.sync_remote_port_, 5000);
				if (r != 0){
					glb_log.write_log("upd_sock connnect failed!");
				}
				msg_cache_sync_complete<remote_socket_ptr> msg_cmpl;
				send_msg(psock, msg_cmpl);

				msg_cache_updator_register<remote_socket_ptr> msg_reg;
				send_msg(upd_sock_, msg_reg);
			}
			else if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_updator_register)){
				msg_cache_updator_register<remote_socket_ptr> * cmd_msg = (msg_cache_updator_register<remote_socket_ptr> *) pmsg.get();
				if(_stricmp(cmd_msg->key_, "FE77AC20-5911-4CCB-8369-27449BB6D806") == 0 ){
					cmd_msg->from_sock_->set_authorized();
				}
			}
			else if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_ping)){
			}
			else {
				glb_msg_list_.push_back(pmsg);
				msg_recved++;
			}
			pmsg = pickup_msg_from_socket(psock, create_msg<remote_socket_ptr>);
		}
	}
}

void		check_sync_reply()
{
	if (upd_sock_->is_connected_ && !glb_is_syncing && !glb_pending_sync_.empty()){
		//如果列表第一个已经处理完成，删除它
		cache_sync_operation<remote_socket_ptr>& op = glb_pending_sync_.front();
		boost::posix_time::ptime pnow = boost::posix_time::microsec_clock::local_time();
		//如果同步已经发送，等待对面回复
		if(op.state_ == op.state_sended){
			//10秒重发操作数据
			if ((pnow - op.sync_time_).total_milliseconds() > 10000){
				send_msg(upd_sock_, *op.the_msg_);
				op.sync_time_ = pnow;
			}
		}
		else if (op.state_ == op.state_init){
			std::string seq = get_guid();
			COPY_STR(op.the_msg_->sequence_, seq.c_str());
			op.sync_time_ = pnow;
			op.state_ = op.state_sended;
			send_msg(upd_sock_, *op.the_msg_);
		}
		
		auto pmsg = pickup_msg_from_socket(upd_sock_, create_msg<boost::shared_ptr<upd_socket>>);

		while (pmsg.get()){
			if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_cmd_ret)){
				auto msg_ret = (msg_cache_cmd_ret<boost::shared_ptr<upd_socket>>*) (pmsg.get());
				if(_stricmp(msg_ret->sequence_, op.the_msg_->sequence_) == 0){
					op.state_ = op.state_replied;
					glb_pending_sync_.pop_front();
					break;
				}
			}
			pmsg = pickup_msg_from_socket(upd_sock_, create_msg<boost::shared_ptr<upd_socket>>);
		}
	}
	//连接断开了，就不需要同步了
	//为了防止连接断开状态，但是需要同步的情况（和对面服务器已完成首次同步，但连接没未来得及建立）
	else{
		glb_pending_sync_.clear();
	}
}

void	pickup_msgs(bool& busy)
{
	//同步的时候不允许外部访问缓存,因此不提取消息
	if (glb_is_syncing) return;
	std::vector<remote_socket_ptr>  remotes = the_net.get_remotes();
	for (unsigned int i = 0 ;i < remotes.size(); i++)
	{
		remote_socket_ptr psock = remotes[i];
		boost::shared_ptr<msg_base<remote_socket_ptr>> pmsg = pickup_msg_from_socket(psock, create_msg<remote_socket_ptr>);
		unsigned int pick_count = 0;
		while (pmsg.get())
		{
			busy = true;
			pick_count++;

			if (pmsg->head_.cmd_ == GET_CLSID(msg_cache_updator_register) ||
				psock->is_authorized()){
					if (pmsg->head_.cmd_ != GET_CLSID(msg_cache_ping)){
						glb_msg_list_.push_back(pmsg);
					}
			}

			msg_recved++;
			if (pick_count < 200){
				pmsg = pickup_msg_from_socket(psock, create_msg<remote_socket_ptr>);
			}
			else{
				break;
			}
		}
	}
}

bool glb_exited = false;
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		glb_exit = true;
	}
	while (!glb_exited)
	{

	}
	return TRUE;
}

int main()
{
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
	{
		return -1;
	}
	task_ptr task;

	std::vector<boost::shared_ptr<boost::thread>> cache_persistent_threads;
	upd_sock_.reset(new upd_socket(upd_ios));

	boost::system::error_code ec;
	the_config.load_from_file();
	db_.reset(new utf8_data_base(the_config.accdb_host_, the_config.accdb_user_,the_config.accdb_pwd_, the_config.accdb_name_));
	if(!db_->grabdb()){
		glb_log.write_log("database start failed!!!!");
		goto _EXIT;
	}

	for (int i = 0; i < db_hash_count; i++)
	{
		db_delay_helper_[i].set_db_ptr(db_);
	}

	if (load_bot_names() != 0){
		goto _EXIT;
	}

	if (load_cache_dataes() != 0){
		goto _EXIT;
	}
	will_persistent_.clear();

	if (sync_from_other_cache(upd_ios, upd_sock_) != 0){
		goto _EXIT;
	}

	for (int i = 0; i < db_hash_count; i++)
	{
		boost::shared_ptr<boost::thread> td(new boost::thread(boost::bind(cache_persistent_thread_fun, db_delay_helper_ + i)));
		cache_persistent_threads.push_back(td);
	}

	if (!glb_goldrank_.empty()){
		std::sort(glb_goldrank_.begin(), glb_goldrank_.end(), bigger_gold_rank);
		if (glb_goldrank_.size() > 10){
			glb_goldrank_.resize(10);
		}
	}
	
	int idle_count = 0;

	tast_on_5sec* ptask = new tast_on_5sec(timer_srv);
	task.reset(ptask);
	ptask->schedule(5000);

	while(!glb_exit)
	{
		bool busy = false;
		upd_ios.reset();
		upd_ios.poll();

		timer_srv.reset();
		timer_srv.poll();

		check_for_sync();
		pickup_msgs(busy);
		handle_cache_msgs();


		pickup_modify();
		
		check_sync_reply();

		int counter = 0;
		auto it = glb_handled_sequence_.begin();
		while (it != glb_handled_sequence_.end() && counter < 1000){
			auto& dat = it->second;
			boost::posix_time::ptime pt = boost::posix_time::second_clock::local_time();
			if ((pt - dat.pt_).total_seconds() > 10){
				it = glb_handled_sequence_.erase(it);
			}
			else{
				it++;
			}
			counter++;
		}
		if (!busy){
			idle_count++;
			if (idle_count > 4999){
				boost::posix_time::milliseconds ms(20);
				boost::this_thread::sleep(ms);
				idle_count = 0;
			}
		}
		else{
			idle_count = 0;
		}
	}
_EXIT:

	task.reset();

	for (unsigned int i = 0 ; i < cache_persistent_threads.size(); i++)
	{
		cache_persistent_threads[i]->join();
	}

	pickup_modify();

	//继续执行没保存完的sql语句
	for (int i = 0 ; i < db_hash_count; i++)
	{
		db_delay_helper_[i].pop_and_execute();
	}

	upd_sock_->close();
	upd_ios.stop();
	timer_srv.stop();

	the_net.stop();
	the_sync_net.stop();
	the_cache_.clear();
	
	glb_log.stop_log();
	cout <<"exited!"<< endl;
	return 0;
}

int tast_on_5sec::routine()
{
	glb_log.write_log("game is running ok, this time msg recved:%d, handled:%d, remain sending data: %d", msg_recved, msg_handled, unsended_data);
	msg_handled = 0;
	msg_recved = 0;
	unsended_data = 0;
	return task_info::routine_ret_continue;
}
