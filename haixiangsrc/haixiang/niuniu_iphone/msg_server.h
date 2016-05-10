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
	int			reason_;				//-1-��ׯ, 0-��Ϊû�з��Ͼ�ׯ���������������ׯ��1.��ׯ��ׯ, 2-��Ϊû����ԭ����ׯ���������ׯ
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
//��
class msg_cards : public msg_base<none_socket>
{
public:
	int			pos_;				//λ��
	int			cards_[5];	//0-12������A-K,13-25, ÷��A-K, 26-38,����A-K, 39-51,����A-K, 52 С��,53����
	msg_cards()
	{
		head_.cmd_ = GET_CLSID(msg_cards);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data<int>(cards_, 5, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

//����ƥ����
class msg_card_match_result : public msg_base<none_socket>
{
public:
	int			pos_;				//λ��
	int			niuniu_point_;	//ţţ����, 
	int			niuniu_level_;	//ţţ����, <0-��ţ��0-��ͨţţ,1-��ţ,2-��ţ
	int			card3_[3];	//��0��3����
	int			card2_[2];	//����2������
	double	final_win_; //������Ӯ���� 
	int			replace_id1_;	//�����滻��
	int			replace_id2_;	//С���滻��
	msg_card_match_result()
	{
		head_.cmd_ = GET_CLSID(msg_card_match_result);
		replace_id1_ = -1;
		replace_id2_ = -1;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(niuniu_point_, data_s);
		write_data(niuniu_level_, data_s);
		write_data<int>(card3_, 3, data_s, true);
		write_data<int>(card2_, 2, data_s, true);
		write_data(final_win_, data_s);
		write_data(replace_id1_, data_s);
		write_data(replace_id2_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//ׯ��Ǯ�仯
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

//�ȴ�������Ϸ����
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
	int							change_to_;			//0��ʼ��ע, 1,��ʼתת,2,��Ϣʱ��
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

//����ƥ����
class msg_card_match_result_test : public msg_card_match_result
{
public:
	msg_card_match_result_test()
	{
		head_.cmd_ = GET_CLSID(msg_card_match_result_test);
	}
};
//�Ʒ�����
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
	int hint_type_;  //1��������
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