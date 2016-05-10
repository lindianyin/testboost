#include "msg_server_common.h"
#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "Database.h"
#include <boost/locale/conversion.hpp>
#include <boost/locale/encoding.hpp>
#include "game_service_base.hpp"

//17173广播消息
void broadcast_17173_msg(player_ptr pp, int level, longlong money) 
{
	platform_broadcast_17173< beauty_beast_service::this_type, platform_config_173 >* broadcast = 
		new platform_broadcast_17173< beauty_beast_service::this_type, platform_config_173 >(the_service, platform_config_173::get_instance());

	boost::shared_ptr< platform_broadcast_17173< beauty_beast_service::this_type, platform_config_173 > > pbroadcast(broadcast);


	char buff[512] = {0};

	if(pbroadcast->platform_config_.game_id_ == GAME_ID_EGG)
	{
		sprintf_s(buff,512,"在砸蛋游戏中获得%lld乐豆奖励",money);
	}
	else
	{
		sprintf_s(buff,512,"获得了%d倍奖励，奖励欢乐豆为%lld",level,money);
	}

	string s = boost::locale::conv::between( buff, "UTF-8", "GB2312" ); 

	pbroadcast->rel_uid_ = pp->platform_uid_;
	pbroadcast->award_ = buff;
	pbroadcast->award_utf8 = s;
	pbroadcast->send_request();

}


int state_wait_start::enter()
{
	glb_log.write_log("state_wait_start::enter()!");
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		if (the_service.wait_for_config_update_){
			plogic->is_waiting_config_ = true;
		}
		//如果没有准备进行更新，则开始本场游戏
		if (!the_service.wait_for_config_update_)	{
			plogic->this_turn_start();
		}
		//暂停游戏，准备游戏配置更新
		else{
			timer_.expires_from_now(boost::posix_time::millisec(2000));
			timer_.async_wait(boost::bind(&state_wait_start::on_wait_config_expired, 
				boost::dynamic_pointer_cast<state_wait_start>(shared_from_this()),
				boost::asio::placeholders::error));
			return ERROR_SUCCESS_0;
		}
	}
	else{
		glb_log.write_log("warning, plogic is null in state_wait_start!");
	}
	timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.wait_start_time_));
	timer_.async_wait(boost::bind(&state_wait_start::on_time_expired,
		boost::dynamic_pointer_cast<state_wait_start>(shared_from_this()),
		boost::asio::placeholders::error));
	state_machine::enter();
	return ERROR_SUCCESS_0;
}

void state_wait_start::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		//暂停游戏，准备游戏配置更新
		plogic->change_to_do_random();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_wait_start!");
	}
}

state_wait_start::state_wait_start() :timer_(the_service.timer_sv_)
{
	state_id_ = GET_CLSID(state_wait_start);
}

void state_wait_start::on_wait_config_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_wait_start();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_wait_start!");
	}
}

int state_do_random::enter()
{
	glb_log.write_log("state_do_random::enter()!");
	logic_ptr plogic = the_logic_.lock();
	timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.do_random_time_));
	timer_.async_wait(boost::bind(&state_do_random::on_time_expired,
		boost::dynamic_pointer_cast<state_do_random>(shared_from_this()),
		boost::asio::placeholders::error));
	if(plogic.get()){
		plogic->random_result();
	}
	state_machine::enter();		
	return ERROR_SUCCESS_0;
}

void state_do_random::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_end_rest();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}
}

state_do_random::state_do_random() :timer_(the_service.timer_sv_)
{
	state_id_ = 2;
}

int state_rest_end::enter()
{
	glb_log.write_log("state_rest_end::enter()!");
	timer_.expires_from_now(boost::posix_time::millisec(the_service.the_config_.wait_end_time_));
	timer_.async_wait(boost::bind(&state_rest_end::on_time_expired, 
		boost::dynamic_pointer_cast<state_rest_end>(shared_from_this()),
		boost::asio::placeholders::error));

	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->balance_result();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}
	state_machine::enter();
	return ERROR_SUCCESS_0;
}

void state_rest_end::on_time_expired( boost::system::error_code e )
{
	logic_ptr plogic = the_logic_.lock();
	if(plogic.get()){
		plogic->change_to_wait_start();
	}
	else{
		glb_log.write_log("warning, plogic is null in state_do_random!");
	}
}

state_rest_end::state_rest_end() :timer_(the_service.timer_sv_)
{
	state_id_ = 3;
}

beatuy_beast_logic::beatuy_beast_logic(int ismatch):timer_(the_service.timer_sv_)
{
	is_waiting_config_ = false;
	strid_ = get_guid();
	turn_ = 0;
	last_is_sysbanker_ = true;
	banker_deposit_ = 0;
	sysbanker_initial_deposit_ = 0;
	cheat_count_ = 0;
	bot_bet_count_ = 0;
	scene_set_ = new scene_set();
	scene_set_->id_ = 1;
}

