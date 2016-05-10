#include "msg_server_common.h"
#include "msg_client.h"

#include "service.h"
#include "msg_server.h"
#include "md5.h"
#include "utility.h"
#include "error_define.h"
#include "error.h"

//申请下庄
int msg_cancel_banker_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	
	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	return pgame->apply_cancel_banker(pp);
}

int msg_apply_banker_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}
	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	if (the_service.wait_for_config_update_){
		return GAME_ERR_WAITING_UPDATE;
	}
	return pgame->apply_banker(pp);
}

int msg_set_bets_req::handle_this()
{
	player_ptr pp = from_sock_->the_client_.lock();
	if (!pp.get()){
		return SYS_ERR_CANT_FIND_CHARACTER;
	}

	logic_ptr pgame = pp->get_game();
	if (!pgame.get()){
		return GAME_ERR_CANT_FIND_GAME;
	}
	return pgame->set_bet(pp, pid_, present_id_);
}

int msg_get_player_count::handle_this()
{
	msg_return_player_count msg;
	msg.player_count_ = rand_r(1,the_service.the_config_.player_count_random_range_) + the_service.players_.size() 
		              + the_service.bots_.size();

	player_ptr pp = from_sock_->the_client_.lock();
	if (pp.get())
		pp->send_msg_to_client(msg);
	
	return ERROR_SUCCESS_0;
}

int msg_get_lottery_record::handle_this()
{
	msg_return_lottery_record msg;
	std::queue<int> temp = the_service.lottery_record_;
	msg.count_ = temp.size();
	int i = 0;
	
	while (!temp.empty() && i < msg_return_lottery_record::MAX_RECORD)
	{
		msg.record_[i] = temp.front();
		temp.pop();
		++ i;
	}
	
	
	player_ptr pp = from_sock_->the_client_.lock();
	if (pp.get())
		pp->send_msg_to_client(msg);

	return ERROR_SUCCESS_0;
}


typedef pair<string, int> PAIR;  

bool cmp_by_value(const PAIR& lhs, const PAIR& rhs) {  
	return lhs.second > rhs.second;  
}  


int msg_get_banker_ranking::handle_this()
{
	if(the_service.riches.get())
	{
		player_ptr p = from_sock_->the_client_.lock();
		if(p.get())
		{
			logic_ptr pgame = p->get_game();
			//长度大于1进行排序
			if(the_service.riches->list.size() > 0)
			{
				vector<riche_info> riche_vector;
				std::map<std::string, riche_info>::iterator it;
				for(it = the_service.riches->list.begin(); it != the_service.riches->list.end();++it)
				{
					if(it->second.val_ > 0){
						riche_vector.push_back(it->second);
					}
				}

				sort(riche_vector.begin(), riche_vector.end(), greater<riche_info>());

				int length = riche_vector.size() > 10 ? 10 : riche_vector.size();
				int type = 0;
				for (int i = 0; i != length; ++i) 
				{
					/*player_ptr pp = the_service.get_player(score_vec[i].first);
					if(!pp.get()){
						for(int j = 0; j < pgame->bots_.size(); j++)
						{
							player_ptr b = pgame->bots_[j];
							if(b->uid_ == score_vec[i].first)
							{
								pp = b;
								break;
							}
						}
					}*/
					riche_info _r = riche_vector[i];

					//if(_r != nullptr)
					//{
						msg_banker_ranking_ret msg;
						COPY_STR(msg.uname_, _r.uname_.c_str());
						msg.rank_val = _r.val_;
						msg.idx = i+1;

						if(i == 0){
							type = 1;
							if(i == (length - 1)){
								type = 3;
							}
						}else{
							type = (i == length - 1) ? 0 : 2;
						}

						msg.type = type;
						p->send_msg_to_client(msg);
					//}
				} 
			}
		}
	}
	return ERROR_SUCCESS_0;
}