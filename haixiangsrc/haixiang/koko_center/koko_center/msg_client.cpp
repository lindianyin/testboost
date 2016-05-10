#include "msg_client.h"
#include "error.h"
#include "safe_list.h"
#include "player.h"
#include <unordered_map>
#include "msg_server.h"
#include "utility.h"
#include "server_config.h"
#include "Database.h"
#include "db_delay_helper.h"
#include "log.h"
#include "utf8_db.h"
#include "Query.h" 
#include "http_request.h"
#include "server_config.h"

typedef boost::shared_ptr<msg_base<koko_socket_ptr>> msg_ptr;
extern safe_list<msg_ptr>		sub_thread_process_msg_lst_;
extern log_file<cout_output> glb_log;
extern player_ptr get_player(std::string uid);
extern service_config the_config;
extern db_delay_helper<std::string, int>& get_delaydb();
extern boost::shared_ptr<utf8_data_base>	db_;
extern std::vector<channel_server> channel_servers;

int msg_user_login::handle_this()
{
	if(from_sock_->is_login_.load())
		return error_success;

	if (sub_thread_process_msg_lst_.size() > 100){
		return error_server_busy;
	}
	
	msg_srv_progress msg;
	msg.pro_type_ = 0;
	msg.step_ = 0;
	send_msg(from_sock_, msg);

	from_sock_->is_login_ = true;
	sub_thread_process_msg_lst_.push_back(
		boost::dynamic_pointer_cast<msg_base<koko_socket_ptr>>(shared_from_this()));
	return error_success;
}

int msg_user_register::handle_this()
{
	if(from_sock_->is_register_.load())
		return error_success;

	if (sub_thread_process_msg_lst_.size() > 100){
		return error_server_busy;
	}

	sub_thread_process_msg_lst_.push_back(
		boost::dynamic_pointer_cast<msg_base<koko_socket_ptr>>(shared_from_this()));

	from_sock_->is_register_ = true;
	return error_success;
}

int msg_change_account_info::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return error_cant_find_player;

	bool mail_used = false, mobile_used = false;
	if (pp->email_ != email_){
		std::string sql = "select * from user_account where mail = '" + std::string(email_) + "'";
		Query q(*db_);
		if (q.get_result(sql) && q.fetch_row()){
			mail_used = true;
		}
		else{
			pp->email_ = email_;
		}
	}

	if (pp->mobile_ != mobile_){
		std::string sql = "select * from user_account where  mobile_number = '" + std::string(mobile_) + "'";
		Query q(*db_);
		if (q.get_result(sql) && q.fetch_row()){
			mobile_used = true;
		}
		else{
			pp->mobile_ = mobile_;
		}
	}
	pp->nickname_ = nick_name_;
	pp->byear_ = byear_;
	pp->bmonth_ = bmonth_;
	pp->bday_ = bday_;
	pp->age_ = age_;
	pp->address_ = address_;
	pp->idnumber_ = idcard_;
	pp->gender_ = gender_;

	BEGIN_UPDATE_TABLE("user_account");
	SET_FIELD_VALUE("nickname", nick_name_);
	SET_FIELD_VALUE("byear", byear_);
	SET_FIELD_VALUE("bmonth", bmonth_);
	SET_FIELD_VALUE("bday", bday_);
	SET_FIELD_VALUE("age", age_);
	SET_FIELD_VALUE("address", address_);
	SET_FIELD_VALUE("addr_province", region1_);
	SET_FIELD_VALUE("addr_city", region2_);
	SET_FIELD_VALUE("addr_region", region3_);
	if (!pp->mobile_v_ && !mobile_used){
		SET_FIELD_VALUE("mobile_number", mobile_);
	}
	
	if (!pp->email_v_ && !mail_used){
		SET_FIELD_VALUE("mail", email_);
	}
	SET_FIELD_VALUE("idnumber", idcard_);
	SET_FINAL_VALUE("gender", gender_);
	WITH_END_CONDITION("where uid = '" + pp->uid_ + "'")
	EXECUTE_NOREPLACE_DELAYED("", get_delaydb());

	msg_account_info_update msg;
	char*	s = (char*)&msg.gender_;
	char* e = ((char*)&msg.region3_) + sizeof(msg.region3_);
	memcpy(&msg.gender_, &gender_, e - s);
	COPY_STR(msg.email_,pp->email_.c_str());
	COPY_STR(msg.mobile_,pp->mobile_.c_str());
	send_msg(from_sock_, msg);
	
	if (mobile_used){
		return error_mobile_inusing;
	}
	else if (mail_used){
		return error_email_inusing;
	}
	else
		return error_success;
}

