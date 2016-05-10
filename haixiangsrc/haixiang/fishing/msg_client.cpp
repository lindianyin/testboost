#include "msg_client.h"
#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "utility.h"
#include "error_define.h"
#include "error.h"
#include <map>
#include "game_logic.h"
#include "telfee_match.h"
#include "platform_7158.h"
#include "platform_17173.h"
#include "platform_t58.h"
#include "game_service_base.hpp"
extern longlong	 g_cash_pool;
extern log_file<file_output> glb_http_log;
longlong			g_total_pay_ = 0;
extern std::map<std::string, longlong> g_todaywin;

longlong glb_costs[10] = {10000};

struct match_report_data
{
	longlong total_coins_;
	longlong total_exp_;
	std::map<std::string, int> mresult;
	match_report_data()
	{
		total_exp_ = total_coins_ = 0;
	}
};

std::map<std::string, match_report_data> glb_results_;

int msg_gun_switch_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if (plogic->is_match_game_ && plogic->the_match_.get())
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (level_ < 0 || level_ > 9) return SYS_ERR_CANT_FIND_CHARACTER;

	msg_gun_switch msg;
	msg.level_ = level_;
	msg.pos_ = pp->pos_;
	plogic->broadcast_msg(msg);

	pp->gun_level_ = level_;

	return ERROR_SUCCESS_0;
}

int msg_fire_at_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get())
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (plogic->run_state_ == 2)
		return GAME_IS_OVER;

	if (plogic->is_match_game_ && plogic->run_state_ == 0)
		return  GAME_ERR_NOT_START;

	if(plogic->the_match_.get()){
		if(plogic->the_match_->state_ != telfee_match_info::state_wait_balance){
			return SYS_ERR_CANT_FIND_CHARACTER;
		}
	}
	if ((boost::posix_time::microsec_clock::local_time() - pp->last_fire_).total_milliseconds() < 300){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	std::map<unsigned int, unsigned int>& lst = plogic->missiles_[pp->uid_];

	if (plogic->is_match_game_){
		unsigned int bet = 0;
		if (plogic->the_match_.get()){
			bet = pp->gun_level_ + 1;
		}
		else{
			bet = glb_costs[pp->gun_level_];
		}

		if (pp->free_coins_ < bet)
			return GAME_ERR_NOT_ENOUGH_CURRENCY;

		pp->free_coins_ -= bet;

		msg_deposit_change2 msg2;
		msg2.pos_ = pp->pos_;
		msg2.credits_ = pp->free_coins_;
		plogic->broadcast_msg(msg2);
		
		lst.insert(std::make_pair(missile_id_, bet));
	}
	else{
		int cost = glb_costs[pp->gun_level_];

		if (pp->coins_ < cost){//todo:
			msg_buy_coin_req msg;
			msg.dir_ = 0;
			msg.count_ = 1;//change by liufapu 100
			msg.from_sock_ = from_sock_;
			msg.handle_this();
			return ERROR_SUCCESS_0;
		}

		pp->add_coin(-cost);
		pp->total_pay_ += cost;

		if (!pp->is_bot_)	{
			g_todaywin[pp->uid_] -= cost;
			the_service.add_stock(cost);
			g_total_pay_ += cost;
			the_service.cache_helper_.async_cost_var(pp->uid_ + KEY_CUR_TRADE_CAP, -1, cost);
		}

		msg_deposit_change2 msg2;
		msg2.pos_ = pp->pos_;
		msg2.credits_ = pp->coins_;
		plogic->broadcast_msg(msg2);

		lst.insert(std::make_pair(missile_id_, cost));
	}

	msg_fire_at msg;
	msg.pos_ = pp->pos_;
	msg.x = x;
	msg.y = y;
	plogic->broadcast_msg(msg);
	pp->last_fire_ = boost::posix_time::microsec_clock::local_time();
	return ERROR_SUCCESS_0;
}

