#pragma once

#define		MAX_SEATS  200

#include "game_player.h"
#include "game_logic.h"
#include "service.h"
#include "platform_gameid.h"

typedef		boost::shared_ptr<remote_socket<beauty_player>> remote_socket_ptr;
extern		beauty_beast_service  the_service;

typedef		beauty_beast_service	service_type;
typedef		beauty_player		game_player_type;
typedef		beatuy_beast_logic		game_logic_type;

#define  MAX_USER 500

#define  CHIP_COUNT 6

//服务器参数信息
class msg_server_parameter : public msg_base<none_socket>
{
public:
	int					min_guarantee_;	//进入房间的最低金额
	int					low_cap_;				//底注
	int					player_tax_;		//茶水费
	int					enter_scene_;		//是否进入场次信息。1是，0服务器参数，2， 开始 3服务器参数配置结束
	int					pid_[PRESETS_COUNT];				//礼物id
	float					rate_[PRESETS_COUNT];				//礼物赔率
	int					chip_id_[CHIP_COUNT];		//筹码id
	int					chip_cost_[CHIP_COUNT];	//筹码价格	
	msg_server_parameter()
	{
		head_.cmd_ = GET_CLSID(msg_server_parameter);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(min_guarantee_, data_s);
		write_data(low_cap_, data_s);
		write_data(player_tax_, data_s);
		write_data(enter_scene_, data_s);
		write_data<int>(pid_, PRESETS_COUNT, data_s, true);
		write_data<float>(rate_, PRESETS_COUNT, data_s, true);
		write_data<int>(chip_id_, CHIP_COUNT, data_s, true);
		write_data<int>(chip_cost_, CHIP_COUNT, data_s, true);
		return ERROR_SUCCESS_0;
	}
};

#define	  SEND_SERVER_PARAM()

#define		EXTRA_LOGIN_CODE()\
	the_service.player_login(pp)