#pragma once
#include "boost/date_time.hpp"

enum
{
	GET_CLSID(msg_cache_cmd) = 100,
	GET_CLSID(msg_cache_cmd_ret) = 101,
	GET_CLSID(msg_cache_ping) = 105,
	GET_CLSID(msg_cache_kick_account) = 207,
	GET_CLSID(msg_cache_updator_register) = 1000,
};

template<class remote_t>
class msg_cache_sync_request : public msg_base<remote_t>
{
public:
	msg_cache_sync_request()
	{
		head_.cmd_ = GET_CLSID(msg_cache_sync_request);
	}
};

template<class remote_t>
class msg_cache_sync_complete : public msg_base<remote_t>
{
public:
	msg_cache_sync_complete()
	{
		head_.cmd_ = GET_CLSID(msg_cache_sync_complete);
	}
};

template<class remote_t>
class msg_cache_updator_register : public msg_base<remote_t>
{
public:
	msg_cache_updator_register()
	{
		head_.cmd_ = GET_CLSID(msg_cache_updator_register);
		COPY_STR(key_, "FE77AC20-5911-4CCB-8369-27449BB6D806");
	}
	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data<char>(key_, max_guid, data_s);
		return 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(key_, max_guid, data_s);
		return 0;
	}
	char	key_[max_guid];
};

template<class remote_t>
class msg_cache_cmd : public msg_base<remote_t>
{
public:
	char		cache_cmd[16];
	char		key_[max_guid];
	char		val_type_; //0-double型 1-string型
	char		sequence_[max_guid];
	union
	{
		__int64	value_;
		char		str_value_[max_guid];
	};

	int			game_id_;
	msg_cache_cmd()
	{
		head_.cmd_ = GET_CLSID(msg_cache_cmd);
		val_type_ = 0;
		memset(sequence_, 0, max_guid);
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data<char>(cache_cmd, 16, data_s);
		read_data<char>(key_, max_guid, data_s);
		read_data(val_type_, data_s);
		if (val_type_ == 0)	{
			read_data(value_, data_s);
		}
		else
			read_data<char>(str_value_, max_guid, data_s);
		read_data(game_id_, data_s);
		read_data<char>(sequence_, max_guid, data_s);
		return 0;
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(cache_cmd, 16, data_s);
		write_data<char>(key_, max_guid, data_s);
		write_data(val_type_, data_s);
		if (val_type_ == 0)	{
			write_data(value_, data_s);
		}
		else
			write_data<char>(str_value_, max_guid, data_s);
		write_data(game_id_, data_s);
		write_data<char>(sequence_, max_guid, data_s);
		return 0;
	}
};

template<class remote_t>
class msg_cache_cmd_ret : public msg_base<remote_t>
{
public:
	msg_cache_cmd_ret()
	{
		head_.cmd_ = GET_CLSID(msg_cache_cmd_ret);
		return_code_ = -1;
		value_ = 0;
		memset(key_, 0, max_guid);
		memset(str_value_, 0, max_guid);
		memset(sequence_, 0, max_guid);
	} 
	int			return_code_;					//回复结果 0 成功， 1 缓存不存在， 2 请求数值过大, 3 不识别的命令
	char		key_[max_guid];
	char		val_type_; //0-double型 1-string型
	char		sequence_[max_guid];
	union
	{
		__int64	value_;
		char		str_value_[max_guid];
	};

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(return_code_, data_s);
		write_data<char>(key_, max_guid, data_s);
		write_data(val_type_, data_s);
		if (val_type_ == 0)	{
			write_data(value_, data_s);
		}
		else{
			write_data<unsigned char>((unsigned char*)str_value_, max_guid, data_s);
		}
		write_data<char>(sequence_, max_guid, data_s);
		return 0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data(return_code_, data_s);
		read_data<char>(key_, max_guid, data_s);
		read_data(val_type_, data_s);
		if (val_type_ == 0)	{
			read_data(value_, data_s);
		}
		else{
			read_data<unsigned char>((unsigned char*)str_value_, max_guid, data_s);
		}
		read_data<char>(sequence_, max_guid, data_s);
		return 0;
	}
};

template<class remote_t>
class msg_cache_ping : public msg_base<remote_t>
{
public:
	msg_cache_ping()
	{
		head_.cmd_ = GET_CLSID(msg_cache_ping);
	}
};


template<class remote_t>
class msg_cache_kick_account : public msg_base<remote_t>
{
public:
	char		uid_[max_guid];
	msg_cache_kick_account()
	{
		head_.cmd_ = GET_CLSID(msg_cache_kick_account);
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		return 0;
	}

	int			read(const char*& data_s, unsigned int l)
	{
		msg_base::read(data_s, l);
		read_data<char>(uid_, max_guid, data_s);
		return 0;
	}
};
