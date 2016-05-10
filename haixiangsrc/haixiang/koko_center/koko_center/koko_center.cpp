#include "boost/asio.hpp"
#include "boost/date_time.hpp"
#include "boost/atomic.hpp"
#include "boost/regex.hpp"
#include <unordered_map>
#include "memcache_defines.h"
#include "schedule_task.h"
#include <set>
#include <math.h>
#include "net_service.h"
#include "utf8_db.h"
#include "Query.h"
#include "db_delay_helper.h"
#include "utility.h"
#include "error.h"
#include "md5.h"
#include "player.h"
#include "msg_server.h"
#include "msg_client.h"
#include "log.h"
#include "server_config.h"
#include "http_request.h"

#include <DbgHelp.h>

#pragma auto_inline (off)
#pragma comment(lib, "dbghelp.lib")

const int			db_hash_count = 1;
boost::shared_ptr<utf8_data_base>	db_;
db_delay_helper<std::string, int>	db_delay_helper_[db_hash_count];
boost::shared_ptr<boost::asio::io_service> timer_srv;

std::unordered_map<unsigned int, task_wptr> task_info::glb_running_task_;
boost::shared_ptr<net_server<koko_socket>> the_net;

boost::atomic_uint	msg_recved(0);
boost::atomic_uint	msg_handled(0);
boost::atomic_uint	unsended_data(0);

typedef boost::shared_ptr<msg_base<koko_socket_ptr>> msg_ptr;		//从客户端发过来的消息类型
typedef boost::shared_ptr<msg_base<nosocket>> smsg_ptr;					//发送给客户端的消息类型

std::list<msg_ptr>	glb_msg_list_;//全局消息队列

log_file<cout_output> glb_log;

std::unordered_map<std::string, player_ptr> online_players;
safe_list<player_ptr> pending_login_users_;

safe_list<msg_ptr>		sub_thread_process_msg_lst_;

safe_list<smsg_ptr>		broadcast_msg_;
service_config the_config;
task_ptr task;
std::vector<boost::shared_ptr<boost::thread>> cache_persistent_threads;

typedef boost::shared_ptr<game_info> gamei_ptr;
std::map<int, gamei_ptr> all_games;
std::vector<channel_server> channel_servers;

extern int	update_user_item(std::string uid, int op, int itemid, __int64 count, __int64& ret, bool sync_to_client);
extern int	do_multi_costs(std::string uid, std::map<int, int> cost);
extern int	sync_user_item(std::string uid, int itemid);
db_delay_helper<std::string, int>& get_delaydb()
{
	return db_delay_helper_[0];
}

player_ptr get_player(std::string uid)
{
	auto itf = online_players.find(uid);
	if (itf != online_players.end()){
		return itf->second;
	}
	return player_ptr();
}

void sql_trim(char* str)
{
	for (unsigned int i = 0; i < strlen(str); i++)
	{
		if (str[i] == '\''){
			str[i] = ' ';
		}
	}
}

template<class remote_t>
void	response_user_login(player_ptr pp, remote_t pconn, msg_user_login_ret& msg)
{	
	msg.gold_ = pp->gold_;
	msg.game_gold_ = pp->gold_game_;
	msg.game_gold_free_ = pp->gold_free_;
	msg.iid_ = pp->iid_;
	msg.vip_level_ = pp->vip_level_;
	msg.sequence_ = pp->seq_;
	COPY_STR(msg.idcard_, pp->idnumber_.c_str());

	COPY_STR(msg.phone, pp->mobile_.c_str());
	msg.phone_verified_ = pp->mobile_v_;

	COPY_STR(msg.email_, pp->email_.c_str());
	msg.email_verified_ = pp->email_v_;
	msg.age_ = pp->age_;
	msg.gender_ = pp->gender_;
	msg.level_ = pp->level_;
	msg.byear_ = pp->byear_;
	msg.bmonth_ = pp->bmonth_;
	msg.region1_ = pp->region1_;
	msg.region2_ = pp->region2_;
	msg.region3_ = pp->region3_;
	msg.bday_ = pp->bday_;

	COPY_STR(msg.address_, pp->address_.c_str());
	COPY_STR(msg.nickname_, pp->nickname_.c_str());
	COPY_STR(msg.uid_, pp->uid_.c_str());
	COPY_STR(msg.token_, pp->token_.c_str());
	send_msg(pconn, msg);
}

