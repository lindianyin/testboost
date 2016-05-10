#include "poker_card.h"
bool				is_equal_match(zero_match_result& r1, zero_match_result& r2)
{
	vector<niuniu_card> v1;		v1.push_back(r1.c1);	v1.push_back(r1.c2);	v1.push_back(r1.c3);
	vector<niuniu_card> v2;		v2.push_back(r2.c1);	v2.push_back(r2.c2);	v2.push_back(r2.c3);
	for (unsigned int i = 0; i < v1.size(); i++)
	{
		bool finded = false;
		for (unsigned int j = 0; j < v2.size(); j++)
		{
			if(v1[i].card_weight_ == v2[j].card_weight_){
				v2.erase(v2.begin() + j);
				finded = true;
				break;
			}
		}
		if (!finded){
			return false;
		}
	}
	return true;
}

bool			is_greater(zero_match_result& r1, zero_match_result& r2)
{
	if (r1.calc_point_ == -1){
		return false;
	}

	if (r2.calc_point_ == -1){
		return true;
	}

	if (r1.calc_point_ == r2.calc_point_){
		if (r1.calc_point_ >= 10){
			if (r1.niuniu_level_ == r2.niuniu_level_){
				return r1.is_greater(r2);
			}
			else
				return r1.niuniu_level_ > r2.niuniu_level_;
		}
		else{
			return r1.is_greater(r2);
		}
	}
	else
		return r1.calc_point_ > r2.calc_point_;
}
vector<niuniu_card>			seek_zero(niuniu_card& c1, niuniu_card& c2, niuniu_card& c3)
{
	vector<niuniu_card> ret;
	vector<niuniu_card> tmp_v;
	tmp_v.push_back(c1); tmp_v.push_back(c2); tmp_v.push_back(c3);

	niuniu_card cqbig, cqsmall;
	auto it = tmp_v.begin();
	while (it != tmp_v.end())
	{
		if ((*it).card_id_ == 52){
			cqsmall = *it;
			it = tmp_v.erase(it);
		}
		else if((*it).card_id_ == 53){
			cqbig = *it;
			it = tmp_v.erase(it);
		}
		else{
			it++;
		}
	}
	//大小王同时在
	if (cqsmall.card_point_ > -1 && cqbig.card_point_ > -1 && !tmp_v.empty()){
		if (tmp_v[0].card_point_ < 10){
			cqbig.replaced_to_id_ = niuniu_card::calc_black_N(10 - tmp_v[0].card_point_);
		}
		ret.push_back(cqsmall);
		ret.push_back(tmp_v[0]);
		ret.push_back(cqbig);
	}
	else if(cqsmall.card_point_ > -1 && tmp_v.size() > 1 && !tmp_v.empty()){
		int n = (tmp_v[0].card_point_ + tmp_v[1].card_point_) % 10;
		if (n == 4){
			cqsmall.replaced_to_id_ = niuniu_card::calc_black_6();
			ret.push_back(cqsmall);
			ret.push_back(tmp_v[0]);
			ret.push_back(tmp_v[1]);
		}
		else if(n == 0){
			ret.push_back(cqsmall);
			ret.push_back(tmp_v[0]);
			ret.push_back(tmp_v[1]);
		}
	}
	else if (cqbig.card_point_ > -1 && tmp_v.size() > 1 && !tmp_v.empty()){
		int n = (tmp_v[0].card_point_ + tmp_v[1].card_point_) % 10;
		if(n == 0){
			ret.push_back(cqbig);
			ret.push_back(tmp_v[0]);
			ret.push_back(tmp_v[1]);
		}
		else{
			cqbig.replaced_to_id_ = niuniu_card::calc_black_N(10 - n);
			ret.push_back(cqbig);
			ret.push_back(tmp_v[0]);
			ret.push_back(tmp_v[1]);
		}

	}
	else{
		if (tmp_v.size() == 3 && 
			((tmp_v[0].card_point_ + tmp_v[1].card_point_ + tmp_v[2].card_point_) % 10 == 0)){
			ret = tmp_v;
		}
	}
	return ret;
}

