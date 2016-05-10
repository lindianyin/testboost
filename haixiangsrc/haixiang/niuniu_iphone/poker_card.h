#pragma once
#include <algorithm>
#include <vector>
using namespace std;

class poker_card
{
public:

	poker_card()
	{
		card_id_ = -1;
		card_weight_ = 0;
	}
	int card_id_;				//�����С, 0-51(), 52С����53������
	int	card_weight_;		//�����С, 1-10�� J-K(11-13), С��(14)������(15)
};


//ţţ��
class		niuniu_card : public poker_card
{
public:
	int			replaced_to_id_;		//��С���ٴ�ɵ�id
	int			card_point_;				//�����ۺ�ţţ����
	//����һ�������
	static vector<niuniu_card>		generate()
	{
		vector<niuniu_card> ret;
		for (int i = 0; i < 52; i++)
		{
			niuniu_card c = generate(i);
			ret.push_back(c);
		}
		return ret;
	}
	static	void					reset_and_shuffle(vector<niuniu_card>& vcards)
	{
		random_shuffle(vcards.begin(), vcards.end());
		for (unsigned int i = 0; i < vcards.size(); i++)
		{
			vcards[i].reset();
		}
	}

	static int						calc_black_6()
	{
		return calc_black_N(6);
	}

	static int						calc_black_N(int N)
	{
		return 3 * 13 + N - 1;
	}

	static niuniu_card		generate(int card_id)
	{
		niuniu_card c;
		c.card_id_ = card_id;
		//С��
		if(card_id == 52){
			c.card_weight_ = 14;
			c.card_point_ = 10;
		}
		//����
		else if (card_id == 53){
			c.card_weight_ = 15;
			c.card_point_ = 10;
		}
		else{
			c.card_weight_ = (card_id % 13) + 1;
			if (c.card_weight_ >= 10)
				c.card_point_ = 10;
			else
				c.card_point_ = c.card_weight_;
		}
		return c;
	}
	niuniu_card()
	{
		replaced_to_id_ = -1;
		card_point_ = -1;
	}
	void		reset()
	{
		replaced_to_id_ = -1;
	}
};

class		zero_match_result
{
public:
	niuniu_card c1, c2, c3;
	niuniu_card c4, c5;
	int					calc_point_;
	int					niuniu_level_;		//0��ͨţţ,1��ţ,2��ţ
	zero_match_result()
	{
		calc_point_ = -1;
		niuniu_level_ = -1;
	}

	void			calc_niuniu_level()
	{
		if (calc_point_ < 10) return;
		niuniu_level_ = 0;
		if ((c1.card_weight_ < 10) || (c2.card_weight_ < 10) ||(c3.card_weight_ < 10) ||(c4.card_weight_ < 10) || (c5.card_weight_ < 10)) return;
		int cnt = 0;
		if (c1.card_weight_ > 10) cnt ++;
		if (c2.card_weight_ > 10) cnt ++;
		if (c3.card_weight_ > 10) cnt ++;
		if (c4.card_weight_ > 10) cnt ++;
		if (c5.card_weight_ > 10) cnt ++;
		if (cnt == 5){
			niuniu_level_ = 2;
		}
		else if (cnt == 4){
			niuniu_level_ = 1;
		}
	}

	bool			is_greater(zero_match_result& rhs)
	{
		int cid1 = -1, cid2 = -1;
		int w1 = get_max_weight(cid1), w2 = rhs.get_max_weight(cid2);
		if (w1 != w2 ){
			return w1 > w2;
		}
		else{
			return cid1 > cid2;
		}
	}
private:
	int				get_max_weight(int& cid)
	{
		vector<niuniu_card> cards;
		cards.push_back(c1);
		cards.push_back(c2);
		cards.push_back(c3);
		cards.push_back(c4);
		cards.push_back(c5);

		int max_w = -1;
		for (unsigned int i = 0; i < cards.size(); i++)
		{
			if (max_w > cards[i].card_weight_) continue;
			//��û�е�����K������С
			if ((cards[i].replaced_to_id_ >= 0))
				continue;

			max_w  = cards[i].card_weight_;
			cid = cards[i].card_id_;
		}
		return max_w;
	}
};

//������е�������
class		cards_dispatched
{
public:
	vector<niuniu_card>			cards_;
	cards_dispatched()
	{
	}
};


vector<niuniu_card>				seek_zero(niuniu_card& c1, niuniu_card& c2, niuniu_card& c3);
//��0�����Ƶĸ������㣬����Ӱ��ԭ������
vector<zero_match_result> seek_zero(vector<niuniu_card> vcards);
int					seek_point(niuniu_card& c1, niuniu_card& c2);
int					seek_point(vector<niuniu_card>& vcards, zero_match_result& zm);
//����ţţ����,�����ü�����
zero_match_result		calc_niuniu_point(vector<niuniu_card>& vcards);
bool				is_greater(zero_match_result& r1, zero_match_result& r2);

bool				is_equal_match(zero_match_result& r1, zero_match_result& r2);