template<class socket_t>
void	send_image(msg_image_data& msg_shoot, std::vector<char> sdata, socket_t sock)
{
	msg_shoot.TAG_ = 1;
	for (int i = 0; i < (int)sdata.size();)
	{
		int dl = sdata.size() - i;
		dl = std::min<int>(512, dl);
		memcpy(msg_shoot.data_, sdata.data() + i, dl);
		i += dl;
		msg_shoot.len_ = dl;
		send_msg(sock, msg_shoot);
		msg_shoot.TAG_ = 0;
	}
	msg_shoot.TAG_ = 2;
	msg_shoot.len_ = 0;
	send_msg(sock, msg_shoot);
}

msg_ptr create_msg_from_client(unsigned short cmd)
{
	DECLARE_MSG_CREATE()
	REGISTER_CLS_CREATE(msg_user_login);
	REGISTER_CLS_CREATE(msg_user_register);
	REGISTER_CLS_CREATE(msg_change_account_info);
	REGISTER_CLS_CREATE(msg_get_verify_code);
	REGISTER_CLS_CREATE(msg_check_data);
	REGISTER_CLS_CREATE(msg_action_record);
	REGISTER_CLS_CREATE(msg_get_game_coordinate);
	REGISTER_CLS_CREATE(msg_get_token);
	END_MSG_CREATE()
	return  ret_ptr;
}

bool glb_exit = false;
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

class task_on_5sec : public task_info
{
public:
	task_on_5sec(boost::asio::io_service& ios):task_info(ios)
	{

	}
	int		routine();
};

void	pickup_player_msgs(bool& busy)
{
	auto  remotes = the_net->get_remotes();
	for (unsigned int i = 0 ;i < remotes.size(); i++)
	{
		koko_socket_ptr psock = remotes[i];
		auto pmsg = pickup_msg_from_socket(psock, create_msg_from_client);
		unsigned int pick_count = 0;
		while (pmsg.get())
		{
			busy = true;
			pick_count++;

			pmsg->from_sock_ = psock;
			glb_msg_list_.push_back(pmsg);

			msg_recved++;
			if (pick_count < 200){
				pmsg =  pickup_msg_from_socket(psock, create_msg_from_client);
			}
			else{
				break;
			}
		}
	}
}

std::string			md5_hash(std::string sign_pat)
{
	CMD5 md5;
	unsigned char	out_put[16];
	md5.MessageDigest((unsigned char*)sign_pat.c_str(), sign_pat.length(), out_put);

	std::string sign_key;
	for (int i = 0; i < 16; i++)
	{
		char aa[4] = {0};
		sprintf_s(aa, 4, "%02x", out_put[i]);
		sign_key += aa;
	}
	return sign_key;
}

template<class remote_t>
int		verify_user_info(msg_user_login_t<remote_t>* plogin, bool check_sign = true)
{
	if (check_sign){
		std::string	sign_pat = plogin->acc_name_ + std::string(plogin->pwd_hash_) + plogin->machine_mark_ + the_config.signkey_;
		std::string sign_key = md5_hash(sign_pat);
		std::string sign_remote;
		sign_remote.insert(sign_remote.begin(), plogin->sign_, plogin->sign_ + 32);
		if (sign_key != sign_remote){
			return error_wrong_sign;
		}
	}
	//防止sql注入攻击
	sql_trim(plogin->acc_name_);
	sql_trim(plogin->machine_mark_);
	Database& db = *db_;
	Query q(db);


	//如果是在禁止列表中，不允许登录
	std::string sql = "select 1 from user_blacklist where machine_mark = '" + std::string(plogin->machine_mark_) + "'";
	if (!q.get_result(sql)){
		return error_sql_execute_error;
	}

	if (q.fetch_row()){
		return error_user_banned;
	}
	q.free_result();

	return error_success;
}

