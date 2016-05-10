#include "msg_client.h"
#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "utility.h"
#include "error_define.h"
#include "error.h"
#include <map>
#include "game_logic.h"

extern longlong	 g_cash_pool;

void sql_trim(std::string& str)
{
	for (unsigned int i = 0; i < str.size(); i++)
	{
		if (str[i] == '\''){
			str[i] = ' ';
		}
	}
}

static int		user_login(remote_socket_ptr from_sock_, 
	std::string uid_,
	std::string uname_,
	std::string headpic)
{
	from_sock_->set_authorized();
	sql_trim(uname_);
	sql_trim(headpic);

	bool break_join = false;
	//有玩家在
	player_ptr pp = the_service.get_player(uid_);
	if(pp.get()){
		remote_socket_ptr pold = pp->from_socket_.lock();
		if (pold.get()){
			//如果是另一个socket发过来的，则断开旧连接
			if (pold != from_sock_){
				msg_logout msg;
				send_msg(pold, msg, true);

				glb_log.write_log("same account login, the old force offline!");

				pp->from_socket_ = from_sock_;
				from_sock_->the_client_ = pp;
			}
			//如果是重复发,则啥事不干
			else{
				break_join = true;
			}
		}
		else{
			pp->from_socket_ = from_sock_;
			from_sock_->the_client_ = pp;
			pp->name_ = uname_;
			pp->head_ico_ = headpic;
		}
	}
	else{
		pp.reset(new fish_player());
		pp->uid_ = uid_;
		pp->name_ = uname_;
		pp->from_socket_ = from_sock_;
		pp->head_ico_ = headpic;
		from_sock_->the_client_ = pp;

		the_service.players_.insert(make_pair(pp->uid_, pp));
	}

	if (!pp.get()){
		return SYS_ERR_INSUFFICIENT_MEMORY;
	}
	pp->is_connection_lost_ = false;
	longlong iid = the_service.get_var(pp->uid_ + KEY_USER_IID);
	if (iid == 0){
		if(the_service.account_cache_.send_cache_cmd<longlong>(pp->uid_, "CMD_BIND", 0, iid, true) != ERROR_SUCCESS_0){
			glb_log.write_log("bind user id failed!, user = %s", pp->uid_.c_str());
		}
	}
	pp->iid_ = iid;
	for (unsigned int i = 0; i < the_service.scenes_.size(); i++)
	{
		scene_set& sc = the_service.scenes_[i];
		msg_server_parameter msg;
		msg.is_free_ = sc.is_free_;
		msg.min_guarantee_ = (int)sc.gurantee_set_;
		msg.banker_set_ = (int)sc.to_banker_set_;
		msg.low_cap_ = (int)(sc.gurantee_set_ - sc.player_tax_) / 50;
		msg.player_tax_ = sc.player_tax_;
		if (i == 0){
			msg.enter_scene_ = 2;
		}
		else if (i == the_service.scenes_.size() - 1){
			msg.enter_scene_ = 3;
		}
		else{
			msg.enter_scene_ = 0;
		}
		send_msg(from_sock_, msg);
	}

	extern int get_level(longlong exp);
	msg_user_login_ret msg_ret;
	msg_ret.iid_ = (int) iid;
	COPY_STR(msg_ret.uid_, pp->uid_.c_str());
	COPY_STR(msg_ret.head_pic_, pp->head_ico_.c_str());
	longlong exp = the_service.get_var(pp->uid_ + KEY_ONLINE_TIME, IPHONE_NIUINIU);
	int lv = get_level(exp);
	msg_ret.lv_ = lv;
	msg_ret.exp_ = (double)exp;
	msg_ret.currency_ = (double)the_service.get_currency(pp->uid_);
	COPY_STR(msg_ret.uname_, uname_.c_str());

	pp->send_msg_to_client(msg_ret);

	if (!break_join){
		Database& db(*the_service.gamedb_);
		BEGIN_REPLACE_TABLE("log_online_players");
		SET_STRFIELD_VALUE("uid", pp->uid_);
		SET_STRFINAL_VALUE("name", pp->name_);
		the_service.delay_db_game_.push_sql_exe(pp->uid_ + "login", -1, str_field, true);
		the_service.player_login(pp);
	}
	the_service.account_cache_.send_cache_cmd(pp->uid_, UID_LOGIN, 0, exp, false);
	return ERROR_SUCCESS_0;
}