//配0，用牌的副本来算，以免影响原来的牌
vector<zero_match_result> seek_zero(vector<niuniu_card> vcards)
{
	vector<zero_match_result> ret;
	vector<niuniu_card> v1 = vcards;
	vector<niuniu_card> v2 = vcards;
	vector<niuniu_card> v3 = vcards;
	for(unsigned int i = 0; i < v1.size(); i++)
	{
		for (unsigned int j = 0; j < v2.size(); j++)
		{
			if (j == i) continue;
			for (unsigned int k = 0; k < v3.size(); k++)
			{
				if (k == i) continue;
				if (k == j) continue;
				vector<niuniu_card> vr = seek_zero(v1[i], v2[j], v3[k]);
				if (vr.size() == 3){
					zero_match_result zm;
					zm.c1 = vr[0];
					zm.c2 = vr[1];
					zm.c3 = vr[2];
					bool has_equal = false;
					for (unsigned int ii = 0; ii < ret.size(); ii++)
					{
						if (is_equal_match(zm, ret[ii])){
							has_equal = true;
						}
					}
					if (!has_equal){
						ret.push_back(zm);
					}
				}
			}
		}
	}
	return ret;
}

int				big_small_point()
{
	return 10;
}

int				big_n_point(niuniu_card& big, niuniu_card& c2)
{
	if(c2.card_point_ < 10)
		big.replaced_to_id_ = niuniu_card::calc_black_N(10 - c2.card_point_);
	return 10;
}

int				small_n_point(niuniu_card& small, niuniu_card& c2)
{
	int w1 = 6 + c2.card_point_;
	int w2 = 10 + c2.card_point_;
	if ((w1 % 10) == 0){
		small.replaced_to_id_ = niuniu_card::calc_black_6();
		return 10;
	}
	else if (w2 % 10 == 0){
		return 10;
	}
	//小王当黑桃6
	else if ((w1 % 10) > (w2 % 10)){
		small.replaced_to_id_ = niuniu_card::calc_black_6();
		return (w1 % 10);
	}
	//小王当黑桃10
	else{
		return (w2 % 10);
	}
}


int					seek_point(niuniu_card& c1, niuniu_card& c2)
{
	//如果第一张牌是小王
	if (c1.card_id_ == 52){
		//如果第二张牌是大王
		if (c2.card_id_ == 53){
			return big_small_point();
		}
		else{
			return small_n_point(c1, c2);
		}
	}
	//如果第一张牌是大王
	else if (c1.card_id_ == 53){
		//如果第二张牌是小王
		if (c2.card_id_ == 52){
			return big_small_point();
		}
		else {
			return big_n_point(c1, c2);
		}
	}
	else{
		//如果是大王
		if (c2.card_id_ == 53){
			return big_n_point(c2, c1);
		}
		//如果是小王
		else if (c2.card_id_ == 52){
			return small_n_point(c2, c1);
		}
		//如果2张都是普通牌
		else{
			int r = (c1.card_point_ + c2.card_point_) % 10;
			if (r == 0) r = 10;
			return r;
		}
	}
}

int						seek_point(vector<niuniu_card>& vcards, zero_match_result& zm)
{
	if(vcards.size() != 5) return -1;

	vector<niuniu_card*> v;

	for (unsigned int i = 0; i < vcards.size(); i++)
	{
		if (vcards[i].card_id_ == zm.c1.card_id_ ||
			vcards[i].card_id_ == zm.c2.card_id_ ||
			vcards[i].card_id_ == zm.c3.card_id_)	{
				continue;
		}
		else{
			v.push_back(&vcards[i]);
		}
	}

	if (v.size() != 2){
		return -1;
	}
	zm.c4 = *v[0];
	zm.c5 = *v[1];
	return  seek_point(*v[0], *v[1]);
}
//计算牛牛点数,并设置计算结果
zero_match_result		calc_niuniu_point(vector<niuniu_card>& vcards)
{
	if (vcards.size() != 5)	return zero_match_result();

	vector<zero_match_result> vz = seek_zero(vcards);
	zero_match_result* best_match = NULL; 
	for (unsigned int i = 0; i < vz.size(); i++)
	{
		int p = seek_point(vcards, vz[i]);
		if (p == 10){
			vz[i].calc_point_ = 10;
		}
		else{
			vz[i].calc_point_ = p;
		}
		vz[i].calc_niuniu_level();

		if (!best_match){
			best_match = &vz[i];
		}
		else if (best_match->calc_point_ < p){
			best_match = &vz[i];
		}
		else if (best_match->calc_point_ == p){
			if(best_match->niuniu_level_ < vz[i].niuniu_level_){
				best_match = &vz[i];
			}
		}
	}
	//如果有最好的点
	if (best_match){
		return *best_match;
	}
	else{
		zero_match_result zm;
		zm.calc_point_ = 0;
		zm.calc_niuniu_level();
		zm.c1 = vcards[0];
		zm.c2 = vcards[1];
		zm.c3 = vcards[2];
		zm.c4 = vcards[3];
		zm.c5 = vcards[4];
		return zm;
	}
}