int		handle_get_verify_code(msg_get_verify_code* pverify)
{
	std::string sql = "call random_select_verify_code(8);";
	Database& db = *db_;
	Query q(db);

	if (!q.get_result(sql)){
		return error_success;
	}
	vector<vector<char>> anwsers;
	vector<char> image, image2, tmp_iamge;
	msg_verify_code msg;
	
	int rows = q.num_rows();
	int ans = rand_r(rows - 1);
	int i = 0;
	//混淆答案
	while (q.fetch_row())
	{
		if (i == ans){
			COPY_STR(msg.question_, q.getstr());
			q.getblob("image", image);
			q.getblob("image2", image2);
		}
		else{
			q.getstr();
			tmp_iamge.clear();
			q.getblob("image", tmp_iamge);
			if (!tmp_iamge.empty()){
				anwsers.push_back(tmp_iamge);
			}
			tmp_iamge.clear();

			q.getblob("image2", tmp_iamge);
			if (!tmp_iamge.empty()){
				anwsers.push_back(tmp_iamge);
			}
		}
		i++;
	}

	send_msg(pverify->from_sock_, msg);

	std::random_shuffle(anwsers.begin(), anwsers.end());
	int index = -1;
	if (!image.empty()){
		index  = rand_r(std::min<int>(7, anwsers.size() - 1));
		anwsers.insert(anwsers.begin() + index, image);
	}

	int index2 = -1;
	if (!image2.empty()){
		index2  = rand_r(std::min<int>(7, anwsers.size() - 1));
		anwsers.insert(anwsers.begin() + index2, image2);
		//如果插在前一副图的前面，则前面一副图的index+1
		if (index2 <= index){
			index++;
		}
	}
	
	for (int i = 0; i < (int)anwsers.size() && i < 8; i++)
	{
		vector<char>& img = anwsers[i];
		msg_image_data msg_img;
		msg_img.this_image_for_ = -2;
		send_image(msg_img, img, pverify->from_sock_);
	}

	std::string vc;
	if (index >= 0)
		vc += boost::lexical_cast<std::string>(index) + ",";

	if (index2 >= 0)
		vc += boost::lexical_cast<std::string>(index2) + ",";

	pverify->from_sock_->verify_code_.store(vc);
	return error_success;
}

int		handle_user_register(msg_user_register* pregister)
{
	int verify = verify_user_info(pregister);
	if (verify != error_success){
		return verify;
	}

	Query q(*db_);
	std::string sql = "select count(*) from user_account where machine_mark = '" + std::string(pregister->machine_mark_) + "'";
	if (q.get_result(sql) && q.fetch_row()){
		if (q.getval() > 100){
			return error_cannt_regist_more;
		}
	}
	q.free_result();

	{
		boost::cmatch what;
		std::string number = pregister->acc_name_;
		if (pregister->type_ == 1){
			boost::regex expr("^1[3578][0-9]{9}$");
			bool matched = boost::regex_match(number.c_str(), what, expr);
			if (!matched){
				return error_invalid_data;
			}
		}
		else if(pregister->type_ == 2){
			boost::regex expr("^[a-zA-Z0-9]+@[a-zA-Z0-9]+\\.[a-zA-Z0-9]+$");
			bool matched = boost::regex_match(number.c_str(), what, expr);
			if (!matched){
				return error_invalid_data;
			}
		}
		else{
			boost::regex expr("^[a-zA-Z0-9]{3,}$");
			bool matched = boost::regex_match(number.c_str(), what, expr);
			if (!matched){
				return error_invalid_data;
			}
		}
	}

	//用户名或邮箱注册，对应验证码
	if (pregister->type_ == 0){
		extern int func_check_verify_code(std::string vcode, koko_socket_ptr fromsock, bool is_check);
		int ret =  func_check_verify_code(pregister->verify_code, pregister->from_sock_, false);
		if (ret != error_success){
			return ret;
		}
	} 

	char saccname[256] = {0};
	int len =	mysql_real_escape_string(&db_->grabdb()->mysql,saccname, pregister->acc_name_, strlen(pregister->acc_name_));


	sql = "call register_user(" + boost::lexical_cast<std::string>(pregister->type_) + 
		", '" + saccname + "', '" + 
		pregister->pwd_hash_ + "', '" +
		pregister->machine_mark_ + "','" +
		pregister->from_sock_->remote_ip() + "', @ret);select @ret";

	if (!q.get_result(sql))
		return error_sql_execute_error;
	//账号已存在
	if (q.fetch_row()){
		if (q.getval() != error_success){
			return error_account_exist;
		}
	}
	q.free_result();
	msg_common_reply<koko_socket_ptr> msg;
	msg.rp_cmd_ = GET_CLSID(msg_user_register);
	msg.err_ = 0;
	send_msg(pregister->from_sock_, msg);

	return error_success;
}