longlong	hit_fish(fish_ptr fh, player_ptr pp, logic_ptr plogic, int bet, int result, int sum = 0)
{
	longlong ret = 0;
	msg_fish_hitted msg;
	msg.iid_ = fh->iid_;
	msg.pos_ = pp->pos_;
	msg.result_ = result;
	//boos鱼小暴金币
	if (fh->data_ == 5 && result == 6){
		msg.data_ = sum + fh->reward_little_ * bet ;
	}
	else
		msg.data_ = sum + fh->reward_ * bet ;

	ret = msg.data_;
	plogic->broadcast_msg(msg);
	
	//如果是result == 4//连锁的鱼
	if (result == 4) return ret;

	auto itf = glb_results_.find(pp->uid_);
	if (itf == glb_results_.end()){
		glb_results_.insert(std::make_pair(pp->uid_, match_report_data()));
	}

	match_report_data& mresult = glb_results_[pp->uid_];
	if (!plogic->is_match_game_){
		mresult.mresult[fh->type_] += 1;
		mresult.total_coins_ += msg.data_;
		if (result != 6){
			pp->add_exp(fh->exp_);
			mresult.total_exp_ += fh->exp_;
		}

		pp->add_coin(msg.data_);
		pp->lucky_ = the_service.cache_helper_.async_add_var(pp->uid_ + KEY_LUCKY, IPHONE_FINSHING, msg.data_);

		if (!plogic->is_match_game_ && !pp->is_bot_){
			pp->total_win_ += msg.data_;
			g_todaywin[pp->uid_] += msg.data_;
			the_service.add_stock( -msg.data_ );
		}

		msg_deposit_change2 msg_dep;
		msg_dep.pos_ = pp->pos_;
		msg_dep.credits_ = pp->coins_;
		plogic->broadcast_msg(msg_dep);

		if (fh->reward_ >= 80 && result != 5 && result != 6){
			msg_broadcast_info msg_bro;
			msg_bro.at_where_ = 0;
			msg_bro.at_data_ = plogic->scene_set_->gurantee_set_;
			COPY_STR(msg_bro.who_, pp->name_.c_str());
			msg_bro.get_what_ = fh->reward_;
			msg_bro.get_what2_ = msg.data_;
			the_service.broadcast_msg(msg_bro);

			if (pp->platform_id_ == "7158"){
				platform_broadcast_7158<fishing_service, platform_config_7158>* http_bro = 
					new platform_broadcast_7158<fishing_service, platform_config_7158>(the_service, platform_config_7158::get_instance());
				boost::shared_ptr<platform_broadcast_7158<fishing_service, platform_config_7158>> pbro(http_bro);

				char buff[512] = {0};

				//全屏炸弹爆了
				if (fh->data_ != 2){
					int indx = rand_r(the_service.the_config_.broadcast_gold_.size() - 1);
					sprintf_s(buff, 512, the_service.the_config_.broadcast_gold_[indx].c_str(), 
						fh->reward_, msg.data_);
				}
				else{
					int indx = rand_r(the_service.the_config_.broadcast_bomb_.size() - 1);
					sprintf_s(buff, 512, the_service.the_config_.broadcast_bomb_[indx].c_str(), 
						msg.data_);
				}

				http_bro->idx_ = pp->uidx_;
				http_bro->nickname_ = pp->name_;
				http_bro->content_ = buff;
				http_bro->do_broadcast();
			}
		}
	}
	else{
		if (plogic->the_match_.get()){
			plogic->the_match_->add_score(pp->uid_, msg.data_);
		}

		pp->free_coins_ += msg.data_;

		msg_deposit_change2 msg;
		msg.pos_ = pp->pos_;
		msg.credits_ = pp->free_coins_;
		plogic->broadcast_msg(msg);
	}
	return ret;
}
extern fish_ptr		glb_boss;
void		record_fish_hitted(player_ptr pp, fish_ptr fh, longlong pay, longlong win)
{
	if (pp.get() && pp->is_bot_) return;

	BEGIN_INSERT_TABLE("log_fish_hitted");
	if (pp.get())
		SET_FIELD_VALUE("uid", pp->uid_);
	else{
		SET_FIELD_VALUE("uid", "none");
	}

	SET_FIELD_VALUE("fish_type", fh->type_);
	SET_FIELD_VALUE("fish_data", fh->data_);
	SET_FIELD_VALUE("rate", fh->reward_);
	SET_FIELD_VALUE("pay", pay);
	SET_FIELD_VALUE("win", win);
	SET_FIELD_VALUE("consume", fh->consume_gold_);
	if (pp.get())
		SET_FINAL_VALUE("iid", pp->iid_);
	else 
		SET_FINAL_VALUE("iid", 0);

	EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
};

