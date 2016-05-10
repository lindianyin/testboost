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
	int score;							//玩家积分
	int win_total;					//胜利次数
	int fail_total;					//失败次数
	int draw_total;					//和棋次数
	int eat_number;					//提子数

	bool is_ready;											//是否已经准备
	int offensive_move;									//是否先手 1为先手
	bool is_pass;												//本轮是否已经PASS
	
	bool is_point;											//是否完成点目
	int	is_result_point;								//点目结果




	int time_out;												//下棋时长 
	int is_undo;												//是否可以悔棋（点击悔棋后+2，走两步悔后才可悔棋）
	
	int total_undo;											//总悔棋次数，超过三级，不允许悔棋

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