template<class remote_t>
int load_user_from_db(msg_user_login_t<remote_t>* plogin, player_ptr& pp, bool check_sign)
{
	int verify = verify_user_info(plogin, check_sign);
	if (verify != error_success){
		return verify;
	}
	char saccname[256] = {0};
	int len =	mysql_real_escape_string(&db_->grabdb()->mysql,saccname, plogin->acc_name_, strlen(plogin->acc_name_));
	Database& db = *db_;
	Query q(db);
	std::string sql = "select uid, pwd, nickname, iid, gold, gold_game, gold_free, vip_level,"
		" idnumber, mobile_number, mobile_verified, mail, mail_verified, level, addr_province, addr_city, addr_region,"
		" gender, byear, bmonth, bday, address, age,"
		" headpic from user_account where uname = '" + std::string(saccname) + "'"
		" or iid = '" + std::string(saccname) + "'"
		" or (mobile_number = '" + std::string(saccname) + "' and mobile_verified = 1)"
		" or (mail = '" + std::string(saccname) + "' and mail_verified = 1)";
	if (!q.get_result(sql))
		return error_sql_execute_error;

	if (!q.fetch_row()){
		return error_no_record;
	}

	std::string uid = q.getstr();
	std::string	pwd = q.getstr();
	std::string nickname = q.getstr();
	__int64 iid = q.getbigint();
	__int64 gold = q.getbigint();
	__int64 gold_game = q.getbigint();
	__int64 gold_free = q.getbigint();
	int			vip_lv = q.getval();
	std::string idnumber = q.getstr();
	std::string mobile = q.getstr();
	int mobile_verified = q.getval();
	std::string mail = q.getstr();
	int mail_verified = q.getval();
	int lv = q.getval();
	int r1 = q.getval();
	int r2 = q.getval();
	int r3 = q.getval();
	int gender = q.getval();
	int	byear = q.getval();
	int bmonth = q.getval();
	int bday = q.getval();
	std::string addr = q.getstr();
	int age = q.getval();

	std::vector<char> headpic;
	q.getblob("headpic", headpic);

	if (md5_hash(pwd) != plogin->pwd_hash_){
		return error_wrong_password;
	}

	if (nickname == ""){
		nickname = "newplayer";
	}

	q.free_result();

	plogin->from_sock_->set_authorized();
	__int64 seq = 0;	
	::time(&seq);

	{
		BEGIN_UPDATE_TABLE("user_account");
		SET_FIELD_VALUE("machine_mark", plogin->machine_mark_);
		SET_FINAL_VALUE("is_online", 1);
		WITH_END_CONDITION("where uid = '" + uid + "'");
		EXECUTE_IMMEDIATE();
	}
	{
		BEGIN_INSERT_TABLE("log_user_loginout");
		SET_FIELD_VALUE("uid", uid);
		SET_FINAL_VALUE("islogin", 1);
		EXECUTE_IMMEDIATE();
	}

	std::string token = md5_hash(uid + boost::lexical_cast<std::string>(seq) + the_config.token_generate_key_);

	pp->uid_ = uid;
	pp->connect_lost_tick_ = boost::posix_time::microsec_clock::local_time();
	pp->is_connection_lost_ = 0;
	pp->iid_ = iid;
	pp->nickname_ = nickname;
	pp->head_ico_ = headpic;
	pp->gold_ = gold;
	pp->vip_level_ = vip_lv;
	pp->token_ = token;
	pp->seq_ = seq;
	pp->idnumber_ = idnumber;
	pp->age_ = age;
	pp->mobile_ = mobile;
	pp->email_ = mail;
	pp->region1_ = r1;
	pp->region2_ = r2;
	pp->region3_ = r3;
	pp->gold_game_ = gold_game;
	pp->mobile_v_ = mobile_verified;
	pp->email_v_ = mail_verified;
	pp->byear_ = byear;
	pp->bmonth_ = bmonth;
	pp->bday_ = bday;
	pp->address_ = addr;
	pp->gender_ = gender;
	pp->gold_free_ = gold_free;
	return error_success;
}