int msg_hit_fish_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	
	auto itf = plogic->fishes_.find(iid_);
	if (itf == plogic->fishes_.end()) return SYS_ERR_CANT_FIND_CHARACTER;

	std::map<unsigned int, unsigned int>& lst = plogic->missiles_[pp->uid_];
	auto iti = lst.find(missile_id_);
	if (iti == lst.end()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	
	unsigned int bet = iti->second;
	lst.erase(iti);

	fish_ptr fh = itf->second;
	int r = rand_r(10000);

	if (!plogic->is_match_game_){
		int cheat = the_service.cheat_win();
		if (cheat == 1){
			r = 0x7FFFFFFF;
		}
		else if (cheat == -1){
			r = 0;
		}
	}

	//击中需要roll点<值
	int hitted = fh->rate_ * the_service.the_config_.rate_control_ * the_service.get_personal_rate(pp);
	//打中了
	if (r <= hitted){
		//机器人不打爆玩家锁定的鱼
		if (pp->is_bot_ && fh->lock_by_player_ > 0){
			return ERROR_SUCCESS_0;
		}
		//幸运小鱼,系统赢了钱，
		if (fh->reward_ < 10 && r < 50 && fh->data_ == 0){
			fh->reward_ = rand_r(1, 3) * 10;
			fh->data_ = 7;
		}

		if (fh->data_ == 0 || fh->data_ == 7){
			longlong c = hit_fish(fh, pp, plogic, bet, 0);
			plogic->fishes_.erase(itf);
			if (!plogic->is_match_game_){
				record_fish_hitted(pp, fh, bet, c);
			}
		}
		//连锁
		else if(fh->data_ == 1 || fh->data_ == 2 || fh->data_ == 3 || fh->data_ == 10){
			longlong	total_gain = 0;
			auto itf2 = plogic->fishes_.begin();
			while (itf2 != plogic->fishes_.end())
			{
				fish_ptr tfh = itf2->second;
				if (fh == tfh) {itf2++; continue;}

				if (fh->data_ == 1 && tfh->strid_ == fh->strid_ && tfh->data_ == fh->data_){
					total_gain += hit_fish(tfh, pp, plogic, bet, 4);
					itf2 = plogic->fishes_.erase(itf2);
				}
				else if (fh->data_ == 2){
					total_gain += hit_fish(tfh, pp, plogic, bet, 4);
					itf2 = plogic->fishes_.erase(itf2);
				}
				else if (fh->data_ == 3 && tfh->type_ == fh->type_){
					total_gain += hit_fish(tfh, pp, plogic, bet, 4);
					itf2 = plogic->fishes_.erase(itf2);
				}
				else if (fh->data_ == 10 && tfh->m_groupId == fh->m_groupId)
				{
					total_gain += hit_fish(tfh, pp, plogic, bet, 4);
					itf2 = plogic->fishes_.erase(itf2);
				}
				else{
					itf2++;
				}
			}
			longlong c = hit_fish(fh, pp, plogic, bet, fh->data_, total_gain);
			if (!plogic->is_match_game_){
				record_fish_hitted(pp, fh, bet, c);
			}
		}
		else if (fh->data_ == 5){
			if (fh->die_enable_ && !pp->is_bot_){
				longlong c = hit_fish(fh, pp, plogic, bet, 5, fh->lottery_);
				plogic->fishes_.erase(itf);
				if (!plogic->is_match_game_){
					record_fish_hitted(pp, fh, bet, c);
				}
				if (fh == glb_boss){
					for (int i = 0; i < (int)the_service.the_golden_games_.size(); i++)
					{
						logic_ptr lg = the_service.the_golden_games_[i];
						if (lg->is_match_game_) continue;

						if (lg != plogic)	{
							msg_fish_hitted msg;
							msg.iid_ = fh->iid_;
							msg.pos_ = -1;
							msg.result_ = 5;
							msg.data_ = c;
							lg->broadcast_msg(msg);
							lg->fishes_.clear();
						}

						msg_broadcast_info msg_bro;
						msg_bro.at_data_ = 0;
						msg_bro.at_where_ = 2;
						COPY_STR(msg_bro.who_, pp->name_.c_str());
						msg_bro.get_what_ = c;
						lg->broadcast_msg(msg_bro);
					}

					if (pp->platform_id_ == "7158"){
						platform_broadcast_7158<fishing_service, platform_config_7158>* http_bro = 
							new platform_broadcast_7158<fishing_service, platform_config_7158>(the_service, platform_config_7158::get_instance());
						boost::shared_ptr<platform_broadcast_7158<fishing_service, platform_config_7158>> pbro(http_bro);

						int indx = rand_r(the_service.the_config_.broadcast_boss_.size() - 1);
						char buff[512] = {0};

						sprintf_s(buff, 512, the_service.the_config_.broadcast_boss_[indx].c_str(), 
							c);

						http_bro->idx_ = pp->uidx_;
						http_bro->nickname_ = pp->name_;
						http_bro->content_ = buff;
						http_bro->do_broadcast();
					}
				}
				the_service.restore_random_spwan();
				glb_boss.reset();
			}
			else{
				//打boss鱼，测试第二概率,机器人只可以打中Boss鱼的小概率
				longlong c = hit_fish(fh, pp, plogic, bet, 6, 0);
				if (!pp->is_bot_){
					fh->consume_gold_ -= c;
				}
			}
		}
	}
	else{
		//如果打boss鱼，测试第二概率
		if (fh->data_ == 5){
			if (r < fh->rate_little_){
				longlong c = hit_fish(fh, pp, plogic, bet, 6, 0);
				if (!pp->is_bot_){
					fh->consume_gold_ -= c;
				}
			}
			//没打中，记录吸收量
			else if (!pp->is_bot_){
				fh->consume_gold_ += bet;
			}
		}
		//没打中，记录吸收量
		else if(!pp->is_bot_){
			fh->consume_gold_ += bet;
		}
	}
	return ERROR_SUCCESS_0;
}

