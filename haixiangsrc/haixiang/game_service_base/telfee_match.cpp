#include "telfee_match.h"
#include "msg_server_common.h"
#include "platform_7158.h"
#include "platform_17173.h"
#include "platform_t58.h"
#include "game_service_base.hpp"
std::string		generate_telfee_key()
{
	std::string ret;
	char A = 'A'; char Z = 'Z';
	for (int i = 0 ; i < 16; i++)
	{
		ret.push_back(rand_r(A, Z));
	}
	return ret;
}
extern std::vector<std::string> g_bot_names;
void telfee_match_info::enter_wait_register_state()
{
	if (auto_change_state_){
		match_wait_start_task* ptask = new match_wait_start_task(the_service.timer_sv_);
		current_schedule_.reset(ptask);
		ptask->the_match_ = shared_from_this();

		if (time_type_ == 1){
			if (first_run_){
				date_info dat = vsche_[indx_++];
				dat.d = 0;
				ptask->schedule_at_next_days(dat);
			}
			else{
				ptask->schedule_at_next_days(vsche_[indx_++]);
			}
			if (indx_ == vsche_.size()){
				indx_ = 0;
				first_run_ = 0;
			}
		}
		else{
			ptask->schedule(register_time_);
		}
	}
	init_scores_.clear();
	state_ = state_wait_register;
}

void telfee_match_info::enter_wait_start_state()
{
	if (state_ == state_wait_start_) return;
	match_wait_begin_task* ptask = new match_wait_begin_task(the_service.timer_sv_);
	current_schedule_.reset(ptask);
	ptask->the_match_ = shared_from_this();
	ptask->schedule(enter_time_);
	state_ = state_wait_start_;
}

void telfee_match_info::enter_wait_balance_state()
{
	match_wait_end_task* ptask = new match_wait_end_task(the_service.timer_sv_);
	current_schedule_.reset(ptask);
	ptask->the_match_ = shared_from_this();
	ptask->schedule(matching_time_);

	state_ = state_wait_balance;

	msg_time_left msg;
	msg.state_ = state_;
	msg.left_ = current_schedule_->time_left();
	for (int i = 0; i < (int)vgames_.size(); i++)
	{
		vgames_[i]->broadcast_msg(msg);
	}

	pt_last_refresh_ = boost::posix_time::microsec_clock::local_time();
}

logic_ptr telfee_match_info::get_free_game(player_ptr pp)
{
	std::vector<logic_ptr> vfree;
	for (auto g : vgames_)
	{
		if (g->get_playing_count(0) < MAX_SEATS && !g->is_ingame(pp)){
			vfree.push_back(g);
		}
	}
	if (!vfree.empty()){
		return vfree[rand_r(vfree.size() - 1)];
	}
	return logic_ptr();
}

void telfee_match_info::update()
{
	if (pt_last.is_not_a_date_time()){
		pt_last = boost::posix_time::microsec_clock::local_time();
	}

	boost::posix_time::ptime pt_now = boost::posix_time::microsec_clock::local_time();
	float dt = (pt_now - pt_last).total_milliseconds() / 1000.0;

	if (dt < 0.016) return;
	pt_last = pt_now;

	//找出不在其它游戏场内的玩家，进场
	if (state_ == state_wait_start_ || state_ == state_wait_balance){
		distribute_player_to_match();
		if ((pt_now - pt_last_refresh_).total_seconds() > 1){

			if (state_ == state_wait_balance)	{
				for (int i = 0; i < (int)vscores_.size(); i++)
				{
					if (vscores_[i].fake_ != 1) continue;
					match_score& ms = vscores_[i];
					extern int fake_telfee_score();
					ms.score_ += fake_telfee_score();
					if(ms.score_ < 0) ms.score_ = 0;
				}
			}

			std::sort(vscores_.begin(), vscores_.end(), 
				[](match_score& b1, match_score&b2)->int{return b1.score_ > b2.score_;});
			for (int i = 0 ; i < (int)vscores_.size(); i++)
			{
				match_score&sc = vscores_[i];
				player_ptr pp = the_service.get_player(sc.uid_);
				if (!pp.get()) continue;

				//通知客户端比赛进程
				msg_match_progress msg;
				msg.high_score_ = vscores_[0].score_;
				if (state_ == state_wait_balance){
					msg.my_rank_ = i + 1;
				}
				else{
					msg.my_rank_ = 0;
				}
				msg.my_score_ = sc.score_;
				msg.player_count_ = vscores_.size();
				pp->send_msg_to_client(msg);

				//通知KOKO服务器比赛进程
				msg_match_score msg_int;
				msg_int.match_id_ = iid_;
				msg_int.operator_ = 0;
				msg_int.score_ = sc.score_;
				COPY_STR(msg_int.uid_, pp->uid_.c_str());
				send_msg(the_service.world_connector_, msg_int);
			}
			pt_last_refresh_ = boost::posix_time::microsec_clock::local_time();
		}

		if (state_ == state_wait_balance || state_ == state_wait_start_){
			for (auto g : vgames_)
			{
				g->update(dt);
			}
		}
	}
	else if(state_ == state_wait_register){
		if ((pt_now - pt_last_refresh_).total_seconds() > 5){
			pt_last_refresh_ = boost::posix_time::microsec_clock::local_time();
		}
	}
}

