#pragma once
#include "error_define.h"
#include "msg.h"
#include "game_service_base.h"
#include "msg_server_common.h"

enum
{
	GET_CLSID(msg_call_board) = 31997,								//信息通知
	GET_CLSID(msg_player_add_seat) = 31998,						//玩家加入坐位
	GET_CLSID(msg_player_game_data)	= 31999,					//发送玩家游戏数据
	GET_CLSID(msg_make_ready_res) = 31001,						//准备
	GET_CLSID(msg_assign_type),												//分配黑子还是白子
	GET_CLSID(msg_round_turn),												//回合转换
	GET_CLSID(msg_move_chess_res),										//走棋
	GET_CLSID(msg_eat_chess),													//吃子
	GET_CLSID(msg_opposite_undo),											//对方悔棋
	GET_CLSID(msg_undo_res),													//悔棋结果
	GET_CLSID(msg_opposite_point),										//对方点目
	GET_CLSID(msg_point_chess_res),										//点目结果
	GET_CLSID(msg_form_chess_res),										//形势判断
	GET_CLSID(msg_give_up_res),												//认输结果
	GET_CLSID(msg_count_res),													//数子
	GET_CLSID(msg_opposite_summation),								//对方求合
	GET_CLSID(msg_summation_res),											//求和结果
	GET_CLSID(msg_coercive_number),										//强制数子结果
	GET_CLSID(msg_board_data),												//棋盘游戏数据
	GET_CLSID(msg_black_chess_data),									//棋盘黑子数据
	GET_CLSID(msg_white_chess_data),									//棋盘白子数据
	GET_CLSID(msg_result_data),												//结算结果
	GET_CLSID(msg_inform_point_data),									//通知点目
	GET_CLSID(msg_result_point_data),									//点目结果
};

//信息通知
class msg_call_board : public msg_base<none_socket>
{
public:
	int cmd;
	MSG_CONSTRUCT(msg_call_board);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(cmd, data_s);
		return 0;
	}
};


//玩家进入坐位
class msg_player_add_seat : public msg_player_seat
{
public:
	int score;
	int win_num;
	int fail_num;
	int draw_num;
	MSG_CONSTRUCT(msg_player_add_seat);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data<char>(head_pic_, max_guid, data_s);
		write_data<char>(uname_, max_name, data_s);
		write_data(pos_, data_s);
		write_data(score, data_s);
		write_data(win_num, data_s);
		write_data(fail_num, data_s);
		write_data(draw_num, data_s);
		return 0;
	}
};


class msg_player_game_data : public msg_base<none_socket>
{
public:
	char uid_[max_guid];
	int score;
	int win_num;
	int fail_num;
	int draw_num;
	MSG_CONSTRUCT(msg_player_game_data);

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data<char>(uid_, max_guid, data_s);
		write_data(score, data_s);
		write_data(win_num, data_s);
		write_data(fail_num, data_s);
		write_data(draw_num, data_s);
		return 0;
	}
};

class msg_make_ready_res : public msg_base<none_socket>
{
public:
	int pos;
	MSG_CONSTRUCT(msg_make_ready_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos, data_s);
		return 0;
	}
};

class msg_assign_type : public msg_base<none_socket>
{
public:
	int pos;
	int type;				//1黑2白
	MSG_CONSTRUCT(msg_assign_type);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(pos, data_s);
		write_data(type, data_s);
		return 0;
	}
};

class msg_round_turn : public msg_base<none_socket>
{
public:
	int status;				//当前状态:0,未开始;1,下棋中;2,吃子状态;3,结算
	int type;					//当前走棋方
	int timer;				//剩余时长
	int count;				//当前步数
	MSG_CONSTRUCT(msg_round_turn);

	void init()
	{
		status = 0;
		type = 0;
		timer = 0;
		count = 0;
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(status, data_s);
		write_data(type, data_s);
		write_data(timer, data_s);
		write_data(count, data_s);
		return 0;
	}
};

class msg_move_chess_res : public msg_base<none_socket>
{
public:
	int type;
	int posX;
	int posY;
	int step;								//当前步数
	int refuse;							//是否拒绝走这步棋0，不拒绝; 1,拒绝
	int why;								//原因:1,当前位置有子;2,当前位置没有气;3,不属于当前你的回合

	MSG_CONSTRUCT(msg_move_chess_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(type, data_s);
		write_data(posX, data_s);
		write_data(posY, data_s);
		write_data(step, data_s);
		write_data(refuse, data_s);
		write_data(why, data_s);
		return 0;
	}
};

class msg_eat_chess : public msg_base<none_socket>
{
public:
	int type;
	unsigned int count;
	int posX [361];
	int posY [361];

	MSG_CONSTRUCT(msg_eat_chess);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(type, data_s);
		write_data(count, data_s);
		for(int i = 0; i < (int)count; i++)
		{
			write_data(posX[i], data_s);
			write_data(posY[i], data_s);
		}
		return 0;
	}
};

class msg_opposite_undo : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_opposite_undo);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		return 0;
	}
};

class msg_undo_res : public msg_base<none_socket>
{
public:
	int result;				//结果:0,拒绝;1,同意;2,不可使用本操作;3,你上轮已经悔过一次棋了;4,你不能再悔棋了
	int operate;			//操作类型:1,添加棋子; 2,删除棋子
	int type;					//类型
	int oPosX;				//位置
	int oPosY;
	int step;					//步数

	MSG_CONSTRUCT(msg_undo_res);

