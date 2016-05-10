#pragma once
#include "msg.h"
#include "utility.h"
#include "error_define.h"
#include "game_service_base.h"

enum
{
	GET_CLSID(msg_state_change)			= 100,
	GET_CLSID(msg_change_to_observer),
	GET_CLSID(msg_query_bet),
	GET_CLSID(msg_cards),
	GET_CLSID(msg_card_match_result),
	GET_CLSID(msg_deposit_change),
	GET_CLSID(msg_wait_last_game),
	GET_CLSID(msg_player_pk),
	GET_CLSID(msg_player_is_ready),
	GET_CLSID(msg_player_see_card),
	GET_CLSID(msg_gaming_info),
	GET_CLSID(msg_player_setbet),
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

//询问是否下注
class msg_query_bet : public msg_base<none_socket>
{
public:
	int		pos_;
	msg_query_bet()
	{
		head_.cmd_ = GET_CLSID(msg_query_bet);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//牌
class msg_cards : public msg_base<none_socket>
{
public:
	int			pos_;				//位置
	int			cards_[3];	//0-12，方块A-K,13-25, 梅花A-K, 26-38,红桃A-K, 39-51,黑桃A-K,
	msg_cards()
	{
		head_.cmd_ = GET_CLSID(msg_cards);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data<int>(cards_, 3, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

//牌型匹配结果
class msg_card_match_result : public msg_base<none_socket>
{
public:
	char		uid_[max_guid];				//谁的结果
	int			result_;							//牌型是什么
	int			winner_pos_;					//最后胜利位置
	int			is_pk_;								//是否PK结果
	msg_card_match_result()
	{
		head_.cmd_ = GET_CLSID(msg_card_match_result);
		winner_pos_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(result_, data_s);
		write_data(winner_pos_, data_s);
		write_data(is_pk_, data_s);
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
//玩家保证金变化
class msg_deposit_change : public msg_currency_change
{
public:
	int			room_id_;
	msg_deposit_change()
	{
		head_.cmd_ = GET_CLSID(msg_deposit_change);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_currency_change::write(data_s, l);
		write_data(room_id_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_state_change : public msg_base<none_socket>
{
public:
	int							change_to_;			//0开始下注, 
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
	int			card_opened_;
	msg_player_setbet()
	{
		head_.cmd_ = GET_CLSID(msg_player_setbet);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(max_setted_, data_s);
		write_data(card_opened_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//两位置发起PK
class msg_player_pk : public msg_base<none_socket>
{
public:
	int			pos1_;			//位置1
	int			pos2_;			//位置2
	int			winner_;		//哪个位置赢了
	msg_player_pk()
	{
		head_.cmd_ = GET_CLSID(msg_player_pk);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos1_, data_s);
		write_data(pos2_, data_s);
		write_data(winner_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_player_is_ready :  public msg_base<none_socket>
{
public:
	int		pos_;
	msg_player_is_ready()
	{
		head_.cmd_ = GET_CLSID(msg_player_is_ready);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_player_see_card : public msg_player_is_ready
{
public:
	msg_player_see_card()
	{
		head_.cmd_ = GET_CLSID(msg_player_see_card);
	}
};

#define MAX_SEATS 5
class msg_gaming_info : public msg_base<none_socket>
{
public:
	struct ginfo
	{
		int				pos_;				//位置
		int				is_playing_;//是否在玩
		int				card_open_;	//日否已开牌
		int				is_giveup_; //是否已弃牌
		double		bet_;				//总下注金额
		ginfo()
		{
			pos_ = -1;
			is_playing_ =0;
			card_open_ = 0;
			is_giveup_ = 0;
			bet_ = 0;
		}
	};
	int				cur_waitbet_;	//正在等待下注的位置
	int				cur_betcount_;//当前需要下注金额
	int				cur_cardopen_;//当前需要下注金额是否已开牌
	double		total_bets_;	//总下注金额
	ginfo			pla_inf_[MAX_SEATS];	//每个位置上的信息 MAX_SEATS = 5
	msg_gaming_info()
	{
		head_.cmd_ = GET_CLSID(msg_gaming_info);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(cur_waitbet_, data_s);
		write_data(cur_betcount_, data_s);
		write_data(cur_cardopen_, data_s);
		write_data(total_bets_, data_s);
		for (int i = 0 ; i < MAX_SEATS; i++)
		{
			write_data(pla_inf_[i].pos_, data_s);
			write_data(pla_inf_[i].is_playing_, data_s);
			write_data(pla_inf_[i].card_open_, data_s);
			write_data(pla_inf_[i].is_giveup_, data_s);
			write_data(pla_inf_[i].bet_, data_s);
		}
		return ERROR_SUCCESS_0;
	}
};