void telfee_match_info::add_score(std::string uid, int score)
{
	for (auto& s : vscores_)
	{
		if (s.uid_ == uid){
			s.score_ += score;//牛牛分数不是加上去的
			return;
		}
	}
	match_score sc;
	sc.uid_ = uid;
	sc.score_ = score;
	vscores_.push_back(sc);
}

//分发玩家到比赛中
std::vector<logic_ptr> telfee_match_info::distribute_player_to_match()
{
	std::vector<logic_ptr> ret;
	auto itn = registers_.begin();
	while (itn != registers_.end())
	{
		player_ptr pp = the_service.get_player(itn->first);
		if (!pp.get()){
			itn++;
			continue;
		}
		
		auto its = init_scores_.find(pp->platform_uid_);
		if (its == init_scores_.end()){
			itn++;
			continue;
		}
		//int意义: 0初始，1询问已发送,2同意，3不同意
		if (itn->second == 0){
			msg_ask_for_join_match msg;
			msg.match_id_ = iid_;
			pp->send_msg_to_client(msg);
			itn->second = 1;
		}
		else if (itn->second == 2){

			logic_ptr logic = pp->the_game_.lock();
			if (logic.get()){
				logic->leave_game(pp, pp->pos_, 4);
			}

			if (its->second.first == ""){
				logic = get_free_game(pp);
			}
			else{
				logic = get_specific_game(its->second.first);
			}
			
			if (!logic.get()){
				logic.reset(new logic_ptr::element_type(1));
				logic->the_match_ = shared_from_this();
				vgames_.push_back(logic);
				logic->strid_ = its->second.first;
				logic->scene_set_ = &scene_set_;
				logic->start_logic();
			}

			pp->free_coins_ = 500000;
			logic->player_login(pp, -1);
			add_score(pp->uid_, its->second.second);
		}
		
		if (itn->second > 1){
			itn = registers_.erase(itn);
		}
		else{
			itn++;
		}
	}
	return ret;
}

void telfee_match_info::register_this(std::string uid)
{
	replace_map_v(registers_, std::make_pair(uid, 0));
}

