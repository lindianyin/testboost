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
	GET_CLSID(msg_173_eggs_service_state)   =  102  //服务器状态
};

class msg_173_eggs_smash_ret : public msg_base<none_socket>
{
public:
	//code:0 成功
	//code:1 其他错误
	//code:2 消费失败
	//code:3 余额不足
	//code:4 请求已被处理
	//code:5 请求已被处理
	//code:6 连接失败
	
	int         code_;  
	int         egg_ret_; //1成功，0失败
	int         cur_level_; //当前等级
	int         op_type_;  //玩家操作类型 0代表砸蛋，1代表开蛋
	int         in_come_;  //加钱
	int         out_lay_;  //扣钱

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
	int         egg_level_; //1成功，0失败

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
		SERVER_STATE_NORMAL    = 0,   //正常
		SERVER_STATE_UPDATEING = 1    //更新中
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