void	send_all_match_to_player(player_ptr pp)
{
	std::string sql = "SELECT `match_type` ,`id`,`game_id`,`match_name`,`thumbnail`,`help_url`,"
			"`prize_desc`,`require_count`,`start_type`,`start_stipe`,`day_or_time`,`h`,`m`,"
			"`s`,`need_players_per_game`,`eliminate_phase1_basic_score`,`eliminate_phase1_inc`,"
			"`eliminate_phase2_count_`,`end_type`,`end_data`,`prizes`,`cost`, `srvip`, `srvport`"
			" FROM `setting_match_list`";

	Query q(*db_);
	if (q.get_result(sql)){
		while (q.fetch_row()){
			int tp = q.getval();
			boost::shared_ptr<match_info> mp(new match_info());
			mp->match_type_ = tp;
			mp->id_ = q.getval();
			mp->game_id_ = q.getval();
			mp->match_name_ = q.getstr();
			mp->thumbnail_ = q.getstr();
			mp->help_url_ = q.getstr();
			mp->prize_desc_ = q.getstr();
			mp->require_count_ = q.getval();
			mp->start_type_ = q.getval();
			mp->start_stripe_ = q.getval();
			mp->wait_register_ = q.getval();
			mp->h = q.getval();
			mp->m = q.getval();
			mp->s = q.getval();
			mp->need_players_per_game_ = q.getval();
			mp->eliminate_phase1_basic_score_ = q.getval();
			mp->eliminate_phase1_inc = q.getval();
			mp->elininate_phase2_count_ = q.getval();
			mp->end_type_ = q.getval();
			mp->end_data_ = q.getval();
			std::string vp = q.getstr();
			std::string vc = q.getstr();
			mp->srvip_ = q.getstr();
			mp->srvport_ = q.getval();

			std::vector<std::string> tmpv;
			std::vector<int> tmpvv;
			if(!split_str(vp.c_str(), vp.length(), ";", tmpv, true)){
				tmpv.clear();
				glb_log.write_log("match prize config error: %s matchid:%d", vc.c_str(), mp->id_);
			}

			for (int i = 0; i < (int)tmpv.size(); i++)
			{
				tmpvv.clear();
				split_str(tmpv[i].c_str(), tmpv[i].length(), ",", tmpvv, true);
				std::vector<std::pair<int, int>> vv;
				for (int ii = 0; ii < (int)tmpvv.size() - 1; ii += 2)
				{
					vv.push_back(std::make_pair(tmpvv[ii], tmpvv[ii + 1]));
				}
				mp->prizes_.push_back(vv);
			}
			tmpvv.clear();

			if(!split_str(vc.c_str(), vc.length(), ",", tmpvv, true)){
				glb_log.write_log("match cost config error: %s matchid:%d", vc.c_str(), mp->id_);
				tmpvv.clear();
			}
			while (tmpvv.size() >= 2)
			{
				mp->cost_.insert(std::make_pair(tmpvv[0], tmpvv[1]));
				tmpvv.erase(tmpvv.begin());
				tmpvv.erase(tmpvv.begin());
			}

			msg_match_data msg;
			msg.inf = *mp;
			send_msg(pp->from_socket_.lock(), msg);
		}
	}
}

int		handle_user_login(msg_user_login* plogin)
{
	msg_srv_progress msg;
	msg.pro_type_ = 0;
	msg.step_ = 1;
	send_msg(plogin->from_sock_, msg);

	player_ptr pp(new koko_player());
	int ret = load_user_from_db(plogin, pp, true);
	if (ret != error_success){
		return ret;
	}
	pp->from_socket_ = plogin->from_sock_;
	plogin->from_sock_->the_client_ = pp;

	msg.pro_type_ = 0;
	msg.step_ = 2;
	send_msg(plogin->from_sock_, msg);

	Database& db = *db_;
	Query q(db);

	msg.pro_type_ = 0;
	msg.step_ = 3;
	send_msg(plogin->from_sock_, msg);

	send_all_match_to_player(pp);

	msg.pro_type_ = 0;
	msg.step_ = 4;
	send_msg(plogin->from_sock_, msg);

	pending_login_users_.push_back(pp);

	return error_success;
}

std::string pickup_value(std::string params, std::string field)
{
	std::string pat = std::string(field + "=");
	char* vstart = strstr((char*)params.c_str(), pat.c_str());
	if (!vstart) return "";
	char* vend = strstr(vstart + pat.length(), "&");
	if (!vend){
		return vstart + pat.length();
	}
	else {
		std::string ret;
		ret.insert(ret.begin(), vstart + pat.length(), vend);
		return ret;
	}
}

