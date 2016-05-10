#include "game_logic.h"
#include "msg_client.h"
#include "msg_server.h"
#include "service.h"
#include "error.h"
#include "db_delay_helper.h"
#include "schedule_task.h"
#include "boost/date_time/posix_time/conversion.hpp"
#include "telfee_match.h"
extern log_file<cout_output> glb_log;

using namespace boost;
using namespace boost::posix_time;
using namespace boost::gregorian;

std::map<std::string, longlong>	g_todaywin;
std::map<std::string, longlong>	g_debug_performance_total;	//每个函数执行时间累计
std::map<std::string, longlong>	g_debug_performance_count;
std::map<std::string, longlong>	g_debug_performance_avg;

static int get_id()
{
	static int gid = 1;
	return gid++;
}

fish_ptr		glb_boss;

class bot_quit_game : public task_info
{
public:
	logic_ptr plogic;

	bot_quit_game(io_service& ios) : task_info(ios)
	{

	}
	virtual	int	routine()
	{
		vector<int> vrand;
		for (int i = 0; i < MAX_SEATS; i++)
		{
			if (!plogic->is_playing_[i].get()) continue;
			if (!plogic->is_playing_[i]->is_bot_) continue;
			vrand.push_back(i);
		}
		if (!vrand.empty()){
			int indx = vrand.size() - 1;
			if (!plogic->is_match_game_){
				indx = rand_r(vrand.size() - 1);
			}
			player_ptr pp = plogic->is_playing_[vrand[indx]];
			pp->on_connection_lost();

			the_service.players_.erase(pp->uid_);
			the_service.free_bots_.insert(std::make_pair(pp->uid_, pp));
		}
		plogic->is_removing_bot_ = 0;
		return routine_ret_break;
	}
};


fishing_logic::fishing_logic(int is_match)
{
	is_waiting_config_ = true;
	id_ = get_id();
	strid_ = get_guid();
	turn_ = 0;
	scene_set_ = NULL;
	is_match_game_ = is_match;
	run_state_ = 0;
	is_adding_bot_ = 0, is_removing_bot_ = 0;
	is_host_game_ = 0;
}

void fishing_logic::broadcast_msg( msg_base<none_socket>& msg, int exclude_pos)
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (i == exclude_pos) continue;
		is_playing_[i]->send_msg_to_client(msg);
	}
}

class bot_rejoin_game : public task_info
{
public:
	logic_ptr plogic;
	bot_rejoin_game(io_service& ios) : task_info(ios)
	{

	}

	virtual	int	routine()
	{
		player_ptr pp = the_service.pop_onebot(plogic);
		if (pp.get())	{
			if(plogic->player_login(pp) != ERROR_SUCCESS_0){
				the_service.free_bots_.insert(std::make_pair(pp->uid_, pp));
			}
			else{
				the_service.players_.insert(std::make_pair(pp->uid_, pp));
				if (plogic->the_match_.get())	{
					plogic->the_match_->add_score(pp->uid_, 0);
				}
			}
		}
		plogic->is_adding_bot_ = 0;
		return routine_ret_break;
	}
};

int get_score_prize(player_ptr pp);

class bot_take_prize_task : public task_info
{
public:
	player_ptr pp;
	bot_take_prize_task(boost::asio::io_service& ios) : task_info(ios)
	{

	}
	int		routine()
	{
		get_score_prize(pp);
		return routine_ret_break;
	}
};

void fishing_logic::will_shut_down()
{
	for (int i = 0; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		is_playing_[i]->on_connection_lost();
		leave_game(is_playing_[i], i, 2);
	}
}

void			get_credits(msg_currency_change& msg, player_ptr pp, fishing_logic* logic)
{
	msg.credits_ = (double) the_service.cache_helper_.get_currency(pp->uid_);
}
extern int get_level(longlong& exp);

