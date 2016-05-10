#pragma once
#include "error_define.h"
#include "msg.h"
#include "game_service_base.h"
#include "msg_server_common.h"

enum
{
	GET_CLSID(msg_call_board) = 31997,								//��Ϣ֪ͨ
	GET_CLSID(msg_player_add_seat) = 31998,						//��Ҽ�����λ
	GET_CLSID(msg_player_game_data)	= 31999,					//���������Ϸ����
	GET_CLSID(msg_make_ready_res) = 31001,						//׼��
	GET_CLSID(msg_assign_type),												//������ӻ��ǰ���
	GET_CLSID(msg_round_turn),												//�غ�ת��
	GET_CLSID(msg_move_chess_res),										//����
	GET_CLSID(msg_eat_chess),													//����
	GET_CLSID(msg_opposite_undo),											//�Է�����
	GET_CLSID(msg_undo_res),													//������
	GET_CLSID(msg_opposite_point),										//�Է���Ŀ
	GET_CLSID(msg_point_chess_res),										//��Ŀ���
	GET_CLSID(msg_form_chess_res),										//�����ж�
	GET_CLSID(msg_give_up_res),												//������
	GET_CLSID(msg_count_res),													//����
	GET_CLSID(msg_opposite_summation),								//�Է����
	GET_CLSID(msg_summation_res),											//��ͽ��
	GET_CLSID(msg_coercive_number),										//ǿ�����ӽ��
	GET_CLSID(msg_board_data),												//������Ϸ����
	GET_CLSID(msg_black_chess_data),									//���̺�������
	GET_CLSID(msg_white_chess_data),									//���̰�������
	GET_CLSID(msg_result_data),												//������
	GET_CLSID(msg_inform_point_data),									//֪ͨ��Ŀ
	GET_CLSID(msg_result_point_data),									//��Ŀ���
};

//��Ϣ֪ͨ
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


//��ҽ�����λ
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
	int type;				//1��2��
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
	int status;				//��ǰ״̬:0,δ��ʼ;1,������;2,����״̬;3,����
	int type;					//��ǰ���巽
	int timer;				//ʣ��ʱ��
	int count;				//��ǰ����
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
	int step;								//��ǰ����
	int refuse;							//�Ƿ�ܾ����ⲽ��0�����ܾ�; 1,�ܾ�
	int why;								//ԭ��:1,��ǰλ������;2,��ǰλ��û����;3,�����ڵ�ǰ��Ļغ�

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
	int result;				//���:0,�ܾ�;1,ͬ��;2,����ʹ�ñ�����;3,�������Ѿ��ڹ�һ������;4,�㲻���ٻ�����
	int operate;			//��������:1,�������; 2,ɾ������
	int type;					//����
	int oPosX;				//λ��
	int oPosY;
	int step;					//����

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
	int result;		//���:0, �ܾ�; 1, ͬ��; 2, ����ʹ�ñ�����
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
	int result;				//���:0,�ܾ�;1,ͬ��;2,����ʹ�ñ�����

	MSG_CONSTRUCT(msg_summation_res);
	int write(char*& data_s, unsigned int l)
	{
		msg_base::write(data_s, l);
		write_data(result, data_s);
		return 0;
	}
};

//ǿ������
class msg_coercive_number : public msg_base<none_socket>
{
public:
	int type;				//˭�����
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
	int cur_status;			//��ǰ״̬��0��δ��ʼ; 1,������; 2,���ӽ׶�;3,�ѽ���
	int cur_turn;				//��ǰ��˭�Ļغ�
	int cur_time;				//ʣ��೤ʱ��
	int cur_count;			//��ǰ����
	int player_time;		//�����ʱ
	int oppo_time;			//������ʱ
	int player_eat;			//��ҳ�����
	int oppo_eat;				//���ֳ�����

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

//���̺�������,������˳������ң����ϵ��£�ֻ��chess_data��Ϊ1�Ĳ���
class msg_black_chess_data : public msg_base<none_socket>
{
public:
	int chess_data[19];
	int chess_count;						//�������
	short chess_num[361];				//���

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

//���̰������ݣ�ͬ������������һ��
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

//����
class msg_result_data : public msg_base<none_socket>
{
public:
	int win_type;				//0,�Ϻ�;1,��Ӯ;2,��Ӯ
	int total_turn;			//�غ���
	int role_score;			//��ҵ÷�
	int role_totle;			//��һ���
	int oppo_score;			//���ֵ÷�
	int oppo_totle;			//���ֻ���

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

//֪ͨ��Ŀ
class msg_inform_point_data : public msg_base<none_socket>
{
public:
	int type;		//0,���õ�Ŀ;1,����;2,����
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

//��Ŀ���
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