int msg_buy_coin_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	
	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	if (plogic->is_match_game_){
		if (!plogic->the_match_.get()){
			if (count_ < 0) count_ = 5000;
			if (count_ > 5000) count_ = 5000;
			if (pp->free_coins_ >= 50000000)
				return ERROR_SUCCESS_0;

			pp->free_coins_ += count_ * glb_costs[pp->gun_level_];
			msg_deposit_change2 msg;
			msg.pos_ = pp->pos_;
			msg.credits_ = pp->free_coins_;
			plogic->broadcast_msg(msg);
		}
		return ERROR_SUCCESS_0;
	}
	//上分
	if (dir_ == 0 && plogic->scene_set_){

// 		if (count_ < 0) count_ = 1000;
// 		if (count_ > 1000) count_ = 1000;

		if (pp->coins_ >= count_/* * glb_costs[pp->gun_level_]*/)
			return ERROR_SUCCESS_0;

		//机器人上分，直接成功。
		if (pp->is_bot_){
			pp->coins_ += count_ /** glb_costs[pp->gun_level_]*/;
			msg_deposit_change2 msg;
			msg.pos_ = pp->pos_;
			msg.credits_ = pp->coins_;
			plogic->broadcast_msg(msg);
		}
		else{
			longlong out_count = 0;
			longlong cost = /*glb_costs[pp->gun_level_] * */count_;
			int ret = the_service.cache_helper_.apply_cost(pp->uid_, 0, cost, true);
			if (ret == ERROR_SUCCESS_0){
				pp->add_coin(cost);
				msg_deposit_change2 msg;
				msg.pos_ = pp->pos_;
				msg.credits_ = pp->coins_;
				plogic->broadcast_msg(msg);
			}
			else
				return GAME_ERR_NOT_ENOUGH_CURRENCY;
// 			if (ret == ERROR_SUCCESS_0){

// 			}
// 			//如果上的分数过大，变成上最高分数
// 			else{
// 				
// 			}
		}
	}
	//下分
	else if(plogic->scene_set_){
		//机器人不下分
		if (!pp->is_bot_){
			longlong cost = pp->coins_;
			longlong out_count = 0;
			int ret = the_service.cache_helper_.give_currency(pp->uid_, cost, out_count);
			if (ret == ERROR_SUCCESS_0){
				pp->add_coin(-cost);
				msg_deposit_change2 msg;
				msg.pos_ = pp->pos_;
				msg.credits_ = 0;
				plogic->broadcast_msg(msg);
			}
			else
				return ret;
		}
	}
	return 0;
}