int fishing_logic::player_login( player_ptr pp, int seat)
{
	if (seat >= MAX_SEATS) return -1;
	//坐位满了
	if (get_playing_count() == MAX_SEATS && seat == -1){
		return GAME_ERR_CANT_SIT;
	}
	else{
		{
			msg_game_info msg;
			COPY_STR(msg.game_id_, strid_.c_str());
			msg.extra_data1_ = cur_map_;
			pp->send_msg_to_client(msg);
		}

		{
			msg_fish_used msg;
			msg.count_ = scene_set_->the_fish_config_.size();
			auto it = scene_set_->the_fish_config_.begin();
			int i = 0;
			while (it != scene_set_->the_fish_config_.end())
			{
				fish_config_ptr pcf = it->second; it++;
				COPY_STR(msg.type_[i++], pcf->type_.c_str());
			}
			if (!is_match_game_){
				msg.count_ += the_service.the_config_.the_fish_config_.size();
				auto it = the_service.the_config_.the_fish_config_.begin();
				int i = 0;
				while (it != the_service.the_config_.the_fish_config_.end())
				{
					fish_config_ptr pcf = it->second; it++;
					COPY_STR(msg.type_[i++], pcf->type_.c_str());
				}
			}
			pp->send_msg_to_client(msg);
		}

		pp->the_game_ = shared_from_this();
		
		{
			msg_prepare_enter msg;
			if (the_match_.get()){
				msg.match_id_ = the_match_->iid_;
			}
			else{
				msg.match_id_ = 0;
			}
			msg.extra_data1_ = cur_map_;
			pp->send_msg_to_client(msg);
		}

		if (is_match_game_){
			pp->gun_level_ = 9;
		}

		pp->join_count_++;
		if (seat == -1){
			for (int i = 0 ; i < MAX_SEATS; i++)
			{
				if (!is_playing_[i].get() || 
					(is_playing_[i].get() && is_playing_[i]->uid_ == pp->uid_)){
					is_playing_[i] = pp;
					pp->pos_ = i;
					break;
				}
			}
			//广播玩家坐下位置
			msg_player_seat msg;
			msg.pos_ = pp->pos_;
			COPY_STR(msg.uname_, pp->name_.c_str());
			COPY_STR(msg.uid_, pp->uid_.c_str());
			COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
			msg.gun_level_ = pp->gun_level_;
			COPY_STR(msg.gun_type_, pp->gun_type_.c_str());
			msg.lv_ = pp->lv_;
			broadcast_msg(msg, pp->pos_);
		}
		else{
			is_playing_[seat] = pp;
			pp->pos_ = seat;
			//广播玩家坐下位置
			msg_player_seat msg;
			msg.pos_ = pp->pos_;
			COPY_STR(msg.uname_, pp->name_.c_str());
			COPY_STR(msg.uid_, pp->uid_.c_str());
			COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
			msg.gun_level_ = pp->gun_level_;
			COPY_STR(msg.gun_type_, pp->gun_type_.c_str());
			msg.lv_ = pp->lv_;
			broadcast_msg(msg, pp->pos_);
		}
	}

	pp->coins_ = the_service.cache_helper_.get_var<longlong>(pp->uid_ + KEY_FISHCOINBK, IPHONE_FINSHING);
	return ERROR_SUCCESS_0;
}

void fishing_logic::leave_game( player_ptr pp, int pos, int why )
{
	msg_player_leave msg;
	msg.pos_ = pos;
	msg.why_ = why;
	broadcast_msg(msg);
	
	missiles_.erase(pp->uid_);

	if (!is_match_game_){
		restore_account(pp);
	}
	else{
		if (the_match_.get() && why != 2 ){
			for (int i = 0 ; i < (int)the_match_->vscores_.size(); i++)
			{
				if (the_match_->vscores_[i].uid_ == pp->uid_){
					the_match_->vscores_.erase(the_match_->vscores_.begin() + i);
					break;
				}
			}
		}
		pp->free_coins_ = 0;
	}
	
	if (pp->pos_ >= 0)	{
		is_playing_[pp->pos_].reset();
	}

	pp->reset_temp_data();

	if (pp->update_task_.get()){
		pp->update_task_->cancel();
	}
}

void fishing_logic::start_logic()
{
	run_state_ = 1;	
	cur_map_ = 1;

	//没有世界boss,或是比赛场时，随机刷新鱼
	if (!glb_boss.get() || is_match_game_){
		task_for_spawn_freely_generate* pgen = new task_for_spawn_freely_generate(the_service.timer_sv_);
		pgen->logic_ = shared_from_this();
		pgen->routine();
	}
	else if (glb_boss.get()){
		join_fish(glb_boss);
	}

	//加入机器人
	bot_rejoin_game join(the_service.timer_sv_);
	join.plogic = shared_from_this();
	int r = rand_r(1, 4);
	for (int i = 0; i < r; i++)
	{
		join.routine();
	}
}

