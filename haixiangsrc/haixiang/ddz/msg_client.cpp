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
extern int glb_everyday_set[7];
extern int	match_prize[5][7];
extern log_file<debug_output> debug_log;
extern std::string		to_string(std::vector<ddz_card> cards);
int msg_set_bets_req::handle_this()
{
	return ERROR_SUCCESS_0;
}

int msg_query_to_become_banker_rsp::handle_this()
{
	glb_log.write_log("### msg_query_to_become_banker_rsp::handle_this()");
DEBUG_COUNT_PERFORMANCE_BEGIN("msg_query_to_become_banker_rsp");
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if (agree_ < pgame->max_bet_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

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
	glb_log.write_log("### msg_want_sit_req::handle_this()");
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if (pos_ < 0 || pos_ >= MAX_SEATS) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if(pgame->is_playing_[pos_].get()) return SYS_ERR_CANT_FIND_CHARACTER;

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
extern	void		build_card_played_msg(msg_cards_deliver& msg_dlv, csf_result& ret);
int msg_play_cards::handle_this()
{
	glb_log.write_log("### msg_play_cards::handle_this()");
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;
	//出牌态状机不对
	if (pgame->current_state_->state_id_ != GET_CLSID(state_gaming)){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	//没轮到你出牌
	if (pp != pgame->is_playing_[pgame->current_order_]){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	msg_cards_deliver msg_dlv;
	msg_dlv.pos_ = pgame->current_order_;
	//0表示放弃出牌
	if (count_ == 0){
		if (pgame->lead_card_){
			pgame->auto_play();
		}
		else{
			pgame->play(pp, csf_result());
		}
	}
	else{
		//防止出不存在的牌,外挂原因等.
		std::vector<ddz_card> vcards;
		for (int i = 0; i < count_; i++)
		{
			if (std::find(pp->turn_cards_.begin(), pp->turn_cards_.end(), ddz_card::generate(card_[i])) != pp->turn_cards_.end()){
				vcards.push_back(ddz_card::generate(card_[i]));
			}
			else{
				return GAME_ERR_CARD_INDEX_INVALID;
			}
		}

		std::sort(vcards.begin(), vcards.end(), [](ddz_card &l,ddz_card &r)->bool{
			return l.card_weight_ < r.card_weight_;
		});

		csf_result ret = analyze_csf(vcards);
		//glb_log.write_log("### ret.csf_val_=%d",ret.csf_val_);
		if (ret.csf_val_ != csf_not_allowed){
			if (!pgame->lead_card_){
				if(is_greater_pattern(ret, pgame->current_csf_)){
					pgame->play(pp, ret);
				}
				else{
					return GAME_ERR_CARD_INDEX_INVALID;
				}
			}
			else{
				pgame->play(pp, ret);
			}
		}
		else{
			return GAME_ERR_CARD_INDEX_INVALID;
		}
	}
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
	glb_log.write_log("### msg_start_random_req::handle_this()");
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
	glb_log.write_log("### msg_misc_data_req::handle_this()");
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

//提示出牌
int msg_play_cards_tips::handle_this()
{
	glb_log.write_log("### msg_play_cards_tips::handle_this()");
	player_ptr pp = from_sock_->the_client_.lock();
	if(!pp.get())
	{
		glb_log.write_log("[error] can't find player");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	//follow_card()
	boost::shared_ptr<ddz_logic> pLogic = pp->get_game();
	if(!pLogic.get())
	{
		glb_log.write_log("[error] can't find ddz_logic");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	pLogic->play_cards_tips();

	return ERROR_SUCCESS_0;
}

//明牌
int msg_play_open_deal::handle_this()
{


	glb_log.write_log("### msg_play_open_deal::handle_this()");
	player_ptr pp = from_sock_->the_client_.lock();
	if(!pp.get())
	{
		glb_log.write_log("[error] can't find player");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	
	
	boost::shared_ptr<ddz_logic> pLogic = pp->get_game();
	if(!pLogic.get())
	{
		glb_log.write_log("[error] can't find ddz_logic");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	

	bool isAnyOpenDeal = false; //判断是否有人已经明牌
	for (int i=0;i<MAX_SEATS;i++)
	{
		player_ptr _pp = pLogic->is_playing_[i];
		if(NULL != _pp &&  em_open_deal_null !=_pp->em_open_deal_type_)
		{
			isAnyOpenDeal = true;
			break;
		}
	}


	if(!isAnyOpenDeal)
	{
		msg_paly_cards_open_deal_type msp;
		memcpy(msp.uid_, pp->uid_.c_str(), max_guid);
		glb_log.write_log("### state = %d",pLogic->current_state_->state_id_);
		if(NULL == pLogic->the_banker_)//还没有地主
		{
			pp->em_open_deal_type_ = em_open_deal_begin;   //明牌开始

			pLogic->current_askbank_order_ = pp->pos_;//明牌开始有优先叫牌权
			pLogic->turn_bank_order_	   = pp->pos_;//明牌开始有优先叫牌权
			glb_log.write_log("### change emOpenDealType to emOpenDeal_Begin");
		}
		else
		{
			if(pp == pLogic->the_banker_)				//地主明牌
			{
				pp->em_open_deal_type_ = em_open_deal_after;
				glb_log.write_log("### change emOpenDealType to emOpenDeal_After");
			}
			else
			{
				glb_log.write_log("### not the_banker");
			}
		}
		if(em_open_deal_null != pp->em_open_deal_type_)
		{
			msp.emOpenDealType_ = pp->em_open_deal_type_;
			pLogic->broadcast_msg(msp); 
			glb_log.write_log("### emOpenDeal_Null != pp->emOpenDealType_");
		}
	}

	return ERROR_SUCCESS_0;
}



//托管
int msg_play_auto::handle_this()
{
	glb_log.write_log("### msg_play_auto::handle_this()");
	player_ptr pp = from_sock_->the_client_.lock();
	if(!pp.get())
	{
		glb_log.write_log("[error] can't find player");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	
	boost::shared_ptr<ddz_logic> pLogic = pp->get_game();
	if(!pLogic.get())
	{
		glb_log.write_log("[error] can't find ddz_logic");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	
	//客户端请求的状态与服务端的一致，ASAP返回
	if(play_auto == pp->em_play_auto_status_){
		glb_log.write_log("[error] the play auto status is same");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	//验证客户端传递过来的参数
	if(em_play_auto_no != play_auto && em_play_auto_yes !=  play_auto){
		glb_log.write_log("[error] the play auto value must be 0 or 1 from client");
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	//将角色的状态设置为托管或者非托管
	pp->em_play_auto_status_ = (em_play_auto_status)play_auto;

	//广播托管结果
	msg_paly_auto_status mpa;
	mpa.em_play_auto_status = play_auto;
	memcpy(mpa.uid_, pp->uid_.c_str(), max_guid);
	pLogic->broadcast_msg(mpa);

	glb_log.write_log("player %s auto success",pp->uid_.c_str());

	return ERROR_SUCCESS_0;
}