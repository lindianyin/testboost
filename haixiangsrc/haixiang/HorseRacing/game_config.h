#pragma once
#include "game_player.h"
#include "game_logic.h"
#include "service.h"

typedef		boost::shared_ptr<remote_socket<horserace_player>> remote_socket_ptr;
extern		horse_racing_service  the_service;

typedef		horserace_player		game_player_type;
typedef		horse_racing_logic	game_logic_type;

#define		EXTRA_LOGIN_CODE()\
	the_service.player_login(pp);

#define		MAX_SEATS 200
#define     MAX_USER  200

//������������Ϣ
class msg_server_parameter : public msg_base<none_socket>
{
public:
	int					racing_time_;		//����ʱ��(��);
	int					road_length_;		//�ܵ�����
	int					chip_id_[5];		//����id
	int					chip_cost_[5];	//����۸�	
	msg_server_parameter()
	{
		head_.cmd_ = GET_CLSID(msg_server_parameter);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(racing_time_, data_s);
		write_data<int>(road_length_, data_s);
		write_data<int>(chip_id_, 5, data_s, true);
		write_data<int>(chip_cost_, 5, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

#define	 SEND_SERVER_PARAM()
