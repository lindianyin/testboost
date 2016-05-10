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

	//-1表示清空压注
	if (pid_ == -1){
		pp->bets_.clear();
		return ERROR_SUCCESS_0;
	}

	if (pid_ < 1 || pid_ > 8){
		return GAME_ERR_CANT_FIND_GAME;
	}
	
	if (the_service.wait_for_config_update_){
		return GAME_ERR_CANT_FIND_GAME;
	}

	if (count_ <= 0 )
		return GAME_ERR_NOT_ENOUGH_BET;

	int ret = pgame->set_bet(pp, pid_, count_);
DEBUG_COUNT_PERFORMANCE_END()
	return ret;
}

static longlong match_result(std::vector<preset_present*>& result, std::map<int, int>& bets)
{
	longlong win = 0;
	auto it_bet = bets.begin();
	while (it_bet != bets.end())
	{
		int betid = it_bet->first;
		for (unsigned int i = 0; i < result.size(); i++)
		{
			if (result[i]->prior_ == betid){
				win += it_bet->second * result[i]->factor_;
			}
		}
		it_bet++;
	}
	return win;
}

int restore_credits(player_ptr pp, longlong temp_win )
{
	logic_ptr plogic = pp->the_game_.lock();
	if (plogic.get())	{

		if (plogic->scene_set_->gurantee_set_ == 0){
			plogic->scene_set_->gurantee_set_ = 100;
		}

		longlong out_count = 0;
		int ret = ERROR_SUCCESS_0;
		if (temp_win < 0){
			ret = the_service.cache_helper_.apply_cost(pp->uid_, -temp_win * plogic->scene_set_->gurantee_set_, out_count);
		}
		else{
			ret = the_service.cache_helper_.give_currency(pp->uid_, temp_win * plogic->scene_set_->gurantee_set_, out_count);
			
		}

		if (ret == ERROR_SUCCESS_0)	{
			msg_deposit_change2 msg_desp;
			msg_desp.credits_ = out_count;
			msg_desp.display_type_ = 0;
			pp->send_msg_to_client(msg_desp);

			//更新玩家gameinfo
			longlong lwin;
			if(temp_win < 0 )
				lwin=  -temp_win * plogic->scene_set_->gurantee_set_;
			else
				lwin=  temp_win * plogic->scene_set_->gurantee_set_;
			pp->game_info_.today_win_ += lwin;
			pp->game_info_.month_win_  +=lwin;
			
			if(lwin > pp->game_info_.one_game_max_win_)
				pp->game_info_.one_game_max_win_ = lwin;

			the_service.save_player_game_info(pp);
			return ERROR_SUCCESS_0;
		}
		else 
			return ret;
	}
	return ERROR_SUCCESS_0;
}

