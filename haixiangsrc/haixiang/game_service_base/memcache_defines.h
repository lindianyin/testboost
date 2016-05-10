#pragma once

enum
{
	CACHE_RET_INPROCESS = -1,
	CACHE_RET_SUCCESS = 0,
	CACHE_RET_UNKNOWN_CMD,			//δ֪����
	CACHE_RET_UNIQUE_CHECK_FAIL,//Ωһ�Լ��δͨ��
	CACHE_RET_WRONG_DAT_TYPE,		//�������Ͳ���
	CACHE_RET_NO_REQ_DATA,			//û�����������
	CACHE_RET_LESS_THAN_COST,		//��ǰֵС�ڿ۷�����
	CACHE_RET_DATA_EXISTS,			//�����Ѵ���
	CACHE_NEED_PLATFORM_DATA,   //��Ҫ��ѯƽ̨����   
	CACHE_RET_NOT_MATCH,
	CACHE_RET_SEQUENCE_LESS,		//���к�̫С
};

#define CMD_ADD							"CMD_ADD"
#define CMD_COST						"CMD_COST"
#define CMD_SET							"CMD_SET"
#define CMD_COST_ALL				"CMD_COST_ALL"
#define CMD_BIND						"CMD_BIND"
#define CMD_GET							"CMD_GET"		//����key�õ�value
#define ACC_UID							"ACCUID"
#define IID_UID							"IDDUID"
#define GOLD_RANK						"GOLD_RANK"


#define KEY_GENUID					"9FC54CFF-1860-4D97-8A31-C073187693FD"
#define KEY_GETRANK					"9FC54CFF-1860-4D97-8A31-C073187693FE"

//KEY���»��߿�ͷ������λ�ò����Գ����»���
#define KEY_ONLINE_TIME					"_ONLINETIME"
#define KEY_ONLINE_GIFT					"_ONLINEGIFT"
#define KEY_CUR_FREE						"_$FREE$"
#define KEY_CUR_GOLD						"_$GOLD$"
#define KEY_CUR_TRADE_CAP				"_$TRADECAP$"
#define KEY_EVERYDAY_LOGIN			"_EVERYDAYLOGIN"
#define KEY_EVERYDAY_LOGIN_TICK "_EVDLGINTICK"
#define KEY_EVERYDAY_LOGIN_GIFT "_EVDLGINGIFT"
#define KEY_LEVEL_GIFT					"_LEVELGIFT"	
#define KEY_USER_IID						"_IID"
#define KEY_USER_UNAME					"_UNAME"
#define KEY_USER_ACCNAME				"_ACCNAME"
#define KEY_USER_ACCPWD					"_ACCPWD"
#define KEY_ACCNAME_REFLECT			"_ACCREFLECT"
#define KEY_USER_HEADPIC				"_UHEADPIC"
#define KEY_RECOMMEND_REWARD		"_RECOMM_REW"
#define KEY_EXP									"_EXP"
#define KEY_LEVEL_GIFT					"_LEVELGIFT"
#define KEY_LUCKY								"_LUCKY"
#define KEY_GOLD_LOCK						"_LOCKGOLD"
#define KEY_ID_CARD							"_IDCARD"
#define KEY_7158_ORDERNUM				"_7158odn"
#define KEY_RECOMMEND_BY				"_RCMDBY"
#define KEY_USERISONLINE				"_UISONLINE"
#define KEY_FISHCOINBK					"_FCBK"
#define KEY_ONLINE_LAST					"_ONLINELAST"
#define KEY_CUR_SHELL                   "_SHELL"
#define KEY_CUR_GUN_ID                  "_GUN_ID"
