#pragma once
#include "boost/shared_ptr.hpp"
#include "msg.h"

#include "game_player.h"
#include "game_logic.h"
#include "service.h"

#define     MAX_USER  500

//服务器参数信息
class msg_server_parameter : public msg_base<none_socket>
{
public:
	int level[16];
	int min_score[16];
	int max_score[16];

	msg_server_parameter()
	{
		head_.cmd_ = 1107;
	}

	int	write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(level, 16, data_s, true);
		write_data<int>(min_score, 16, data_s, true);
		write_data<int>(max_score, 16, data_s, true);
		return 0;
	}
};

#define	  SEND_SERVER_PARAM()

#define		EXTRA_LOGIN_CODE()

typedef game_player	game_player_type;
typedef boost::shared_ptr<remote_socket<game_player>> remote_socket_ptr;
extern GoGame_service the_service;
typedef GoGame_logic game_logic_type;
typedef GoGame_service service_type;