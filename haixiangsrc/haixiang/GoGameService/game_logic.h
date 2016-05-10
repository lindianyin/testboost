#pragma once
#include "game_player.h"
#include "Board.h"
#define		MAX_SEATS 2			//在要"logic_base.h"前
#define		COUNT_DOWN 2		//比赛倒计时
#define		TIMEWAIT 3			//吃子时长

#include "logic_base.h"

class GoGame_logic;
typedef boost::shared_ptr<GoGame_logic> logic_ptr;
typedef boost::shared_ptr<Board> board_ptr;

class telfee_match_info;
typedef boost::shared_ptr<telfee_match_info> match_ptr;


struct Boarda
{
	int cur_turn;				//回合（1黑2白）
	int cur_timer;			//当前计时
	int history;				//步数

	Boarda()
	{
		cur_turn = 0;
		cur_timer = 0;
		history = 0;
	}
};


typedef boost::weak_ptr<GoGame_logic> logic_wptr;
class task_clock : public task_info
{
public:
	logic_wptr logic_;
	task_clock(asio::io_service& ios):task_info(ios){}
	virtual int routine();
};




enum Error_cmd
{
	PLAYER_LOST_CONNECTION = 41001,			//对手已掉线
};

class GoGame_logic : public logic_base<GoGame_logic>
{
public:

	enum Game_state
	{
		NOT_START = 0,						//未开始
		IN_GAME	 = 1,							//下棋状态
		REMOVE_DEAD = 2,					//吃子状态
		POINT = 3,								//点目状态
		END = 4,									//结束
	};

	std::vector<player_ptr>	players_;
	match_ptr		the_match_;						//比赛场信息
	board_ptr		the_board_;						//棋盘信息
	int					is_adding_bot_;
	int					count_down;						//比赛计时器
	task_clock* the_clock;						//计时器
	Game_state	the_state;						//状态机
	bool				force_number;					//强制数子		

	explicit GoGame_logic(int is_match);
	void		start_logic();
	int			player_login(player_ptr pp, int pos = -1);
	void		join_player(player_ptr pp);
	void		leave_game( player_ptr pp, int pos, int why = 0);
	bool		is_ingame(player_ptr pp);
	void		stop_logic();
	void		reset_logic();
	int			update(float dt);
	void		broadcast_msg(msg_base<none_socket>& msg, int exclude_pos = -1);
	void		scheduledTask();																	//定时期任务
	void		moveChessState();
	void		eatChessState(int type = 0, int posX = 0, int posY = 0);

	//玩家掉线后逻辑变化
	void		player_lost_connection(player_ptr pp);


	//------------------------------------------------------------------
	// 业务逻辑
	//------------------------------------------------------------------
	void		inNewRound();																			//进入新一的局
	void		startNewRound();																	//回合开始
	void		turn_convert();																		//回合转换
	int			move_chess(player_ptr pp, int posX, int posY);		//走棋
	int			ask_undo(player_ptr pp);													//悔棋询问
	int			reply_undo(player_ptr pp, int result);						//悔棋回复
	int			pass_chess(player_ptr pp);												//pass
	int			point_chess(player_ptr pp);												//点目
	int			reply_point_chess(player_ptr pp, int result);			//点目回复
	int			give_up(player_ptr pp);														//认输
	int			compel_chess(player_ptr pp);											//强制数子
	int			ask_summation(player_ptr pp);											//求和
	int			reply_summation(player_ptr pp, int result);				//求和结果
	int			set_point(player_ptr pp, int posX, int posY, int isSet);//设置点目
	int			submit_point(player_ptr pp);											//完成点目
	int			result_point(player_ptr pp, int result);					//提交点目结果

	//进入结算
	//is_lost判定谁输了:0,不判定;1,黑输;2,白输;3,和棋
	//is_flee是否为逃跑:0,正常;1,逃跑
	int			game_result(int is_lost = 0, int is_flee = 0);											
private:
	boost::posix_time::ptime last_time;

	player_ptr getopponent(player_ptr pp);										//获得对手
	player_ptr getPlayerByType(int type);											//根据类型获得玩家
};