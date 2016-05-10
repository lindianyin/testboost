#pragma once
#include "utility.h"
#include "error_define.h"
#include "msg.h"
#include "msg_server_common.h"
enum 
{
	GET_CLSID(msg_fish_sync) = 10,
	GET_CLSID(msg_fish_evade),
	GET_CLSID(msg_gun_switch),
	GET_CLSID(msg_fire_at),
	GET_CLSID(msg_fish_hitted),
	GET_CLSID(msg_fish_cloud_coming),
	GET_CLSID(msg_match_report),
	GET_CLSID(msg_fish_used),
	GET_CLSID(msg_lock_fish),
	GET_CLSID(msg_fish_clean),
	GET_CLSID(msg_update_path),
	GET_CLSID(msg_boss_lottery_change),
	GET_CLSID(msg_beauty_host_info),
	GET_CLSID(msg_host_screenshoot),
	GET_CLSID(msg_send_beauty_present),
	GET_CLSID(msg_beauty_present_set),
	GET_CLSID(msg_beauty_host_leave),
	GET_CLSID(msg_host_start),
};

//鱼信息同步
class msg_fish_sync : public msg_base<none_socket>
{
public:
	int			iid_;
	char		fish_type_[max_name];
	float		x, y;				//位置
	float		speed_;			//速度
	float		mv_step_;		//步长
	int			reward_;			//倍率
	float		percent_;			//第一段路完成比
	int			data_;
	longlong	lottery_;
	short		path_count_;
	std::vector<short>	path_;	//路径要包含当前的前一点
	MSG_CONSTRUCT(msg_fish_sync);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data<char>(fish_type_,max_name, data_s);
		write_data(x, data_s);
		write_data(y, data_s);
		write_data(speed_, data_s);
		write_data(mv_step_, data_s);
		write_data(reward_, data_s);
		write_data(percent_, data_s);
		write_data(data_, data_s);
		write_data(lottery_, data_s);
		write_data(path_count_, data_s);
		write_data<short>(path_, data_s, true);
		return 0;
	}
};

class msg_fish_evade : public msg_base<none_socket>
{
public:
	int		iid_;
	int		acc_start_;
	int		acc_mid_;
	int		acc_end_;
	int		time_elapse_;
	float	duration_;
	MSG_CONSTRUCT(msg_fish_evade);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data(acc_start_, data_s);
		write_data(acc_mid_, data_s);
		write_data(acc_end_, data_s);
		write_data(time_elapse_, data_s);
		write_data(duration_, data_s);
		return 0;
	}
};

//火力切换
class msg_gun_switch : public msg_base<none_socket>
{
public:
	int			pos_;
	int			level_;
	MSG_CONSTRUCT(msg_gun_switch);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(level_, data_s);
		return 0;
	}
};

//开火
class msg_fire_at : public msg_base<none_socket>
{
public:
	int			pos_;
	short		x, y;
	MSG_CONSTRUCT(msg_fire_at);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(x, data_s);
		write_data(y, data_s);

		return 0;
	}
};

//击中鱼
class	msg_fish_hitted : public msg_base<none_socket>
{
public:
	int					iid_;
	int					result_;		//0,死亡 1.连锁，2.全屏，3 同类消除 4 被连带杀死，不飘字，5 boss鱼被杀死, 6 boss鱼被击中暴金币
	longlong		data_;			//数据,如果死亡，则表示多少钱
	int					pos_;				//被谁打死的
	MSG_CONSTRUCT(msg_fish_hitted);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data(result_, data_s);
		write_data(data_, data_s);
		write_data(pos_, data_s);
		return 0;
	}
};

class msg_fish_cloud_coming : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_fish_cloud_coming);
	int			type_; //0 随机鱼，1鱼群 2 boss
	int			map_id_;
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(type_, data_s);
		write_data(map_id_, data_s);
		return 0;
	}
};

class msg_match_report : public msg_base<none_socket>
{
public:
	longlong		total_gold_;
	longlong		total_exp_;
	unsigned int		count_;
	char	name_[18][32];
	int		ncount_[18];
	MSG_CONSTRUCT(msg_match_report);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(total_gold_, data_s);
		write_data(total_exp_, data_s);
		write_data(count_, data_s);
		if (count_ > 18) count_ = 18;
		for (int i = 0 ; i <(int) count_; i++)
		{
			write_data<char>(name_[i] + 0, 32, data_s);
			write_data(ncount_[i], data_s);
		}
		return 0;
	}	
};