int msg_url_user_login_req::handle_this()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("msg_url_user_login_req");
	std::string sign_pattn, s;
	s.assign(uid_, max_guid);
	sign_pattn +=s.c_str();
	sign_pattn +="|";

	s.assign(uname_, max_name);
	sign_pattn += s.c_str();
	sign_pattn +="|";

	sign_pattn += lex_cast_to_str(vip_lv_);
	sign_pattn +="|";

	s.assign(head_icon_, max_guid);
	sign_pattn += s.c_str();
	sign_pattn +="|";

	sign_pattn += lex_cast_to_str(uidx_);
	sign_pattn +="|";

	s.assign(session_, max_guid);
	sign_pattn += s.c_str();
	sign_pattn +="|";

	sign_pattn += lex_cast_to_str(sex_);
	sign_pattn +="|";

	sign_pattn += the_service.the_config_.php_sign_key_;

	std::string remote_sign = url_sign_;
	unsigned char out_put[16];
	CMD5 md5;
	md5.MessageDigest((const unsigned char*)sign_pattn.c_str(), sign_pattn.length(), out_put);
	std::string sign_key;
	for (int i = 0; i < 16; i++)
	{
		char aa[4] = {0};
		sprintf_s(aa, 4, "%02x", out_put[i]);
		sign_key += aa;
	}

	if(sign_key != remote_sign){
		glb_log.write_log("wrong sign, local: %s remote:%s", sign_pattn.c_str(), remote_sign.c_str());
		return SYS_ERR_SESSION_TIMEOUT;
	}
	
	if (the_service.players_.size() > 2001){
		return SYS_ERR_SERVER_FULL;
	}

	if (the_service.the_config_.shut_down_){
		return SYS_ERR_SESSION_TIMEOUT;
	}

	if (strstr(uid_, "ipbot_test") == uid_){
		return SYS_ERR_SESSION_TIMEOUT;
	}
	std::string uname = the_service.get_var_str(std::string(uid_) + KEY_USER_UNAME, -1);
	std::string headpic = the_service.get_var_str(std::string(uid_) + KEY_USER_HEADPIC, IPHONE_NIUINIU);
	
	if (uname == ""){
		the_service.set_var(std::string(uid_) + KEY_USER_UNAME, -1, uname_);
		uname = uname_;
	}
	if (headpic == ""){
		the_service.set_var(std::string(uid_) + KEY_USER_HEADPIC, IPHONE_NIUINIU, head_icon_);
		headpic = head_icon_;
	}
	user_login(from_sock_, uid_, uname, headpic);

DEBUG_COUNT_PERFORMANCE_END()
return ERROR_SUCCESS_0;
}


int msg_game_lst_req::handle_this()
{
	if (from_sock_.get()){
		msg_room_info msg;
		if (page_set_ > 0){
			for (unsigned int i = page_ * page_set_; i < the_service.the_games_.size() && i < (page_ + 1) * page_set_; i++)
			{
				msg.room_id_ = the_service.the_games_[i]->id_;
				msg.free_pos_ = MAX_SEATS - the_service.the_games_[i]->get_playing_count();
				send_msg(from_sock_, msg);
			}
		}
	}
	return ERROR_SUCCESS_0;
}

void quick_join_game(player_ptr pp, logic_ptr old_game, scene_set* pset = NULL)
{
	vector<logic_ptr> free_games;
	//如果是换桌，需要找出和原来一样场次配置的游戏
	if (old_game.get()){
		for (unsigned int i = 0; i < the_service.the_games_.size(); i++)
		{
			if (the_service.the_games_[i]->get_playing_count() < MAX_SEATS && the_service.the_games_[i] != old_game &&
				 the_service.the_games_[i]->scene_set_->id_ == old_game->scene_set_->id_ &&
				 !the_service.the_games_[i]->is_ingame(pp))	{
					free_games.push_back(the_service.the_games_[i]);
			}
		}
	}
	//如果是新进入
	else{
		for (unsigned int i = 0; i < the_service.the_games_.size(); i++)
		{
			//换桌要换到相同场次的桌
			if (the_service.the_games_[i]->get_playing_count() < MAX_SEATS && 
				the_service.the_games_[i]->scene_set_->id_ == pset->id_ && 
				!the_service.the_games_[i]->is_ingame(pp))	{
					free_games.push_back(the_service.the_games_[i]);
			}
		}
	}

	//如果没有游戏可以进，则新建一个游戏
	if (free_games.empty() || the_service.the_games_.empty()){
		logic_ptr pl(new fishing_logic);
		the_service.the_games_.push_back(pl);
		//如果是换桌，则复制场次配置
		if (old_game.get()){
			pl->scene_set_ = old_game->scene_set_;
		}
		else {
			pl->scene_set_ = pset;
		}
		pl->start_logic();
		pl->player_login(pp);
	}
	//随机进入一个有空位的游戏
	else{
		logic_ptr pl = free_games[rand_r(free_games.size() - 1)];
		pl->player_login(pp);
	}
};

