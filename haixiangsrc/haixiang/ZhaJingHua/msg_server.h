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

//��ɹ۲���
class msg_change_to_observer : public msg_base<none_socket>
{
public:
	int				reason_;			//Ϊʲô����˹۲���
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

//ѯ���Ƿ���ע
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

//��
class msg_cards : public msg_base<none_socket>
{
public:
	int			pos_;				//λ��
	int			cards_[3];	//0-12������A-K,13-25, ÷��A-K, 26-38,����A-K, 39-51,����A-K,
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

//����ƥ����
class msg_card_match_result : public msg_base<none_socket>
{
public:
	char		uid_[max_guid];				//˭�Ľ��
	int			result_;							//������ʲô
	int			winner_pos_;					//���ʤ��λ��
	int			is_pk_;								//�Ƿ�PK���
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

//�ȴ�������Ϸ����
class msg_wait_last_game : public msg_base<none_socket>
{
public:
	msg_wait_last_game()
	{
		head_.cmd_ = GET_CLSID(msg_wait_last_game);
	}
};
//��ұ�֤��仯
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
	int							change_to_;			//0��ʼ��ע, 
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
	double	max_setted_;	//���עһ������ע
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

//��λ�÷���PK
class msg_player_pk : public msg_base<none_socket>
{
public:
	int			pos1_;			//λ��1
	int			pos2_;			//λ��2
	int			winner_;		//�ĸ�λ��Ӯ��
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
		int				pos_;				//λ��
		int				is_playing_;//�Ƿ�����
		int				card_open_;	//�շ��ѿ���
		int				is_giveup_; //�Ƿ�������
		double		bet_;				//����ע���
		ginfo()
		{
			pos_ = -1;
			is_playing_ =0;
			card_open_ = 0;
			is_giveup_ = 0;
			bet_ = 0;
		}
	};
	int				cur_waitbet_;	//���ڵȴ���ע��λ��
	int				cur_betcount_;//��ǰ��Ҫ��ע���
	int				cur_cardopen_;//��ǰ��Ҫ��ע����Ƿ��ѿ���
	double		total_bets_;	//����ע���
	ginfo			pla_inf_[MAX_SEATS];	//ÿ��λ���ϵ���Ϣ MAX_SEATS = 5
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