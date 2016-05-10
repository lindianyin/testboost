#pragma once

#define		MAX_SEATS  200

#include "game_player.h"
#include "game_logic.h"
#include "service.h"
#include "platform_gameid.h"
#include "msg_server_common.h"

typedef		boost::shared_ptr<remote_socket<beauty_player>> remote_socket_ptr;
extern		beauty_beast_service  the_service;

typedef		beauty_beast_service	service_type;
typedef		beauty_player		game_player_type;
typedef		beatuy_beast_logic		game_logic_type;

#define  MAX_USER 500

#ifdef _WHEEL
#define  CHIP_COUNT 6
#else
#define  CHIP_COUNT 5
#endif

//服务器参数信息
class msg_server_parameter : public msg_base<none_socket>
{
public:

	double			banker_deposit;							//上庄保证金
	int					pid_[PRESETS_COUNT];				//礼物id
	float				rate_[PRESETS_COUNT];				//礼物赔率
	int					chip_id_[CHIP_COUNT];				//筹码id
	int					chip_cost_[CHIP_COUNT];			//筹码价格	

	msg_server_parameter()
	{
		head_.cmd_ = GET_CLSID(msg_server_parameter);
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(banker_deposit, data_s);
		write_data<int>(pid_, PRESETS_COUNT, data_s, true);
		write_data<float>(rate_, PRESETS_COUNT, data_s, true);
		write_data<int>(chip_id_, CHIP_COUNT, data_s, true);
		write_data<int>(chip_cost_, CHIP_COUNT, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

typedef	beauty_beast_service service_type;

#define	  SEND_SERVER_PARAM()

#define		MAX_SEATS  200
#define		EXTRA_LOGIN_CODE()\
	the_service.player_login(pp)