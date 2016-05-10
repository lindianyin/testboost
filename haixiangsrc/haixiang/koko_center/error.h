#pragma once
#include "boost/thread.hpp"

#define GET_CLSID(m) m##_id

enum 
{
	error_wrong_sign = -1000,	//����ǩ������
	error_wrong_password,			//���벻��
	error_sql_execute_error,	//sql���ִ��ʧ��
	error_no_record,					//���ݿ���û����Ӧ��¼
	error_user_banned,				//�û�����ֹ
	error_account_exist,			//ע���˻����Ѵ���
	error_server_busy,				//��������æ
	error_cant_find_player,		//�Ҳ������
	error_cant_find_match,
	error_cant_find_server,
	error_msg_ignored,
	error_cancel_timer,
	error_cannt_regist_more,  //������ע����
	error_email_inusing,
	error_mobile_inusing,
	error_wrong_verify_code,
	error_time_expired,
	error_invalid_data,
	error_success = 0,
	error_business_handled,
	error_invalid_request,
	error_not_enough_gold,
	error_not_enough_gold_game,
	error_not_enough_gold_free,
	error_cant_find_coordinate,
};

enum 
{
	GET_CLSID(msg_koko_trade_inout_ret) = 998,
	GET_CLSID(msg_user_login_ret_delegate) = 999,
	GET_CLSID(msg_user_login_ret) = 1000,
	GET_CLSID(msg_common_reply) = 1001,//ռλ����
	GET_CLSID(msg_common_reply_internal) = 6000,//ռλ����
	GET_CLSID(msg_player_info) = 1002,
	GET_CLSID(msg_player_leave),
	GET_CLSID(msg_chat_deliver),
	GET_CLSID(msg_same_account_login),
	GET_CLSID(msg_live_show_start),
	GET_CLSID(msg_turn_to_show),
	GET_CLSID(msg_host_apply_accept),
	GET_CLSID(msg_channel_server),
	GET_CLSID(msg_image_data),
	GET_CLSID(msg_user_image),
	GET_CLSID(msg_account_info_update),
	GET_CLSID(msg_verify_code),
	GET_CLSID(msg_check_data_ret),
	GET_CLSID(msg_live_show_stop),
	GET_CLSID(msg_sync_item),
	GET_CLSID(msg_region_data),
	GET_CLSID(msg_srv_progress),
	GET_CLSID(msg_sync_token),
	GET_CLSID(msg_confirm_join_game_deliver),
};

enum
{
	GET_CLSID(msg_koko_trade_inout) = 998,
	GET_CLSID(msg_user_login) = 100,
	GET_CLSID(msg_user_register),
	GET_CLSID(msg_join_channel),
	GET_CLSID(msg_chat),
	GET_CLSID(msg_leave_channel),
	GET_CLSID(msg_host_apply),
	GET_CLSID(msg_show_start),
	GET_CLSID(msg_user_login_channel),
	GET_CLSID(msg_upload_screenshoot),
	GET_CLSID(msg_show_stop),
	GET_CLSID(msg_change_account_info),
	GET_CLSID(msg_get_verify_code),
	GET_CLSID(msg_check_data),
	GET_CLSID(msg_user_login_delegate),
	GET_CLSID(msg_send_present),
	GET_CLSID(msg_get_token),
	GET_CLSID(msg_action_record),
	GET_CLSID(msg_get_game_coordinate),
};

enum 
{
	GET_CLSID(msg_register_to_world) = 19999,
	GET_CLSID(msg_match_info) = 20000,
	GET_CLSID(msg_match_start) = 20001,
	GET_CLSID(msg_match_score) = 20002,
	GET_CLSID(msg_match_end) = 20003,
	GET_CLSID(msg_player_distribute) = 20004,
	GET_CLSID(msg_confirm_join_game) = 20005,
	GET_CLSID(msg_confirm_join_game_reply) = 20006,
	GET_CLSID(msg_match_register) = 20007,
	GET_CLSID(msg_match_cancel) = 20008,
	GET_CLSID(msg_query_info) = 20009,
	GET_CLSID(msg_match_data) = 20010,
	GET_CLSID(msg_game_data) = 20011,
	GET_CLSID(msg_host_data) = 20012,
	GET_CLSID(msg_game_room_data) = 20013,
	GET_CLSID(msg_my_match_score) = 20014,
	GET_CLSID(msg_my_game_data) = 20015,
	GET_CLSID(msg_match_champion) = 20016,
	GET_CLSID(msg_match_report) = 20017,
	GET_CLSID(msg_match_over) = 20018,
	GET_CLSID(msg_query_data_begin),
	GET_CLSID(msg_player_eliminated_by_game),
};

enum 
{
	item_id_gold,
	item_id_gold_game,
	item_id_gold_free,
	item_id_gold_game_lock,
};

enum 
{
	client_action_logingame,
	client_action_logoutgame,

};
struct end_point
{
	char	ip_[32];
	int		port_;
};

//ƽ̨����ʱ����������������Ϸ�������������ļ�
//�ͻ��˾����˴�������

//����������Ϣ
struct host_info
{
	int			roomid_;	//����ID
	char		roomname_[128];	//��������
	char		host_uid_[64]; //����ID
	char		thumbnail_[128]; //����ͼ
	end_point	avaddr_; //��Ƶ��������ַ
	end_point	game_addr_; //��Ϸ��������ַ
	int			gameid_;//������Ϸid
	int			popular_;
	int			online_players_;
	int			is_online_;
	int			avshow_start_;
	host_info()
	{
		popular_ = 0;
		online_players_ = 0;
		is_online_ = 0;
		gameid_ = 0;
		roomid_ = 0;
		roomname_[0] = 0;
		host_uid_[0] = 0;
		thumbnail_[0] = 0;
		avshow_start_ = 0;
	}
};


struct game_room_inf
{
	int						game_id_;
	int						room_id_;
	char					room_name_[64];
	char					room_desc_[64];
	char					ip_[32];
	int						port_;
	char					thumbnail_[128];			//����ͼ
	char					require_[256];
	game_room_inf()
	{
		require_[0] = 0;
	}
};

//��Ϸ������Ϣ��ƽ̨����ʱ��������
struct game_info
{
	enum 
	{
		game_cat_classic,
		game_cat_match,
		game_cat_host,
	};
	enum 
	{

	};
	int						id_;							//��Ϸid	
	int						type_;						//0 �ͻ�����Ϸ��1 flash��Ϸ
	char					dir_[64];					//Ŀ¼��ֻ������һ��Ŀ¼������������·��
	char					exe_[64];					//��ִ�г�����
	char					update_url_[128];	//����·��
	char					help_url_[128];		//����·��
	char					game_name_[128];	//��Ϸ����
	char					thumbnail_[128];	//����ͼ
	char					solution_[16];		//��Ϸ����Ʒֱ���,���Ϊnoembed,��ʾ��Ƕ��ƽ̨����
	int						no_embed_;
	char					catalog_[128];
	std::vector<game_room_inf> rooms_;
};

struct channel_server
{
	std::string ip_;
	int		port_;
	int		online_;
	std::vector<int> game_ids_;
};

struct match_info : public boost::enable_shared_from_this<match_info>
{
	int						id_;						//����id
	int						match_type_;		//�������� 0-��ʱ��, 1-���˴�����,2-��̭��
	int						game_id_;				//��Ϸid����Ӧgame_info����Ϸid
	std::string		match_name_;		//��������
	std::string		thumbnail_;			//����ͼ
	std::string		help_url_;			//��������	
	std::string		prize_desc_;		//��������
	int						start_type_	;		//����ʱ��,0��ʱ����,1�������� 2�汨�濪
	int						require_count_;	//�����������Ҫ��

	int						start_stripe_;	//start_type_ = 0ʱ������ֶ���Ч, 0 ���ʱ�俪, 1 �̶�ʱ�俪
	union 
	{
		int					wait_register_; //start_strip_ = 0 ʱ�� �ȴ�ע��ʱ��,��Ϊ��λ
		struct											//start_strip_ = 1 ʱ��
		{
			int				day, h, m, s;		//�������, ������� ʱ���֣���
		};
	};

	int						need_players_per_game_;					//ÿ����Ϸ��Ҫ���˲���
	int						eliminate_phase1_basic_score_;	//��̭����ѡ�׶γ�ʼ��
	int						eliminate_phase1_inc;						//��̭����ѡ�׶λ���������(���ڻ����ֵı���̭,�Է���Ϊ��λ��)
	int						elininate_phase2_count_;				//��̭����������׶�����

	int						end_type_;		//����ʱ�� 0��ʱ������1 ����ʤ��
	int						end_data_;		//���� end_type_ = 0, �����ǽ���ʱ��, = 1���Ǿ���ʤ����

	std::vector<std::vector<std::pair<int, int>>>	prizes_;		//����(��������Ĵ���)����Ʒid,������
	std::map<int, int>	cost_;										//������ ��Ʒid,������
	std::string		srvip_;
	int						srvport_;
	match_info() 
	{
		eliminate_phase1_inc = 0;
		elininate_phase2_count_ = 0;
		eliminate_phase1_basic_score_ = 0;
		need_players_per_game_ = 4;
		end_type_ = 0;
		end_data_ = 0;
		start_stripe_ = 0;
		wait_register_ = 0;
		h = m = s = 0;
	};
	virtual ~match_info(){}
};

template<class data_type>
class atomic_value
{
public:
	void				store(data_type v)
	{
		boost::lock_guard<boost::mutex> lk(mtu_);
		dat_ = v;
	}
	data_type		load()
	{
		data_type ret;
		boost::lock_guard<boost::mutex> lk(mtu_);
		ret = dat_;
		return ret;
	}
protected:
	data_type		dat_;
	boost::mutex	mtu_;
};

template<class socket_t>
void	send_all_game_to_player(socket_t psock)
{
	auto it = all_games.begin();
	while (it !=  all_games.end())
	{
		gamei_ptr pgame = it->second; it++;
		msg_game_data msg;
		msg.inf = *pgame;
		send_msg(psock, msg);
	}
}

template<class socket_t>
void	send_all_match_to_player(socket_t psock)
{
	auto it = match_manager::get_instance()->all_matches.begin();
	while (it !=  match_manager::get_instance()->all_matches.end())
	{
		match_ptr pmatch = it->second; it++;
		msg_match_data msg;
		msg.inf = *pmatch;
		send_msg(psock, msg);
	}
}
