#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "db_delay_helper.h"
#include <boost/locale/conversion.hpp>
#include <boost/locale/encoding.hpp>


void broadcast_17173_msg(player_ptr pp, int level, longlong money) 
{
	platform_broadcast_17173< horse_racing_service::this_type, platform_config_173 >* broadcast = 
		new platform_broadcast_17173< horse_racing_service::this_type, platform_config_173 >(the_service, platform_config_173::get_instance());

	boost::shared_ptr< platform_broadcast_17173< horse_racing_service::this_type, platform_config_173 > > pbroadcast(broadcast);


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

unsigned int state_wait_start::get_time_total()
{
	return the_service.the_config_.wait_start_time_;
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
	state_id_ = GET_CLSID(state_do_random);
}

unsigned int state_do_random::get_time_total()
{
	return the_service.the_config_.do_random_time_;
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
	state_id_ = GET_CLSID(state_rest_end);
}

unsigned int state_rest_end::get_time_total()
{
	return the_service.the_config_.wait_end_time_;
}
static int get_id()
{
	static int gid = 1;
	return gid++;
}

horse_racing_logic::horse_racing_logic(int is_match)
{
	scene_set_ = NULL;
	is_waiting_config_ = false;
	strid_ = get_guid();
	turn_ = 0;
	this_turn_result_ = NULL;
	horse_group gr;
	for (int i = 1; i < 7; i++)
	{
		gr.horse1_ = i;
		for (int j = i + 1; j < 7; j++)
		{
			gr.horse2_ = j;
			group_sets_.push_back(gr);
		}
	}
	id_ = get_id();
}

void horse_racing_logic::broadcast_msg( msg_base<none_socket>& msg )
{
	auto it = players_.begin();
	while (it != players_.end())
	{
		player_ptr pp = *it; it++;
		pp->send_msg_to_client(msg);
	}
}

vector<float>	calc_speed(vector<int> rank)
{
	vector<float> ret;
	if(the_service.the_config_.riding_time_ < 1000) the_service.the_config_.riding_time_ = 15000;
	float avg_speed = the_service.the_config_.riding_road_length_ / float(the_service.the_config_.riding_time_ / 1000.0);
	for (int i = 0 ; i < (int)rank.size(); i++)
	{
		float speed1 = avg_speed * (0.8 + 0.3 * (rand_r(100) / 100.0));
		float speed2 = avg_speed * (0.8 + 0.3 * (rand_r(100) / 100.0));						//2 * ((the_service.the_config_.riding_road_length_) / 4.0 - rand_r(50)) / (the_service.the_config_.riding_time_ / 4.0)  - speed1;
		float speed3 = avg_speed;																									//2 * ((the_service.the_config_.riding_road_length_) / 4.0 - rand_r(200)) / (the_service.the_config_.riding_time_ / 4.0)  - speed2;
		float l = ((speed1 + speed2) / 2  + (speed2 + speed3) / 2) * (the_service.the_config_.riding_time_ /4000.0);
		float speed4 = 0;
		//前两名的区别大一点
		if(i < 2)
			speed4 = 2 * ((the_service.the_config_.riding_road_length_ - l) - i * (50 + rand_r(30))) / (the_service.the_config_.riding_time_ / 2000.0) - speed3;
		//手面的区别小一点
		else
			speed4 = 2 * ((the_service.the_config_.riding_road_length_ - l) - (120 + (i - 2) * 30 + rand_r(20))) / (the_service.the_config_.riding_time_ / 2000.0) - speed3;

		ret.push_back(speed1);
		ret.push_back(speed2);
		ret.push_back(speed3);
		ret.push_back(speed4);
	}
	return ret;
}

void horse_racing_logic::random_result()
{
	if (presets_.empty()) return;
	int		cheat_win = 0;
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

	map<int, preset_present> lowest_bets;
	map<int, longlong> summury = sum_bets();
	if(cheat_win == 1){
		auto it_present = presets_.begin();
		while (it_present != presets_.end())
		{
			//如果没有人压注
			if(summury.find((*it_present).pid_) == summury.end()){
				lowest_bets.insert(make_pair((*it_present).pid_, *it_present));
			}
			it_present++;
		}
		//如果没有压全，只有空格大于
		if(lowest_bets.size() > 0 && lowest_bets.size() < 6){
			glb_log.write_log("=========>not all betted, force normal random, benefits banker canceled!");
			cheat_win = 0;
		}
	}

	if (summury.empty()){
		cheat_win = 0;
	}

	//正常局
	if (cheat_win == 0){
		auto it_present = presets_.begin();
		while (it_present != presets_.end())
		{
			lowest_bets.insert(make_pair((*it_present).pid_, *it_present));
			it_present++;
		}
		if (!lowest_bets.empty()){
			do_random(lowest_bets);
		}
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

		if(lowest_bets.empty()){
			for (int i = 0 ; i < v.size(); i++)
			{
				lowest_bets.insert(make_pair(v[i], presets_[v[i]]));
			}
		}

		if (!lowest_bets.empty()){
			do_random(lowest_bets);
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
		if(lowest_bets.empty()){
			for (int i = 0 ; i < v.size(); i++)
			{
				lowest_bets.insert(make_pair(v[i], presets_[v[i]]));
			}
		}
		if (!lowest_bets.empty()){
			do_random(lowest_bets);
		}
	}

	if(!this_turn_result_){
		glb_log.write_log("===========> random_result() cant random a proper result, the presets config could be wrong!");
		this_turn_result_ = &(*presets_.begin());
	}


	vhorse.push_back(1); vhorse.push_back(2); vhorse.push_back(3); 
	vhorse.push_back(4); vhorse.push_back(5); vhorse.push_back(6);
	std::random_shuffle(vhorse.begin(), vhorse.end());
	if(this_turn_result_->pid_ < group_sets_.size()){
		vhorse.erase(std::find(vhorse.begin(), vhorse.end(), group_sets_[this_turn_result_->pid_].horse1_));
		vhorse.erase(std::find(vhorse.begin(), vhorse.end(), group_sets_[this_turn_result_->pid_].horse2_));
		if (rand_r(100) < 50){
			ranks_.push_back(group_sets_[this_turn_result_->pid_].horse2_);
			ranks_.push_back(group_sets_[this_turn_result_->pid_].horse1_);
		}
		else{
			ranks_.push_back(group_sets_[this_turn_result_->pid_].horse1_);
			ranks_.push_back(group_sets_[this_turn_result_->pid_].horse2_);
		}
		for (int i = 0; i < vhorse.size(); i++)
		{
			ranks_.push_back(vhorse[i]);
		}
		turn_info ti;
		ti.present_id_ = this_turn_result_->pid_;
		ti.time_ = time(NULL);
		ti.turn_ = turn_;
		ti.factor_ = this_turn_result_->factor_;
		ti.first_ = ranks_[0];
		ti.second_ = ranks_[1];
		last_random_.insert(last_random_.begin(), ti);
		speeds_ = calc_speed(ranks_);

		msg_rand_result msg;
		build_rand_result(msg);
		broadcast_msg(msg);
	}
}
void horse_racing_logic::build_rand_result(msg_rand_result& msg)
{
	msg.rand_result_ = this_turn_result_->pid_;
	msg.ranks_ = ranks_;
	msg.speeds_ = speeds_;
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

int horse_racing_logic::set_bet( player_ptr pp, int pid, int present_id )
{
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (the_service.the_config_.chips_.find(pid) == the_service.the_config_.chips_.end()){
		return GAME_ERR_CANT_FIND_CHIP_SET;
	}

	if (present_id < 0 || present_id >= presets_.size())
		return GAME_ERR_CANT_FIND_CHIP_SET;

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
	group_sets_[present_id].betted_++;
	longlong out_count = 0;
// 	get_today_win(pp->uid_, out_count);
// 	if ((out_count < 0 && abs(out_count) > the_service.the_config_.max_lose_)){
// 		return GAME_ERR_LOSE_CAP_EXCEED;
// 	}
// 	else if ((out_count > 0 && abs(out_count) > the_service.the_config_.max_win_)){
// 		return GAME_ERR_WIN_CAP_EXCEED;
// 	}


	out_count = 0;
	if(the_service.apply_cost(pp->uid_, this_time_bet, out_count) == ERROR_SUCCESS_0){
		bet_info bet;
		bet.bet_count_ = this_time_bet;
		bet.pid_ = present_id;
		bet.uid_ = pp->uid_;
		bets_.push_back(bet);

		Database& db(*the_service.gamedb_);
		BEGIN_INSERT_TABLE("set_bets");
		SET_STRFIELD_VALUE("uid", pp->uid_);
		SET_FIELD_VALUE("pid", pid);
		SET_FIELD_VALUE("turn", turn_);
		SET_FIELD_VALUE("present_id", present_id);
		SET_FIELD_VALUE("bet_count", this_time_bet);
		SET_STRFINAL_VALUE("game_id", strid_);
		EXECUTE_IMMEDIATE();

		msg_currency_change msg;
		msg.credits_ = out_count;
		pp->send_msg_to_client(msg);
		
		msg_player_setbet msg_set;
		msg_set.pid_ = pid;
		msg_set.present_id_ = present_id;
		msg_set.max_setted_ = get_total_bets(present_id);
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

void horse_racing_logic::do_random(map<int, preset_present>& vpr)
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
			this_turn_result_ = &presets_[pr.pid_];		//必须是一个长久对象不能是pr,因为vpr是临时的
			break;
		}
		else{
			rnd -= pr.rate_;
		}
	}
}
void horse_racing_logic::this_turn_start()
{
	turn_++;
	vhorse.clear();
	presets_.clear();
	speeds_.clear();
	ranks_.clear();
	auto it_preset = the_service.the_config_.presets_.begin();
	while (it_preset != the_service.the_config_.presets_.end())
	{
		presets_.push_back(it_preset->second);
		it_preset++;
	}

	std::random_shuffle(presets_.begin(), presets_.end());
	msg_presets_change msg;
	for (int i = 0 ; i < presets_.size(); i++)
	{
		presets_[i].pid_ = i;
		msg.pids_.push_back(i);
		msg.factors_.push_back(presets_[i].factor_);
	}
	broadcast_msg(msg);
	
}

void horse_racing_logic::balance_result()
{
	longlong player_bets_ = 0;
	longlong player_total_win_ = 0;
	//统计庄家收到下注和
	for (int i = 0; i < bets_.size(); i++)
	{
		bet_info& bi = bets_[i];
		player_bets_ += bi.bet_count_;
		game_report_data rpt;
		rpt.uid_ = bi.uid_;
		rpt.present_id_ = bi.pid_;
		rpt.pay_ = bi.bet_count_;
		turn_reports_.push_back(rpt);
	}

	for (int i = 0; i < bets_.size(); i++)
	{
		bet_info& bi = bets_[i];
		//如果中奖
		if (bi.pid_ == this_turn_result_->pid_)	{
			record_winner(bi, i);
		}
	}
	bets_.clear();

	map<string, game_report_data> summury;
	//应用输赢
	auto it_rpt = turn_reports_.begin();
	while (it_rpt != turn_reports_.end())
	{
		game_report_data& grd = *it_rpt;		it_rpt++;
		player_ptr pp = the_service.get_player(grd.uid_);
		game_report_data& sum = summury[grd.uid_];
		sum.uid_ = grd.uid_;
		sum.pay_ += grd.pay_;
		sum.actual_win_ += grd.actual_win_;
		sum.should_win_ += grd.should_win_;

		player_total_win_ += grd.actual_win_;
		{
			Database& db(*the_service.gamedb_);
			BEGIN_INSERT_TABLE("log_player_win");
			SET_STRFIELD_VALUE("game_id", strid_);
			SET_STRFIELD_VALUE("uid", grd.uid_);
			SET_FIELD_VALUE("turn", turn_);
			SET_FIELD_VALUE("pay", grd.pay_);
			SET_FIELD_VALUE("random_result", this_turn_result_->pid_);
			SET_FIELD_VALUE("factor", this_turn_result_->factor_);
			SET_FIELD_VALUE("present_id", grd.present_id_);
			SET_FINAL_VALUE("win", grd.actual_win_);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}
	}
	turn_reports_.clear();

	auto it_sum = summury.begin();
	while (it_sum != summury.end())
	{
		game_report_data& grd = it_sum->second;		it_sum++;
		player_ptr pp = the_service.get_player(grd.uid_);
		if(!pp.get()) continue;
		longlong out_count = 0;
		if(the_service.give_currency(grd.uid_, grd.actual_win_, out_count) != ERROR_SUCCESS_0){
			glb_log.write_log("give currency error in balance_winner! uid = %s, count = %u", grd.uid_.c_str(), grd.actual_win_);
		}
		else{
			if(grd.actual_win_ > 0){
				msg_currency_change msg;
				msg.credits_ = out_count;
				if (pp.get()){
					pp->send_msg_to_client(msg);
				}
			}
		}
		//发送报表到客户端

		msg_game_report msg;
		msg.actual_win_ = grd.actual_win_;
		msg.pay_ = grd.pay_;
		msg.should_win_ = grd.should_win_;
		pp->send_msg_to_client(msg);

		
		if(pp->platform_id_ == "live"  || pp->platform_id_ == "show"){
			if(this_turn_result_->factor_ >= 15){
				broadcast_17173_msg(pp, (int)this_turn_result_->factor_, grd.actual_win_);
			}
		}
	}

	{
		Database& db(*the_service.gamedb_);
		BEGIN_INSERT_TABLE("log_gameturn_info");
		SET_STRFIELD_VALUE("game_id", strid_);
		if (this_turn_result_){
			SET_FIELD_VALUE("random_result", this_turn_result_->db_pid_);
		}
		SET_FIELD_VALUE("banker_deposit", 0);
		SET_FIELD_VALUE("this_turn_win", 0);
		SET_FIELD_VALUE("total_players", summury.size());
		SET_FIELD_VALUE("total_bets", player_bets_);
		SET_FIELD_VALUE("total_player_win", player_total_win_);
		SET_FIELD_VALUE("turn", turn_);
		SET_FINAL_VALUE("total_tax", 0);
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
	}

}

void horse_racing_logic::will_shut_down()
{

}

void horse_racing_logic::change_to_state( ms_ptr ms )
{
	if (current_state_.get()){
		current_state_->leave();
	}
	current_state_ = ms;
	current_state_->enter();
}


void horse_racing_logic::record_winner( bet_info& bi, int i)
{
	longlong should_win = longlong(bi.bet_count_) * presets_[bi.pid_].factor_;
	game_report_data& rpt = turn_reports_[i];
	rpt.actual_win_ = should_win;
	rpt.should_win_ = should_win;
}

int horse_racing_logic::can_bet(int pid, unsigned int this_time_bet)
{
	if (current_state_->state_id_ != GET_CLSID(state_wait_start)){
		return GAME_ERR_CANT_BET_NOW;
	}
	return ERROR_SUCCESS_0;
}

void horse_racing_logic::player_login( player_ptr pp, int pos )
{
	players_.push_back(pp);
	pp->the_game_ = shared_from_this();

	msg_server_parameter msg;
	msg.racing_time_ = the_service.the_config_.riding_time_;
	msg.road_length_ = the_service.the_config_.riding_road_length_;
	auto it2 = the_service.the_config_.chips_.begin();
	int i = 0;
	while (it2 != the_service.the_config_.chips_.end() && i < 5)
	{
		msg.chip_id_[i] = it2->first;
		msg.chip_cost_[i] = it2->second;
		it2++;
		i++;
	}
	pp->send_msg_to_client(msg);

	if (current_state_->state_id_ == GET_CLSID(state_do_random)){
		msg_rand_result msg;
		build_rand_result(msg);
		pp->send_msg_to_client(msg);
	}

	//发送游戏状态机
	{
		msg_state_change msg;
		msg.change_to_ = current_state_->state_id_;
		msg.time_left = current_state_->get_time_left();
		msg.time_total_ = current_state_->get_time_total();
		pp->send_msg_to_client(msg);
	}
	//发送已开奖项
	msg_last_random msg_rd;
	for (int i = 0; i < last_random_.size() && i  < 10; i++)
	{
		msg_rd.r_[i] = last_random_[i].present_id_;
		msg_rd.time_[i] = last_random_[i].time_;
		msg_rd.turn_[i] = last_random_[i].turn_;
		msg_rd.factor_[i] = last_random_[i].factor_;
		msg_rd.first_[i] = last_random_[i].first_;
		msg_rd.second_[i] = last_random_[i].second_;
	}
	pp->send_msg_to_client(msg_rd);

	//发送赔率
	{
		msg_presets_change msg;
		for (int i = 0 ; i < presets_.size(); i++)
		{
			presets_[i].pid_ = i;
			msg.pids_.push_back(i);
			msg.factors_.push_back(presets_[i].factor_);
		}
		pp->send_msg_to_client(msg);
	}

	//发送人气马
	{
		msg_horse_popular msg;
		for (int i = 0; i < group_sets_.size(); i++)
		{
			msg.pop_[group_sets_[i].horse1_ - 1] += group_sets_[i].betted_;
			msg.pop_[group_sets_[i].horse2_ - 1] += group_sets_[i].betted_;
		}
		pp->send_msg_to_client(msg);
	}
}

void horse_racing_logic::leave_game( player_ptr pp, int pos, int why )
{
	pp->the_game_.reset();
	auto itf = std::find(players_.begin(), players_.end(), pp);
	if(itf != players_.end())
		players_.erase(itf);
}

void horse_racing_logic::start_logic()
{
	change_to_wait_start();
}

map<int, longlong> horse_racing_logic::sum_bets()
{
	map<int, longlong>  ret;
	for (int i = 0 ; i < bets_.size(); i++)
	{
		ret[bets_[i].pid_] += longlong(bets_[i].bet_count_) * presets_[bets_[i].pid_].factor_;
	}
	return ret;
}

int horse_racing_logic::get_today_win(string uid, longlong& win )
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

longlong horse_racing_logic::get_total_bets( int pid )
{
	longlong	total = 0;
	for (int i = 0; i < bets_.size(); i++)
	{
		bet_info& bet = bets_[i];
		if (bet.pid_ == pid){
			total	+= bet.bet_count_;
		}
	}
	return total;
}

template<class service_t>
int state_machine<service_t>::enter()
{
	msg_state_change msg;
	msg.change_to_ = state_id_;
	msg.time_left = get_time_left();
	msg.time_total_	= get_time_total();
	boost::shared_ptr<service_t> plogic = the_logic_.lock();
	if(plogic.get())
		plogic->broadcast_msg(msg);
	return ERROR_SUCCESS_0;
}