int msg_get_match_report::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	msg_match_report msg;
	msg.total_exp_ = 0;
	msg.total_gold_ = 0;
	auto itf = glb_results_.find(pp->uid_);
	if (itf == glb_results_.end()){
		msg.count_ = 0;
	}
	else {
		match_report_data & mresult = itf->second;
		if (mresult.mresult.empty()){
			msg.count_ = 0;
		}
		else{
			msg.count_ = mresult.mresult.size();
			if (msg.count_ > 18) msg.count_ = 18;
			int i = 0;
			auto it = mresult.mresult.begin();
			while (it != mresult.mresult.end() && i < 18)
			{
				COPY_STR(msg.name_[i], it->first.c_str());
				msg.ncount_[i] = it->second;
				it++; i++;
			}
			msg.total_exp_ =	mresult.total_exp_;
			msg.total_gold_ = mresult.total_coins_;
		}
		glb_results_.erase(itf);
	}
	pp->send_msg_to_client(msg);
	return ERROR_SUCCESS_0;
}

int msg_lock_fish_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	if (!pp->is_bot_){
		if (iid_ >= 0){
			auto itf = plogic->fishes_.find(iid_);
			if (itf != plogic->fishes_.end()){
				itf->second->lock_by_player_++;
				pp->the_lockfish_ = itf->second;
			}
		}
		else {
			fish_ptr fh = pp->the_lockfish_.lock();
			if (fh.get()){
				fh->lock_by_player_--;
			}
		}
	}

	msg_lock_fish msg;
	msg.pos_ = pp->pos_;
	msg.iid_ = iid_;
	plogic->broadcast_msg(msg);
	return ERROR_SUCCESS_0;
}

int msg_enter_host_game_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	beauty_host* phs = nullptr;
	auto ith = the_service.beauty_hosts_.find(room_id_);
	if (ith != the_service.beauty_hosts_.end()){
		phs = &ith->second;
	}
	//如果不是主播
	else{
		beauty_host bh;
		bh.avroom_id_ = room_id_;
		the_service.beauty_hosts_.insert(std::make_pair(room_id_, bh));
		phs = &the_service.beauty_hosts_[room_id_];
	}

	logic_ptr plogic;
	auto itl = phs->games_.begin();

	while (itl != phs->games_.end())
	{
		boost::weak_ptr<fishing_logic> wlg = itl->second;		itl++;
		logic_ptr tlg = wlg.lock();
		if (!tlg.get()) continue;
		if (!plogic.get()){
			plogic = tlg;
		}
		if (tlg->get_playing_count() < MAX_SEATS && tlg->is_ingame(pp)){
			plogic = tlg;
			break;
		}
	}
	
	if (!plogic.get()){
		plogic.reset(new fishing_logic(0));
		plogic->scene_set_ = &the_service.scenes_[2];
		plogic->start_logic();
		plogic->is_host_game_ = true;
		//加入到金币场游戏列表是为了boss鱼能刷过来。
		the_service.the_golden_games_.push_back(plogic);
		plogic->the_host_ = the_service.get_player(phs->uid_);
		phs->games_.insert(std::make_pair(plogic->strid_, plogic));
	}

	logic_ptr ingame = pp->the_game_.lock();
	if (plogic != ingame && ingame.get()){
		ingame->leave_game(pp, pp->pos_, 1);
	}

	int r = plogic->player_login(pp, -1);

	return ERROR_SUCCESS_0;
}