int msg_enter_game_req::handle_this()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("msg_enter_game_req")
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get())
		return SYS_ERR_CANT_FIND_CHARACTER;

	//如果上次已经在游戏中，则替换掉上次的
	logic_ptr pingame = pp->the_game_.lock();
	if (pingame.get()){
		//如果是换桌
		if (room_id_ == 0xFFFFFFFF){
			pingame->leave_game(pp, pp->pos_, 1);
			quick_join_game(pp, pingame);
		}
		else {
			pingame->is_playing_[pp->pos_].reset();
			pingame->player_login(pp, pp->pos_);
		}
	}
	else{
		if (room_id_ < 0 || room_id_ >= the_service.scenes_.size())
			return SYS_ERR_CANT_FIND_CHARACTER;

		longlong c = the_service.get_currency(pp->uid_);
		//钱不够，不让进
		if (c < the_service.scenes_[room_id_].gurantee_set_){
			msg_low_currency msg;
			pp->send_msg_to_client(msg);
			return ERROR_SUCCESS_0;
		}
		quick_join_game(pp, logic_ptr(), &the_service.scenes_[room_id_]);
	}
DEBUG_COUNT_PERFORMANCE_END()
	return ERROR_SUCCESS_0;
}


int glb_lvset[50] = {
	30	,
	50	,
	70	,
	90	,
	110	,
	150	,
	220	,
	330	,
	480	,
	600	,
	740	,
	875	,
	1010,
	1200,
	1500,
	1800,
	2100,
	2400,
	2700,
	3000,
	3500,
	4200,
	5000,
	6000,
	6750,
	7580,
	8410,
	9240,
	12000,
	14500,
	17800,
	21000,
	23900,
	27150,
	30290,
	33430,
	36570,
	39710,
	42850,
	45990,
	49130,
	52270,
	55410,
	58550,
	61690,
	64830,
	67970,
	71110,
	74250,
	77390	
};

int glb_giftset[50] = {
	100	 ,
	150	 ,
	200	 ,
	250	 ,
	300	 ,
	350	 ,
	400	 ,
	450	 ,
	500	 ,
	550	 ,
	1000	,
	1100	,
	1200	,
	1300	,
	1400	,
	1500	,
	1600	,
	1700	,
	1800	,
	1900	,
	2000	,
	2200	,
	2400	,
	2600	,
	2800	,
	3000	,
	3200	,
	3400	,
	3600	,
	3800	,
	4000	,
	4200,
	4400,
	4600,
	4800,
	5000,
	5200,
	5400,
	5600,
	5800,
	6000,
	6200,
	6400,
	6600,
	6800,
	7000,
	7200,
	7400,
	7600,
	8000
};

int get_level(longlong exp)
{
	int i = 0;
	for (; i < 50; i++)
	{
		if (exp < glb_lvset[i]){
			return i + 1;
		}
		else{
			exp -= glb_lvset[i];
		}
	}
	return i + 1;
}

int glb_everyday_set[] = {
	5000,
	6000,
	7000,
	8000,
	9000,
	10000,
	11000
};