void beatuy_beast_logic::broadcast_msg( msg_base<none_socket>& msg )
{
	auto it = players_.begin();
	while (it != players_.end())
	{
		player_ptr pp = *it; it++;
		pp->send_msg_to_client(msg);
	}
}
void beatuy_beast_logic::do_random(map<int, preset_present>& vpr)
{

	int rate_cap = 0;
	auto it = vpr.begin();
	while (it != vpr.end())
	{
		preset_present& pr = it->second; it++;
		rate_cap += pr.rate_;
	}

	int	rnd = rand_r(rate_cap);
	it = vpr.begin();
	while (it != vpr.end())
	{
		preset_present& pr = it->second; it++;
		if (pr.rate_ > rnd){
			this_turn_result_ = the_service.the_config_.presets_[pr.pid_];
			break;
		}
		else{
			rnd -= pr.rate_;
		}
	}

	//this_turn_result_ = the_service.the_config_.presets_[100];
}
void beatuy_beast_logic::random_result()
{
	//机器人自动上庄
	for (int i = 0 ; i < bots_.size(); i++)
	{
		if (std::find(banker_applications_.begin(), banker_applications_.end(), bots_[i]->uid_) == banker_applications_.end() && 
			the_banker_ != bots_[i])	{
			longlong cr = the_service.the_config_.banker_deposit_ * 0.1;
			//如果机器人没钱了,加点钱给他
			if(the_service.cache_helper_.get_currency(bots_[i]->uid_) <= cr){
				if(the_service.cache_helper_.give_currency(bots_[i]->uid_, rand_r(the_service.the_config_.banker_deposit_, 3 * the_service.the_config_.banker_deposit_), cr) == ERROR_SUCCESS_0){
					
				}
			}
			apply_banker(bots_[i]);
		}
	}

	int		cheat_win = 0;
	if (the_banker_.get() && !the_banker_->is_bot_){
		if (the_service.the_config_.benefit_banker_ > 0){
			if (rand_r(100) < abs(the_service.the_config_.benefit_banker_)){
				cheat_win = 1;
			}
		}
		else if(the_service.the_config_.benefit_banker_ < 0){
			if (rand_r(100) < abs(the_service.the_config_.benefit_banker_)){
				cheat_win = 2;
			}
		}
		//如果庄家在黑名单中,则尽赔
		if(the_service.the_config_.black_names_.find(the_banker_->uid_) != the_service.the_config_.black_names_.end()){
			cheat_win = 2;
		}
	}
	else{
			if (rand_r(100) < abs(the_service.the_config_.benefit_sysbanker_)){
				cheat_win = 1;
			}
	}

	map<int, preset_present> cheat_bets;
	map<int, longlong> summury = sum_bets(false);

	//没有人压注的奖项也要放在可出结果中，否则cheat时太假
	auto it_present = the_service.the_config_.presets_.begin();
	while (it_present != the_service.the_config_.presets_.end())
	{
		//如果没有人压注
		if(summury.find(it_present->first) == summury.end()){
			cheat_bets.insert(make_pair(it_present->first, it_present->second));
		}
		it_present++;
	}

	if (summury.empty()){
		cheat_win = 0;
	}

	this_turn_result_ = the_service.the_config_.presets_.begin()->second;
//正常局
	if (cheat_win == 0){
		do_random(the_service.the_config_.presets_);
	}
	//利庄局,赔最小
	else if (cheat_win == 1){
		cheat_count_++;
		glb_log.write_log("========>benefit banker instead!, rate: %d, %d, %.2f", cheat_count_, turn_, float(cheat_count_) / turn_);
		vector<int> v;
		longlong n = 0x7FFFFFFFFFFFFFFF;
		auto it = summury.begin();
		while (it != summury.end())
		{
			if (n > it->second){
				n = it->second;
				v.clear();
				v.push_back(it->first);
			}
			else if(n == it->second){
				v.push_back(it->first);
			}
			it++;
		}

		if(cheat_bets.empty()){
			for (int i = 0 ; i < v.size(); i++)
			{
				int itemp = v[i];
				//此处需要转换
				if(!the_service.the_config_.is_pid(v[i]))
					itemp = the_service.the_config_.get_min_rate_pid_by_prior(v[i]);

				cheat_bets.insert(make_pair(v[i], the_service.the_config_.presets_[itemp]));
			}
		}

		if (!cheat_bets.empty()){
			do_random(cheat_bets);
		}
	}
	//打庄局,赔最大
	else if (cheat_win == 2){
		glb_log.write_log("========>benefit player instead!, rate: %d, %d, %.2f", cheat_count_, turn_, float(cheat_count_)/ turn_);
		vector<int> v;
		longlong n = 0;
		auto it = summury.begin();
		while (it != summury.end())
		{
			if (n < it->second){
				n = it->second;
				v.clear();
				v.push_back(it->first);
			}
			else if (n == it->second){
				v.push_back(it->first);
			}
			it++;
		}
		if(cheat_bets.empty()){
			for (int i = 0 ; i < v.size(); i++)
			{
				int itemp = v[i];
				//此处需要转换
				if(!the_service.the_config_.is_pid(v[i]))
					itemp = the_service.the_config_.get_max_rate_pid_by_prior(v[i]);

				cheat_bets.insert(make_pair(v[i], the_service.the_config_.presets_[itemp]));
			}
		}
		if (!cheat_bets.empty()){
			do_random(cheat_bets);
		}
	}
	
	if (last_random_.size() > 20)	{
		last_random_.pop_back();
	}

	last_random_.insert(last_random_.begin(), this_turn_result_.pid_);

	if(the_service.lottery_record_.size() >= 100 )
		the_service.lottery_record_.pop();
		
	the_service.lottery_record_.push(this_turn_result_.prior_);
	

	msg_rand_result msg;
	msg.rand_result_ = this_turn_result_.pid_;
	broadcast_msg(msg);
}

int beatuy_beast_logic::apply_banker( player_ptr pp )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (pp->is_bot_ && !the_service.the_config_.bot_enable_banker_){
		return ERROR_SUCCESS_0;
	}

	auto itf = std::find(banker_applications_.begin(), banker_applications_.end(), pp->uid_);
	if(itf != banker_applications_.end()){
		return ERROR_SUCCESS_0;
	}

	int r = can_promote_banker(pp);
	if (r != ERROR_SUCCESS_0){
		return r;
	}

	banker_applications_.push_back(pp->uid_);

	msg_new_banker_applyed msg;
	COPY_STR(msg.name_, pp->name_.c_str());
	COPY_STR(msg.uid_, pp->uid_.c_str());
	broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}

int beatuy_beast_logic::apply_cancel_banker( player_ptr pp )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	auto it = std::find(banker_applications_.begin(), banker_applications_.end(), pp->uid_);
	if (it != banker_applications_.end()){
		msg_apply_banker_canceled msg;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		broadcast_msg(msg);
		banker_applications_.erase(it);
		return ERROR_SUCCESS_0;
	}
	else if (pp != the_banker_)
		return GAME_ERR_NOT_BANKER;

	if (pp->banker_turns_ < the_service.the_config_.min_banker_turn_)
		return GAME_ERR_BANKER_TURN_LESS;

	apply_cancel_ = true;
	return ERROR_SUCCESS_0;
}