class	msg_fish_used : public msg_base<none_socket>
{
public:
	int		count_;
	char  type_[64][8];
	MSG_CONSTRUCT(msg_fish_used);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(count_, data_s);
		for (int i = 0; i < count_; i++)
		{
			write_data<char>(type_[i] + 0, 8, data_s);
		}
		return 0;
	}
};

class msg_lock_fish : public msg_base<none_socket>
{
public:
	int			pos_;
	int			iid_; //<0表示取消锁定
	MSG_CONSTRUCT(msg_lock_fish);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos_, data_s);
		write_data(iid_, data_s);
		return 0;
	}
};

class msg_fish_clean : public msg_base<none_socket>
{
public:
	int		iid_;
	MSG_CONSTRUCT(msg_fish_clean);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		return 0;
	}
};


class msg_update_path : public msg_base<none_socket>
{
public:
	int			iid_;
	float		speed_;
	int			x;
	int			y;
	short		path_count_;
	std::vector<short>		path_;	//路径要包含当前的前一点
	MSG_CONSTRUCT(msg_update_path);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data(speed_, data_s);
		write_data(x, data_s);
		write_data(y, data_s);
		write_data(path_count_, data_s);
		write_data<short>(path_, data_s, true);
		return 0;
	}
};

class msg_boss_lottery_change : public msg_base<none_socket>
{
public:
	longlong	lottery_;
	MSG_CONSTRUCT(msg_boss_lottery_change);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(lottery_, data_s);
		return 0;
	}
};

//主播信息
class	msg_beauty_host_info : public msg_base<none_socket>
{
public:
	int					room_id_;
	int					host_id_;
	char				ip_[max_name];
	short				port_;
	int					is_online_;
	int					TAG_;	//广播中的最后一个主播, 1 主播列表中的第一条，3 主播列表中的最后一条，8 进入的游戏主播， 0 主播列表的中间条
	int					player_count_;
	int					popular_;		//人气
	char				host_name_[max_name];
	MSG_CONSTRUCT(msg_beauty_host_info);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(room_id_, data_s);
		write_data(host_id_, data_s);
		write_data<char>(ip_, max_name, data_s);
		write_data(port_, data_s);
		write_data(is_online_, data_s);
		write_data(TAG_, data_s);
		write_data(player_count_, data_s);
		write_data(popular_, data_s);
		write_data<char>(host_name_, max_name, data_s);
		return 0;
	}
};

class msg_host_screenshoot : public msg_base<none_socket>
{
public:
	int		iid_;
	int		TAG_;	//1开始，2，结束，0，中间
	int		len_;
	char	data_[512];
	MSG_CONSTRUCT(msg_host_screenshoot);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(iid_, data_s);
		write_data(TAG_, data_s);
		write_data(len_, data_s);
		write_data<unsigned char>((unsigned char*)data_, len_, data_s);
		return 0;
	}
};

class msg_send_beauty_present : public msg_base<none_socket>
{
public:
	char		pos_[max_name];
	int			to_host_;
	int			present_id_;
	int			count_;
	int			popular_;
	MSG_CONSTRUCT(msg_send_beauty_present);
	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(pos_, max_name, data_s);
		write_data(to_host_, data_s);
		write_data(present_id_, data_s);
		write_data(count_, data_s);
		write_data(popular_, data_s);
		return 0;
	}
};

class	msg_beauty_present_set : public msg_base<none_socket>
{
public:
	int			id[16];
	int			cost[16];
	MSG_CONSTRUCT(msg_beauty_present_set);
	void		init()
	{
		memset(id, 0, sizeof(id));
		memset(cost, 0, sizeof(cost));
	}

	int			write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<int>(id, 16, data_s, true);
		write_data<int>(cost, 16, data_s, true);
		return 0;
	}
};

class msg_beauty_host_leave : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_beauty_host_leave);
};

class msg_host_start : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_host_start);
};