int fishing_logic::get_today_win(string uid, __int64& win )
{
	longlong n = g_todaywin[uid];
	win = n;
	return ERROR_SUCCESS_0;
}

void fishing_logic::stop_logic()
{
	run_state_ = 2;
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->is_bot_){
			the_service.free_bots_.insert(std::make_pair(is_playing_[i]->uid_, is_playing_[i]));
		}
		leave_game(is_playing_[i], i, 2);
	}
	the_cloud_generator_.reset();
}

bool fishing_logic::is_ingame( player_ptr pp )
{
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i].get()) continue;
		if (is_playing_[i]->uid_ == pp->uid_){
			return true;
		}
	}
	return false;
}

void fishing_logic::join_fish(fish_ptr fh)
{
	fishes_.insert(std::make_pair(fh->iid_, fh));
	if (fh->data_ != 5){
		fh->move_to_next();
	}
	sync_fish(fh, player_ptr());	
}
extern void		record_fish_hitted(player_ptr pp, fish_ptr fh, longlong pay, longlong win);
int fishing_logic::update(float dt)
{
	if (the_service.the_config_.shut_down_){
		will_shut_down();
		return 1;
	}
	if (run_state_ == 2) return 0;

	adjust_bot();

	map<int, fish_ptr> cpy = fishes_;
	auto fh = cpy.begin();
	while (fh != cpy.end())
	{
		//boss鱼不在这里更新
		if (fh->second->data_ == 5) {
			fh++;
			continue;
		}
		if (fh->second->update(dt) == 1) {
			auto itf = fishes_.find(fh->first);
			if (itf != fishes_.end()) {
				msg_fish_clean msg;
				msg.iid_ = itf->second->iid_;
				broadcast_msg(msg);

				//如果有鱼没有被打暴，记录下来以便统计
				if (itf->second->consume_gold_ > 0 && !is_match_game_) {
					record_fish_hitted(player_ptr(), itf->second, 0, 0);
				}
				fishes_.erase(itf);
			}
		}
		fh++;
	}

	if (the_cloud_generator_.get()){
		the_cloud_generator_->update(dt);
	}

	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (!is_playing_[i]) continue;
		if (is_playing_[i]->last_fire_.is_not_a_date_time()){
			is_playing_[i]->last_fire_ = posix_time::microsec_clock::local_time();
		}
		else {
			if (is_playing_[i]->is_connection_lost_){
				leave_game(is_playing_[i], i, 0);
			}
			//2分钟没打炮，T出
			else if ((posix_time::microsec_clock::local_time() - is_playing_[i]->last_fire_).total_seconds() > 120 && ((
				is_playing_[i] != the_host_.lock() && is_host_game_) || !is_host_game_)){
				if (is_playing_[i]->is_bot_){
					the_service.free_bots_.insert(std::make_pair(is_playing_[i]->uid_, is_playing_[i]));
				}
				leave_game(is_playing_[i], i, 3);
			}
		}
	}
	return 0;
}

void fishing_logic::sync_fish(fish_ptr fh, player_ptr pp)
{
	msg_fish_sync msg;
	msg.iid_ = fh->iid_;
	msg.mv_step_ = fh->mv_step_;
	msg.data_ = fh->data_;
	COPY_STR(msg.fish_type_, fh->type_.c_str());
	msg.reward_ = fh->reward_;
	if (fh->pmov_.get()){
		msg.speed_ = fh->pmov_->get_acc();
	}
	else{
		msg.speed_ = 0;
	}
	msg.lottery_ = fh->lottery_;
	msg.x = fh->pos_.x;
	msg.y = fh->pos_.y;
	if (fh->pmov_.get()){
		msg.percent_ = fh->pmov_->get_mov_percent();
	}
	else{
		msg.percent_ = 1.0;
	}
	msg.path_count_ = fh->path_.size();
	int i = 0;
	for (auto pt : fh->path_){
		msg.path_.push_back(pt.x);
		msg.path_.push_back(pt.y);
	}
	if (pp.get()){
		pp->send_msg_to_client(msg);
	}
	else{
		broadcast_msg(msg);
	}
}

