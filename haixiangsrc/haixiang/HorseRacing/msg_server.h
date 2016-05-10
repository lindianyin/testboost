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
	GET_CLSID(msg_game_report)			= 12,
	GET_CLSID(msg_player_setbet)		= 13,
	GET_CLSID(msg_my_setbet)				= 14,
	GET_CLSID(msg_last_random)			= 15,
	GET_CLSID(msg_presets_change)		= 16,
	GET_CLSID(msg_my_setbet_history) = 17,
	GET_CLSID(msg_horse_popular)		= 18,
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

class msg_rand_result : public msg_base<none_socket>
{
public:
	int		rand_result_;
	vector<int>	ranks_;			//6����ƥ������
	vector<float>	speeds_;	//6����ƥ������
	msg_rand_result()
	{
		head_.cmd_ = GET_CLSID(msg_rand_result);
		rand_result_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(rand_result_, data_s);
		write_data<int>(ranks_, data_s, true);
		write_data<float>(speeds_, data_s, true);
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
	int r_[10];
	int	time_[10];
	int	turn_[10];
	int	factor_[10];
	int	first_[10];
	int second_[10];
	msg_last_random()
	{
		head_.cmd_ = GET_CLSID(msg_last_random);
		memset(r_, 0, sizeof(r_));
		memset(time_, 0, sizeof(time_));
		memset(turn_, 0, sizeof(turn_));
		memset(factor_, 0, sizeof(factor_));
		memset(first_, 0, sizeof(first_));
		memset(second_, 0, sizeof(second_));
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(r_, 10, data_s, true);
		write_data<int>(time_, 10, data_s, true);
		write_data<int>(turn_, 10, data_s, true);
		write_data<int>(factor_, 10, data_s, true);
		write_data<int>(first_, 10, data_s, true);
		write_data<int>(second_, 10, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

class msg_presets_change : public msg_base<none_socket>
{
public:
	vector<int>	pids_;		//pids
	vector<int> factors_;	//����
	msg_presets_change()
	{
		head_.cmd_ = GET_CLSID(msg_presets_change);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(pids_, data_s, true);
		write_data<int>(factors_, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

class msg_my_setbet_history : public msg_base<none_socket>
{
public:
	int		time_[10];				//ʱ��
	int		result_[10];			//���
	int		pay_[10];				//ѹ����
	int		win_[10];				//Ӯ����
	int		factor_[10];			//���
	int		turn_[10];
	msg_my_setbet_history()
	{
		head_.cmd_ = GET_CLSID(msg_my_setbet_history);
		memset(time_, 0, sizeof(time_) * 6);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(time_, 10, data_s, true);
		write_data<int>(result_, 10, data_s, true);
		write_data<int>(pay_, 10, data_s, true);
		write_data<int>(win_, 10, data_s, true);
		write_data<int>(factor_, 10, data_s, true);
		write_data<int>(turn_, 10, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

class msg_horse_popular : public msg_base<none_socket>
{
public:
	int		pop_[6];
	msg_horse_popular()
	{
		head_.cmd_ = GET_CLSID(msg_horse_popular);
		memset(pop_, 0, sizeof(pop_));
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(pop_, 6, data_s, true);
		return ERROR_SUCCESS_0;
	}
};