longlong sum_setbets(string uid, vector<bet_info>& bets_)
{
	longlong ret = 0;
	for (int i = 0; i < bets_.size(); i++)
	{
		if( bets_[i].uid_ == uid){
			ret += bets_[i].bet_count_;
		}
	}
	return ret;
}
//此处的present_id应该理解为押注ID 对应的值应该是prior_
int beatuy_beast_logic::set_bet( player_ptr pp, int pid, int present_id )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (pp == the_banker_)
		return GAME_ERR_BANKER_CANT_BET;

	if (the_service.the_config_.chips_.find(pid) == the_service.the_config_.chips_.end()){
		return GAME_ERR_CANT_FIND_CHIP_SET;
	}


	unsigned int this_time_bet = the_service.the_config_.chips_[pid];
	
	int r = can_bet(present_id, this_time_bet);
	
	if ( r != ERROR_SUCCESS_0)
		return r;

	if ((sum_setbets(pp->uid_, bets_) + this_time_bet) > the_service.the_config_.bet_cap_){
		return GAME_ERR_BET_CAP_EXCEED;
	}



	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	if (!pp->is_bot_){
// 		longlong	total_win = 0;
// 		int ret = get_today_win(pp->uid_, total_win);
// 		//赢钱达到上限
// 		if (ret != ERROR_SUCCESS_0){
// 			return SYS_ERR_SQL_EXE_FAIL;
// 		}
// 
// 		if((total_win > 0) && (abs(total_win) > the_service.the_config_.max_win_)){
// 			return GAME_ERR_WIN_CAP_EXCEED;
// 		}
// 		if((total_win < 0) && (abs(total_win) > the_service.the_config_.max_lose_)){
// 			return GAME_ERR_LOSE_CAP_EXCEED;
// 		}
	}
	longlong out_count = 0;
	if(the_service.cache_helper_.apply_cost(pp->uid_, this_time_bet, out_count) == ERROR_SUCCESS_0){
		bet_info bet;
		bet.bet_count_ = this_time_bet;
		bet.present_id_ = present_id;
		bet.chip_id_ = pid;
		bet.uid_ = pp->uid_;
		bets_.push_back(bet);

		if (!pp->is_bot_){
			Database& db(*the_service.gamedb_);
			BEGIN_INSERT_TABLE("set_bets");
			SET_STRFIELD_VALUE("uid", pp->uid_);
			SET_FIELD_VALUE("pid", pid);
			SET_FIELD_VALUE("turn", turn_);
			SET_FIELD_VALUE("present_id", present_id);
			SET_FIELD_VALUE("bet_count", this_time_bet);
			SET_FIELD_VALUE("is_bot", int(pp->is_bot_));
			SET_STRFINAL_VALUE("game_id", strid_);
			EXECUTE_IMMEDIATE();
		}

		msg_currency_change msg;
		msg.credits_ = out_count;
		pp->send_msg_to_client(msg);
		
		msg_player_setbet msg_set;
		msg_set.pid_ = pid;
		msg_set.present_id_ = present_id;
		msg_set.max_setted_ = get_present_chips(present_id);
		broadcast_msg(msg_set);
		
		msg_my_setbet msg_my;
		msg_my.present_id_ = present_id;
		msg_my.set_ = this_time_bet;
		pp->send_msg_to_client(msg_my);
	}
	else	{
		return GAME_ERR_NOT_ENOUGH_CURRENCY;
	}
	return ERROR_SUCCESS_0;
}

void beatuy_beast_logic::this_turn_start()
{
	if ((turn_ % 11) == rand_r(8, 10)){
		clear_bots();
		join_bots();
	}
	//检查玩家下庄
	if (the_banker_.get()){
		longlong	total_win = 0;
		player_ptr pb = get_player(the_banker_->uid_);
		int ret = get_today_win(the_banker_->uid_, total_win);
		//赢钱达到上限
		if (ret != ERROR_SUCCESS_0 || total_win > the_service.the_config_.max_win_ ||
			banker_deposit_ < the_service.the_config_.banker_deposit_ ||//保证金不够
			apply_cancel_ ||//玩家申请下庄
			the_banker_->banker_turns_ >= the_service.the_config_.max_banker_turn_ ||	//到了最大作庄次数
			!pb.get() || pb != the_banker_){
				cancel_banker();
		}
	}

	//如果当前没有玩家庄家则从申请表中找一个玩家做庄
	if (!the_banker_.get()){
		next_banker();
	}

	if (the_banker_.get()){ 
		the_banker_->banker_turns_++;
	}
	//如果没有玩家可以当庄家，则系统做庄
	if (!the_banker_.get() && (!last_is_sysbanker_ || banker_deposit_ < the_service.the_config_.sys_banker_deposit_) && the_service.the_config_.enable_sysbanker_){
		//系统爆庄了,记录爆庄情况
		if ((banker_deposit_ < the_service.the_config_.sys_banker_deposit_) && (turn_ > 0)){
			Database& db(*the_service.gamedb_);
			BEGIN_INSERT_TABLE("warning_on_banker_breakdown");
			SET_FINAL_VALUE("initial_deposit", sysbanker_initial_deposit_ - banker_deposit_);
			EXECUTE_IMMEDIATE();
		}
		last_is_sysbanker_ = true;
		apply_cancel_ = false;
		banker_deposit_ = rand_r(the_service.the_config_.sys_banker_deposit_, the_service.the_config_.sys_banker_deposit_max_);
		sysbanker_initial_deposit_ = banker_deposit_;
		msg_banker_promote msg;
		msg.deposit_ = banker_deposit_;
		COPY_STR(msg.name_, the_service.the_config_.sysbanker_name_.c_str());
		msg.is_sys_banker_ = 1;
		broadcast_msg(msg);
	}
	bot_bet_count_ = 0;
	bot_random_bet();
	turn_++;
}