class update_player_data : public task_info
{
public:
	update_player_data() : task_info(the_service.timer_sv_)
	{

	}
	boost::weak_ptr<fish_player>		the_player_;
	int		routine()
	{
		player_ptr pp = the_player_.lock();
		if (!pp.get())  return task_info::routine_ret_break;

		logic_ptr plogic = pp->the_game_.lock();
		if (!plogic.get()) return task_info::routine_ret_break;
		if (!pp->is_bot_){
			the_service.cache_helper_.set_var(pp->uid_ + KEY_FISHCOINBK, IPHONE_FINSHING, pp->coins_);
		}
		{
			BEGIN_REPLACE_TABLE("rank");
			SET_FIELD_VALUE("uid", pp->uid_);
			SET_FIELD_VALUE("uname", pp->name_);
			SET_FIELD_VALUE("rank_type", 2);
			SET_FINAL_VALUE("rank_data", pp->lucky_);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}
		if((pp->total_win_ != 0 || pp->total_pay_ != 0) && !pp->is_bot_){
			BEGIN_INSERT_TABLE("log_player_win");
			SET_FIELD_VALUE("uid", pp->uid_);
			SET_FIELD_VALUE("uname", pp->name_);
			SET_FIELD_VALUE("iid_", pp->iid_);
			SET_FIELD_VALUE("win", pp->total_win_);
			SET_FINAL_VALUE("pay", pp->total_pay_);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
			pp->total_pay_ = 0; pp->total_win_ = 0;
		}
		return task_info::routine_ret_continue;
	}
};

void fishing_logic::join_player(player_ptr pp)
{
	if (is_match_game_){
		if (the_match_.get() && the_match_->current_schedule_.get()){
			msg_time_left msg;
			msg.state_ = the_match_->state_;
			msg.left_ = the_match_->current_schedule_->time_left();
			pp->send_msg_to_client(msg);
		}
	}

	if (!is_match_game_ && scene_set_){
		msg_server_parameter msg;
		msg.is_free_ = scene_set_->is_free_;
		msg.enter_scene_ = 1;
		msg.min_guarantee_ = scene_set_->gurantee_set_;
		msg.low_cap_ = scene_set_->rate_;
		pp->send_msg_to_client(msg);

		if (!pp->update_task_.get()){
			update_player_data* ptask = new update_player_data();
			task_ptr task(ptask);
			ptask->the_player_ = pp;
			pp->update_task_ = task;
			ptask->schedule(10000);
		}
	}

	{
		pp->last_fire_ = boost::posix_time::ptime();
		{
			msg_player_seat msg;
			msg.pos_ = pp->pos_;
			COPY_STR(msg.uname_, pp->name_.c_str());
			COPY_STR(msg.uid_, pp->uid_.c_str());
			COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
			msg.gun_level_ = pp->gun_level_;
			COPY_STR(msg.gun_type_, pp->gun_type_.c_str());
			msg.lv_ = pp->lv_;
			pp->send_msg_to_client(msg);
		}
		{
			msg_deposit_change2 msg;
			msg.pos_ = pp->pos_;
			if (is_match_game_){
				msg.credits_ = pp->free_coins_;
			}
			else{
				msg.credits_ = pp->coins_;
			}
			pp->send_msg_to_client(msg);
		}
	}

	//向玩家广播游戏场内玩家
	for (int i = 0 ; i < MAX_SEATS; i++)
	{
		if (is_playing_[i].get()){

			if(pp != is_playing_[i]){
				//玩家坐下位置
				{
					msg_player_seat msg;
					msg.pos_ = i;
					COPY_STR(msg.uname_, is_playing_[i]->name_.c_str());
					COPY_STR(msg.uid_, is_playing_[i]->uid_.c_str());
					COPY_STR(msg.head_pic_, is_playing_[i]->head_ico_.c_str());
					msg.gun_level_ = is_playing_[i]->gun_level_;
					COPY_STR(msg.gun_type_, is_playing_[i]->gun_type_.c_str());
					msg.lv_ = is_playing_[i]->lv_;
					pp->send_msg_to_client(msg);
				}

				{
					msg_deposit_change2 msg;
					msg.pos_ = i;
					if (is_match_game_){
						msg.credits_ = is_playing_[i]->free_coins_;
					}
					else{
						msg.credits_ = is_playing_[i]->coins_;
					}
					pp->send_msg_to_client(msg);
				}
			}
		}
	}
	//向玩家广播所有鱼
	{
		auto it = fishes_.begin();
		while (it != fishes_.end())
		{
			sync_fish(it->second, pp);
			it++;
		}
	}

	auto the_host = the_host_.lock();
	//如果是主播房
	if (!the_host.get()){
		msg_beauty_host_info msg;
		msg.host_id_ = -1;
		COPY_STR(msg.ip_, "");
		msg.port_ = 0;
		msg.room_id_ = 0;
		msg.TAG_ = 8;
		msg.player_count_ = 0;
		msg.popular_ = 0;
		msg.is_online_ = 0;
		pp->send_msg_to_client(msg);
	}
	else{
	}
}