int msg_start_random_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return 
		SYS_ERR_CANT_FIND_CHARACTER;

	 

	logic_ptr plogic = pp->the_game_.lock();
	if (!plogic.get()) 
		return SYS_ERR_CANT_FIND_CHARACTER;
	++pp->game_info_.game_count_;
	std::string str_bets, str_results, sql;
	if (view_present_ == 0){
		pp->temp_win_ = 0;
		
		std::string str_bets;
		int total_bet = 0;
		auto itb = pp->bets_.begin();
		while(itb != pp->bets_.end())
		{
			total_bet += itb->second;
			str_bets += lex_cast_to_str(itb->first) + "," + lex_cast_to_str(itb->second) + ",";
			itb++;
		}

		longlong out_count = 0;
		int cst = the_service.cache_helper_.apply_cost(pp->uid_, total_bet * plogic->scene_set_->gurantee_set_, out_count);
		if (cst != ERROR_SUCCESS_0){
			return cst;
		}
		//更新gameinfo
		pp->game_info_.today_win_ -= total_bet * plogic->scene_set_->gurantee_set_, out_count;
		pp->game_info_.month_win_ -= total_bet * plogic->scene_set_->gurantee_set_, out_count;
		int	r = rand_r(10000);

		preset_present* result = NULL;
		std::size_t i = 0;
		for (; i < the_service.the_config_.presets_.size(); i++)
		{
			if (r > the_service.the_config_.presets_[i].rate_){
				r -= the_service.the_config_.presets_[i].rate_;
			}
			else{
				result = &the_service.the_config_.presets_[i];
				break;
			}
		}
		//二次抽奖
		if (result){
			str_results +=  lex_cast_to_str(result->pid_) + ",";
			msg_random_present_ret msg;
			msg.result_ = result->pid_;
			std::vector<preset_present*> vresult;
			if (result->pid_ == 7 || result->pid_ == 19){
				r = rand_r(100);
				if (r < 50){
					msg.result2nd_ = 1;
					for (int i = 0 ; i < 3; i++)
					{
						int n =  rand_r(23);
						vresult.push_back(the_service.the_config_.presets_.data() + n);
						msg.result2_[i] = the_service.the_config_.presets_[n].pid_;
						str_results +=  lex_cast_to_str(the_service.the_config_.presets_[n].pid_) + ",";
					}
				}
				else if (r < 70){
					msg.result2nd_ = 2;
					int r = rand_r(23);
					for (int i = 0 ; i < 7; i++)
					{
						int n = (r--) % 24; 
						if (n < 0) n += 24;
						vresult.push_back(the_service.the_config_.presets_.data() + n);
						msg.result2_[i] = the_service.the_config_.presets_[n].pid_;
						str_results +=  lex_cast_to_str(the_service.the_config_.presets_[n].pid_) + ",";
					}
				}
				else{
					msg.result2nd_ = 0;
				}
			}
			else{
				msg.result2nd_ = 0;
				vresult.push_back(result);
			}

			pp->temp_win_ = match_result(vresult, pp->bets_);
			msg.win_ = pp->temp_win_;

			pp->send_msg_to_client(msg);

			restore_credits(pp, pp->temp_win_);

			pp->bets_.clear();

			BEGIN_INSERT_TABLE("set_bets");
			SET_FIELD_VALUE("uid", pp->uid_);
			SET_FIELD_VALUE("pid", str_bets);
			SET_FIELD_VALUE("present_id", str_results);
			SET_FIELD_VALUE("win", pp->temp_win_);
			SET_FINAL_VALUE("factor", plogic->scene_set_->gurantee_set_);
			EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);
		}
		else{
			glb_log.write_log("random failed! preset rate not 100%!");
		}
	}
	else if( view_present_ == 1 || view_present_ == 2 || view_present_ == 5 ||view_present_ == 6 ){
		if (pp->temp_win_ <= 0){
			return SYS_ERR_CANT_FIND_CHARACTER;
		}
		int r = restore_credits(pp, -pp->temp_win_);
		if (r != ERROR_SUCCESS_0){
			return r;
		}

		r = rand_r(1, 100);

		int result = 0;

	     for(auto iter: the_service.the_new_config_.presets_)
		 {
			 if(iter.pid_ >= 200)
			 {
				 if(r < iter.rate_)
				 {
					 if(result < iter.factor_)
						 result = iter.factor_;
				 }
			 }
		 }

		 
		msg_random_present_ret msg;
		msg.result_ = 0;
		msg.result2nd_ = 3;
		msg.result2_[0] = view_present_; //玩家压的宝箱号
		msg.result2_[1] = result;        //宝箱的倍数
		msg.win_ = pp->temp_win_ * result;
		pp->temp_win_  = pp->temp_win_ * result;
		if(result > 0)
		{
			restore_credits(pp, pp->temp_win_);
		}
		else //已经输光了所有的钱，不能再开宝箱了
			pp->temp_win_ = 0;



		/*msg_random_present_ret msg;
		msg.result_ = 0;
		msg.result2nd_ = 3;
		msg.result2_[0] = r;
		msg.win_ = 0;
		//押大
		if (view_present_ == 1){
			if (r > 6){
				pp->temp_win_ += pp->temp_win_;
				restore_credits(pp, pp->temp_win_);
				msg.win_ = pp->temp_win_;
			}
		}
		//押小
		else{
			if (r < 7){
				pp->temp_win_ += pp->temp_win_;
				restore_credits(pp, pp->temp_win_);
				msg.win_ = pp->temp_win_;
			}
		}

		BEGIN_INSERT_TABLE("set_bets");
		SET_STRFIELD_VALUE("uid", pp->uid_);
		SET_STRFIELD_VALUE("pid", view_present_ + 8);
		SET_STRFIELD_VALUE("present_id", r);
		SET_FIELD_VALUE("win", msg.win_);
		SET_FINAL_VALUE("factor", plogic->scene_set_->gurantee_set_);
		EXECUTE_NOREPLACE_DELAYED("", the_service.delay_db_game_);*/

		pp->send_msg_to_client(msg);
	}
	else if (view_present_ == 3){
		pp->temp_win_--;
	}
	else if (view_present_ == 4){
		pp->temp_win_++;
	}
	return ERROR_SUCCESS_0;
}

int msg_player_game_info_req::handle_this()
{

	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()) return 
		SYS_ERR_CANT_FIND_CHARACTER;

	the_service.set_player_game_info(pp);

	msg_player_game_info_ret ret;
	ret.game_count_ = pp->game_info_.game_count_;
	ret.max_win_    = pp->game_info_.one_game_max_win_;
	ret.today_win_  = pp->game_info_.today_win_;
	ret.month_win_  = pp->game_info_.month_win_;

	pp->send_msg_to_client(ret);

	return ERROR_SUCCESS_0;
}
