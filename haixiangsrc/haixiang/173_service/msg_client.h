#pragma once
#include "msg_from_client.h"
#include "error_define.h"
#include "utility.h"
#include "service.h"
#include "msg_client_common.h"

enum 
{
	GET_CLSID(msg_173_eggs_login) = 10,
	GET_CLSID(msg_173_eggs_smash) = 11,  //砸蛋
	GET_CLSID(msg_173_eggs_open) =  12  //开蛋
};



class msg_173_eggs_login : public msg_from_client<remote_socket_ptr>
{
public:
	char		uid_[max_guid];
	char		uname_[max_name];
	int			vip_lv_;
	char		head_icon_[max_guid];
	char		uidx_[max_name];//php用到
	char		session_[max_guid];
	char		url_sign_[max_guid];
	int			sex_;
	msg_173_eggs_login()
	{
		head_.cmd_ = GET_CLSID(msg_173_eggs_login);
		uidx_[4] = 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data(vip_lv_, data_s);
		write_data<char>(head_icon_, max_guid, data_s);
		write_data<char>(uidx_, max_name, data_s);
		write_data<char>(session_, max_guid, data_s);
		write_data<char>(url_sign_, max_guid, data_s);
		write_data(sex_, data_s);
		return ERROR_SUCCESS_0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		read_data<char>(uname_, max_name, data_s);
		read_data(vip_lv_, data_s);
		read_data<char>(head_icon_, max_guid, data_s);
		read_data<char>(uidx_, max_name, data_s);
		read_data<char>(session_, max_guid, data_s);
		read_data<char>(url_sign_, max_guid, data_s);
		read_data(sex_, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};


class msg_173_eggs_smash : public msg_from_client<remote_socket_ptr>
{
public:
	msg_173_eggs_smash()
	{
		head_.cmd_ = GET_CLSID(msg_173_eggs_smash);
	}

	char		uid_[max_guid];   //用户ID
	char		uidx_[max_name];  //平台ID

	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uidx_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		read_data<char>(uidx_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

class msg_173_eggs_open : public msg_from_client<remote_socket_ptr>
{
public:
	msg_173_eggs_open()
	{
		head_.cmd_ = GET_CLSID(msg_173_eggs_open);
	}

	char		uid_[max_guid];   //用户ID
	char		uidx_[max_name];  //平台ID

	int			write(char*& data_s, unsigned int l)
	{
		msg_from_client::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(uidx_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_from_client::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		read_data<char>(uidx_, max_name, data_s);
		return ERROR_SUCCESS_0;
	}
	int			handle_this();
};

