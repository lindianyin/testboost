#include "poker_card.h"

//�Ƿ�ͬ��
bool							is_same_color(vector<jinghua_card>& vcards)
{
	return (vcards[0].card_id_ / 13 == vcards[1].card_id_ / 13) && 
		(vcards[0].card_id_ / 13 == vcards[2].card_id_/ 13);
}
//����ͬ����
bool							is_same_weight(vector<jinghua_card>& vcards)
{
	if (vcards.size() != 3) return false;
	return (vcards[0].card_weight_ == vcards[1].card_weight_ ) && 
		(vcards[0].card_weight_ == vcards[2].card_weight_);
}
//�Ƿ�������,ע����������:A23��QKA�У�A��С������
bool							is_continous(vector<jinghua_card>& vcards)
{
	vector<jinghua_card> tmp = vcards;
	std::sort(tmp.begin(), tmp.end());
	//�����A23,��Ҫ�ı�A��Ȩ��
	if (tmp[2].card_weight_ == 14){
		if (tmp[0].card_weight_ == 2 && tmp[1].card_weight_ == 3){
			tmp[2].card_weight_ = 1;
			vcards = tmp;
			return true;
		}
	}
	if((tmp[0].card_weight_ + 1 == tmp[1].card_weight_ ) &&
		tmp[0].card_weight_ + 2 == tmp[2].card_weight_)
	{
		return true;
	}
	return false;
}
//�Ƿ��ж���
bool							has_pair(vector<jinghua_card>& vcards)
{
	return (vcards[0].card_weight_ == vcards[1].card_weight_ ) ||
		(vcards[0].card_weight_ == vcards[2].card_weight_) ||
		(vcards[1].card_weight_ == vcards[2].card_weight_);
}

bool							is_greater(jinhua_match_result& r1, jinhua_match_result& r2)
{
	if (r1.card_match_ == r2.card_match_){
		jinghua_card c1 = r1.get_max_card();
		jinghua_card c2 = r2.get_max_card();
		if (c1.card_weight_ == c2.card_weight_)	{
			return c1.card_id_ > c2.card_id_;
		}
		else
			return c1.card_weight_ > c2.card_weight_;
	}
	else{
		return r1.card_match_ > r2.card_match_;
	}
}

match_result			calc_match_result(vector<jinghua_card>& vcards)
{
	if (is_same_weight(vcards)){
		return match_result_baozhi;
	}
	else{
		bool b1 = is_same_color(vcards);
		bool b2 = is_continous(vcards);
		if (b1 && b2)	{
			return match_result_jingtiao;
		}
		else if (b1){
			return match_result_qingyishe;
		}
		else if (b2){
			return match_result_tuolaji;
		}
		else {
			if (has_pair(vcards)){
				return match_result_duizhi;
			}
			else{
				return match_result_none;
			}
		}
	}
}

jinhua_match_result		analyse_cards(vector<jinghua_card>& vcards)
{
	jinhua_match_result ret;
	if (vcards.size() == 3){
		ret.card_match_ = calc_match_result(vcards);//�����ȵ��������vcards���ܻ�������������ú����ı䡣
		ret.cards_ = vcards;
	}
	return ret;
}