void beatuy_beast_logic::balance_result()
{
	//机器人下注不能删除，否则客户端的钱不会变化。
	if (bets_.empty() && the_service.the_config_.bot_enable_bet_){
		//广播庄家赢钱
		msg_banker_deposit_change msg;
		if(the_banker_.get()){
			msg.turn_left_ = the_service.the_config_.max_banker_turn_ - the_banker_->banker_turns_;
		}
		msg.credits_ = banker_deposit_;
		broadcast_msg(msg);
		return;
	}

	longlong old_deposit = banker_deposit_;
	//大乐透，归还团队下注，个人的就不返还
	if (this_turn_result_.pid_ == 100){
		for (int i = 0; i < bets_.size(); i++)
		{
			bet_info& bi = bets_[i];
			if( the_service.the_config_.is_pid(bi.present_id_) )
			{
				longlong c = 0;
				the_service.cache_helper_.give_currency(bets_[i].uid_, bi.bet_count_, c);
			}
			else //如果不是团队押注，还是要把钱扣掉
			{
				record_lose(bi);
				banker_deposit_ += bi.bet_count_;
			}

		}
	}
	else{
		//统计庄家收到下注和
		for (int i = 0; i < bets_.size(); i++)
		{
			bet_info& bi = bets_[i];
			record_lose(bi);
			banker_deposit_ += bi.bet_count_;
		}
	}

	for (int i = 0; i < bets_.size(); i++)
	{
		bet_info& bi = bets_[i];
		//如果中奖
		auto itg = the_service.the_config_.groups_.find(bi.present_id_);
		if (itg != the_service.the_config_.groups_.end())	{
			for (int j = 0; j < itg->second.group_by_.size(); j++)
			{
				vector<preset_present> vp = the_service.get_present_bysetbet(itg->second.group_by_[j]);
				for (int ii = 0; ii < (int)vp.size(); ii++)
				{
					if (vp[ii].pid_ == this_turn_result_.pid_){
						record_winner(bi, itg->second.factor_);
						break;
					}
				}
			}
		}
		else{
			vector<preset_present> vp = the_service.get_present_bysetbet(bi.present_id_);
			for (int i = 0; i < (int)vp.size(); i++)
			{
				if (vp[i].pid_ == this_turn_result_.pid_){
					record_winner(bi, vp[i].factor_);
					break;
				}
			}
		}
	}
	bets_.clear();

	longlong bot_lose_to_botbanker = 0;	//机器人输给机器人的，这部分数据在最后的时候要剃除出统计。
	longlong this_time_tax = 0;					//这次总抽税
	DWORD tw = GetTickCount();

	//应用输赢
	auto it_rpt = turn_reports_.begin();
	while (it_rpt != turn_reports_.end())
	{
		game_report_data& grd = it_rpt->second;		it_rpt++;
		player_ptr pp = get_player(grd.uid_);

		if (pp.get() && pp->is_bot_)	{
			bot_lose_to_botbanker += grd.pay_;
			bot_lose_to_botbanker -= grd.actual_win_;
		}
		//机器人不扣税
		else{
			longlong tax = grd.actual_win_  / 100.0 * the_service.the_config_.player_tax2_;
			grd.actual_win_ -= tax;
			grd.should_win_ -= grd.should_win_  / 100.0 * the_service.the_config_.player_tax2_;
			this_time_tax += tax;
		}
		longlong out_count = 0;
		if(the_service.cache_helper_.give_currency(grd.uid_, grd.actual_win_, out_count) != ERROR_SUCCESS_0){
			glb_log.write_log("give currency error in balance_winner! uid = %s, count = %u", grd.uid_.c_str(), grd.actual_win_);
		}
		else{
			msg_currency_change msg;
			msg.credits_ = out_count;
			if (pp.get()){
				pp->send_msg_to_client(msg);
			}
		}
		if((pp.get() && !pp->is_bot_) || !pp.get()){
			Database& db(*the_service.gamedb_);
			BEGIN_INSERT_TABLE("log_player_win");
			SET_STRFIELD_VALUE("game_id", strid_);
			SET_STRFIELD_VALUE("uid", grd.uid_);
			SET_FIELD_VALUE("turn", turn_);
			SET_FIELD_VALUE("pay", grd.pay_);
			if (pp.get()){
				SET_FIELD_VALUE("is_bot", int(pp->is_bot_));
			}
			else{
				SET_FIELD_VALUE("is_bot", strstr(grd.uid_.c_str(), "ipbot_test_") ? 1 : 0 );
			}
			SET_FINAL_VALUE("win", grd.actual_win_);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}

		//发送报表到客户端
		if (pp.get()){
			//删除白名单
			if (pp->in_white_name_){
				pp->in_white_name_ = false;
				string sql = "delete from white_names where uid = '" + pp->uid_ + "'";
				Query q(*the_service.gamedb_);
				q.execute(sql);
			}

			msg_game_report msg;
			msg.actual_win_ = grd.actual_win_;
			msg.pay_ = grd.pay_;
			msg.should_win_ = grd.should_win_;
			pp->send_msg_to_client(msg);
			//发送广播消息给7158
			if(pp->platform_id_ == "7158")
			{
				 broadcast_msg_to_7158(grd);
			}
			else if(pp->platform_id_ == "live"  || pp->platform_id_ == "show")
			{
				if(grd.factor_ >= 10)
				{
						broadcast_17173_msg(pp,(int)grd.factor_,grd.actual_win_);
				}
			}
		}
	}
	int t = GetTickCount() - tw;
	glb_log.write_log("==============>balance player cost %dms, handled count:%d", t, turn_reports_.size());
	
	banker_deposit_ -= bot_lose_to_botbanker;
	longlong banker_win = banker_deposit_ - old_deposit;
	if(the_banker_.get()){
		//如果庄家赢钱了，扣税
		if (banker_win > 0 && !the_banker_->is_bot_){
			longlong tax = banker_win  / 100.0 * the_service.the_config_.player_banker_tax_;
			this_time_tax += tax;
			banker_deposit_ -= tax;
			banker_win -= tax;
		}

		msg_game_report msg;
		msg.actual_win_ =	banker_win;
		msg.should_win_ = 0;
		msg.pay_ = 0;
		the_banker_->send_msg_to_client(msg);
		
		//备份庄家的钱
		{
			Database& db(*the_service.gamedb_);
			BEGIN_UPDATE_TABLE("banker_deposit");
			SET_FINAL_VALUE("deposit", banker_deposit_);
			WITH_END_CONDITION("where uid = '" + the_banker_->uid_ + "' and game_id = '" + strid_ + "'");
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}
		//记录玩家输赢
		if(!the_banker_->is_bot_){
			Database& db(*the_service.gamedb_);
			BEGIN_INSERT_TABLE("log_player_win");
			SET_STRFIELD_VALUE("game_id", strid_);
			SET_STRFIELD_VALUE("uid", the_banker_->uid_);
			SET_FIELD_VALUE("turn", turn_);
			SET_FIELD_VALUE("is_bot", int(the_banker_->is_bot_));
			SET_FINAL_VALUE("win", banker_win);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}

	}
	//系统庄
	else {
		Database& db(*the_service.gamedb_);
		BEGIN_INSERT_TABLE("log_player_win");
		SET_STRFIELD_VALUE("game_id", strid_);
		SET_FIELD_VALUE("turn", turn_);
		SET_FIELD_VALUE("is_bot", 1);	
		SET_FINAL_VALUE("win", banker_win);
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
	}

	if(the_banker_.get() && !the_banker_->is_bot_)
	{
		//广播庄家赢钱
		msg_banker_deposit_change msg;
		if(the_banker_.get()){
			msg.turn_left_ = the_service.the_config_.max_banker_turn_ - the_banker_->banker_turns_;
		}
		msg.credits_ = banker_deposit_;
		broadcast_msg(msg);
	}

	{
		Database& db(*the_service.gamedb_);
		BEGIN_INSERT_TABLE("log_gameturn_info");
		SET_STRFIELD_VALUE("game_id", strid_);

		if(the_banker_.get()){
			SET_STRFIELD_VALUE("banker_uid", the_banker_->uid_);
		}
		else{
			SET_STRFIELD_VALUE("banker_uid", "");
		}
		SET_FIELD_VALUE("random_result", this_turn_result_.pid_);
		SET_FIELD_VALUE("banker_deposit", banker_deposit_);
		SET_FIELD_VALUE("this_turn_win",  banker_win);
		SET_FIELD_VALUE("total_players", turn_reports_.size());
		SET_FIELD_VALUE("turn", turn_);
		if (the_banker_.get()){
			SET_FIELD_VALUE("is_bot", int(the_banker_->is_bot_));	
		}
		else{
			SET_FIELD_VALUE("is_bot", 1);	
		}
		SET_FINAL_VALUE("total_tax", this_time_tax);
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
	}
	turn_reports_.clear();
}

