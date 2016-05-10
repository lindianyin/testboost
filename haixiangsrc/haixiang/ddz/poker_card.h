#pragma once
#include <algorithm>
#include <vector>

using namespace std;

enum 
{
	pattern_is_accepted,
	pattern_is_not_allowed,
};

class poker_card
{
public:

	poker_card()
	{
		card_id_ = -1;
		card_weight_ = 0;
		memset(face_, 0 ,sizeof(face_));
	}
	char	face_[4];
	int		card_id_;				//牌面大小, 0-51(), 52小王，53，大王
	int		card_weight_;		//牌面大小, 3-K(1-11), A(12), 2(13) 小王(14)，大王(15)
};


//牛牛牌
class		ddz_card : public poker_card
{
public:
	//产生一副随机牌
	static vector<ddz_card>		generate();
	static void					reset_and_shuffle(vector<ddz_card>& vcards);
	static ddz_card				generate(int card_id);
	static ddz_card				generate(std::string c);
	bool	operator == (const ddz_card& other);
};

//牌型从小到大排序,简化后面的逻辑
enum card_stand_for
{
	csf_not_allowed,			//不允许的牌型

	csf_single,						//单牌
	csf_multi_single,			//单连(顺子)

	csf_dual,							//单对
	csf_multi_dual,				//连对

	csf_triple,						//三张牌(可能带单,可能连三张)
	csf_triple_dual,			//三带对(可能连三张)

	csf_quadruple_single,	//四张带单(最多2张),
	csf_quadruple_dual,		//四张带对(最多2对)

	csf_quadruple,				//四张(炸弹)
	csf_jet,							//大小王炸。
};

struct csf_result
{
	int			csf_val_;						//牌型值
	int			main_count_;				//主牌型数
	int			attachment_count_;	//附加数
	int			weight_;						//牌型权重值,是一手牌中的最小牌面代表这手牌的权值
	std::vector<ddz_card> main_cards_;
	std::vector<ddz_card> attachments_;
	csf_result()
	{
		csf_val_ = csf_not_allowed;
		main_count_ = 0;
		attachment_count_ = 0;
		weight_ = 0;
	}

	int			min_weight();

};
int		remap_pos(int ori_pos,
								std::vector<ddz_card>& lead_cards,
								std::vector<ddz_card>& next_cards,
								std::vector<ddz_card>& next_next_cards,
								std::vector<ddz_card>* v1,
								std::vector<ddz_card>* v2,
								std::vector<ddz_card>* v3);

void	follow_card(int		banker_pos,
									int		leadpos,
									csf_result& lead, 
									csf_result& ret,
									std::vector<ddz_card>& my_cards, 
									std::vector<ddz_card>& next_cards = std::vector<ddz_card>(),	//对手1牌
									std::vector<ddz_card>& next_next_cards = std::vector<ddz_card>(),//对手2的牌
									bool deadly_follow = 0,  //死亡跟牌(不计代价跟牌)
									bool is_test = false);

int		lead_card(int banker_pos,
								csf_result& to_play,											//我要出的牌
								std::vector<ddz_card>& my_cards,					//我手上的牌
								std::vector<ddz_card>& next_cards,				//对手1手上的牌
								std::vector<ddz_card>& next_next_cards,		//对手2手上的牌
								bool is_test = false);

bool	will_lose(int		banker_pos,
								int	leadpos,
								bool	isdeadly,
								csf_result& toplay, 
								std::vector<ddz_card>& my_cards,			//我手上的牌
								std::vector<ddz_card>& next_cards,		//对手1牌
								std::vector<ddz_card>& next_next_cards//对手2的牌
							 );

csf_result	analyze_csf(std::vector<ddz_card>& plan);
int is_greater_pattern(csf_result& plan, csf_result& refer);
int		next_same(int pos, std::vector<ddz_card>& thisp, int weight);
bool	should_pass_for_alliance(csf_result& current_csf_);
//评估是否要做庄
int		evalue_score_to_be_banker(std::vector<ddz_card>& mycards);
std::vector<ddz_card>	remove_cards(std::vector<ddz_card>& my_cards, csf_result& to_remove);