int msg_get_prize_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr pgame = pp->the_game_.lock();
	//领取等级奖励
	if (type_ == 0){
		longlong exp = the_service.get_var(pp->uid_ + KEY_ONLINE_TIME, IPHONE_NIUINIU);
		int lv = get_level(exp);
		//读取已领取级别
		longlong lv_get = the_service.get_var(pp->uid_ + KEY_LEVEL_GIFT, IPHONE_NIUINIU);
		//查看
		if (data_ == 0){
			msg_everyday_gift msg;
			msg.type_ = type_;
			msg.conitnue_days_ = (int)lv_get; 
			if (lv_get < lv && lv_get < 50){
				msg.getted_ = 1;
			}
			else{
				msg.getted_ = 0;
			}
			pp->send_msg_to_client(msg);
		}
		else{
			if (lv_get < lv && lv_get < 50){
				longlong out_count = 0;
				bool sync = !pgame.get();
				int r = the_service.give_currency(pp->uid_, glb_giftset[lv_get], out_count, sync);
				if (r == ERROR_SUCCESS_0){
					the_service.set_var(pp->uid_ + KEY_LEVEL_GIFT, IPHONE_NIUINIU, lv_get + 1);
					msg_currency_change msg;   
					msg.credits_ = glb_giftset[lv_get];
					msg.why_ = msg_currency_change::why_levelup;
					pp->send_msg_to_client(msg);
				}
				the_service.add_var(pp->uid_ + KEY_CUR_TRADE_CAP, -1, glb_giftset[lv_get]);
			}
		}
	}
	//领取每日登录奖励
	else if(type_ == 1) {
		longlong last_get_tick = the_service.get_var(pp->uid_ + KEY_EVERYDAY_LOGIN_TICK, IPHONE_NIUINIU); //上次领取时间
		date dat1 = date_time::day_clock<date>::local_day();
		date dat2 = from_string("1900-1-1");
		int this_day_tick = (dat1 - dat2).days();

		if (data_ == 1){
			//第一次登录，还不存在连续登录数据
			if (last_get_tick == 0){
				the_service.set_var(pp->uid_ + KEY_EVERYDAY_LOGIN_TICK, IPHONE_NIUINIU, this_day_tick);
				the_service.set_var(pp->uid_ + KEY_EVERYDAY_LOGIN, IPHONE_NIUINIU, 1);
			}
			else{
				//如果是第二天登录
				if ((last_get_tick + 1) == this_day_tick){
					the_service.set_var(pp->uid_ + KEY_EVERYDAY_LOGIN_TICK, IPHONE_NIUINIU, this_day_tick);
					the_service.add_var(pp->uid_ + KEY_EVERYDAY_LOGIN, IPHONE_NIUINIU, 1);
				}
				//连续登录断掉了
				else if(last_get_tick != this_day_tick){
					the_service.set_var(pp->uid_ + KEY_EVERYDAY_LOGIN_TICK, IPHONE_NIUINIU, this_day_tick);
					the_service.set_var(pp->uid_ + KEY_EVERYDAY_LOGIN, IPHONE_NIUINIU, 1);
				}
				//如果最后领取时间是今天，则返回
				else {
					return ERROR_SUCCESS_0;
				}
			}

			longlong continue_days = the_service.get_var(pp->uid_ + KEY_EVERYDAY_LOGIN, IPHONE_NIUINIU);	//连续签到多少天了
			if (continue_days > 7){
				continue_days = 7;
			}

			longlong out_count = 0;
			bool sync = !pgame.get();
			int r = the_service.give_currency(pp->uid_, glb_everyday_set[continue_days - 1], out_count, sync);
			if (r == ERROR_SUCCESS_0){
				msg_currency_change msg;
				msg.credits_ = glb_everyday_set[continue_days - 1];
				msg.why_ = msg_currency_change::why_everyday_login;
				pp->send_msg_to_client(msg);
			}
			the_service.add_var(pp->uid_ + KEY_CUR_TRADE_CAP, -1, glb_everyday_set[continue_days - 1]);
		}
		else{
			msg_everyday_gift msg;
			msg.type_ = type_;
			msg.conitnue_days_ = (int)the_service.get_var(pp->uid_ + KEY_EVERYDAY_LOGIN, IPHONE_NIUINIU);	//连续签到多少天了
			if (last_get_tick == 0){
				msg.conitnue_days_ = 1;
			}
			else{
				//如果是第二天登录
				if ((last_get_tick + 1) == this_day_tick){
					msg.conitnue_days_++;
				}
				//连续登录断掉了
				else if(last_get_tick != this_day_tick){
					msg.conitnue_days_ = 1;
				}
				//如果最后领取时间是今天，则返回
				else {
					msg.getted_ = 1;
				}
			}
			pp->send_msg_to_client(msg);
		}
	}
	return ERROR_SUCCESS_0;
}

