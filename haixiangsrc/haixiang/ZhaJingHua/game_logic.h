#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include "utility.h"
#include "game_service_base.h"
#include "game_player.h"
#include "poker_card.h"

class jinghua_logic;
typedef boost::shared_ptr<jinghua_logic> logic_ptr;
class preset_present;
class tax_info;
class msg_rand_result;
//״̬��
template<class service_t>
class state_machine : public boost::enable_shared_from_this<state_machine<service_t>>
{
public:
	state_machine()
	{
		state_id_ = 0;
	}
	boost::weak_ptr<service_t> the_logic_;
	int				state_id_; 
	virtual		int		enter();
	virtual		int		update()	{return 0;}
	virtual		int		leave()		{cancel(); return 0;}
	virtual		void	cancel()	{the_logic_.reset();}
	virtual		unsigned int		get_time_left() {return 0;};
	virtual		unsigned int		get_time_total() {return 0;};
	virtual ~state_machine()	{}
};

//״̬��
enum
{
	GET_CLSID(state_wait_start) = 1,			//��һ�ֿ�ʼ
	GET_CLSID(state_do_random) = 2,				//����
	GET_CLSID(state_rest_end) = 3,				//������
	GET_CLSID(state_wait_bet) = 5,				//�µ�ע
	GET_CLSID(state_gaming) = 6,					//��ʽ��Ϸ��(ѭ���ȴ������ע,û�о���ĳ���ʱ��,����׶β���Ҫ��ʱ)
};

class state_wait_bet : public state_machine<jinghua_logic>
{
public:
	state_wait_bet();
	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};

	void			check_all_ready(boost::system::error_code e);
private:
	boost::asio::deadline_timer timer_;
	boost::asio::deadline_timer timer2_;
};

class state_wait_start : public state_machine<jinghua_logic>
{
public:
	state_wait_start();
	bool			start_success_;
	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	void			on_wait_config_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};
private:
	boost::asio::deadline_timer timer_;
};

class state_do_random : public state_machine<jinghua_logic>
{
public:
	state_do_random();

	virtual		int		enter();
	void					on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};
	virtual		unsigned int		get_time_total();
private:
	boost::asio::deadline_timer timer_;
};

class state_rest_end : public state_machine<jinghua_logic>
{
public:
	state_rest_end();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		boost::posix_time::time_duration du = timer_.expires_from_now();
		return du.total_milliseconds();
	};
private:
	boost::asio::deadline_timer timer_;
};

class		query_bet
{
public:
	string				uid_;
	int						replied_;
	int						pos_;
	query_bet()
	{
		replied_ = 0;
		pos_ = 0;
	}
};
//û��ȷ���ĳ���ʱ��
class state_gaming : public state_machine<jinghua_logic>
{
public:
	state_gaming();

	virtual		int		enter();
	void			on_time_expired(boost::system::error_code e);
	virtual		unsigned int		get_time_left() 
	{
		return -1;
	};
private:
	int		count;
	boost::asio::deadline_timer timer_;
};

typedef boost::shared_ptr<state_machine<jinghua_logic>> ms_ptr;

class bet_info
{
public:
	string				uid_;
	unsigned int	bet_count_;
};

class		game_report_data
{
public:
	string				uid_;
	longlong			pay_;
	longlong			actual_win_;		//ʵ��
	longlong			should_win_;		//Ӧ��Ӯ����
	int						present_id_;		//Ѻ�ĸ�
	game_report_data()
	{
		pay_ = actual_win_ = should_win_ = present_id_ = 0;
	}
};

class		turn_info
{
public:
	unsigned int	time_;
	unsigned int	present_id_;
	unsigned int  turn_;
	unsigned int	factor_;
	int						first_;
	int						second_;
	turn_info()
	{
		time_ = present_id_ = turn_ = 0;
		first_ = second_ = 0;
	}
};


