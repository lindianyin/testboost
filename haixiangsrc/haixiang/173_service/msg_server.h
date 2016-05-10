#pragma once
#include "msg.h"
#include "utility.h"
#include "error_define.h"
#include "game_service_base.h"
#include "msg_server_common.h"

enum 
{
	GET_CLSID(msg_173_eggs_smash_ret)		=  100,
	GET_CLSID(msg_173_eggs_ret_level)		=  101,
	GET_CLSID(msg_173_eggs_service_state)   =  102  //������״̬
};

class msg_173_eggs_smash_ret : public msg_base<none_socket>
{
public:
	//code:0 �ɹ�
	//code:1 ��������
	//code:2 ����ʧ��
	//code:3 ����
	//code:4 �����ѱ�����
	//code:5 �����ѱ�����
	//code:6 ����ʧ��
	
	int         code_;  
	int         egg_ret_; //1�ɹ���0ʧ��
	int         cur_level_; //��ǰ�ȼ�
	int         op_type_;  //��Ҳ������� 0�����ҵ���1������
	int         in_come_;  //��Ǯ
	int         out_lay_;  //��Ǯ

	msg_173_eggs_smash_ret()
	{
		head_.cmd_ = GET_CLSID(msg_173_eggs_smash_ret);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(code_, data_s);
		write_data(egg_ret_, data_s);
		write_data(cur_level_, data_s);
		write_data(op_type_, data_s);
		write_data(in_come_, data_s);
		write_data(out_lay_, data_s);
		return ERROR_SUCCESS_0;
	}
};


class msg_173_eggs_ret_level: public msg_base<none_socket>
{
public:
	int         egg_level_; //1�ɹ���0ʧ��

	msg_173_eggs_ret_level()
	{
		head_.cmd_ = GET_CLSID(msg_173_eggs_ret_level);
	}
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(egg_level_, data_s);
		return ERROR_SUCCESS_0;
	}
};

class msg_173_eggs_service_state : public msg_base<none_socket>
{
public:
	enum 
	{
		SERVER_STATE_NORMAL    = 0,   //����
		SERVER_STATE_UPDATEING = 1    //������
	};

	msg_173_eggs_service_state()
	{
		head_.cmd_ = GET_CLSID(msg_173_eggs_service_state);
	}

	char     state_;

	int		write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(state_, data_s);
		return ERROR_SUCCESS_0;
	}
};