void beatuy_beast_logic::will_shut_down()
{
	cancel_banker();
}

void beatuy_beast_logic::change_to_state( ms_ptr ms )
{
	if (current_state_.get()){
		current_state_->leave();
	}
	current_state_ = ms;
	current_state_->enter();
}

int beatuy_beast_logic::restore_deposit()
{
	if (!the_banker_.get()) return ERROR_SUCCESS_0;
	if (the_banker_->is_bot_) return ERROR_SUCCESS_0;

	longlong out_count = 0;
	//防止断电时更新账号数据库失败，先标记准备更新记录
	{
		Database& db(*the_service.gamedb_);
		BEGIN_UPDATE_TABLE("banker_deposit");
		SET_FINAL_VALUE("state", 1);
		WITH_END_CONDITION("where uid = '" + the_banker_->uid_ + "' and game_id = '" + strid_ + "'");
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
	}
	if (the_service.cache_helper_.give_currency(the_banker_->uid_, banker_deposit_, out_count) != ERROR_SUCCESS_0){
		glb_log.write_log("restore player's deposit failed on update account db, uid = %s, count = %u", the_banker_->uid_.c_str(), banker_deposit_);
		return GAME_ERR_RESTORE_FAILED;
	}
	//账号加钱成功之后，标记为更新成功
	else{
		Database& db(*the_service.gamedb_);
		BEGIN_UPDATE_TABLE("banker_deposit");
		SET_FINAL_VALUE("state", 2);
		WITH_END_CONDITION("where uid = '" + the_banker_->uid_ + "' and game_id = '" + strid_ + "'");
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
	}
	return ERROR_SUCCESS_0;
}

void beatuy_beast_logic::cancel_banker()
{
	//如果不是玩家庄，则不作处理
	if (!the_banker_.get()) return;
	if (restore_deposit() != ERROR_SUCCESS_0){

	}
	banker_deposit_ = 0;
	apply_cancel_ = false;

	the_banker_.reset();
	next_banker();
}

void beatuy_beast_logic::next_banker()
{
	if(the_service.the_config_.shut_down_) return;
	while (banker_applications_.size() > 0){
		string uid = banker_applications_[0];
		player_ptr pp = get_player(uid);
		banker_applications_.erase(banker_applications_.begin());

		//通知客户端取消第一个列表
		msg_apply_banker_canceled msg;
		COPY_STR(msg.uid_, uid.c_str());
		broadcast_msg(msg);

		if (!pp.get()) continue;
		if (promote_to_banker(pp) != ERROR_SUCCESS_0) continue;
		break;
	}
}

