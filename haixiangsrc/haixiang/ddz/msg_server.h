#pragma once
#include "msg.h"
#include "utility.h"
#include "error_define.h"
//#include "game_service_base.h"
#include "msg_server_common.h"

enum
{
	GET_CLSID(msg_state_change) = 10,
	GET_CLSID(msg_change_to_observer),
	GET_CLSID(msg_query_to_become_banker),
	GET_CLSID(msg_promote_banker),
	GET_CLSID(msg_cards),
	GET_CLSID(msg_banker_deposit),
	GET_CLSID(msg_card_match_result),
	GET_CLSID(msg_wait_last_game),
	GET_CLSID(msg_query_banker_rsp),
	GET_CLSID(msg_niuniu_score),
	GET_CLSID(msg_cash_pool),
	GET_CLSID(msg_random_present_info),
	GET_CLSID(msg_random_present_ret),
	GET_CLSID(msg_card_match_result_test),
	GET_CLSID(msg_cards_complete),
	GET_CLSID(msg_player_setbet),
	GET_CLSID(msg_everyday_set), 
	GET_CLSID(msg_match_prize_set),
	GET_CLSID(msg_player_hint),
	GET_CLSID(msg_extra_cards),
	GET_CLSID(msg_turn_to_play),
	GET_CLSID(msg_cards_deliver),
	GET_CLSID(msg_cards_deliver_tips),//发送提示牌型
	GET_CLSID(msg_paly_cards_open_deal_type),//发送明牌类型
	GET_CLSID(msg_paly_auto_status), //广播发送给客户端的托管状态
};

