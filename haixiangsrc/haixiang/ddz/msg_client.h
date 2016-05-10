#pragma once
#include "msg_from_client.h"
#include "utility.h"
#include "error_define.h"
//#include "service.h"
#include "msg_client_common.h"

enum
{
	GET_CLSID(msg_set_bets_req) = 100,
	GET_CLSID(msg_query_to_become_banker_rsp),
	GET_CLSID(msg_want_sit_req),
	GET_CLSID(msg_play_cards),				//����
	GET_CLSID(msg_start_random_req),		
	GET_CLSID(msg_misc_data_req),			//��������
	GET_CLSID(msg_play_cards_tips),		//��ʾ����
	GET_CLSID(msg_play_open_deal),		//����
	GET_CLSID(msg_play_auto),					//�й�
};

class msg_set_bets_req : public msg_authrized<remote_socket_ptr>
{
public:
	int			count_;
	msg_set_bets_req()
	{
		head_.cmd_ = GET_CLSID(msg_set_bets_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data(count_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			handle_this();
};

class msg_query_to_become_banker_rsp : public msg_authrized<remote_socket_ptr>
{
public:
	int			agree_;		//�Ƿ�ͬ��
	msg_query_to_become_banker_rsp()
	{
		head_.cmd_ = GET_CLSID(msg_query_to_become_banker_rsp);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(agree_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_want_sit_req : public msg_authrized<remote_socket_ptr>
{
public:
	int				pos_;
	msg_want_sit_req()
	{
		head_.cmd_ = GET_CLSID(msg_want_sit_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(pos_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

//����
class msg_play_cards : public msg_authrized<remote_socket_ptr>
{
public:
	int		count_;		//���Ƶ�����
	int		card_[20];	//������
	msg_play_cards()
	{
		head_.cmd_ = GET_CLSID(msg_play_cards);
		count_ = 0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(count_, data_s);
		read_data<int>(card_, count_, data_s, true);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_start_random_req : public msg_authrized<remote_socket_ptr>
{
public:
	int		view_present_;
	msg_start_random_req()
	{
		head_.cmd_ = GET_CLSID(msg_start_random_req);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(view_present_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_misc_data_req : public msg_authrized<remote_socket_ptr>
{
public:
	msg_misc_data_req()
	{
		head_.cmd_ = GET_CLSID(msg_misc_data_req);
	}

	int			handle_this();
};


//������ʾ
class msg_play_cards_tips : public msg_authrized<remote_socket_ptr>
{
public:
	msg_play_cards_tips()
	{
		head_.cmd_ = GET_CLSID(msg_play_cards_tips);
	}
	int			handle_this();
};


//����
class msg_play_open_deal : public msg_authrized<remote_socket_ptr>
{
public:
	msg_play_open_deal()
	{
		head_.cmd_ = GET_CLSID(msg_play_open_deal);
	}
	int			handle_this();
};


//�й�
class msg_play_auto : public msg_authrized<remote_socket_ptr>
{
public:
	int		play_auto;    //1��ʾ�й� 0��ʾȡ���й�

	int	read(const char*& data_s, unsigned int l)
	{
		msg_authrized::read(data_s, l);
		read_data(play_auto, data_s);
		return ERROR_SUCCESS_0;
	}

	msg_play_auto()
	{
		head_.cmd_ = GET_CLSID(msg_play_auto);
	}
	int			handle_this();
};

