#pragma once
#include "boost/shared_ptr.hpp"
#include "game_player.h"
#include "service.h"
#include "game_logic.h"

#define		GAME_ID		WEB_ZHAJINGHUA

//������������Ϣ
class msg_server_parameter : public msg_base<none_socket>
{
public:
	int					min_guarantee_;	//�μ���Ϸ��ͻ�������
	int					is_free_;				//ʹ�õ�����ѵĻ���
	int					max_cap_;				//��ҿ������Ǯ
	int					min_cap_;				//��ע
	int					total_cap_;			//�ܷⶥ��
	int					player_tax_;		//������
	msg_server_parameter()
	{
		head_.cmd_ = GET_CLSID(msg_server_parameter);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(min_guarantee_, data_s);
		write_data(is_free_, data_s);
		write_data(max_cap_, data_s);
		write_data(min_cap_, data_s);
		write_data(total_cap_, data_s);
		write_data(player_tax_, data_s);
		return ERROR_SUCCESS_0;
	}
};

#define	 SEND_SERVER_PARAM()\
for(int i = 0 ; i < (int)the_service.scenes_.size(); i++)\
{\
	scene_set& sc = the_service.scenes_[i];\
	msg_server_parameter msg;\
	msg.is_free_ = the_service.the_config_.is_free_;\
	msg.min_guarantee_ = the_service.the_config_.currency_require_;\
	msg.max_cap_ = the_service.the_config_.player_bet_cap_;\
	msg.min_cap_ = the_service.the_config_.player_bet_lowcap_;\
	msg.total_cap_ = the_service.the_config_.total_bet_cap_;\
	msg.player_tax_ = the_service.the_config_.player_tax_;\
	send_msg(from_sock_, msg);\
}

typedef boost::shared_ptr<remote_socket<jinghua_player>> remote_socket_ptr;
extern	jinghua_service		the_service;
typedef jinghua_logic			game_logic_type;
typedef jinghua_player		game_player_type;

#define		EXTRA_LOGIN_CODE()