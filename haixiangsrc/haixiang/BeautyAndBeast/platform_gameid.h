#pragma once
#if defined(_FRUIT_GAME) //水果
#define  PRESETS_COUNT 8
#define	 GAME_ID_7158	GAME_ID_7158_FRUIT
#define  GAME_ID_173	GAME_ID_FRUIT
#define	 GAME_ID		FRUIT

#elif defined(_BEAUTY) //四大美女
#define  PRESETS_COUNT 8
#define	 GAME_ID_7158	GAME_ID_7158_BEAUTY
#define  GAME_ID_173	GAME_ID_BGIRL
#define	 GAME_ID		BEAUTY_BEAST

#elif defined(_FOUR_BOOK) //四大名著
#define  PRESETS_COUNT 28
#define	 GAME_ID_7158	GAME_ID_7158_FOURBOOK
#define  GAME_ID_173	GAME_ID_FBOOK
#define	 GAME_ID		FOURBOOK

#elif defined(_GAME_)		//运动会
#define  PRESETS_COUNT 8
#define	 GAME_ID_7158	GAME_ID_7158_GAME
#define  GAME_ID_173	GAME_ID_GAME
#define	 GAME_ID		GAME

#elif defined(_MOTORS_)	//车行
#define  PRESETS_COUNT 8
#define	 GAME_ID_7158	GAME_ID_7158_MOTORS
#define  GAME_ID_173	GAME_ID_MOTORS
#define	 GAME_ID		MOTORS

#elif defined(_DRAGON_)	//九转神龙
#define PRESETS_COUNT 8
#define	 GAME_ID_7158	GAME_ID_7158_DRAGON
#define  GAME_ID_173	GAME_ID_DRAGON
#define	 GAME_ID			DRAGON

#elif defined(_WHEEL) //大转盘
#define  PRESETS_COUNT 54
#define	 GAME_ID_7158	GAME_ID_7158_WHEEL
#define  GAME_ID_173	GAME_ID_WHEEL
#define	 GAME_ID		WHEEL

#endif // _FRUIT_GAME

class bet_info
{
public:
	std::string		uid_;
	int						present_id_;//美女id
	int						chip_id_;		//筹码id
	unsigned int	bet_count_;
};

class		game_report_data
{
public:
	std::string				uid_;
	__int64			pay_;
	__int64			actual_win_;		//实际
	__int64			should_win_;		//应该赢多少
	float               factor_;     //赢的倍数
	game_report_data()
	{
		pay_ = actual_win_ = should_win_ = factor_= 0;
	}
};

class group_set
{
public:
	int			pid_;			//奖项id
	float		factor_;	//赔率
	std::vector<int>	group_by_; //由哪几个id组成
	group_set()
	{
		pid_ = factor_ = 0;
	}
	bool	operator == (group_set& pr)
	{
		bool eq = (pr.pid_ == pid_ && pr.factor_ == factor_ && group_by_.size() == pr.group_by_.size());
		if (!eq) return false;

		for (int i = 0 ; i < group_by_.size(); i++)
		{
			if (group_by_[i] != pr.group_by_[i]){
				return false;
			}
		}
		return true;
	}
};

class preset_present
{
public:
	int			pid_;				//奖项id
	int			rate_;			//概率
	float		factor_;		//赔率
	int			prior_;			//赔付优先级 (押注项)
	preset_present()
	{
		pid_ = -1;
		rate_ = 0;
		factor_ = 0;
		prior_ = 0;
	}
	bool	operator == (preset_present& pr);
};

//富豪榜
struct riche_info
{
	std::string uid_;							//玩家UID
	std::string uname_;						//玩家昵称
	__int64		val_;								//押注流水

	bool operator < (const riche_info& rhs) const//升序排序时必须写的函数
	{   
		return val_ < rhs.val_; 
	}
	bool operator > (const riche_info& rhs)  const   //降序排序时必须写的函数
	{   
		return val_ > rhs.val_; 
	}
};

class preset_riches
{
public:
	std::string getTime;	//获取时间
	std::map<std::string, riche_info> list;

	preset_riches()
	{
		getTime = "";
		list.clear();
	}
};


#define		MAX_SEATS  200


