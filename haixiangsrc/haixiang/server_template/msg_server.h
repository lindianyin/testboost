#pragma once
#include "msg.h"
#include "utility.h"
#include "error_define.h"
#include "game_service_base.h"
enum
{
	GET_CLSID(msg_user_login_ret)		= 10,
	GET_CLSID(msg_common_reply)			= 1001,
	GET_CLSID(msg_logout)						= 1034,
	GET_CLSID(msg_currency_change)	= 1007,//������Ϣid��Ҫ�̶������������ڹ�������
	GET_CLSID(msg_state_change)			= 1100,
	GET_CLSID(msg_server_parameter)	= 1107,
	GET_CLSID(msg_player_setbet)		= 1108,
	GET_CLSID(msg_low_currency)		= 1109,
	GET_CLSID(msg_player_seat)		= 1110,
	GET_CLSID(msg_change_to_observer),
	GET_CLSID(msg_query_to_become_banker),
	GET_CLSID(msg_promote_banker),
	GET_CLSID(msg_cards),
	GET_CLSID(msg_room_info),
	GET_CLSID(msg_banker_deposit),
	GET_CLSID(msg_card_match_result),
	GET_CLSID(msg_player_leave),
	GET_CLSID(msg_deposit_change),
	GET_CLSID(msg_wait_last_game),
	GET_CLSID(msg_deposit_change2),
	GET_CLSID(msg_chat_deliver),
	GET_CLSID(msg_query_banker_rsp),
	GET_CLSID(msg_niuniu_score),
	GET_CLSID(msg_cash_pool),
  GET_CLSID(msg_random_present_info),
	GET_CLSID(msg_random_present_ret),
	GET_CLSID(msg_everyday_gift),
	GET_CLSID(msg_levelup),
	GET_CLSID(msg_set_account_ret),
	GET_CLSID(msg_recommend_data),
	GET_CLSID(msg_trade_data),
	GET_CLSID(msg_card_match_result_test),
	GET_CLSID(msg_cards_complete),
};