//变成观察者
class msg_change_to_observer : public msg_base<none_socket>
{
public:
	int				reason_;			//为什么变成了观察者
	msg_change_to_observer()
	{
		head_.cmd_ = GET_CLSID(msg_change_to_observer);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(reason_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//
class msg_query_to_become_banker : public msg_base<none_socket>
{
public:
	int		pos_;
	msg_query_to_become_banker()
	{
		head_.cmd_ = GET_CLSID(msg_query_to_become_banker);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_promote_banker : public msg_base<none_socket>
{
public:
	char		uid_[max_guid];
	int			reason_;				//-1-下庄, 0-因为没有符合竞庄条件而随机出来的庄，1.竞庄的庄, 2-因为没有人原意做庄随机出来的庄
	msg_promote_banker()
	{
		head_.cmd_ = GET_CLSID(msg_promote_banker);
		reason_ = -1;
		memset(uid_, 0 , max_guid);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(reason_, data_s);
		return ERROR_SUCCESS_0;
	}
};
//牌
class msg_cards : public msg_base<none_socket>
{
public:
	int			pos_;				//位置
	int			cards_[20];	//0-12，方块A-K,13-25, 梅花A-K, 26-38,红桃A-K, 39-51,黑桃A-K, 52 小王,53大王
	msg_cards()
	{
		head_.cmd_ = GET_CLSID(msg_cards);
		memset(cards_, -1, sizeof(cards_));
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data<int>(cards_, 20, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

//牌型匹配结果
class msg_card_match_result : public msg_base<none_socket>
{
public:
	int			pos_;				//位置
	__int64	final_win_; //最终输赢多少 
	msg_card_match_result()
	{
		head_.cmd_ = GET_CLSID(msg_card_match_result);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(final_win_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//庄家钱变化
class msg_banker_deposit : public msg_base<none_socket>
{
public:
	double	deposit_;
	msg_banker_deposit()
	{
		head_.cmd_ = GET_CLSID(msg_banker_deposit);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(deposit_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//等待上轮游戏结束
class msg_wait_last_game : public msg_base<none_socket>
{
public:
	msg_wait_last_game()
	{
		head_.cmd_ = GET_CLSID(msg_wait_last_game);
	}
};

class msg_state_change : public msg_base<none_socket>
{
public:
	int							change_to_;			//0开始下注, 1,开始转转,2,休息时间
	unsigned int		time_left;
	unsigned int		time_total_;
	msg_state_change()
	{
		head_.cmd_ = GET_CLSID(msg_state_change);
		time_left = 0;
		change_to_ = 0;
		time_total_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(change_to_, data_s);
		write_data(time_left, data_s);
		write_data(time_total_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_player_setbet : public msg_base<none_socket>
{
public:
	char		uid_[max_guid];
	double	max_setted_;	//这个注一共已下注
	msg_player_setbet()
	{
		head_.cmd_ = GET_CLSID(msg_player_setbet);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(max_setted_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_query_banker_rsp : public msg_base<none_socket>
{
public:
	char			uid_[max_guid];
	int				agree_;
	msg_query_banker_rsp()
	{
		head_.cmd_ = GET_CLSID(msg_query_banker_rsp);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(agree_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_niuniu_score : public msg_base<none_socket>
{
public:
	int			pos_;
	int			score;
	int			base_score_;
	msg_niuniu_score()
	{
		head_.cmd_ = GET_CLSID(msg_niuniu_score);
		base_score_ = 100;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(score, data_s);
		write_data(base_score_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_cash_pool : public msg_currency_change
{
public:
	msg_cash_pool()
	{
		head_.cmd_ = GET_CLSID(msg_cash_pool);
	}
};

class msg_random_present_info : public msg_base<none_socket>
{
public:
	int			percents_[5];
	double	golds_[5];
	msg_random_present_info()
	{
		head_.cmd_ = GET_CLSID(msg_random_present_info);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(percents_, 5, data_s, true);
		write_data<double>(golds_, 5, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

class msg_random_present_ret : public msg_base<none_socket>
{
public:
	int			result_;
	double	gold_get_;
	char		uid_[max_guid];
	char		uname_[max_name];
	msg_random_present_ret()
	{
		head_.cmd_ = GET_CLSID(msg_random_present_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(result_, data_s);
		write_data(gold_get_, data_s);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}
};

//牌型匹配结果
class msg_card_match_result_test : public msg_card_match_result
{
public:
	msg_card_match_result_test()
	{
		head_.cmd_ = GET_CLSID(msg_card_match_result_test);
	}
};
//牌发完了
class msg_cards_complete : public msg_base<none_socket>
{
public:
	msg_cards_complete()
	{
		head_.cmd_ = GET_CLSID(msg_cards_complete);
	}
};


class msg_everyday_set : public msg_base<none_socket>
{
public:
	int everyday_set[7];

	msg_everyday_set()
	{
		head_.cmd_ = GET_CLSID(msg_everyday_set);
		memset(everyday_set,0,sizeof(everyday_set));
	}

	int	write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(everyday_set, 7, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

class msg_match_prize_set : public msg_base<none_socket>
{
public:
	int prize_[5][7];
	msg_match_prize_set()
	{
		head_.cmd_      = GET_CLSID(msg_match_prize_set);
		memset(prize_,0,sizeof(prize_));
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		for(int i = 0 ;i < 5 ; ++i)
			write_data<int>(prize_[i], 7, data_s, true);

		return ERROR_SUCCESS_0;
	}
};

class msg_player_hint : public msg_base<none_socket>
{
public:
	int hint_type_;  //1断线重连
	msg_player_hint()
	{
		head_.cmd_      = GET_CLSID(msg_player_hint);
		hint_type_ = 0;
	}

	int	 write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(hint_type_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//地主额外的牌
class msg_extra_cards : public msg_cards
{
public:
	msg_extra_cards()
	{
		head_.cmd_      = GET_CLSID(msg_extra_cards);
	}
};

//轮到玩家出牌了
class msg_turn_to_play : public msg_base<none_socket>
{
public:
	msg_turn_to_play()
	{
		head_.cmd_      = GET_CLSID(msg_turn_to_play);
	}
	int		pos_;
	int		timeleft_;
	int	 write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(timeleft_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//玩家打牌数据广播
class msg_cards_deliver : public msg_base<none_socket>
{
public:
	int		pos_;
	int		count_;
	int		card_[20];
	int		result_;			//牌型匹配结果
	int		mainc_;				//主要牌型数量
	int		attach_c;			//附带牌数
	msg_cards_deliver()
	{
		head_.cmd_ = GET_CLSID(msg_cards_deliver);
		count_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(count_, data_s);
		write_data<int>(card_, count_, data_s, true);
		write_data(result_, data_s);
		write_data(mainc_, data_s);
		write_data(attach_c, data_s);
		return ERROR_SUCCESS_0;
	}
};

//玩家提示牌型
class msg_cards_deliver_tips : public msg_base<none_socket>
{
public:
	int		pos_;
	int		count_;
	int		card_[20];
	int		result_;			//牌型匹配结果
	int		mainc_;				//主要牌型数量
	int		attach_c;			//附带牌数
	msg_cards_deliver_tips()
	{
		head_.cmd_ = GET_CLSID(msg_cards_deliver_tips);
		count_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(count_, data_s);
		write_data<int>(card_, count_, data_s, true);
		write_data(result_, data_s);
		write_data(mainc_, data_s);
		write_data(attach_c, data_s);
		return ERROR_SUCCESS_0;
	}
};

//玩家的明牌类型
class msg_paly_cards_open_deal_type : public msg_base<none_socket>
{
public:
	int				emOpenDealType_; //明牌类型
	char			uid_[max_guid];  //明牌的人
	msg_paly_cards_open_deal_type()
	{
		head_.cmd_ = GET_CLSID(msg_paly_cards_open_deal_type);
		emOpenDealType_ = em_open_deal_null;
	}

	int		write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(emOpenDealType_, data_s);
		write_data(uid_,max_guid,data_s);
		return ERROR_SUCCESS_0;
	}
};


//玩家的托管结果
class msg_paly_auto_status : public msg_base<none_socket>
{
public:
	int				em_play_auto_status; //托管的状态
	char			uid_[max_guid];			 //托管的人
	msg_paly_auto_status()
	{
		head_.cmd_ = GET_CLSID(msg_paly_auto_status);
		em_play_auto_status = em_play_auto_no;
	}

	int		write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(em_play_auto_status, data_s);
		write_data(uid_,max_guid,data_s);
		return ERROR_SUCCESS_0;
	}
};