int			db_thread_func(db_delay_helper<std::string, int>* db_delay)
{
	unsigned int idle = 0;
	while(!glb_exit)
	{
		bool busy = false;
		if(!db_delay->pop_and_execute(nullptr))
			idle++;	
		else {idle = 0;}

		msg_ptr pmsg;
		if(sub_thread_process_msg_lst_.pop_front(pmsg)){
			int ret = error_success;
			if (pmsg->head_.cmd_ == GET_CLSID(msg_user_login) ||
				pmsg->head_.cmd_ == GET_CLSID(msg_user_login_channel)){
				ret = handle_user_login((msg_user_login*)pmsg.get());
				if (ret != error_success){
					pmsg->from_sock_->is_login_ = false;
				}
			}
			else if (pmsg->head_.cmd_ == GET_CLSID(msg_user_register)){
				ret = handle_user_register((msg_user_register*)pmsg.get());
				pmsg->from_sock_->is_register_ = false;
			}
			else if (pmsg->head_.cmd_ == GET_CLSID(msg_get_verify_code)){
				ret = handle_get_verify_code((msg_get_verify_code*) pmsg.get());
			}
			if (ret != error_success){
				msg_common_reply<nosocket> msg;
				msg.rp_cmd_ = pmsg->head_.cmd_;
				msg.err_ = ret;
				send_msg(pmsg->from_sock_, msg);
			}
			busy = true;
		}

		if (!busy){
			boost::posix_time::milliseconds ms(20);
			boost::this_thread::sleep(ms);
		}
	}
	return 0;
};

int	handle_pending_logins()
{
	while (pending_login_users_.size() > 0)
	{
		player_ptr pp;
		if(!pending_login_users_.pop_front(pp))
			break;

		auto itp = online_players.find(pp->uid_);
		if (itp == online_players.end() ){
			online_players.insert(std::make_pair(pp->uid_, pp));
		}
		//如果用户已在线
		else{
			msg_same_account_login msg;
			auto conn = itp->second->from_socket_.lock();
			if (conn == pp->from_socket_.lock()){
				return error_success;
			}
			if(conn.get())
				send_msg(conn, msg, true);
			replace_map_v(online_players, std::make_pair(pp->uid_, pp));
		}

		auto pconn = pp->from_socket_.lock();
		if (pconn.get()){
			pconn->is_login_ = false;
			send_all_game_to_player(pconn);

			msg_user_login_ret msg;
			response_user_login(pp, pconn, msg);

			msg_user_image msg_headpic;
			COPY_STR(msg_headpic.uid_, pp->uid_.c_str());
			send_image(msg_headpic, pp->head_ico_, pconn);
		}
	}
	return 0;
}
//从全局消息队列中取出消息并处理完队列里面所有的消息
int handle_player_msgs()
{
	auto it_cache = glb_msg_list_.begin();
	while (it_cache != glb_msg_list_.end())
	{
		auto cmd = *it_cache;	it_cache++;
		auto pauth_msg = boost::dynamic_pointer_cast<msg_authrized<koko_socket_ptr>>(cmd);
		//如果需要授权的消息，没有得到授权，则退出
		if (pauth_msg.get() && 
			!pauth_msg->from_sock_->is_authorized()){
			pauth_msg->from_sock_->close();
		}
		else{
			int ret = cmd->handle_this();
			if (ret != 0){
				msg_common_reply<koko_socket_ptr> msg;
				msg.err_ = ret;
				msg.rp_cmd_ = cmd->head_.cmd_;
				send_msg(cmd->from_sock_, msg);
			}
		}
		msg_handled++;
	}
	glb_msg_list_.clear();
	return 0;
}

int		start_network()
{
	the_net->add_acceptor(the_config.client_port_);

	if(the_net->run()){
		glb_log.write_log("net service start success!");
	}
	else{
		glb_log.write_log("!!!!!net service start failed!!!!");
		return -1;
	}
	return 0;
}

void		broadcast_msg(smsg_ptr msg)
{
	auto itp = online_players.begin();
	while (itp != online_players.end()){
		koko_socket_ptr cnn = itp->second->from_socket_.lock();	itp++;
		if (cnn.get()){
			send_msg(cnn, *msg);
		}
	}
}