void fishing_logic::restore_account(player_ptr pp)
{
	longlong out_count = 0;
	if (!pp->is_bot_){
		int ret = the_service.cache_helper_.give_currency(pp->uid_, pp->coins_, out_count);
		if (ret == ERROR_SUCCESS_0){
			pp->add_coin(-pp->coins_);
		}
	}
	else
		pp->coins_ = 0;

	//add by bergkamp
	if (!pp->is_bot_){
		the_service.cache_helper_.set_var(pp->uid_ + KEY_FISHCOINBK, IPHONE_FINSHING, pp->coins_);
	}
}

void fishing_logic::fishes_runout()
{
	auto fh = fishes_.begin();
	while (fh != fishes_.end())
	{
		if (fh->second->pmov_.get()) {
			fh->second->pmov_->speed_up(16.0);
			fh->second->acc_ = 16.0;

			std::list<Point> path;
			the_service.update_fish_path(fh->second, path, shared_from_this());
		}
		fh++;
	}
}

void fishing_logic::adjust_bot()
{
	int cnt_player = get_playing_count(1);
	int cnt_bot = get_playing_count(2);
	if (!is_removing_bot_){
		bot_quit_game* ptask = new bot_quit_game(the_service.timer_sv_);
		task_ptr task(ptask);
		ptask->plogic = shared_from_this();
		ptask->schedule(1000 * rand_r(10, 300));
		is_removing_bot_ = 1;
	}

	if(!is_adding_bot_){
		bot_rejoin_game* ptask = new bot_rejoin_game(the_service.timer_sv_);
		task_ptr task(ptask);
		ptask->schedule(1000 * rand_r(5 * (cnt_bot + cnt_player), 20 * (cnt_bot + cnt_player)));
		ptask->plogic = shared_from_this();
		is_adding_bot_ = 1;
	}
}

extern fcloud_ptr create_cloud_from_xml(std::string xml, Point offset, logic_ptr plogic);
int task_for_spawn_cloud_generate::routine()
{
	logic_ptr plogic = logic_.lock();
	if (plogic.get()){
		int r = rand_r(10000); int n = 0;
		cloud_config* pnf = nullptr;
		auto itf = plogic->scene_set_->the_cloud_config_.begin();
		while (itf != plogic->scene_set_->the_cloud_config_.end())
		{
			cloud_config& cnf = itf->second;				itf++;
			if (n > r){
				pnf = &cnf;
				break;
			}
			n += cnf.rate_;
		}

		if (!(pnf || plogic->scene_set_->the_cloud_config_.empty()))	{
			pnf = &plogic->scene_set_->the_cloud_config_.begin()->second;
		}

		if (pnf){
			plogic->the_cloud_generator_ = create_cloud_from_xml(pnf->type_, Point(0, 0), plogic);
			plogic->the_cloud_generator_->the_logic_ = plogic;
			plogic->fishes_.clear();
		}

		task_for_clean_fish_for_random_spawn* pcl = new task_for_clean_fish_for_random_spawn(the_service.timer_sv_);
		task_ptr ptask(pcl);
		pcl->logic_ = plogic;
		pcl->schedule(20000);
		plogic->current_spwan_ = ptask;
	}
	return task_info::routine_ret_break;
}