	void init()
	{
		result = 0;
		operate = 0;
		type = 0;
		oPosX = 0;
		oPosY = 0;
		step = 0;
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(result, data_s);
		write_data(operate, data_s);
		write_data(type, data_s);
		write_data(oPosX, data_s);
		write_data(oPosY, data_s);
		write_data(step, data_s);
		return 0;
	}
};

class msg_opposite_point : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_opposite_point);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		return 0;
	}
};


class msg_point_chess_res : public msg_base<none_socket>
{
public:
	int result;		//结果:0, 拒绝; 1, 同意; 2, 不可使用本操作
	MSG_CONSTRUCT(msg_point_chess_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(result, data_s);
		return 0;
	}
};

class msg_form_chess_res : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_form_chess_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		return 0;
	}
};

class msg_give_up_res : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_give_up_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		return 0;
	}
};

class msg_count_res : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_count_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		return 0;
	}
};

class msg_opposite_summation : public msg_base<none_socket>
{
public:
	MSG_CONSTRUCT(msg_opposite_summation);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		return 0;
	}
};

class msg_summation_res : public msg_base<none_socket>
{
public:
	int result;				//结果:0,拒绝;1,同意;2,不可使用本操作

	MSG_CONSTRUCT(msg_summation_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(result, data_s);
		return 0;
	}
};

//强制数子
class msg_coercive_number : public msg_base<none_socket>
{
public:
	int type;				//谁提出的
	MSG_CONSTRUCT(msg_coercive_number);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(type, data_s);
		return 0;
	}
};


class msg_board_data : public msg_base<none_socket>
{
public:
	int cur_status;			//当前状态：0，未开始; 1,比赛中; 2,吃子阶段;3,已结束
	int cur_turn;				//当前是谁的回合
	int cur_time;				//剩余多长时间
	int cur_count;			//当前步长
	int player_time;		//玩家用时
	int oppo_time;			//对手用时
	int player_eat;			//玩家吃子数
	int oppo_eat;				//对手吃子数

	MSG_CONSTRUCT(msg_board_data);

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(cur_status, data_s);
		write_data(cur_turn, data_s);
		write_data(cur_time, data_s);
		write_data(cur_count, data_s);
		write_data(player_time, data_s);
		write_data(oppo_time, data_s);
		write_data(player_eat, data_s);
		write_data(oppo_eat, data_s);
		return 0;
	}
};

//棋盘黑子数据,这里编号顺序从左到右，从上到下，只有chess_data里为1的才有
class msg_black_chess_data : public msg_base<none_socket>
{
public:
	int chess_data[19];
	int chess_count;						//编号数量
	short chess_num[361];				//编号

	MSG_CONSTRUCT(msg_black_chess_data);

	void init()
	{
		memset(chess_data, 0, sizeof(chess_data));
		chess_count = 0;
		memset(chess_num, 0, sizeof(chess_num));
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);

		for(int i = 0; i < 19; i++)
		{
			write_data(chess_data[i], data_s);
		}

		write_data(chess_count, data_s);

		for(int i = 0; i < chess_count; i++)
		{
			write_data(chess_num[i], data_s);
		}
		return 0;
	}
};

//棋盘白子数据，同黑子数据类型一致
class msg_white_chess_data : public msg_base<none_socket>
{
public:
	int chess_data[19];
	int chess_count;
	short chess_num[361];

	MSG_CONSTRUCT(msg_white_chess_data);

	void init()
	{
		memset(chess_data, 0, sizeof(chess_data));
		chess_count = 0;
		memset(chess_num, 0, sizeof(chess_num));
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);

		for(int i = 0; i < 19; i++)
		{
			write_data(chess_data[i], data_s);
		}

		write_data(chess_count, data_s);

		for(int i = 0; i < chess_count; i++)
		{
			write_data(chess_num[i], data_s);
		}
		return 0;
	}
};

//结算
class msg_result_data : public msg_base<none_socket>
{
public:
	int win_type;				//0,合和;1,黑赢;2,白赢
	int total_turn;			//回合数
	int role_score;			//玩家得分
	int role_totle;			//玩家积分
	int oppo_score;			//对手得分
	int oppo_totle;			//对手积分

	MSG_CONSTRUCT(msg_result_data);

	void init()
	{
		win_type = 0;
		total_turn = 0;
		role_score = 0;
		role_totle = 0;
		oppo_score = 0;
		oppo_totle = 0;
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(win_type, data_s);
		write_data(total_turn, data_s);
		write_data(role_score, data_s);
		write_data(role_totle, data_s);
		write_data(oppo_score, data_s);
		write_data(oppo_totle, data_s);
		return 0;
	}
};

//通知点目
class msg_inform_point_data : public msg_base<none_socket>
{
public:
	int type;		//0,重置点目;1,黑子;2,白子
	int posX;
	int posY;
	int is_set;
	MSG_CONSTRUCT(msg_inform_point_data);

	void init()
	{
		type = 0;
		posX = 0;
		posY = 0;
		is_set = 0;
	}

	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(type, data_s);
		write_data(posX, data_s);
		write_data(posY, data_s);
		write_data(is_set, data_s);
		return 0;
	}
};

//点目结果
class msg_result_point_data : public msg_base<none_socket>
{
public:
	float fBlack;
	float fWhite;

	MSG_CONSTRUCT(msg_result_point_data);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(fBlack, data_s);
		write_data(fWhite, data_s);
		return 0;
	}
};