class msg_set_account_ret : public msg_base<none_socket>
{
public:
	unsigned int	iid_;
	msg_set_account_ret()
	{
		head_.cmd_ = GET_CLSID(msg_set_account_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_user_login_ret:public msg_base<none_socket>
{
public:
	int			iid_;
	int			lv_;
	double	currency_;
	double	exp_;
	char		uname_[max_name];
	char		uid_[max_guid];
	char		head_pic_[max_guid];
	msg_user_login_ret()
	{
		head_.cmd_ = GET_CLSID(msg_user_login_ret);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data(lv_, data_s);
		write_data(currency_, data_s);
		write_data(exp_, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_low_currency : public msg_base<none_socket>
{
public:
	msg_low_currency()
	{
		head_.cmd_ = GET_CLSID(msg_low_currency);
	}
};

//�������λ��
class msg_player_seat : public msg_base<none_socket>
{
public:
	char			uid_[max_guid];	//˭
	char			head_pic_[max_guid];
	char			uname_[max_name];
	int				pos_;						//�����ĸ�λ��
	int				iid_;
	msg_player_seat()
	{
		head_.cmd_ = GET_CLSID(msg_player_seat);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data(pos_, data_s);
		return ERROR_SUCCESS_0;
	}
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

class msg_room_info : public msg_base<none_socket>
{
public:
	int			room_id_;		//����id
	int			free_pos_;	//����λ��
	int			observers_;	//�۲�������

	msg_room_info()
	{
		head_.cmd_ = GET_CLSID(msg_room_info);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(room_id_, data_s);
		write_data(free_pos_, data_s);
		write_data(observers_, data_s);
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

class msg_player_leave : public msg_base<none_socket>
{
public:
	int				pos_;		//�ĸ�λ���뿪��Ϸ
	int				why_;		//0,�뿪��Ϸ 1������
	msg_player_leave()
	{
		head_.cmd_ = GET_CLSID(msg_player_leave);
		why_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(why_, data_s);
		return ERROR_SUCCESS_0;
	}
};
//�˺��������ط���¼��ʾ
class msg_logout : public msg_base<none_socket>
{
public:
	char uid_[max_guid];	//40�ֽ�
	msg_logout()
	{
		head_.cmd_ = GET_CLSID(msg_logout);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		return ERROR_SUCCESS_0;
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

//��ұ�֤��仯
class msg_deposit_change2 : public msg_currency_change
{
public:
	int		pos_;
	int		display_type_;		//0������ҪƮ�� 1,ҪƮ��, 2 ��֤��仯
	msg_deposit_change2()
	{
		head_.cmd_ = GET_CLSID(msg_deposit_change2);
		display_type_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_currency_change::write(data_s, l);
		write_data(pos_, data_s);
		write_data(display_type_, data_s);
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

//������������Ϣ
class msg_server_parameter : public msg_base<none_socket>
{
public:
	int					min_guarantee_;	//�μ���Ϸ��ͻ�������
	int					is_free_;				//ʹ�õ�����ѵĻ���
	int					banker_set_;		//��ׯ��֤��
	int					low_cap_;				//��ע
	int					player_tax_;		//������
	int					enter_scene_;		//�Ƿ���볡����Ϣ��1�ǣ�0������������2�� ��ʼ 3�������������ý���
	msg_server_parameter()
	{
		head_.cmd_ = GET_CLSID(msg_server_parameter);
		enter_scene_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(min_guarantee_, data_s);
		write_data(is_free_, data_s);
		write_data(banker_set_, data_s);
		write_data(low_cap_, data_s);
		write_data(player_tax_, data_s);
		write_data(enter_scene_, data_s);
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

class msg_chat_deliver : public msg_base<none_socket>
{
public:
	int						channel_;						//Ƶ��
	char					name_[max_name];		//˭
	int						sex_;								//��ʱû��
	unsigned char	len_;								//˵�����ݳ���
	char					content_[256];			//˵��ʲô
	msg_chat_deliver()
	{
		head_.cmd_ = GET_CLSID(msg_chat_deliver);
		sex_ = -1;
		len_ = 0;
		memset(name_, 0, max_name);
		memset(content_, 0, 256);
	}
	void		set_content(string s)
	{
		if(s.length() > 255) return;
		len_ = s.length() + 1;
		COPY_STR(content_, s.c_str());
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(channel_, data_s);
		write_data<char>(name_, max_name, data_s);
		write_data<int>(sex_, data_s);
		write_data<unsigned char>(len_, data_s);
		write_data<char>(content_, std::min<unsigned char>(len_, 255), data_s, false);
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

class msg_everyday_gift : public msg_base<none_socket>
{
public:
	msg_everyday_gift()
	{
		head_.cmd_ = GET_CLSID(msg_everyday_gift);
		getted_ = 0;
		conitnue_days_ = 0;
		type_ = 0;
	}

	int			conitnue_days_;
	int			getted_;
	int			type_;		//��msg_get_prize_req���type��ͬ����

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(conitnue_days_, data_s);
		write_data(getted_, data_s);
		write_data(type_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_levelup : public msg_base<none_socket>
{
public:
	int lv_;
	double exp_;
	msg_levelup()
	{
		head_.cmd_ = GET_CLSID(msg_levelup);
		lv_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(lv_, data_s);
		write_data(exp_, data_s);
		return ERROR_SUCCESS_0;
	}
};

//�Ƽ���Ϣ����
class msg_recommend_data : public  msg_base<none_socket>
{
public:
	short SS;	//��ʼ���
	short	SE;	//�������	
	char uid_[max_guid];
	char uname_[max_name];
	char head_pic_[max_guid];
	int reward_getted_;	//�����Ƿ�����ȡ 0��δ��ȡ��1��ȡ
	msg_recommend_data()
	{
		head_.cmd_ = GET_CLSID(msg_recommend_data);
		SS = 0;
		SE = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(SS, data_s);
		write_data(SE, data_s);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data(reward_getted_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_trade_data : public  msg_base<none_socket>
{
public:
	short SS;	//��ʼ���
	short	SE;	//�������	
	int		iid_;					//����id
	double	gold_;				//Ǯ
	double	time;					//ʱ��

	msg_trade_data()
	{
		head_.cmd_ = GET_CLSID(msg_trade_data);
		SS = 0;
		SE = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(SS, data_s);
		write_data(SE, data_s);
		write_data(iid_, data_s);
		write_data(gold_, data_s);
		write_data(time, data_s);
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
