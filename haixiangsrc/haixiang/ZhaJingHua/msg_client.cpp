#include "msg_client.h"
#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "utility.h"
#include "error_define.h"
#include "error.h"
#include "game_config.h"

int msg_bet_operator::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;
	
	if (pgame->query_inf.uid_ != pp->uid_){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	//如果玩家跟牌
	if (agree_ > 0){
		int fac1 = 10; int fac2 = (op_code_ == 1 ? 2 : 1);
		//如果上次玩家没开牌
		if (!pgame->last_is_openset_){
			//如果玩家已经开牌了，或者看牌PK，需要下2倍的注
			if (pp->card_opened_){
				fac1 = 20;
			}
		}
		//如果上家已经开牌
		else {
			//如果玩家没有开牌，可以下一半
			if (!pp->card_opened_)	{
				fac1 = 5;
			}
		}
		if (agree_ < ((pgame->last_bet_ * fac1 * fac2) / 10)) 	{
			return GAME_ERR_NOT_ENOUGH_BET;
		}

		if (agree_ < the_service.the_config_.player_bet_lowcap_){
			return GAME_ERR_NUMBER_INVALID;
		}

		if (agree_ > the_service.the_config_.player_bet_cap_){
			return GAME_ERR_NUMBER_INVALID;
		}

		//如果大于2个人，不能开牌
		if (op_code_ == 1 && pgame->get_still_gaming_count() > 2 )	{
			return GAME_ERR_CANT_OPEN_NOW;
		}

		if (op_code_ == 2 && (pk_pos_ < 0 || pk_pos_ >= MAX_SEATS))
			return GAME_ERR_NUMBER_INVALID;

		if (op_code_ == 2 && pgame->is_playing_[pk_pos_].get()){
			if (pgame->is_playing_[pk_pos_]->is_giveup_ || pgame->is_playing_[pk_pos_] == pp){
				return GAME_ERR_WRONG_PKTARGET;
			}
			if (pp->turn_cards_.empty() || pgame->is_playing_[pk_pos_]->turn_cards_.empty())	{
				return GAME_ERR_WRONG_PKTARGET;
			}
		}
		


		int ret = pgame->set_bet(pp, agree_);
		if (ret != ERROR_SUCCESS_0){
			return ret;
		}

		pgame->last_is_openset_ = pp->card_opened_;
		pgame->last_bet_ = agree_;
		pgame->bets_pool_ += agree_;

		pgame->query_inf.replied_ = 1;
		
		msg_player_setbet msg_set;
		COPY_STR(msg_set.uid_, pp->uid_.c_str());
		msg_set.max_setted_ = agree_;
		msg_set.card_opened_ = pp->card_opened_;
		pgame->broadcast_msg(msg_set);

		//如果达到所有压注上限,则自动看牌,
		if (the_service.get_currency(pp->uid_) < (pgame->last_bet_ * fac1 / 10)){
			pgame->force_over_ = 1;
		}
		else{
			//看对方牌
			if (op_code_ == 1)	{
				pgame->force_over_ = 1;
			}
			//PK位置
			else if (op_code_ == 2){
				pgame->pk(pp->pos_, pk_pos_);
			}
		}
	}
	//如果玩家不跟
	else{
		pgame->query_inf.replied_ = 1;
		pp->is_giveup_ = 1;

		msg_player_setbet msg;
		msg.max_setted_ = 0;
		msg.card_opened_ = 0;
		COPY_STR(msg.uid_, pp->uid_.c_str());
		pgame->broadcast_msg(msg);
	}
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


	msg_deposit_change2 msg2;
	msg2.pos_ = pos_;
	msg2.credits_ = the_service.get_currency(pp->uid_);
	pgame->broadcast_msg(msg2);

	return ERROR_SUCCESS_0;
}

int msg_ready_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	pp->is_ready_ = true;

	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;

	msg_player_is_ready msg;
	msg.pos_ = pp->pos_;
	pgame->broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}

int msg_see_card_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return SYS_ERR_CANT_FIND_CHARACTER;
	if (pp->card_opened_) return ERROR_SUCCESS_0;
	if (pp->turn_cards_.empty()) return ERROR_SUCCESS_0;
	logic_ptr pgame = pp->the_game_.lock();
	if (!pgame) return SYS_ERR_CANT_FIND_CHARACTER;

	pp->card_opened_ = 1;
	pgame->send_card_to_player(pp, pp);

	msg_player_see_card msg;
	msg.pos_ = pp->pos_;
	pgame->broadcast_msg(msg);

	return ERROR_SUCCESS_0;
}