int task_for_clean_fish_for_cloud_spawn::routine()
{
	logic_ptr plogic = logic_.lock();
	if (plogic.get()){
		plogic->the_cloud_generator_.reset();
		msg_fish_cloud_coming msg;
		msg.type_ = 1;
		msg.map_id_ = rand_r(1, 6);
		
		plogic->cur_map_ = msg.map_id_;

		plogic->broadcast_msg(msg);
		plogic->fishes_runout();

		task_for_spawn_cloud_generate* psch = new task_for_spawn_cloud_generate(the_service.timer_sv_);
		task_ptr ptask(psch);
		psch->logic_ = plogic;
		psch->schedule(1000);
		plogic->current_spwan_ = ptask;
	}
	return routine_ret_break;
}

int task_for_clean_fish_for_random_spawn::routine()
{
	logic_ptr plogic = logic_.lock();
	if (plogic.get()){
		//如果到执行时间了还没有结束，最多等5秒结束
		if (plogic->the_cloud_generator_.get() && 
				plogic->the_cloud_generator_->cloud_type_ != fish_cloud::cloud_free &&
				plogic->the_cloud_generator_->is_finished_ == false ||
				wait_time_ < 10000){
					delay_ = 500;
					wait_time_ += 500;
					return routine_ret_continue;
		}
		else{
			plogic->the_cloud_generator_.reset();

			msg_fish_cloud_coming msg;
			msg.type_ = 0;
			msg.map_id_ = rand_r(1, 6);

			plogic->cur_map_ = msg.map_id_;

			plogic->broadcast_msg(msg);
			plogic->fishes_runout();

			task_for_spawn_freely_generate* psch = new task_for_spawn_freely_generate(the_service.timer_sv_);
			task_ptr ptask(psch);
			psch->logic_ = plogic;
			psch->schedule(1000);
			plogic->current_spwan_ = ptask;
			return routine_ret_break;
		}
	}
	else {
		return routine_ret_break;
	}
}

int task_for_spawn_freely_generate::routine()
{
	logic_ptr plogic = logic_.lock();
	if (plogic.get()){
		fish_freely_generator* gen = new fish_freely_generator();
		gen->the_logic_ = plogic;
		plogic->the_cloud_generator_.reset(gen);
		plogic->fishes_.clear();

		task_for_clean_fish_for_random_spawn* pcl = new task_for_clean_fish_for_random_spawn(the_service.timer_sv_);
		task_ptr ptask(pcl);
		pcl->logic_ = plogic;
		boost::posix_time::minutes mins(5);
		pcl->schedule(mins.total_milliseconds());
		plogic->current_spwan_ = ptask;
	}
	return routine_ret_break;
}

int task_for_spwan_global_boss_generate::routine()
{
	do 
	{
		std::vector<fish_ptr> v = fish_freely_generator::spawn(the_service.the_config_.the_fish_config_,
			the_service.the_config_.the_path_, false);

		int ii = 0;
		while (v.empty() && ii < 100)
		{
			v = fish_freely_generator::spawn(the_service.the_config_.the_fish_config_,
				the_service.the_config_.the_path_, false);
			ii++;
		}

		glb_boss = v[0];

		glb_boss->lottery_ = glb_boss->consume_gold_ * the_service.the_config_.lottery_rate_ + glb_boss->reward_*1000;//change from 500000 by lfp test 1000


		//应该是先不用刷新一次
// 		longlong lot = glb_boss->consume_gold_ * the_service.the_config_.lottery_rate_;
// 		if (lot < 0) lot = 0;
// 
// 		__int64 lotty_inc_ = glb_boss->reward_*1000;//boss鱼的初始彩金 test 1000
// 		lotty_inc_ += rand_r(the_service.the_config_.lottery_min_, the_service.the_config_.lottery_max_);
// 
// 		if (lotty_inc_ > 5000000){
// 			lotty_inc_ = 5000000;
// 		}
// 
// 		if (glb_boss->lottery_ < lot + lotty_inc_){
// 			glb_boss->lottery_  = lot + lotty_inc_;
// 		}


		if (glb_boss->lottery_ < 0) 
			glb_boss->lottery_ = 0;

		glb_boss->rate_little_ = 2000;
		glb_boss->reward_little_ = 1;
// 		glb_boss->rate_ = 4;
// 		glb_boss->reward_ = 500;
		glb_boss->die_enable_ = 0;//不允许死亡
		glb_boss->consume_gold_ = 0;
		glb_boss->move_to_next();
		for (int j = 0; j < (int)the_service.the_golden_games_.size(); j++)
		{
			logic_ptr plogic = the_service.the_golden_games_[j];
			if (plogic->is_match_game_) continue;
			plogic->join_fish(glb_boss);
		}

	} while (0);

	{
		task_for_enable_boss_die* die = new task_for_enable_boss_die(the_service.timer_sv_);
		task_ptr task(die);
		die->the_wboss_ = glb_boss;
		boost::posix_time::seconds scnd1(20), scnd2(140);
		die->schedule(rand_r(scnd1.total_milliseconds(), scnd2.total_milliseconds()));
	}

	{
		task_for_forece_boss_die* die = new task_for_forece_boss_die(the_service.timer_sv_);
		task_ptr task(die);
		die->the_wboss_ = glb_boss;
		boost::posix_time::minutes mins(5);
		die->schedule(mins.total_milliseconds());
	}

	{
		task_for_adjust_rate* die = new task_for_adjust_rate(the_service.timer_sv_);
		task_ptr task(die);
		die->the_wboss_ = glb_boss;
		boost::posix_time::seconds scnd(5);//5秒钟更新一次彩金
		die->schedule(scnd.total_milliseconds());
	}

	{
		task_for_clean_fish_for_boss_spwan* die = new task_for_clean_fish_for_boss_spwan(the_service.timer_sv_);
		task_ptr task(die);
		boost::posix_time::minutes mins(30);//test:30
		die->schedule(mins.total_milliseconds());
	}
	
	return routine_ret_break;
}

