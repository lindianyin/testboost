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
#include "game_service_base.hpp"

extern longlong	 g_cash_pool;
extern std::map<std::string, niuniu_score> g_scores;
extern	int glb_everyday_set[7];
extern  int	match_prize[5][7];

int msg_set_bets_req::handle_this()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("msg_set_bets_req")
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	if (count_ <= 0 )
		return GAME_ERR_NOT_ENOUGH_BET;

	int ret = pgame->set_bet(pp, count_);
DEBUG_COUNT_PERFORMANCE_END()
	return ret;
}

int msg_query_to_become_banker_rsp::handle_this()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("msg_query_to_become_banker_rsp");
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;
	
	for (unsigned int i = 0 ; i < pgame->query_bankers_.size(); i++)
	{
		if (pgame->query_bankers_[i].uid_ == pp->uid_){
			pgame->query_bankers_[i].replied_ = agree_;
		}
	}
	msg_query_banker_rsp msg;
	msg.agree_ = agree_;
	COPY_STR(msg.uid_, pp->uid_.c_str());
	pgame->broadcast_msg(msg);
DEBUG_COUNT_PERFORMANCE_END()
	return ERROR_SUCCESS_0;
}

int msg_want_sit_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if (pos_ < 0 || pos_ >= MAX_SEATS) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if(pgame->is_playing_[pos_].get()) return GAME_ERR_CANT_SIT;

	pgame->is_playing_[pos_] = pp;
	pp->pos_ = pos_;

	auto itf = std::find(pgame->observers_.begin(), pgame->observers_.end(), pp);
	if(itf != pgame->observers_.end())
		pgame->observers_.erase(itf);

	msg_player_seat msg;
	msg.pos_ = pos_;
	COPY_STR(msg.uname_, pp->name_.c_str());
	COPY_STR(msg.uid_, pp->uid_.c_str());
	COPY_STR(msg.head_pic_, pp->head_ico_.c_str());
	pgame->broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}

int msg_open_card_req::handle_this()
{
DEBUG_COUNT_PERFORMANCE_BEGIN("msg_open_card_req")
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;

	niuniu_card c1 = niuniu_card::generate(card3_[0]);
	niuniu_card c2 = niuniu_card::generate(card3_[1]);
	niuniu_card c3 = niuniu_card::generate(card3_[2]);
	niuniu_card c4 = niuniu_card::generate(card2_[0]);
	niuniu_card c5 = niuniu_card::generate(card2_[1]);

	if(give_up_ == 0) {
		//看牌有没有重复
		std::map<int, int> m;
		m[card3_[0]] = 1;
		if (m.find(card3_[1]) == m.end()){
			m[card3_[1]] = 1;
		}
		else{
			SYS_ERR_CANT_FIND_CHARACTER;
		}

		if (m.find(card3_[2]) == m.end()){
			m[card3_[2]] = 1;
		}
		else{
			SYS_ERR_CANT_FIND_CHARACTER;
		}

		if (m.find(card2_[0]) == m.end()){
			m[card2_[0]] = 1;
		}
		else{
			SYS_ERR_CANT_FIND_CHARACTER;
		}

		if (m.find(card2_[0]) == m.end()){
			m[card2_[0]] = 1;
		}
		else{
			SYS_ERR_CANT_FIND_CHARACTER;
		}

		//看玩家有没有这张牌
		if (!pp->has_card(c1) )
			return SYS_ERR_CANT_FIND_CHARACTER;
		if (!pp->has_card(c2) )
			return SYS_ERR_CANT_FIND_CHARACTER;
		if (!pp->has_card(c3) )
			return SYS_ERR_CANT_FIND_CHARACTER;
		if (!pp->has_card(c4) )
			return SYS_ERR_CANT_FIND_CHARACTER;
		if (!pp->has_card(c5) )
			return SYS_ERR_CANT_FIND_CHARACTER;
	}

	zero_match_result zm;
	//自动开牌
	if (give_up_ == 2 || give_up_ == 3){
		zm = calc_niuniu_point(pp->turn_cards_);
	}
	else if (give_up_ == 1){
		zm = calc_niuniu_point(pp->turn_cards_);
	}
	else{
		zm.c1 = c1;
		zm.c2 = c2;
		zm.c3 = c3;
		zm.c4 = c4;
		zm.c5 = c5;
		vector<niuniu_card> v0 = seek_zero(c1, c2, c3);

		if(v0.empty()){
			zm.calc_point_ = 0;
		}
		else{
			zm.calc_point_ = seek_point(c4, c5);
		}

		if (zm.calc_point_ == 0){
			zm = calc_niuniu_point(pp->turn_cards_);
		}
	}
	zm.calc_niuniu_level();
	//提示牌型
	if (give_up_ == 3){
		extern void	build_match_result_msg(msg_card_match_result& msg, player_ptr pp, int pos, zero_match_result& zm );
		msg_card_match_result_test msg;
		build_match_result_msg(msg, pp, pp->pos_, zm);
		pp->send_msg_to_client(msg);
	}
	else{
		pp->match_result_ = zm;
		pgame->broadcast_match_result(pp, pp->pos_);
	}
DEBUG_COUNT_PERFORMANCE_END()
	return ERROR_SUCCESS_0;
}

