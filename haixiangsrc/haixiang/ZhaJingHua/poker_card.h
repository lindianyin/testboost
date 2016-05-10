#pragma once
#include <algorithm>
#include <vector>
#include <map>
using namespace std;

class poker_card
{
public:

	poker_card()
	{
		card_id_ = -1;
		card_weight_ = 0;
	}
	int card_id_;				//牌面, 0-51(), 52小王，53，大王
	int	card_weight_;		//牌面大小, 1-10， J-K(11-13), 小王(14)，大王(15)
};

enum match_result:int{
	match_result_fail = -1,
	match_result_none = 1,			//散牌
	match_result_duizhi = 2,		//对子
	match_result_tuolaji = 3,		//连牌
	match_result_qingyishe = 4,	//清一色
	match_result_jingtiao = 5,	//清一色+ 连牌
	match_result_baozhi = 6,		//豹子
};

//牛牛牌
class		jinghua_card : public poker_card
{
public:
	//产生一副随机牌
	static vector<jinghua_card>		generate()
	{
		vector<jinghua_card> ret;
		//没有大小王
		for (int i = 0; i < 52; i++)
		{
			jinghua_card c;
			c.card_id_ = i;
			if (c.card_id_ % 13 == 0){
				c.card_weight_ = 14;
			}
			else{
				c.card_weight_ = (i % 13) + 1;
			}
			ret.push_back(c);
		}
		return ret;
	}
	static	void					reset_and_shuffle(vector<jinghua_card>& vcards)
	{
		random_shuffle(vcards.begin(), vcards.end());
	}

	bool			operator < (const jinghua_card& rhs)
	{
		return card_weight_ < rhs.card_weight_;
	}
};

class		jinhua_match_result
{
public:
	vector<jinghua_card> cards_;
	match_result	card_match_;
	jinhua_match_result()
	{
		card_match_ = match_result_fail;
	}
	jinghua_card get_max_card(){
		if (card_match_ == match_result_duizhi){
			map<int, vector<jinghua_card>> w_group;
			for (int i = 0; i < cards_.size(); i++)
			{
				vector<jinghua_card>& v = w_group[cards_[i].card_weight_];
				v.push_back(cards_[i]);
			}
			auto it = w_group.begin();
			while (it != w_group.end())
			{
				vector<jinghua_card>& v = it->second; it++;
				if (v.size() > 1){
					return get_max_card(v);
				}
			}
			return jinghua_card();
		}
		else
			return get_max_card(cards_);
	}
protected:
	jinghua_card get_max_card(vector<jinghua_card> v)
	{
		jinghua_card c;
		c.card_weight_ = -1;
		for (int i = 0; i < v.size(); i++)
		{
			if (c.card_weight_ == v[i].card_weight_){
				if (c.card_id_ < v[i].card_id_)	{
					c = v[i];
				}
			}
			else if (c.card_weight_ < v[i].card_weight_){
				c = v[i];
			}
		}
		return c;
	}
};

//是否同花
bool							is_same_color(vector<jinghua_card>& vcards);
//是相同牌面
bool							is_same_weight(vector<jinghua_card>& vcards);
//是否连续牌,注意拖拉机牌:A23和QKA中，A大小的区别
bool							is_continous(vector<jinghua_card>& vcards);

//是否有对子
bool							has_pair(vector<jinghua_card>& vcards);


bool							is_greater(jinhua_match_result& r1, jinhua_match_result& r2);

match_result			calc_match_result(vector<jinghua_card>& vcards);


jinhua_match_result		analyse_cards(vector<jinghua_card>& vcards);