void	update_players()
{
	auto it = online_players.begin();
	while (it != online_players.end())
	{
		player_ptr pp = it->second;
		if (pp->is_connection_lost_ == 1 && 
			(boost::posix_time::microsec_clock::local_time() - pp->connect_lost_tick_).total_seconds() > 5){
				pp->connection_lost();
				it = online_players.erase(it);
		}
		else{
			it++;
		}
	}
}

int		load_all_games()
{
	all_games.clear();
	std::string sql = "SELECT `id`,`tech_type`,\
		`dir`,	`exe`,	`update_url`,	`help_url`,\
		`game_name`,	`thumbnail`,	`solution`, `no_embed`, `catalog` \
		FROM `setting_game_list`";
	Query q(*db_);
	if (!q.get_result(sql))
		return error_sql_execute_error;

	while (q.fetch_row())
	{
		gamei_ptr pgame(new game_info);
		pgame->id_ = q.getval();
		pgame->type_ = q.getval();
		COPY_STR(pgame->dir_, q.getstr());
		COPY_STR(pgame->exe_, q.getstr());
		COPY_STR(pgame->update_url_, q.getstr());
		COPY_STR(pgame->help_url_, q.getstr());
		COPY_STR(pgame->game_name_, q.getstr());
		COPY_STR(pgame->thumbnail_, q.getstr());
		COPY_STR(pgame->solution_, q.getstr());
		pgame->no_embed_ = q.getval();
		COPY_STR(pgame->catalog_, q.getstr());
		all_games.insert(std::make_pair(pgame->id_, pgame));
	}
	q.free_result();

	sql = "SELECT `gameid`,	`roomid`,	`room_name`,\
		`room_desc`,	`thumbnail`,	`srvip`,		`srvport`, `require` \
		FROM `setting_game_room_info`";

	if (!q.get_result(sql))
		return error_sql_execute_error;

	while (q.fetch_row())
	{
		game_room_inf inf;
		inf.game_id_ = q.getval();
		inf.room_id_ = q.getval();
		COPY_STR(inf.room_name_, q.getstr());
		COPY_STR(inf.room_desc_, q.getstr());
		COPY_STR(inf.thumbnail_, q.getstr());
		COPY_STR(inf.ip_, q.getstr());
		inf.port_ = q.getval();
		COPY_STR(inf.require_, q.getstr());
		if (all_games.find(inf.game_id_) != all_games.end()){
			all_games[inf.game_id_]->rooms_.push_back(inf);
		}
	}
	q.free_result();

	return error_success;
}

void	clean()
{
	task.reset();

	for (unsigned int i = 0 ; i < cache_persistent_threads.size(); i++)
	{
		cache_persistent_threads[i]->join();
	}

	//继续执行没保存完的sql语句

	db_delay_helper_[0].pop_and_execute();

	timer_srv->stop();
	timer_srv.reset();

	the_net->stop();
	the_net.reset();
	glb_log.stop_log();
}

int run()
{
	timer_srv.reset(new boost::asio::io_service());
	the_net.reset(new net_server<koko_socket>(2));

	boost::system::error_code ec;
	the_config.load_from_file();

	db_.reset(new utf8_data_base(the_config.accdb_host_, the_config.accdb_user_,the_config.accdb_pwd_, the_config.accdb_name_));
	if(!db_->grabdb()){
		glb_log.write_log("database start failed!!!!");
		return -1;
	}

	db_delay_helper_[0].set_db_ptr(db_);

	if (start_network() != 0){
		goto _EXIT;
	}

	{
		boost::shared_ptr<boost::thread> td(new boost::thread(boost::bind(db_thread_func, db_delay_helper_)));
		cache_persistent_threads.push_back(td);
	}

	int idle_count = 0;

	task_on_5sec* ptask = new task_on_5sec(*timer_srv);
	task.reset(ptask);
	ptask->schedule(1000);
	ptask->routine();
	while(!glb_exit)
	{
		bool busy = false;
		
		timer_srv->reset();
		timer_srv->poll();
		
		the_net->ios_.reset();
		the_net->ios_.poll();

		handle_pending_logins();

		pickup_player_msgs(busy);
		handle_player_msgs();

		update_players();

		smsg_ptr pmsg;
		broadcast_msg_.pop_front(pmsg);
		if(pmsg.get())
			broadcast_msg(pmsg);

		boost::posix_time::milliseconds ms(20);
		boost::this_thread::sleep(ms);
	}

_EXIT:
	return 0;
}