void telfee_match_info::do_balance()
{
	for (auto g : vgames_)
	{
		g->stop_logic();
	}
	vgames_.clear();

	int rank = 0; 
	//需要排序一下
	for (auto vs : vscores_)
	{
		rank++;
		if (vs.fake_ == 1){
			//如果是虚假机器人第一名了。
			if (rank == 1){
				msg_broadcast_info msg_bro;
				msg_bro.at_where_ = 1;
				msg_bro.at_data_ = iid_;
				if (!g_bot_names.empty()){
					COPY_STR(msg_bro.who_, g_bot_names[rand_r(g_bot_names.size() - 1)].c_str());
				}
				else{
					COPY_STR(msg_bro.who_, "unknown");
				}
				msg_bro.get_what_ = iid_;
				msg_bro.get_what2_ = prizes_[0];
	#if FISHING_7158
				platform_broadcast_7158<service_type, platform_config_7158>* http_bro = 
					new platform_broadcast_7158<service_type, platform_config_7158>(the_service, platform_config_7158::get_instance());
				boost::shared_ptr<platform_broadcast_7158<service_type, platform_config_7158>> pbro(http_bro);
				if (!the_service.the_config_.broadcast_telfee_.empty())	{
					int indx = rand_r(the_service.the_config_.broadcast_telfee_.size() - 1);
					char buff[512] = {0};

					sprintf_s(buff, 512, the_service.the_config_.broadcast_telfee_[indx].c_str(), 
						prizes_[0]);

					http_bro->idx_ = "1";
					http_bro->nickname_ = msg_bro.who_;
					http_bro->content_ = buff;
					http_bro->do_broadcast();

				}

				the_service.broadcast_msg(msg_bro);
	#endif
			}
			continue;
		}
		player_ptr pp = the_service.get_player(vs.uid_);
		if (!pp.get()) continue;

		//通知KOKO服务器比赛进程
		msg_match_score msg_int;
		msg_int.match_id_ = iid_;
		msg_int.operator_ = 0 | (1 << 16); //比赛结束同步分标志
		msg_int.score_ = vs.score_;
		COPY_STR(msg_int.uid_, pp->uid_.c_str());
		send_msg(the_service.world_connector_, msg_int);

		msg_match_result msg;
		msg.rank_ = rank;
		if (rank <= 6 && rank < prizes_.size()){
			msg.money_ = prizes_[rank - 1];
		}
		else{
			msg.money_ = 0;
		}

		longlong outc = 0;
		if (msg.money_ > 0){
			the_service.add_stock(-msg.money_);
			int r = the_service.cache_helper_.give_currency(pp->uid_, msg.money_, outc);
			Database& db = *the_service.gamedb_;
			BEGIN_INSERT_TABLE("log_send_gold_detail");
			SET_FIELD_VALUE("reason", 4);
			SET_FIELD_VALUE("uid", vs.uid_);
			SET_FINAL_VALUE("val", msg.money_);
			EXECUTE_IMMEDIATE();
		}

		if (rank < 6){
			std::string ky = generate_telfee_key();
			COPY_STR(msg.key_, ky.c_str());
			BEGIN_INSERT_TABLE("telfee_wins");
			SET_FIELD_VALUE("uid", pp->uid_);
			SET_FIELD_VALUE("iid",pp->iid_);
			SET_FIELD_VALUE("win", msg.money_);
			SET_FINAL_VALUE("key", ky);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}

		if(rank == 1){
			msg.telfee_ =  iid_;

			msg_broadcast_info msg_bro;
			msg_bro.at_where_ = 1;
			msg_bro.at_data_ = iid_;
			COPY_STR(msg_bro.who_, pp->name_.c_str());
			msg_bro.get_what_ = iid_;
			msg_bro.get_what2_ = msg.money_;
			the_service.broadcast_msg(msg_bro);
	#if FISHING_7158
			platform_broadcast_7158<service_type, platform_config_7158>* http_bro = 
				new platform_broadcast_7158<service_type, platform_config_7158>(the_service, platform_config_7158::get_instance());
			boost::shared_ptr<platform_broadcast_7158<service_type, platform_config_7158>> pbro(http_bro);
			if (!the_service.the_config_.broadcast_telfee_.empty())	{
				int indx = rand_r(the_service.the_config_.broadcast_telfee_.size() - 1);
				char buff[512] = {0};

				sprintf_s(buff, 512, the_service.the_config_.broadcast_telfee_[indx].c_str(), 
					msg.money_);

				http_bro->idx_ = pp->uidx_;
				http_bro->nickname_ = pp->name_;
				http_bro->content_ = buff;
				http_bro->do_broadcast();

			}
	#endif
		}else{
			msg.telfee_ = 0;
			COPY_STR(msg.key_, "");
		}
		pp->send_msg_to_client(msg);
	}
	vscores_.clear();
}

void telfee_match_info::enter_wait_end_state()
{
	//koko调用和自身状态机切换时调用，避免调用两次
	if (state_ != state_wait_end){
		do_balance();
		match_wait_register_task* ptask = new match_wait_register_task(the_service.timer_sv_);
		current_schedule_.reset(ptask);
		ptask->the_match_ = shared_from_this();
		ptask->schedule(2000);
		state_ = state_wait_end;
	}
}

logic_ptr telfee_match_info::get_specific_game(std::string strid)
{
	for (auto g : vgames_)
	{
		if (g->get_playing_count(0) < MAX_SEATS && g->strid_ == strid){
			return g;
		}
	}
	return logic_ptr();
}

int match_wait_start_task::routine()
{
	the_match_->enter_wait_start_state();
	return routine_ret_break;
}

int match_wait_register_task::routine()
{
	the_match_->enter_wait_register_state();
	return routine_ret_break;
}

int match_wait_begin_task::routine()
{
	the_match_->enter_wait_balance_state();
	return routine_ret_break;
}

int match_wait_end_task::routine()
{
	the_match_->enter_wait_end_state();
	return routine_ret_break;
}