int beatuy_beast_logic::promote_to_banker( player_ptr pp )
{
	//如果玩家庄
	if(pp.get()){
		
		int r = can_promote_banker(pp);
		if (r != ERROR_SUCCESS_0){
			return r;
		}

		longlong	out_count = 0;
		if(the_service.cache_helper_.apply_cost(pp->uid_, the_service.the_config_.banker_deposit_, out_count, true) != ERROR_SUCCESS_0)
			return GAME_ERR_NOT_ENOUGH_CURRENCY;

		banker_deposit_ = out_count;
		pp->banker_turns_ = 0;
		Database& db(*the_service.gamedb_);
		BEGIN_REPLACE_TABLE("banker_deposit");
		SET_STRFIELD_VALUE("uid", pp->uid_);
		SET_STRFIELD_VALUE("game_id", strid_);
		SET_FINAL_VALUE("deposit", out_count);
		EXECUTE_IMMEDIATE();

		msg_banker_promote msg;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		msg.deposit_ = out_count;
		COPY_STR(msg.name_, pp->name_.c_str());
		COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
		broadcast_msg(msg);

		the_banker_ = pp;
		apply_cancel_ = false;
		last_is_sysbanker_ = false;
		return ERROR_SUCCESS_0;
	}
	return SYS_ERR_CANT_FIND_CHARACTER;
}

void beatuy_beast_logic::record_winner( bet_info& bi, double fator)
{
	longlong win = 0;
	longlong should_win = longlong(bi.bet_count_) * floor(fator * 10 + 0.5) / 10;
	//如果庄家钱够赔
	if (banker_deposit_ >= should_win){
		win = should_win;
		banker_deposit_ -= win;
	}
	//如果庄家钱不够赔
	else{
		win = banker_deposit_;
		banker_deposit_ = 0;
	}
	auto itf = turn_reports_.find(bi.uid_);
	if (itf != turn_reports_.end()){
		game_report_data& rpt = itf->second;
		rpt.actual_win_ += win;
		rpt.should_win_ += should_win;

		if(fator > rpt.factor_ )
			rpt.factor_ = fator;
	}
	else{
		game_report_data rpt;
		rpt.uid_ = bi.uid_;
		rpt.actual_win_ = win;
		rpt.should_win_ = should_win;

		rpt.factor_ = fator;
		turn_reports_.insert(make_pair(rpt.uid_, rpt));
	}
}

void beatuy_beast_logic::record_lose( bet_info& bi )
{
	auto itf = turn_reports_.find(bi.uid_);
	if (itf != turn_reports_.end()){
		game_report_data& rpt = itf->second;
		rpt.pay_ += bi.bet_count_;
	}
	else{
		game_report_data rpt;
		rpt.uid_ = bi.uid_;
		rpt.pay_ = bi.bet_count_;
		turn_reports_.insert(make_pair(rpt.uid_, rpt));
	}
}

bool beatuy_beast_logic::sort_tax_fun( tax_info& t1, tax_info& t2 )
{
	return t1.bound_ < t1.bound_;
}

longlong beatuy_beast_logic::calc_sys_tax( longlong n )
{
	return n / 100.0 * the_service.the_config_.sys_banker_tax_;
}



int beatuy_beast_logic::can_bet(int pid, unsigned int this_time_bet)
{
	if (current_state_->state_id_ != GET_CLSID(state_wait_start)){
		return GAME_ERR_CANT_BET_NOW;
	}

	int iprior  = pid;
	//如果不是团队的押注，pid的值其实就prior的值，需要转换
	if(!the_service.the_config_.is_pid(pid))
		pid = the_service.the_config_.get_max_rate_pid_by_prior(pid);
	

	if(the_service.the_config_.have_banker_ != 0)
	{
		longlong	could_be_lose = 0;
		map<int, longlong> summury = sum_bets(true);
		could_be_lose = summury[iprior] + longlong(this_time_bet) * the_service.the_config_.presets_[pid].factor_;
		if (could_be_lose > banker_deposit_){
			return GAME_ERR_CANT_BET_MORE;
		}
	}

	return ERROR_SUCCESS_0;
}