static int dmp = 0;
LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
	HANDLE lhDumpFile = CreateFileA(("crash" + boost::lexical_cast<std::string>(dmp) + ".dmp").c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL ,NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

	CloseHandle(lhDumpFile);
	return EXCEPTION_EXECUTE_HANDLER;
}

int main()
{
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
	{
		return -1;
	}

	__try{
		if (run() == 0){
			cout <<"exited!"<< endl;
		}
		else
			cout <<"exited with error!"<< endl;
 	}
	__except(MyUnhandledExceptionFilter(GetExceptionInformation()),
		EXCEPTION_EXECUTE_HANDLER){

 	}
	clean();
	return 0;
}

int task_on_5sec::routine()
{
	glb_log.write_log("game is running ok, this time msg recved:%d, handled:%d, remain sending data: %d", msg_recved, msg_handled, unsended_data);
	msg_handled = 0;
	msg_recved = 0;
	unsended_data = 0;

	load_all_games();
	
	//账号服务器，加载所有在线服务器列表，做动态平衡,
	//10秒内更新过的服务器才算
	std::string sql = "select public_ip, port, name, online_user, gameid from system_server_auto_balance where unix_timestamp(update_time) > (unix_timestamp() - 10)";
	Query q(*db_);
	if(q.get_result(sql)){
		channel_servers.clear();
		while (q.fetch_row())
		{
			channel_server srv;
			srv.ip_ = q.getstr();
			srv.port_ = q.getval();
			srv.online_ = q.getval();
			std::string ids = q.getstr();
			split_str(ids.c_str(), ids.length(), ",", srv.game_ids_, true);
			if (srv.game_ids_.empty()){
				srv.game_ids_.push_back(-1);
			}
			channel_servers.push_back(srv);
		}
	}

	{
		std::string sql = "select rowid, uid, itemid from user_item_changes order by rowid asc";
		Query q(*db_);
		if(q.get_result(sql) && q.fetch_row()){
			std::vector<__int64> v;
			do{
				__int64 rowid = q.getbigint();
				std::string uid = q.getstr();
				//如果用户存在,则更新
				if (get_player(uid).get()){
					int	itemid = q.getval();
					sync_user_item(uid, itemid);
					v.push_back(rowid);
				}
				if (v.size() > 100)	{
					Query q2(*db_);
					std::string rowids = combin_str(v, ",", false);
					sql = "delete from user_item_changes where rowid in(" + rowids + ")";
					q2.execute(sql);
					v.clear();
				}
			}
			while (q.fetch_row());

			if (v.size() > 0)	{
				Query q2(*db_);
				std::string rowids = combin_str(v, ",", false);
				sql = "delete from user_item_changes where rowid in(" + rowids + ")";
				q2.execute(sql);
			}
		}
	}

	{
		std::string sql = "select rowid, uid, matchid, matchins, register_count, ip, port from setting_match_notify order by rowid asc";
		Query q(*db_);
		if(q.get_result(sql) && q.fetch_row()){
			std::vector<__int64> v;
			do{
				__int64 rowid = q.getbigint();
				std::string uid = q.getstr();
				//如果用户存在,则通知比赛开始.
				player_ptr pp = get_player(uid);

				if (pp.get()){
					msg_confirm_join_game_deliver msg;
					msg.match_id_ = q.getval();
					COPY_STR(msg.ins_id_, q.getstr());
					msg.register_count_ = q.getval();
					COPY_STR(msg.ip_, q.getstr());
					msg.port_ = q.getval();

					auto pcnn = pp->from_socket_.lock();
					if (pcnn.get()){
						send_msg(pp->from_socket_.lock(), msg);
					}
					v.push_back(rowid);
				}
				if (v.size() > 100)	{
					Query q2(*db_);
					std::string rowids = combin_str(v, ",", false);
					sql = "delete from setting_match_notify where rowid in(" + rowids + ")";
					q2.execute(sql);
					v.clear();
				}
			}
			while (q.fetch_row());

			if (v.size() > 0)	{
				Query q2(*db_);
				std::string rowids = combin_str(v, ",", false);
				sql = "delete from setting_match_notify where rowid in(" + rowids + ")";
				q2.execute(sql);
			}
		}

	}
	return task_info::routine_ret_continue;
}
