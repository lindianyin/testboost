#pragma once
#include "msg.h"
#include "utility.h"
#include "error_define.h"
#include "game_service_base.h"
#include "msg_server_common.h"

enum
{
	GET_CLSID(msg_state_change)			= 10,
	GET_CLSID(msg_rand_result)			= 11,
	GET_CLSID(msg_banker_promote)		= 12,
	GET_CLSID(msg_game_report)			= 13,
	GET_CLSID(msg_banker_deposit_change) = 14,
	GET_CLSID(msg_new_banker_applyed) = 15,
	GET_CLSID(msg_apply_banker_canceled) = 16,
	GET_CLSID(msg_player_setbet) = 17,
	GET_CLSID(msg_my_setbet) = 18,
	GET_CLSID(msg_last_random) = 19,
	GET_CLSID(msg_return_player_count) = 20,
	GET_CLSID(msg_return_lottery_record) = 21,
	GET_CLSID(msg_broadcast_msg) = 22,
};
class msg_state_change : public msg_base<none_socket>
{
public:
	int							change_to_;			//0-��ע, 1-תȦ, 2-��Ϣ�ȴ�
	unsigned int		time_left_;						//ÿ��״̬��ʣ��ʱ��
	msg_state_change()
	{
		head_.cmd_ = GET_CLSID(msg_state_change);
		time_left_ = 0;
		change_to_ = 0;
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(change_to_, data_s);
		write_data(time_left_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_rand_result : public msg_base<none_socket>
{
public:
	int		rand_result_;
	msg_rand_result()
	{
		head_.cmd_ = GET_CLSID(msg_rand_result);
		rand_result_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(rand_result_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_banker_promote : public msg_base<none_socket>
{
public:
	char				uid_[max_guid];
	char				name_[max_name];
	char				head_pic_[max_guid];
	double			deposit_;					//�ʽ�� 8�ֽ�
	int					is_sys_banker_;		//�Ƿ���ϵͳׯ 1��
	msg_banker_promote()
	{
		is_sys_banker_ = 0;
		uid_[0] = 0;
		name_[0] = 0;
		head_pic_[0] = 0;
		deposit_ = 0;
		head_.cmd_ = GET_CLSID(msg_banker_promote);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(name_, max_name, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data(deposit_, data_s);
		write_data(is_sys_banker_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_game_report : public msg_base<none_socket>
{
public:
	unsigned int	pay_;						//��������Ǯ
	double				actual_win_;		//ʵ��Ӯ����	8�ֽ�
	double				should_win_;		//������Ӯ���٣�����ׯ�ұ�ׯ�����ܻ����� 8�ֽ�
	msg_game_report()
	{
		head_.cmd_ = GET_CLSID(msg_game_report);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pay_, data_s);
		write_data(actual_win_, data_s);
		write_data(should_win_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_banker_deposit_change : public msg_currency_change
{
public:
	int				turn_left_;
	msg_banker_deposit_change()
	{
		head_.cmd_ = GET_CLSID(msg_banker_deposit_change);
	}	

	int			write(char*& data_s, unsigned int l)
	{
		msg_currency_change::write(data_s, l);
		write_data(turn_left_, data_s);
		return ERROR_SUCCESS_0;
	}

};

//���������б����
class msg_new_banker_applyed : public msg_base<none_socket>
{
public:
	char			uid_[max_guid];
	char			name_[max_name];
	msg_new_banker_applyed()
	{
		head_.cmd_ = GET_CLSID(msg_new_banker_applyed);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(name_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}
};

//ɾ�������б����
class msg_apply_banker_canceled : public msg_base<none_socket>
{
public:
	char			uid_[max_guid];
	msg_apply_banker_canceled()
	{
		head_.cmd_ = GET_CLSID(msg_apply_banker_canceled);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		return ERROR_SUCCESS_0;
	}
};


class msg_player_setbet : public msg_base<none_socket>
{
public:
	int			pid_;				//����id
	int			present_id_;//��Ůid
	double	max_setted_;	//���עһ������ע
	msg_player_setbet()
	{
		head_.cmd_ = GET_CLSID(msg_player_setbet);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pid_, data_s);
		write_data(present_id_, data_s);
		write_data(max_setted_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_my_setbet : public msg_base<none_socket>
{
public:
	int present_id_;
	unsigned int set_;
	msg_my_setbet()
	{
		head_.cmd_ = GET_CLSID(msg_my_setbet);
	}


	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(present_id_, data_s);
		write_data(set_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_last_random : public msg_base<none_socket>
{
public:
	int r_[15];
	msg_last_random()
	{
		head_.cmd_ = GET_CLSID(msg_last_random);
		memset(r_, 0, sizeof(int) * 15);
	}


	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(r_, 15, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

class msg_return_player_count : public msg_base<none_socket>
{
public:
	int player_count_;
	msg_return_player_count()
	{
		head_.cmd_ = GET_CLSID(msg_return_player_count);
		player_count_ = 0;
	}


	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(player_count_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_return_lottery_record : public msg_base<none_socket>
{
public:
	enum 
	{
		MAX_RECORD = 100
	};
	int count_;
	int record_[MAX_RECORD];
	msg_return_lottery_record()
	{
		head_.cmd_ = GET_CLSID(msg_return_lottery_record);
		count_ = 0;
		memset( record_,-1,sizeof(record_) );
	}


	int	 write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		
		write_data(count_, data_s);

		if(count_ > MAX_RECORD)
			count_ = MAX_RECORD;

		for(int i = 0 ; i < count_; ++i)
		{
			write_data(record_[i], data_s);
		}
		
		return ERROR_SUCCESS_0;
	}
};

class msg_broadcast_msg : public msg_base<none_socket>
{
public:
	char	str_[512];
	msg_broadcast_msg()
	{
		head_.cmd_ = GET_CLSID(msg_broadcast_msg);
		memset(str_,0,512);
	}

	int	write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(str_, 512, data_s);
		return ERROR_SUCCESS_0;
	}
};