void beatuy_beast_logic::player_login( player_ptr pp, int pos)
{
	{
		//发送服务器参数
		msg_server_parameter msg;
		auto itf = the_service.the_config_.presets_.begin();
		int i = 0;
		while ((itf != the_service.the_config_.presets_.end()) && (i < PRESETS_COUNT))
		{
			preset_present& pri = itf->second;			itf++;
			msg.pid_[i] = pri.pid_;
			msg.rate_[i] = pri.rate_;
			msg.rate_[i]   = pri.factor_;
			msg.min_guarantee_ = (int)scene_set_->gurantee_set_;
			msg.low_cap_ = (int)(scene_set_->gurantee_set_ - scene_set_->player_tax_) / 50;
			msg.player_tax_ = scene_set_->player_tax_;
			msg.enter_scene_ = 1;
			i++;
		}

		auto gitf = the_service.the_config_.groups_.begin();

		for(;i <PRESETS_COUNT && gitf != the_service.the_config_.groups_.end(); ++i,++gitf )
		{
			group_set gri = gitf->second;
			msg.pid_[i] = gri.pid_;
			msg.rate_[i] = gri.factor_;
		}
		auto it2 = the_service.the_config_.chips_.begin();
		i = 0;
		while ((it2 != the_service.the_config_.chips_.end()) && (i < CHIP_COUNT))
		{
			msg.chip_id_[i] = it2->first;
			msg.chip_cost_[i] = it2->second;
			it2++;
			i++;
		}
		pp->send_msg_to_client(msg);
	}
	if (!is_ingame(pp)){
		players_.push_back(pp);
	}
	pp->the_game_ = shared_from_this();

	{
		msg_rand_result msg_result;
		msg_result.rand_result_ = this_turn_result_.pid_;
		pp->send_msg_to_client(msg_result);
	}

	//发送游戏状态机
	{
		msg_state_change msg;
		msg.change_to_ = current_state_->state_id_;
		msg.time_left_ = current_state_->get_time_left();
		pp->send_msg_to_client(msg);

	}

	//发送已开奖项
	msg_last_random msg_rd;
	for (int i = 0; i < last_random_.size() && i  < 15; i++)
	{
		msg_rd.r_[i] = last_random_[i];
	}
	pp->send_msg_to_client(msg_rd);

	//发送已下注
	for (int i = 0 ;i < bets_.size(); i++)
	{
		msg_player_setbet msg;
		msg.pid_ = bets_[i].chip_id_;
		msg.present_id_ = bets_[i].present_id_;
		msg.max_setted_ = get_present_chips(bets_[i].present_id_);
		pp->send_msg_to_client(msg);

		if (bets_[i].uid_ == pp->uid_)
		{
			msg_my_setbet myBetmsg;
			myBetmsg.present_id_ = bets_[i].present_id_;
			myBetmsg.set_ = bets_[i].bet_count_;
			pp->send_msg_to_client(myBetmsg);
		}
	}

#if 0
	//发送申请上庄列表
	for (int i = 0 ; i < banker_applications_.size(); i++)
	{
		player_ptr tp = get_player(banker_applications_[i]);
		if (!tp.get()) continue;
		
		msg_new_banker_applyed msg;
		COPY_STR(msg.uid_, tp->uid_.c_str());
		COPY_STR(msg.name_, tp->name_.c_str());
		pp->send_msg_to_client(msg);
	}

	msg_banker_promote msg;
	msg.deposit_ = banker_deposit_;
	//发送庄家
	if (!the_banker_.get()){
		COPY_STR(msg.name_, the_service.the_config_.sysbanker_name_.c_str());
		msg.is_sys_banker_ = 1;
	}
	else{
		COPY_STR(msg.uid_, the_banker_->uid_.c_str());
		COPY_STR(msg.name_, the_banker_->name_.c_str());
		COPY_STR(msg.head_pic_, the_banker_->head_ico_.c_str());
		msg.is_sys_banker_ = 0;
	}	
	pp->send_msg_to_client(msg);
#endif
}

void beatuy_beast_logic::leave_game(player_ptr pp, int pos, int why)
{
	pp->the_game_.reset();
	auto itf = std::find(players_.begin(), players_.end(), pp);
	if(itf != players_.end())
		players_.erase(itf);
	
	auto itf2 = std::find(banker_applications_.begin(), banker_applications_.end(), pp->uid_);
	if(itf2 != banker_applications_.end()){
		banker_applications_.erase(itf2);

		msg_apply_banker_canceled msg;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		broadcast_msg(msg);
	}
}

void beatuy_beast_logic::start_logic()
{
	join_bots();
	change_to_wait_start();
}

map<int, longlong> beatuy_beast_logic::sum_bets(bool include_bot)
{
	map<int, longlong>  ret;
	for (int i = 0 ; i < (int)bets_.size(); i++)
	{
		if (!include_bot && is_bot(bets_[i].uid_)) continue;

		auto itg = the_service.the_config_.groups_.find(bets_[i].present_id_);
		if (itg != the_service.the_config_.groups_.end()){
			for (int j = 0; j < itg->second.group_by_.size(); j++)
			{
				ret[itg->second.group_by_[j]] += longlong(bets_[i].bet_count_) * itg->second.factor_;
			}
		}
		else{
			vector<preset_present> vp = the_service.get_present_bysetbet(bets_[i].present_id_);
			for (int j = 0; j < (int)vp.size(); j++)
			{
				ret[bets_[i].present_id_] += longlong(bets_[i].bet_count_) * vp[j].factor_;
			}
		}
	}
	return ret;
}

int beatuy_beast_logic::get_today_win(string uid, longlong& win )
{
	string sql = "select sum(win - pay) from log_player_win where uid = '" + uid +"' and time > date(now())";
	Query q(*the_service.gamedb_);
	if (q.get_result(sql) && q.fetch_row()){
		win = q.getbigint();
		return ERROR_SUCCESS_0;
	}
	else
		return SYS_ERR_SQL_EXE_FAIL;
}

longlong beatuy_beast_logic::get_present_chips( int pid )
{
	longlong	total = 0;
	for (int i = 0; i < bets_.size(); i++)
	{
		bet_info& bet = bets_[i];
		if (bet.present_id_ == pid){
			total	+= bet.bet_count_;
		}
	}
	return total;
}

int beatuy_beast_logic::can_promote_banker( player_ptr pp )
{
	longlong cred = the_service.cache_helper_.get_currency(pp->uid_);
	if (cred < the_service.the_config_.banker_deposit_){
		return GAME_ERR_NOT_ENOUGH_CURRENCY;
	}

	if (!pp->is_bot_){
		longlong	total_win = 0;
		int ret = get_today_win(pp->uid_, total_win);
		//赢钱达到上限
		if (ret != ERROR_SUCCESS_0){
			return SYS_ERR_SQL_EXE_FAIL;
		}
		if((total_win > 0) && (abs(total_win) > the_service.the_config_.max_win_)){
			return GAME_ERR_WIN_CAP_EXCEED;
		}
		if((total_win < 0) && (abs(total_win) > the_service.the_config_.max_lose_)){
			return GAME_ERR_LOSE_CAP_EXCEED;
		}
	}
	return ERROR_SUCCESS_0;
}

longlong beatuy_beast_logic::calc_player_tax( longlong n )
{
	return n / 100.0 * the_service.the_config_.player_tax2_;
}

void beatuy_beast_logic::bot_join( player_ptr pp )
{
	bots_.push_back(pp);
	pp->the_game_ = shared_from_this();

}

class timer_handle
{
public:
	logic_ptr plogic;
	void operator()(boost::system::error_code e )
	{
		plogic->random_bet();
	}
};