int msg_leave_room::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;

	player_ptr new_pp = pp->clone();
	new_pp->on_connection_lost();
	new_pp->from_socket_.reset();
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (pgame->is_playing_[i] == pp){
			pgame->is_playing_[i] = new_pp;
			break;
		}
	}

	pp->the_game_.reset();
	pp->reset_temp_data();
	pp->pos_ = -1;
	pp->credits_ = 0;

	msg_player_leave msg_leave;
	msg_leave.pos_ = new_pp->pos_;
	msg_leave.why_ = 0;
	pp->send_msg_to_client(msg_leave);
	return ERROR_SUCCESS_0;
}

int msg_get_wealth_rank_req::handle_this()
{
	for (unsigned int i = 0 ; i < the_service.glb_goldrank_.size(); i++)
	{
		boost::shared_ptr<msg_cache_cmd_ret> msg = the_service.glb_goldrank_[i];
		send_msg(from_sock_, *msg);
	}
	return ERROR_SUCCESS_0;
}

int msg_set_account::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	if ((longlong)relate_iid_ > 0){
		std::string uid;
		int r = the_service.account_cache_.send_cache_cmd<longlong>("", IID_UID, (longlong)relate_iid_, uid, true );
		if (r != ERROR_SUCCESS_0 || ((longlong)relate_iid_ == pp->iid_))	{
			return GAME_ERR_WRONG_IID;
		}
	}

	longlong out = 0;
	int r = the_service.account_cache_.send_cache_cmd(pp->uid_, ACC_REG, acc_name_, out, true);
	if (r == ERROR_SUCCESS_0){
		std::string out_s;
		r = the_service.account_cache_.send_cache_cmd<std::string>(pp->uid_ + KEY_USER_ACCPWD, CMD_SET, pwd_, out_s, true);
		msg_set_account_ret msg;
		msg.iid_ = (unsigned int)out;
		pp->send_msg_to_client(msg);

		r = the_service.set_var(pp->uid_ + KEY_RECOMMEND_REWARD, IPHONE_NIUINIU, 2);
		r = the_service.give_currency(pp->uid_, 500000, out);
		if (r == ERROR_SUCCESS_0){
			the_service.add_var(pp->uid_ + KEY_CUR_TRADE_CAP, -1, 500000);
		}
		if ((longlong)relate_iid_ > 0){
			Database& db = *the_service.gamedb_;
			BEGIN_INSERT_TABLE("recommend_info");
			SET_STRFIELD_VALUE("uid", pp->uid_);
			SET_STRFIELD_VALUE("head_pic", pp->head_ico_);
			SET_STRFIELD_VALUE("uname", pp->name_);
			SET_FINAL_VALUE("recommender_uid", (longlong)relate_iid_);
			EXECUTE_IMMEDIATE();
		}
	}
	return r;
}