int msg_get_host_list::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	auto ith = the_service.beauty_hosts_.begin();
	int TAG = 1;
	while (ith != the_service.beauty_hosts_.end())
	{
		beauty_host& bhs = ith->second; ith++;
		if (!bhs.screen_shoot_.empty()){
			msg_host_screenshoot msg;
			msg.iid_ = bhs.iid_;
			msg.TAG_ = 1;
			for (int i = 0 ; i < bhs.screen_shoot_.size();)
			{
				int dl = bhs.screen_shoot_.size() - i;
				dl = std::min<int>(512, dl);
				memcpy(msg.data_, bhs.screen_shoot_.data() + i, dl);
				i += dl;
				msg.len_ = dl;
				pp->send_msg_to_client(msg);
				msg.TAG_ = 0;
			}
			msg.TAG_ = 2;
			msg.len_ = 0;
			pp->send_msg_to_client(msg);
		}
		{
			msg_beauty_host_info msg;
			msg.host_id_ = bhs.iid_;
			COPY_STR(msg.ip_, bhs.avserver_ip_.c_str());
			msg.port_ = bhs.avserver_port_;
			msg.room_id_ = bhs.avroom_id_;
			msg.is_online_ = bhs.is_online_;
			msg.TAG_ = 0;
			msg.player_count_  = rand_r(10, 100);
			msg.popular_ = bhs.popular_;
			msg.TAG_ = TAG;
			TAG = 0;
			if (ith == the_service.beauty_hosts_.end()){
				msg.TAG_ |= 1 << 1;
			}
			COPY_STR(msg.host_name_, bhs.host_name_.c_str());
			pp->send_msg_to_client(msg);
		}
	}
	return 0;
}

int msg_upload_screenshoot::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	auto itf = the_service.beauty_hosts_.find(pp->iid_);
	if (itf == the_service.beauty_hosts_.end())
		return SYS_ERR_CANT_FIND_CHARACTER;

	beauty_host& bhs = itf->second;
	if ((TAG_ >> 16) == 1){
		bhs.temp_shoot_.clear();
		bhs.temp_shoot_.insert(bhs.temp_shoot_.end(), data_, data_ + len_);
	}
	else if((TAG_ >> 16) == 2){
		bhs.screen_shoot_.clear();
		bhs.screen_shoot_.insert(bhs.screen_shoot_.end(), bhs.temp_shoot_.begin(),  bhs.temp_shoot_.end());
		bhs.temp_shoot_.clear();
	}
	else{
		bhs.temp_shoot_.insert(bhs.temp_shoot_.end(), data_, data_ + len_);
	}
	return 0;
}

int msg_send_present_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get())
		return SYS_ERR_CANT_FIND_CHARACTER;

	auto the_host = plogic->the_host_.lock();
	if (!the_host.get())
		return SYS_ERR_CANT_FIND_CHARACTER;

	auto it = the_service.the_config_.beauty_host_presentset_.find(present_id_);
	if (it == the_service.the_config_.beauty_host_presentset_.end())
		return SYS_ERR_CANT_FIND_CHARACTER;

	if (plogic->the_host_.lock() == pp){
		return ERROR_SUCCESS_0;
	}

	longlong cost = it->second * count_;
	longlong left = 0;
	int ret = the_service.cache_helper_.apply_cost(pp->uid_, cost, left);
	if (ret != ERROR_SUCCESS_0)
		return GAME_ERR_NOT_ENOUGH_CURRENCY;


	BEGIN_INSERT_TABLE("beauty_hosts_present");
	SET_FIELD_VALUE("host_id", the_host->iid_);
	SET_FIELD_VALUE("uid", pp->uid_);
	SET_FIELD_VALUE("present_id", it->first);
	SET_FIELD_VALUE("count", count_);
	SET_FINAL_VALUE("cost", cost);
	EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);

	auto ith = the_service.beauty_hosts_.find(the_host->iid_);
	if (ith != the_service.beauty_hosts_.end()){
		ith->second.popular_ += cost / 100;

		msg_send_beauty_present msg;
		msg.present_id_ = it->first;
		msg.count_ = count_;
		msg.to_host_ = the_host->iid_;
		COPY_STR(msg.pos_, pp->name_.c_str());
		msg.popular_ = ith->second.popular_;
		ith->second.broadcast_msg(msg);
	}

	return ERROR_SUCCESS_0;
}

int msg_version_verify::handle_this()
{
	from_sock_->priv_flag_ = version_;
	return ERROR_SUCCESS_0;
}

int msg_host_start_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;

	auto ith = the_service.beauty_hosts_.find(pp->iid_);
	if (ith != the_service.beauty_hosts_.end()){
		beauty_host& bi = ith->second;
		msg_host_start msg;
		bi.broadcast_msg(msg);
	}
	return ERROR_SUCCESS_0;
}