void beatuy_beast_logic::bot_random_bet()
{
	if (!the_banker_.get() || !the_banker_->is_bot_) return;
	if (!the_service.the_config_.bot_enable_bet_) return;

	timer_.expires_from_now(boost::posix_time::millisec(rand_r(200, 500)));
	timer_handle th;
	th.plogic = shared_from_this();
 	timer_.async_wait(th);
}

void beatuy_beast_logic::random_bet()
{
	bot_bet_count_++;
	vector<player_ptr> vpla;
	for (int i = 0; i < bots_.size(); i++)
	{
		if (bots_[i] != the_banker_){
			vpla.push_back(bots_[i]);
		}
	}
	if (vpla.empty()) return;

	player_ptr pp = vpla[rand_r(vpla.size() - 1)];
	int present_id = -1, chp = -1;

	vector<int> vp;
	auto it_p = the_service.the_config_.presets_.begin();
	while (it_p != the_service.the_config_.presets_.end())
	{
		//vp.push_back(it_p->second.pid_);
		vp.push_back(it_p->second.prior_);
		it_p++;
	}

	if (!vp.empty()){
		int r = rand_r(vp.size() - 1);
		present_id = vp[r];
	}
	vp.clear();

	auto it_c = the_service.the_config_.chips_.begin();
	while (it_c != the_service.the_config_.chips_.end())
	{
		vp.push_back(it_c->first);
		it_c++;
	}

	if (!vp.empty()){
		int r = rand_r(vp.size() - 1);
		chp = vp[r];
	}
	if (chp > -1 && present_id > -1){
		set_bet(pp, chp, present_id);
	}
	if (bot_bet_count_ < 30 && get_player_betcount() < 5){
		bot_random_bet();
	}
}

player_ptr beatuy_beast_logic::get_player( string uid )
{
	for(int i = 0; i < players_.size(); i++)
	{
		if (players_[i]->uid_ == uid){
			return players_[i];
		}
	}
	for (int i = 0; i < bots_.size(); i++)
	{
		if (bots_[i]->uid_ == uid)	{
			return bots_[i];
		}
	}
	return player_ptr();
}

void beatuy_beast_logic::join_bots()
{
	int count = rand_r(3, 5);
	vector<player_ptr> v;
	auto itf_join = the_service.bots_.begin();
	while (itf_join != the_service.bots_.end())
	{
		v.push_back(itf_join->second);
		itf_join++;
	}
	std::random_shuffle(v.begin(), v.end());
	for (int i = bots_.size(); i < count; i++)
	{
		if (!v.empty()){
			player_ptr pbot = *v.begin();
			bot_join(pbot);
			apply_banker(pbot);
			v.erase(v.begin());
			the_service.bots_.erase(pbot->uid_);
		}
	}
}

bool beatuy_beast_logic::is_bot( string uid )
{
	for(int i = 0; i < bots_.size(); i++)
	{
		if (bots_[i]->uid_ == uid)	{
			return true;
		}
	}
	return false;
}

int beatuy_beast_logic::get_player_betcount()
{
	map<string, string> vm;
	for (int i = 0 ; i < bets_.size(); i++)
	{
		if (!is_bot(bets_[i].uid_)){
			vm.insert(make_pair(bets_[i].uid_, bets_[i].uid_));
		}
	}
	return vm.size();
}

void beatuy_beast_logic::clear_bots()
{
	//取消机器人排庄
	auto it = bots_.begin();
	while (it != bots_.end()){
		//如果当前机器人在做庄，则不删除它
		if (*it == the_banker_){
			it++;
		}
		else{
			player_ptr pbot = *it;
			auto it_find = std::find(banker_applications_.begin(), banker_applications_.end(), pbot->uid_);
			if (it_find != banker_applications_.end()){
				msg_apply_banker_canceled msg;
				COPY_STR(msg.uid_, pbot->uid_.c_str());
				broadcast_msg(msg);
				banker_applications_.erase(it_find);
			}
			pbot->the_game_.reset();
			it = bots_.erase(it);
			the_service.bots_.insert(make_pair(pbot->uid_, pbot));
			break;//只删除一个机器人，否则太假了
		}
	}
}

bool beatuy_beast_logic::is_ingame(player_ptr pp)
{
	for (int i = 0 ; i < (int) players_.size(); i++)
	{
		if (players_[i]->uid_ == pp->uid_){
			return true;
		}
	}
	return false;
}

void beatuy_beast_logic::join_player(player_ptr pp)
{
}


void beatuy_beast_logic::broadcast_msg_to_7158(game_report_data report_)
{
	 //大于10倍才广播
	if(report_.factor_ < 10)
			return;

	 player_ptr pp = get_player(report_.uid_);

	 if(!pp.get())
		 return;

	 platform_broadcast_7158<beauty_beast_service, platform_config_7158>* http_bro = 
		 new platform_broadcast_7158<beauty_beast_service, platform_config_7158>(the_service, platform_config_7158::get_instance());
	 boost::shared_ptr<platform_broadcast_7158<beauty_beast_service, platform_config_7158>> pbro(http_bro);

	 char buff[512] = {0};

	 sprintf_s(buff,512,"获得了%d倍奖励，奖励金币为%lld",(int)report_.factor_,report_.actual_win_);
	 string s = boost::locale::conv::between( buff, "UTF-8", "GB2312" );  

	 http_bro->idx_ = pp->uidx_;
	 http_bro->nickname_ = pp->name_;
	 http_bro->content_ = s;
	 http_bro->do_broadcast();

	 /* msg_broadcast_msg msg;
	 msg.str_ = buff;
	 COPY_STR(msg.str_,s.c_str());
	 pp->send_msg_to_client(msg);
	 broadcast_msg(msg);*/

}

template<class service_t>
int state_machine<service_t>::enter()
{
	msg_state_change msg;
	msg.change_to_ = state_id_;
	msg.time_left_ = get_time_left();
	boost::shared_ptr<service_t> plogic = the_logic_.lock();
	if(plogic.get())
		plogic->broadcast_msg(msg);
	return ERROR_SUCCESS_0;
}