#define MAX_SEATS 5
struct scene_set;
class jinghua_logic : public logic_base<jinghua_logic>
{
public:
	int									id_;											//�ڼ�����Ϸ
	std::string					strid_;						
	player_ptr					is_playing_[MAX_SEATS];		//��Ϸ��
	vector<player_ptr>	observers_;								//�۲���
	unsigned int				turn_;
	int									last_win_pos_;						//�ϴ�
	bool								is_waiting_config_;
	vector<jinghua_card>	shuffle_cards_;
	scene_set*					scene_set_;	

	//ÿ�ֵ���Ϸ���ݣ�����ÿ�ֿ�ʼʱ����
	longlong						bets_pool_;								//Ѻ���
	int									next_pos_;								//��ǰѺעλ��
	query_bet						query_inf;

	int									last_is_openset_;					//�Ƿ��ѿ��������ע
	int									last_bet_;								//�ϴ���ע
	int									force_over_;							//�ﵽ�������������ǿ�ƽ���
	jinghua_logic(int is_match);
	//�õ���һ����Ч����ҵ�λ��,��Ϊ��ҿ�������;�˳�
	int								get_next_valid_pos();
	bool							is_ingame( player_ptr pp )
	{
		for (int i = 0 ; i < MAX_SEATS; i++)
		{
			if (!is_playing_[i].get()) continue;
			if (is_playing_[i]->uid_ == pp->uid_){
				return true;
			}
		}
		return false;
	}
	//ѯ����ע
	bool							query_next_bet();
	bool							this_turn_overed();
	void							start_logic();
	void							stop_logic();
	int								get_playing_count(bool ready_only = false)
	{
		int ret = 0;
		for (int i = 0; i < MAX_SEATS; i++){
			if(is_playing_[i].get()){
				if (ready_only && is_playing_[i]->is_ready_)	{
					ret++;
				}
				else if(!ready_only)
					ret++;
			}
		}
		return ret;
	}

	//�õ�δ�����������
	int							get_still_gaming_count()
	{
		int ret = 0;
		for (int i = 0; i < MAX_SEATS; i++){
			if (!is_playing_[i].get()) continue;
			if (is_playing_[i]->turn_cards_.empty()) continue;
			if (!is_playing_[i]->is_giveup_){
				ret++;
			}
		}
		return ret;
	}

	void						pk(int pos1, int pos2);

	//���ѡ��һ��ׯ��
	int							random_choose_start();

	void						change_to_gaming()
	{
		ms_ptr pms;
		pms.reset( new state_gaming);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}
	void						change_to_wait_start()
	{
		ms_ptr pms;
		pms.reset( new state_wait_start);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}
	void						change_to_wait_bet()
	{
		ms_ptr pms;
		pms.reset(new state_wait_bet);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}

	void						change_to_do_random()
	{
		ms_ptr pms;
		pms.reset( new state_do_random);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}

	void						change_to_end_rest()
	{
		ms_ptr pms;
		pms.reset( new state_rest_end);
		pms->the_logic_ = shared_from_this();
		change_to_state(pms);
	}

	void						broadcast_msg(msg_base<none_socket>& msg, bool include_observers = true);
	void						random_result();
	void						pay_deposit();	//Ѻ��ע
	int							set_bet(player_ptr pp, longlong count);
	//��һ�ֿ�ʼ
	int							this_turn_start();

	//���ֽ���
	void						will_shut_down();
	void						player_login(player_ptr pp, int seat = -1);
	void						leave_game(player_ptr pp, int pos, int why = 0 );
	void						join_player(player_ptr pp){}
	int							get_today_win(string uid, longlong& win);
	void						balance_result();
	void						send_match_result_to_player(player_ptr pp_src, int winner, player_ptr pp_dest, int is_pk = 0);
	void						send_card_to_player( player_ptr pp_src, player_ptr  pp_dest);
	void						save_match_result(player_ptr pp);
	//����Ƿ���Լ�������ȥ
	int							can_continue_play(player_ptr pp);
private:
	vector<game_report_data> turn_reports_;		//���ֽ�����
	ms_ptr							current_state_;
	void								change_to_state(ms_ptr ms);
};