int msg_trade_gold_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	static unsigned int order = 0;

	msg_common_reply<none_socket> msg_rpl;
	msg_rpl.rp_cmd_ = head_.cmd_;
	msg_rpl.err_ = 0;

	longlong out_cap = the_service.get_var(pp->uid_ + KEY_CUR_TRADE_CAP, -1);
	longlong credit = the_service.get_currency(pp->uid_);
	if (credit <= out_cap ){
		msg_rpl.err_ = -1;
	}
	else if (credit - out_cap < gold_){
		msg_rpl.err_ = -6;
	}

	if (msg_rpl.err_ == 0 && gold_ > the_service.the_config_.max_trade_cap_){
		msg_rpl.err_ = -2;
	}

	if(msg_rpl.err_ == 0){
		std::string tuid;
		the_service.account_cache_.send_cache_cmd("", IID_UID, iid_, tuid, true);
		if (tuid == ""){
			msg_rpl.err_ = -3;
		}
		else{
			longlong out_count = 0;

			{
				Database& db = *the_service.gamedb_;
				BEGIN_INSERT_TABLE("trade_record");
				SET_STRFIELD_VALUE("uid", pp->uid_);
				SET_STRFIELD_VALUE("uname", pp->name_);
				SET_FIELD_VALUE("target_iid", iid_);
				SET_FIELD_VALUE("gold", gold_);
				SET_FIELD_VALUE("state", 0);
				SET_FIELD_VALUE("iid_", pp->iid_);
				SET_FINAL_VALUE("order", ++order);
				EXECUTE_IMMEDIATE();
			}

			int r = the_service.apply_cost(pp->uid_, gold_, out_count);

			if (r == ERROR_SUCCESS_0){
				r = the_service.give_currency(tuid, (longlong)(gold_ * (1.0 - (the_service.the_config_.trade_tax_ / 10000.0))) , out_count, 0);
				if (r!= ERROR_SUCCESS_0){
					msg_rpl.err_ = -5;
					Database& db = *the_service.gamedb_;
					BEGIN_INSERT_TABLE("trade_record");
					SET_STRFIELD_VALUE("uid", pp->uid_);
					SET_STRFIELD_VALUE("uname", pp->name_);
					SET_FIELD_VALUE("target_iid", iid_);
					SET_FIELD_VALUE("gold", gold_);
					SET_FIELD_VALUE("state", 2);
					SET_FIELD_VALUE("iid_", pp->iid_);
					SET_FINAL_VALUE("order", order);
					EXECUTE_IMMEDIATE();
				}
				else{
					Database& db = *the_service.gamedb_;
					BEGIN_INSERT_TABLE("trade_record");
					SET_STRFIELD_VALUE("uid", pp->uid_);
					SET_STRFIELD_VALUE("uname", pp->name_);
					SET_FIELD_VALUE("target_iid", iid_);
					SET_FIELD_VALUE("gold", (longlong) gold_ * (1.0 - (the_service.the_config_.trade_tax_ / 10000.0)));
					SET_FIELD_VALUE("state", 1);
					SET_FIELD_VALUE("iid_", pp->iid_);
					SET_FINAL_VALUE("order", order);
					EXECUTE_IMMEDIATE();
				}
			}
			else {
				msg_rpl.err_ = -4;
			}
		}
	}
	pp->send_msg_to_client(msg_rpl);
	return ERROR_SUCCESS_0;
}

int msg_user_info_changed::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	pp->name_ =  uname_;
	sql_trim(pp->name_);
	pp->head_ico_ = head_icon_;
	the_service.set_var(pp->uid_ + KEY_USER_UNAME, -1, pp->name_);
	the_service.set_var(pp->uid_ + KEY_USER_HEADPIC, IPHONE_NIUINIU, pp->head_ico_);
	return ERROR_SUCCESS_0;
}

int msg_account_login_req::handle_this()
{
	if (the_service.players_.size() > 2001){
		return SYS_ERR_SERVER_FULL;
	}

	if (the_service.the_config_.shut_down_){
		return SYS_ERR_SESSION_TIMEOUT;
	}

	bool is_digit = true;
	for (int i = 0 ; i < max_name; i++)
	{
		if(acc_name_[i] >= '0' && acc_name_[i] <='9'){
			continue;
		}
		else if (acc_name_[i] == 0){
			break;
		}
		else{
			is_digit = false;
			break;
		}
	}

	std::string uid; 
	int r = ERROR_SUCCESS_0;
	if (!is_digit){
		r = the_service.account_cache_.send_cache_cmd(acc_name_, ACC_LOGIN, pwd_, uid, true);
	}
	else{
		longlong iid = 0;
		try	{
			iid = boost::lexical_cast<longlong>(acc_name_, strlen(acc_name_));
			r = the_service.account_cache_.send_cache_cmd(pwd_, IID_LOGIN, iid, uid, true);
			if (r != ERROR_SUCCESS_0){
				r = the_service.account_cache_.send_cache_cmd(acc_name_, ACC_LOGIN, pwd_, uid, true);
			}
		}
		catch (boost::bad_lexical_cast* e)	{
			r = SYS_ERR_SESSION_TIMEOUT;
		}
	}

	if (r == ERROR_SUCCESS_0){
		std::string uname_ = the_service.get_var_str(uid + KEY_USER_UNAME, -1);
		std::string headpic = the_service.get_var_str(uid + KEY_USER_HEADPIC, IPHONE_NIUINIU);
		user_login(from_sock_, uid, uname_, headpic);
	}
	return r;
}

int msg_change_pwd_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	std::string pwdcache = the_service.get_var_str(pp->uid_ + KEY_USER_ACCPWD);
	if (pwdcache != "" && pwdcache == pwd_old_){
		std::string ss = the_service.set_var(pp->uid_ + KEY_USER_ACCPWD, -1, pwd_);
		return ERROR_SUCCESS_REPORT;
	}
	else{
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
}

