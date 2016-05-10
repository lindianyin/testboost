#pragma once
#include "game_logic.h"
#include "game_player.h"
#include "service.h"
#include "msg_server_common.h"

typedef		boost::shared_ptr<remote_socket<tiger_player>> remote_socket_ptr;
extern		tigermc_service		the_service;

typedef		tigermc_service		service_type;
typedef		tiger_player			game_player_type;
typedef		tiger_logic				game_logic_type;

typedef     tigermc_service         service_type;


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
	int					enter_scene_;		//�Ƿ���볡����Ϣ��1�ǣ�0����
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

#define	 SEND_SERVER_PARAM()\
	for (unsigned int i = 0; i < the_service.scenes_.size(); i++)\
{\
	scene_set& sc = the_service.scenes_[i];\
	msg_server_parameter msg;\
	msg.is_free_ = sc.is_free_;\
	msg.min_guarantee_ = (int)sc.gurantee_set_;\
	msg.banker_set_ = (int)sc.to_banker_set_;\
	msg.low_cap_ = (int)(sc.gurantee_set_ - sc.player_tax_) / 50;\
	msg.player_tax_ = sc.player_tax_;\
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
#define EXTRA_LOGIN_CODE()