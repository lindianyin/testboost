#pragma once
#include "error_define.h"
enum
{
	GAME_ERR_BANKER_CANT_BET = ERROR_USER_DEF_BEGIN,//ׯ�Ҳ���ѹע 100000
	GAME_ERR_CANT_BET_NOW = 100001,				//���ڲ���ѹע
	GAME_ERR_RESTORE_FAILED = 100002,			//�ָ���¼ʧ��
	GAME_ERR_DELETE_LOCAL_FAILED = 100003,	//ɾ�����ؼ�¼ʧ��
	GAME_ERR_NOT_BANKER = 100004,					//�Ҳ���ׯ��
	GAME_ERR_CANT_FIND_GAME = 100005,			//�Ҳ�����Ϸ
	GAME_ERR_CANT_FIND_CHIP_SET = 100006,	//�Ҳ�����������
	GAME_ERR_WAITING_UPDATE = 100007,			//���ڵȴ����ø���
	GAME_ERR_CONFIG_WRONG = 100008,				//���ô���
	GAME_ERR_BANKER_TURN_LESS = 100009,		//��ׯ��������
	GAME_ERR_LOSE_CAP_EXCEED = 100010,		//��������������
	GAME_ERR_WIN_CAP_EXCEED = 100011,			//��������Ӯ����
	GAME_ERR_BET_CAP_EXCEED = 100012,			//ׯע����
	GAME_ERR_CANT_BET_MORE = 100013,			//������Ѻ��
};