extern boost::shared_ptr<boost::asio::io_service> timer_srv;
class		send_verify_code_request : public http_request<send_verify_code_request>
{
public:
	koko_socket_ptr response_;
	send_verify_code_request():
		http_request<send_verify_code_request>(*timer_srv)
	{

	}
	void	write_log(const char* log_fmt, ...)
	{
		va_list ap;
		va_start(ap, log_fmt);
		int ret = glb_log.write_log(log_fmt, ap);
		va_end(ap);
	}

	void	on_request_failed(int reason, std::string err_msg)
	{
		response_->mverify_code_.clear();
		response_->mverify_error_ = "http error:" + boost::lexical_cast<std::string>(reason);
		http_request::on_request_failed(reason, err_msg);
	}

	void	handle_http_body(std::string str_body)
	{
		std::string		ret_code, content;
		std::string		pat = "\"errorCode\":\"";
		char* c = strstr((char*)str_body.c_str(), pat.c_str());
		//如果找到
		if (c){
			char* e = strstr(c + pat.length(), "\"");
			if (e){
				ret_code.insert(ret_code.end(), c + pat.length(), e);
			}
		}

		pat = "\"content\":\"";
		c = strstr((char*)str_body.c_str(), pat.c_str());
		//如果找到
		if (c){
			char* e = strstr(c + pat.length(), "\"");
			content.insert(content.end(), c + pat.length(), e);
		}

		if (ret_code == "10000"){
			response_->mverify_code_ = content;
			response_->mverify_time_ = boost::posix_time::microsec_clock::local_time();
		}
		else{
			response_->mverify_error_ = content;
			response_->mverify_code_.clear();
		}
	}
};

extern service_config the_config;
int msg_get_verify_code::handle_this()
{
	if(type_ == 0){
		if (sub_thread_process_msg_lst_.size() > 100){
			return error_server_busy;
		}

		sub_thread_process_msg_lst_.push_back(
			boost::dynamic_pointer_cast<msg_base<koko_socket_ptr>>(shared_from_this()));
	}
	else{
		boost::shared_ptr<send_verify_code_request> req(new send_verify_code_request());
		char buff[128] = {0};
		if (type_ == 1)	{
			sprintf_s(buff, 128, "/kokoportal/user/sendCode/%s/2.htm", mobile_);
		}
		else{
			sprintf_s(buff, 128, "/kokoportal/user/sendCode/%s/1.htm", mobile_);
		}
		
		std::string request = build_http_request(the_config.shortmsg_host_, buff);
		req->response_ = from_sock_;
		req->request(request, the_config.shortmsg_host_, the_config.shortmsg_port_);
	}
	return error_success;
}

int func_check_verify_code(std::string vcode, koko_socket_ptr fromsock, bool is_check)
{
	if (is_check){
		if (fromsock->verify_code_.load() == ""){
			return error_wrong_verify_code;
		}
	}
	else{
		if (fromsock->verify_code_backup_.load() == ""){
			return error_wrong_verify_code;
		}
	}

	std::vector<std::string> v1, v2;
	split_str(vcode.c_str(), vcode.length(), ",", v1, true);
	std::string vc;
	if (is_check){
		vc = fromsock->verify_code_.load();
	}
	else{
		vc = fromsock->verify_code_backup_.load();
	}
	
	split_str(vc.c_str(), vc.length(), ",", v2, true);

	auto it1 = v1.begin();
	while (it1 != v1.end())
	{
		auto it2 = v2.begin();
		bool	finded = false;
		while (it2 != v2.end())
		{
			if (*it1 == *it2){
				it2 = v2.erase(it2);
				finded = true;
			}
			else{
				it2++;
			}
		}
		if (finded){
			it1 = v1.erase(it1);
		}
		else{
			it1++;
		}
	}
	//必须v1和v2中的每一项都相同，验证码才能通过
	if (!v1.empty() || !v2.empty()){
	 return error_wrong_verify_code;
	}

	//每个图片验证码只能验证一次
	if (is_check){
		fromsock->verify_code_backup_.store(fromsock->verify_code_.load());
		fromsock->verify_code_.store("");
	}
	else{
		fromsock->verify_code_backup_.store("");
	}
	return error_success;
}