int get_score_prize(player_ptr pp)
{
	if (g_scores[pp->uid_].score_ < the_service.the_config_.base_score_ * (1 + 2 + 4 + 8 + 16)){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	msg_random_present_ret msg;
	int n = rand_r(the_service.the_config_.random_present_.size() - 1);
	auto it = the_service.the_config_.random_present_.begin();
	for (int i = 0; i < n; i++){
		it++;
	}
	msg.result_ = n;
	COPY_STR(msg.uid_, pp->uid_.c_str());
	COPY_STR(msg.uname_, pp->name_.c_str());
	msg.gold_get_ = (longlong)g_cash_pool * (it->second / 10000.0);
	the_service.broadcast_msg(msg);

	g_cash_pool -= (longlong)msg.gold_get_;

	//the_service.add_stock((longlong)msg.gold_get_);

	g_scores[pp->uid_].score_ = 0;

	logic_ptr plogic = pp->the_game_.lock();
	longlong out_count = 0;
	if (plogic.get()){
		int ret = the_service.cache_helper_.give_currency(pp->uid_, (longlong)msg.gold_get_, out_count, plogic->the_banker_ != pp );
		if (ret == ERROR_SUCCESS_0)	{
			msg_niuniu_score msg_score;
			msg_score.base_score_ = the_service.the_config_.base_score_;
			msg_score.pos_ = pp->pos_;
			msg_score.score = 0;
			plogic->broadcast_msg(msg_score);

			msg_cash_pool msg_pool;
			msg_pool.credits_ =(double)g_cash_pool;
			plogic->broadcast_msg(msg_pool);

			msg_currency_change msg_cur;
			msg_cur.credits_ = msg.gold_get_;
			msg_cur.why_ = msg_currency_change::why_rnd_result;
			pp->send_msg_to_client(msg_cur);
		}
	}
	return ERROR_SUCCESS_0;
}

int msg_start_random_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;

	if (view_present_ == 1){
		msg_random_present_info msg;
		auto it = the_service.the_config_.random_present_.begin();
		int i = 0;
		while (it != the_service.the_config_.random_present_.end())
		{
			msg.percents_[i] = i;
			msg.golds_[i] = g_cash_pool * (it->second / 10000.0);
			it++; i++;
		}
		pp->send_msg_to_client(msg);
	}
	else{
		get_score_prize(pp);
	}
	return ERROR_SUCCESS_0;
}

int msg_misc_data_req::handle_this()
{

	player_ptr pp = from_sock_->the_client_.lock();

	if(!pp.get())
		 return SYS_ERR_CANT_FIND_CHARACTER;

	//登陆奖励
	msg_everyday_set everyday_msg;
	for(int i = 0 ; i < 7; ++ i)
		everyday_msg.everyday_set[i] = glb_everyday_set[i];

	pp->send_msg_to_client(everyday_msg);
	
	//房间配置

#ifdef HAS_TELFEE_MATCH
		msg_match_prize_set prize_msg;

		memcpy(prize_msg.prize_,match_prize,sizeof(match_prize));
		pp->send_msg_to_client(prize_msg);
#endif // HAS_TELFEE_MATCH


	
	return ERROR_SUCCESS_0;
}