int task_for_clean_fish_for_boss_spwan::routine()
{
	for (int i = 0; i < (int)the_service.the_golden_games_.size(); i++)
	{
		logic_ptr plogic = the_service.the_golden_games_[i];
		if (plogic->is_match_game_) continue;

		if(plogic->current_spwan_.get()){
			//取消当前鱼刷新
			plogic->current_spwan_->cancel();
			plogic->current_spwan_.reset();
		}
		if (plogic->the_cloud_generator_.get()){
			plogic->the_cloud_generator_.reset();
		}
		msg_fish_cloud_coming msg;
		msg.type_ = 2;
		msg.map_id_ = rand_r(1, 6);

		plogic->cur_map_ = msg.map_id_;

		plogic->broadcast_msg(msg);
		plogic->fishes_runout();
	}

	task_for_spwan_global_boss_generate* psch = new task_for_spwan_global_boss_generate(the_service.timer_sv_);
	task_ptr ptask(psch);
	psch->schedule(1000);
	return routine_ret_break;
}

int task_for_enable_boss_die::routine()
{
	fish_ptr the_boss_ = the_wboss_.lock();
	if (the_boss_.get() && the_boss_ == glb_boss){
		the_boss_->die_enable_ = 1;
	}
	return routine_ret_break;
}

int task_for_forece_boss_die::routine()
{
	fish_ptr the_boss_ = the_wboss_.lock();
	if (the_boss_.get() && the_boss_ == glb_boss){
		the_service.boss_runout();
	}
	return routine_ret_break;
}

int fake_telfee_score()
{
	return rand_r(20, 500);
}

int task_for_adjust_rate::routine()
{
	fish_ptr the_boss_ = the_wboss_.lock();
	if (!the_boss_.get() || the_boss_ != glb_boss)
		return task_info::routine_ret_break;

	longlong lot = the_boss_->consume_gold_ * the_service.the_config_.lottery_rate_;
	if (lot < 0) lot = 0;

	lotty_inc_ = the_boss_->reward_*1000;//boss鱼的初始彩金 test 1000
	lotty_inc_ += rand_r(the_service.the_config_.lottery_min_, the_service.the_config_.lottery_max_);

	if (lotty_inc_ > 5000000){
		lotty_inc_ = 5000000;
	}

	if (the_boss_->lottery_ < lot + lotty_inc_){
		the_boss_->lottery_  = lot + lotty_inc_;
	}

	msg_boss_lottery_change msg;
	msg.lottery_ = the_boss_->lottery_;
	for (int i = 0; i < the_service.the_golden_games_.size(); i++)
	{
		if (the_service.the_golden_games_[i]->is_match_game_) continue;
		the_service.the_golden_games_[i]->broadcast_msg(msg);
	}
	return task_info::routine_ret_continue;
}
