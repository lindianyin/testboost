#pragma once
#include "net_service.h"
#include <string>
#include "player_base.h"
#include "net_service.h"
#include "msg.h"

using namespace std;
class GoGame_logic;
class game_player : public player_base<game_player, remote_socket<game_player>>
{
public:
	int score;							//��һ���
	int win_total;					//ʤ������
	int fail_total;					//ʧ�ܴ���
	int draw_total;					//�������
	int eat_number;					//������

	bool is_ready;											//�Ƿ��Ѿ�׼��
	int offensive_move;									//�Ƿ����� 1Ϊ����
	bool is_pass;												//�����Ƿ��Ѿ�PASS
	
	bool is_point;											//�Ƿ���ɵ�Ŀ
	int	is_result_point;								//��Ŀ���




	int time_out;												//����ʱ�� 
	int is_undo;												//�Ƿ���Ի��壨��������+2���������ں�ſɻ��壩
	
	int total_undo;											//�ܻ���������������������������

	boost::posix_time::ptime last_msg_;
	boost::weak_ptr<GoGame_logic> the_game_;

	game_player()
		:is_pass(false)
		,offensive_move(0)
		,is_ready(false)
		,time_out(0)
		,is_undo(0)
		,is_point(false)
		,total_undo(0)
		,eat_number(0)
	{
		is_bot_ = false;
		is_connection_lost_ = false;
		sex_ = 0;

		is_result_point = -1;
	}

	int update();

	boost::shared_ptr<GoGame_logic> get_game()
	{
		return the_game_.lock();
	}

	void reset_player();

	void sync_account(__int64 count);
	void on_connection_lost();
	void reset_temp_data();
};
typedef boost::shared_ptr<game_player> player_ptr;