int msg_get_recommend_lst::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	
	std::string sql = "select uid, head_pic, uname, reward_getted from recommend_info where recommender_uid ='" + lex_cast_to_str(pp->iid_) + "' order by reward_getted asc";
	Query q(*the_service.gamedb_);
	if (q.get_result(sql)){
		long n = q.num_rows(); if (n > 20) n = 20;
		long i = 0;
		while (q.fetch_row() && i < 20)
		{
			msg_recommend_data msg;
			if (i == 0){	msg.SS = 1;	}
			if (i == (n - 1))	{ msg.SE = 1;}

			std::string uid = q.getstr();
			std::string headpic = q.getstr();
			std::string uname = q.getstr();
			COPY_STR(msg.uid_, uid.c_str());
			COPY_STR(msg.head_pic_, headpic.c_str());
			COPY_STR(msg.uname_, uname.c_str());
			msg.reward_getted_ = q.getval();
			pp->send_msg_to_client(msg);
			i++;
		}
	}
	return ERROR_SUCCESS_0;
}

int msg_get_recommend_reward::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	longlong r = the_service.cost_var(std::string(uid_) + KEY_RECOMMEND_REWARD, IPHONE_NIUINIU, 1);
	//未被领取，
	if (r == 1){
		longlong out_count = 0;
		int ret =	the_service.give_currency(pp->uid_, 500000, out_count);
		if (ret == ERROR_SUCCESS_0){
			the_service.add_var(pp->uid_ + KEY_CUR_TRADE_CAP, -1, 500000);
			Database& db = *the_service.gamedb_;
			BEGIN_UPDATE_TABLE("recommend_info");
			SET_FINAL_VALUE("reward_getted", 1);
			WITH_END_CONDITION("where uid = '" + std::string(uid_) + "' and recommender_uid = '" + lex_cast_to_str(pp->iid_) +"'");
			EXECUTE_IMMEDIATE();
		}
		return ret;
	}
	else{
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
}

int msg_talk_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	msg_chat_deliver msg;
	msg.channel_ = channel_;
	msg.set_content(content_);
	msg.sex_ = 0;
	int ret = ERROR_SUCCESS_0;
	COPY_STR(msg.name_, pp->name_.c_str());
	if (channel_ == CHAT_CHANNEL_BROADCAST){
		longlong out_count = 0;
		logic_ptr plogic = pp->the_game_.lock();
		ret = the_service.apply_cost(pp->uid_, 50000, out_count);
		if (ret == ERROR_SUCCESS_0){
			the_service.broadcast_msg(msg);		
		}
	}
	return ret;
}

int msg_get_trade_to_lst::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	std::string sql = "select `target_iid`, `gold`, UNIX_TIMESTAMP(`time`) from trade_record where uid = '" + pp->uid_ + "' and `state` = 1";
	Query q(*the_service.gamedb_);
	if (q.get_result(sql)){
		long n = q.num_rows(); if (n > 20) n = 20;
		int i = 0;
		while(q.fetch_row() && i < 20)
		{
			msg_trade_data msg;
			if (i == 0) msg.SS = 1;
			if (i == (n - 1)) msg.SE = 1;
	
			msg.iid_ = q.getval();
			msg.gold_ = (double) q.getbigint();
			longlong tim = (double) q.getbigint();
			msg.time = tim;
			pp->send_msg_to_client(msg);
			i++;
		}
	}
	return ERROR_SUCCESS_0;
}

int msg_get_trade_from_lst::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	std::string sql = "select `iid_`, `gold`, UNIX_TIMESTAMP(`time`) from trade_record where target_iid = '" + lex_cast_to_str(pp->iid_) + "' and `state` = 1";
	Query q(*the_service.gamedb_);
	if (q.get_result(sql)){
		long n = q.num_rows(); if (n > 20) n = 20;
		int i = 0;
		while(q.fetch_row() && i < 20)
		{
			msg_trade_data msg;
			if (i == 0) msg.SS = 1;
			if (i == (n - 1)) msg.SE = 1;

			msg.iid_ = (double)q.getbigint();
			msg.gold_ = (double) q.getbigint();
			longlong tim = (double) q.getbigint();
			msg.time = tim;
			pp->send_msg_to_client(msg);
			i++;
		}
	}
	return ERROR_SUCCESS_0;
}
