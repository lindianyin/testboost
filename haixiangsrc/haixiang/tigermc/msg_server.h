#pragma once
#include "msg.h"
#include "utility.h"
#include "error_define.h"
#include "game_service_base.h"
#include "msg_server_common.h"
enum
{
	GET_CLSID(msg_player_setbet)		= 5,
	GET_CLSID(msg_random_present_ret),
	GET_CLSID(msg_player_game_info_ret),
	GET_CLSID(msg_exp)
};


class msg_player_setbet : public msg_base<none_socket>
{
public:
	char		uid_[max_guid];
	int			pid;
	int			max_setted_;	//���עһ������ע
	msg_player_setbet()
	{
		head_.cmd_ = GET_CLSID(msg_player_setbet);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(pid, data_s);
		write_data(max_setted_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_random_present_ret : public msg_base<none_socket>
{
public:
	int			result_;					//����id,˳ʱ���������1��ʼ��12�㷽��, 0 �������ֵ����������
	int			result2nd_;				//�ܻ𳵽�� 1���������ͣ�2�������ͣ�0��û���н�, 3 Ѻ��С
	int			result2_[7];			//�ڻ�����н���
	int			win_;							//�ܹ�Ӯ�˶���
	msg_random_present_ret()
	{
		head_.cmd_ = GET_CLSID(msg_random_present_ret);
		memset(result2_, 0, sizeof(result2_));
		result_ = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(result_, data_s);
		write_data(result2nd_, data_s);
		write_data<int>(result2_, 7, data_s, true);
		write_data(win_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_player_game_info_ret : public msg_base<none_socket>
{
public:
	int			        game_count_;					
	longlong			max_win_;				
	longlong			today_win_;			
	longlong            month_win_;

	msg_player_game_info_ret()
	{
		head_.cmd_ = GET_CLSID(msg_player_game_info_ret);
		game_count_ = 0;
		max_win_    = 0;
		today_win_  = 0;
		month_win_  = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(game_count_, data_s);
		write_data(max_win_, data_s);
		write_data(today_win_, data_s);
		write_data(month_win_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_exp : public msg_base<none_socket>
{
public:
	double	 exp_;
	double exp_max_;
	int lv_;
	msg_exp()
	{
		head_.cmd_ = GET_CLSID(msg_exp);

	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(lv_, data_s);
		write_data(exp_, data_s);
	    write_data(exp_max_, data_s);
		
		return ERROR_SUCCESS_0;
	}
};