int msg_check_data::handle_this()
{
	Query q(*db_);

	char sdata[256] = {0};
	int ret = error_success;
	do {
		int len =	mysql_real_escape_string(&db_->grabdb()->mysql,sdata, query_sdata_, strlen(query_sdata_));

		if (query_type_ == check_account_name_exist){
			std::string sql = "select 1 from user_account where uname = '" + std::string(sdata) + "'";
			if (q.get_result(sql) && q.fetch_row()){
				ret = error_account_exist;
			}
		}
		else if (query_type_ == check_email_exist)	{
			std::string sql = "select 1 from user_account where mail = '" + std::string(sdata) + "'";
			if (q.get_result(sql) && q.fetch_row()){
				ret = error_email_inusing;
			}
		}
		else if (query_type_ == check_mobile_exist)	{
			std::string sql = "select 1 from user_account where mobile_number = '" + std::string(sdata) + "'";
			if (q.get_result(sql) && q.fetch_row()){
				ret = error_mobile_inusing;
			}
		}
		else if (query_type_ == check_verify_code){
			ret = func_check_verify_code(query_sdata_, from_sock_, true);
		}
		else if (query_type_ == check_mverify_code){
			if (from_sock_->mverify_code_ == ""){
				ret = error_wrong_verify_code;
			}
			else if(from_sock_->mverify_code_ != query_sdata_){
				ret = error_wrong_verify_code;
			}
			else if ((boost::posix_time::microsec_clock::local_time() - from_sock_->mverify_time_).total_seconds() > 120){
				ret = error_time_expired;
			}
		}
	} while (0);

	msg_check_data_ret msg;
	msg.query_type_ = query_type_;
	msg.result_ = ret;
	send_msg(from_sock_, msg);

	return error_success;
}

static std::map<std::string, __int64> handled_trade;
extern std::string	md5_hash(std::string sign_pat);

int msg_action_record::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();

	Database& db(*db_);
	BEGIN_INSERT_TABLE("log_client_action");
	if (pp.get()){
		SET_FIELD_VALUE("uid", pp->uid_);
	}
	SET_FIELD_VALUE("action_id", action_id_);
	SET_FIELD_VALUE("action_data", action_data_);
	SET_FINAL_VALUE("from_ip", from_sock_->remote_ip());
	EXECUTE_IMMEDIATE();

	return error_success;
}

int msg_get_token::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return error_cant_find_player;

	__int64 seq = 0;	::time(&seq);

	std::string token = md5_hash(pp->uid_ + boost::lexical_cast<std::string>(seq) + the_config.token_generate_key_);
	
	msg_sync_token msg;
	COPY_STR(msg.token_, token.c_str());
	msg.sequence_ = seq;
	send_msg(from_sock_, msg);

	return error_success;
}

bool func_2(channel_server& a1, channel_server& a2)
{
	return a1.online_ < a2.online_;
}

template<class socket_t>
int	distribute_channel_server(socket_t psock, int gameid)
{
	if (channel_servers.empty())
		return error_cant_find_coordinate;

	std::vector<channel_server> v;
	for (int i = 0; i < (int)channel_servers.size(); i++)
	{
		auto it = std::find(channel_servers[i].game_ids_.begin(), channel_servers[i].game_ids_.end(), gameid);
		if (it != channel_servers[i].game_ids_.end() || 
			(!channel_servers[i].game_ids_.empty() && channel_servers[i].game_ids_[0] == -1)){
				v.push_back(channel_servers[i]);
		}
	}

	if (v.empty())
		return error_cant_find_coordinate;

	std::sort(v.begin(), v.end(), func_2);

	msg_channel_server msg;
	msg.for_game_ = gameid;
	COPY_STR(msg.ip_, v[0].ip_.c_str());
	msg.port_ = v[0].port_;
	send_msg(psock, msg);

	return error_success;
}

int msg_get_game_coordinate::handle_this()
{
	return distribute_channel_server(from_sock_, gameid_);
}
