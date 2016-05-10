#pragma once
#include "boost/shared_ptr.hpp"
#include "msg.h"

#include "game_player.h"
#include "game_logic.h"
#include "service.h"

#define     MAX_USER  500

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
		head_.cmd_ = 1107;
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
		return 0;
	}
};

#define	 SEND_SERVER_PARAM()\
	for (unsigned int i = 0; i < the_service.scenes_.size(); i++)\
	{\
		scene_set& sc = the_service.scenes_[i];\
		msg_server_parameter msg;\
		msg.is_free_ = sc.is_free_;\
		msg.low_cap_ = sc.rate_;\
		msg.min_guarantee_ = (int)sc.gurantee_set_;\
		if (i == 0){\
			msg.enter_scene_ = 2;\
		}\
		else if (i == the_service.scenes_.size() - 1){\
			msg.enter_scene_ = 3;\
		}\
		else{\
			msg.enter_scene_ = 0;\
		}\
		send_msg(from_sock_, msg);\
	}

typedef fish_player			game_player_type;
typedef boost::shared_ptr<remote_socket<fish_player>> remote_socket_ptr;
extern	fishing_service  the_service;
typedef fishing_logic		game_logic_type;
typedef	fishing_service	service_type;

#define		EXTRA_LOGIN_CODE()