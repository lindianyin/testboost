/************************************************************************/
/* 斗地主出牌,结算逻辑,包括出牌AI
/* manage by hjt
/* create @ 2016-2-15
/************************************************************************/

#include "poker_card.h"
//#include <debugapi.h>
#include "log.h"


log_file<debug_output> debug_log;

bool		asc_sort_csf(csf_result& a, csf_result& b)
{
	return a.weight_ < b.weight_;
}

void		probe_combinations(std::vector<ddz_card>& cards, std::vector<csf_result>& results);
void		do_follow_card(std::vector<ddz_card>& my_cards, csf_result& lead, csf_result& ret, bool isdead = false, int follow_level = -1);
int			pickup_attachment(std::vector<csf_result>& results, csf_result& to_play, int is_freely, bool deadly, int attach_csf_to_pick = 3);

//是否需要pass
bool		should_pass_for_alliance(csf_result& csf)
{
	bool ret = (csf.weight_ >= 13 && csf.csf_val_ == csf_single && csf.main_count_ == 1);
	ret |= (csf.weight_ >= 8 && csf.csf_val_ == csf_single && csf.main_count_ > 1);
	ret |= (csf.weight_ >= 10 && csf.csf_val_ == csf_dual);
	ret |= (csf.csf_val_ == csf_quadruple || csf.csf_val_ == csf_quadruple_dual || csf.csf_val_ == csf_quadruple_single);
	ret |= (csf.csf_val_ == csf_triple || csf.csf_val_ == csf_triple_dual) && csf.weight_ >= 8;
	return ret;
}

int evalue_score_to_be_banker(std::vector<ddz_card>& mycards)
{
	int score = 0;
	std::vector<csf_result> v;
	probe_combinations(mycards, v);

	for (unsigned i = 0; i < v.size(); i++)
	{
		csf_result& csf = v[i];
		//单牌
		if (csf.csf_val_ == csf_single){
			//有大单牌
			for (unsigned ii = 0; ii < csf.main_cards_.size(); ii++)
			{
				if (csf.main_cards_[ii].card_weight_ == 13){
					score++;
				}
				else if (csf.main_cards_[ii].card_weight_ == 14){
					score += 2;
				}
				else if (csf.main_cards_[ii].card_weight_ == 15){
					score += 4;
				}
			}
		}
		//有大的对
		else if (csf.csf_val_ == csf_dual && csf.weight_ > 12){
			score += 2;
		}
		//有大三张
		else if (csf.csf_val_ == csf_triple && csf.weight_ > 9){
			score += (14 -  csf.weight_);
		}
		//有炸
		else if (csf.csf_val_ == csf_quadruple && csf.weight_ > 9){
			score += 4;
		}
	}
	return score;
}

std::vector<ddz_card>	remove_cards(std::vector<ddz_card>& my_cards, std::vector<ddz_card>& to_remove)
{
	std::vector<ddz_card> ret = my_cards;
	for (unsigned i = 0; i < to_remove.size(); i++)
	{
		auto it = std::find(ret.begin(), ret.end(), to_remove[i]);
		if (it != ret.end())	{
			ret.erase(it);
		}
	}
	return ret;
}


//************************************
// Method:    remove_cards
// FullName:  remove_cards
// Access:    public 
// Returns:   std::vector<ddz_card>				移除后剩余的牌
// Qualifier:									 
// Parameter: std::vector<ddz_card> & my_cards	我的手牌
// Parameter: csf_result & to_remove			待移除的手牌
//************************************
std::vector<ddz_card>	remove_cards(std::vector<ddz_card>& my_cards, csf_result& to_remove)
{
	auto v = remove_cards(my_cards, to_remove.main_cards_);
	return remove_cards(v, to_remove.attachments_);
}

int	next_same(int pos, std::vector<ddz_card>& thisp, int weight)
{
	int nsame = 0;
	for (unsigned i = pos; i < thisp.size(); i++)
	{
		if (thisp[i].card_weight_ != weight){
			break;
		}
		nsame++;
	}
	return nsame;
}


#define check_should_contine_or_break(c, keep_i)\
if (mnumber >= c){\
	break;\
}\
else{\
	mnumber = 0;\
	dcount = 0;\
	vout.clear();\
	if(keep_i){\
		i = ii - 1; \
	}\
	continue;\
}

//单连牌，拆1个三张牌可以接受或拆2个对子可以接受,拆了3张不能拆对子，对子也一样。不能再多
void		analyze_single(std::vector<ddz_card>& thisp,
											 csf_result& ret,
											 bool deadly,	
											 int cnt,			//配多少个,0表示尽可能多配
											 int weight		//
											 )
{
	std::vector<ddz_card> backup = thisp;
	std::vector<ddz_card> vout;
	int mnumber = 0;//顺子中牌的数量
	int dcount  = 0;  
	
	int bc = (cnt >= 1000 ? 5 : cnt);	//要配几张牌,如果cnt > 100 则表示尽力配多的牌,不固定数目

	for (unsigned i = 0; i < thisp.size(); i++)
	{
		if (weight >= 0 && thisp[i].card_weight_ <= weight) continue;
		//2以上的牌都不是连牌
		if (thisp[i].card_weight_ >= 13 && cnt > 1) continue;
		//下个相同的牌数
		int nsa = next_same(i, thisp, thisp[i].card_weight_);

		int ii = i;
		i += (nsa - 1);

		//如果拆了一个对
		if (nsa == 2){
			dcount += 1;
		}
		else if (nsa == 3){
			if (thisp[i].card_weight_ < 8){
				dcount += 2;
			}
			else{
				dcount += 3;
			}
		}
		else if (nsa == 4){
			dcount += 5;
		}
		//如果拆牌过多,则不再继续拆牌
		if (dcount > 2 && !deadly){
			check_should_contine_or_break(bc, false);
		}

		//不拆大的三张牌
		if (nsa > 2 && thisp[ii].card_weight_ > 8 && !deadly){
			check_should_contine_or_break(bc, false);
		}

		if (vout.empty()){
			vout.push_back(thisp[ii]);
			mnumber++;
			if (mnumber >= cnt)
				break;
		}
		else{
			//连牌断掉了,决定是继续探测还是跳出
			if (thisp[ii].card_weight_ != vout[vout.size() - 1].card_weight_ + 1){
				//已经5连了.
				check_should_contine_or_break(bc, true);
			}
			else{
				vout.push_back(thisp[ii]);
				mnumber++;
				if (mnumber >= cnt)
					break;
			}
		}
	}

	if ((cnt >= 1000 &&mnumber >= 5) || mnumber == cnt){
		ret.csf_val_ = csf_multi_single;
		ret.main_count_ = mnumber;
		ret.weight_ = vout[0].card_weight_;//顺子中最小的牌就是该牌型的权值
		ret.main_cards_ = vout;
	}
}

//对连牌 拆1个三张牌可以接受,不可以拆炸弹
void		analyze_dual(std::vector<ddz_card>& thisp,
										 csf_result& ret, 
										 bool deadly,
										 int cnt, //配多少个,0表示尽可能多配
										 int weight
										 )
{
	std::vector<ddz_card> backup = thisp;
	std::vector<ddz_card> vout;
	int mnumber = 0;	int dcount = 0;

	int bc = (cnt >= 1000 ? 3 : cnt);	//要配几张牌,如果cnt > 100 则表示尽力配多的牌,不固定数目

	for (unsigned i = 0; i < thisp.size(); i++)
	{
		if (weight >= 0 && thisp[i].card_weight_ <= weight) continue;
		if (thisp[i].card_weight_ >= 13 && cnt > 1) continue;
		if (weight >=0 && !deadly && thisp[i].card_weight_ > (weight + 4) && thisp[i].card_weight_ > 10){
			continue;
		}
		//下个相同的牌数
		int nsa = next_same(i, thisp, thisp[i].card_weight_);
		int	ii = i;
		i += (nsa - 1);

		//如果是单牌,跳过
		if (nsa == 1)
			check_should_contine_or_break(bc, false);
	
		//如果拆了一个三张
		if (nsa == 3){
			dcount += 1;
		}
		else if (nsa == 4){//拆了一个四张
			dcount += 2;
		}

		//先不拆3个
		if (dcount > 0 && !deadly){
			check_should_contine_or_break(bc, false);
		}

		//如果只是打一对,则不拆牌
		if (nsa > 2 && cnt == 1 && !deadly){
				mnumber = 0;
				dcount = 0;
				vout.clear();
				continue;
		}

		if (vout.empty()){
			//接收2个
			vout.insert(vout.end(), thisp.begin() + ii, thisp.begin() + ii + 2);
			mnumber++;
			if (mnumber >= cnt)
				break;
		}
		else{
			//连牌断掉了,决定是继续探测还是跳出
			if (thisp[ii].card_weight_ != vout[vout.size() - 1].card_weight_ + 1){
				check_should_contine_or_break(bc, true);
			}
			else{
				vout.insert(vout.end(), thisp.begin() + ii, thisp.begin() + ii + 2);
				mnumber++;
				if (mnumber >= cnt)
					break;
			}
		}
	}

	if((cnt >= 1000 && (mnumber == 1 || mnumber >= 3)) || mnumber == cnt){
		ret.csf_val_ = csf_dual;
		if (mnumber >= 3){
			ret.csf_val_ = csf_multi_dual;//change by lfp
		}
		ret.main_count_ = mnumber;
		ret.weight_ = vout[0].card_weight_;
		ret.main_cards_ = vout;
	}
}
//三张
void		analyze_triple(std::vector<ddz_card>& thisp,
											 csf_result& ret, 
											 bool deadly,
											 int cnt, //配多少个,0表示尽可能多配
											 int weight)
{
	std::vector<ddz_card> backup = thisp;
	std::vector<ddz_card> vout;
	int mnumber = 0;	int dcount = 0;

	int bc = cnt >= 100 ? 2 : cnt;	//要配几张牌,如果cnt > 100 则表示尽力配多的牌,不固定数目

	for (unsigned i = 0; i < thisp.size(); i++)
	{
		if (weight >= 0 && thisp[i].card_weight_ <= weight) continue;
		if (thisp[i].card_weight_ >= 13 && cnt > 1) continue;
		//如果牌面比较大,不因出小牌面而出
		if (weight >=0 && !deadly && thisp[i].card_weight_ > (weight + 4) && thisp[i].card_weight_ > 10){
			continue;
		}
		//下个相同的牌数
		int nsa = next_same(i, thisp, thisp[i].card_weight_);

		int ii = i;
		i += (nsa - 1);

		//如果是单牌,对牌跳过
		if (nsa != 3)
			check_should_contine_or_break(bc, false);

		if (vout.empty()){
			//接收2个
			vout.insert(vout.end(), thisp.begin() + ii, thisp.begin() + ii + 3);
			mnumber++;
			if (mnumber >= cnt)
				break;
		}
		else{
			///连牌断掉了,决定是继续探测还是跳出
			if (thisp[ii].card_weight_ != vout[vout.size() - 1].card_weight_ + 1){
				check_should_contine_or_break(bc, true);
			}
			else{
				vout.insert(vout.end(), thisp.begin() + ii, thisp.begin() + ii + 3);
				mnumber++;
				if (mnumber >= cnt)
					break;
			}
		}
	}

	if((cnt >= 1000 && mnumber > 0) || mnumber == cnt){
		ret.csf_val_ = csf_triple;
		ret.main_count_ = mnumber;
		ret.weight_ = vout[vout.size() - 1].card_weight_;
		ret.main_cards_ = vout;
	}
}

//四张(炸弹)
void		analyze_quadruple(std::vector<ddz_card>& thisp, 
													csf_result& ret, 
													int cnt, //配多少个,0表示尽可能多配
													int weight)
{
	std::vector<ddz_card> backup = thisp;
	std::vector<ddz_card> vout;
	int mnumber = 0;	int dcount = 0;

	for (unsigned i = 0 ; i < thisp.size(); i++)
	{
		if (weight >= 0 && thisp[i].card_weight_ <= weight) continue;
		if (thisp[i].card_weight_ > 13 && cnt > 1) continue;
		//下个相同的牌数
		int nsa = next_same(i, thisp, thisp[i].card_weight_);
		int ii = i;
		i += (nsa - 1);

		//如果是单牌,对牌跳过
		if (nsa != 4)
			check_should_contine_or_break(1, false);

		if (vout.empty()){
			//接收2个
			vout.insert(vout.end(), thisp.begin() + ii, thisp.begin() + ii + 4);
			mnumber++;
			break;
		}
	}

	if(mnumber > 0){
		ret.csf_val_ = csf_quadruple;
		ret.weight_ = vout[0].card_weight_;
		ret.main_cards_ = vout;
		ret.main_count_ = mnumber;
	}
}
//大小王
void	analyze_jet(std::vector<ddz_card>& thisp, 
									csf_result& ret)
{
	int cnt = 0;
	for (unsigned i = 0; i < thisp.size(); i++)
	{
		if (thisp[i].card_weight_ == 14 || thisp[i].card_weight_ == 15){
			ret.main_cards_.push_back(thisp[i]);
		}
	}

	if (ret.main_cards_.size() == 2){
		ret.main_count_ = 1;
		ret.csf_val_ = csf_jet;
		ret.weight_ = 15;
	}
	else{
		ret.main_cards_.clear();
	}
}

//分析牌型,plan必须先按单牌从小到大排序
csf_result	analyze_csf(std::vector<ddz_card>& plan)
{
	std::vector<ddz_card> thisp = plan;
	std::vector<csf_result> v;
	csf_result csf_ret;
	probe_combinations(plan, v);
	if (plan.size() == 1){
		csf_ret.csf_val_ = csf_single;
		csf_ret.weight_ = plan[0].card_weight_;
		csf_ret.main_count_ = 1;
		csf_ret.main_cards_ = plan;
	}
	else if (plan.size() == 2){
		if (plan[0].card_weight_ == plan[1].card_weight_){
			csf_ret.csf_val_ = csf_dual;
			csf_ret.main_count_ = 1;
			csf_ret.weight_ = plan[0].card_weight_;
			csf_ret.main_cards_ = plan;
		}
		//如果是大小王，则是飞机
		else if (plan[0].card_weight_ == 14 && plan[1].card_weight_ == 15){
			csf_ret.csf_val_ = csf_jet;
			csf_ret.weight_ = plan[0].card_weight_;
			csf_ret.main_cards_ = plan;
			csf_ret.main_count_ = 1;
		}
		else{
			csf_ret.csf_val_ = csf_not_allowed;
		}
	}
	else { // > 3 张
		do 
		{
			analyze_quadruple(thisp, csf_ret, 1000, -1);
			if (csf_ret.csf_val_ != csf_not_allowed){
				if(pickup_attachment(v, csf_ret, 1, true, csf_single) > 0){
					thisp = remove_cards(thisp, csf_ret);
					//不合法
					if (thisp.size() > 0 ){
						csf_ret = csf_result();
					}
				}
				else if (pickup_attachment(v, csf_ret, 1, true, csf_dual) > 0){
					thisp = remove_cards(thisp, csf_ret);
					//不合法
					if (thisp.size() > 0 ){
						csf_ret = csf_result();
					}
				}
				else{
					thisp = remove_cards(thisp, csf_ret);
					if(thisp.size() > 0){
						csf_ret = csf_result();
					}
				}
				break;
			}

			thisp = plan;
			analyze_triple(thisp, csf_ret, true, 1000, -1);
			if (csf_ret.csf_val_ != csf_not_allowed){
				if(pickup_attachment(v, csf_ret, 1, true, csf_single) > 0){
					thisp = remove_cards(thisp, csf_ret);
					//不合法
					if (thisp.size() > 0 ){
						csf_ret = csf_result();
					}
				}
				else if (pickup_attachment(v, csf_ret, 1, true, csf_dual) > 0){
					thisp = remove_cards(thisp, csf_ret);
					//不合法
					if (thisp.size() > 0 ){
						csf_ret = csf_result();
					}
				}
				else
				{
					thisp = remove_cards(thisp, csf_ret);
					if(thisp.size() > 0){
						csf_ret = csf_result();
					}
				}
				break;
			}

			thisp = plan;
			analyze_triple(thisp, csf_ret, true, 1, -1);
			if (csf_ret.csf_val_ != csf_not_allowed){
				if(pickup_attachment(v, csf_ret, 1, true, csf_single) > 0){
					thisp = remove_cards(thisp, csf_ret);
					//不合法
					if (thisp.size() > 0 ){
						csf_ret = csf_result();
					}
				}
				else if (pickup_attachment(v, csf_ret, 1, true, csf_dual) > 0){
					thisp = remove_cards(thisp, csf_ret);
					//不合法
					if (thisp.size() > 0 ){
						csf_ret = csf_result();
					}
				}
				else{
					thisp = remove_cards(thisp, csf_ret);
					if(thisp.size() > 0){
						csf_ret = csf_result();
					}
				}
				break;
			}

			thisp = plan;
			analyze_dual(thisp, csf_ret, true, 1000, -1);
			if (csf_ret.csf_val_ != csf_not_allowed){
				if (csf_ret.main_count_ < 3){
					csf_ret = csf_result();
				}
				break;
			}

			thisp = plan;
			analyze_single(thisp, csf_ret, true, 1000, -1);
			if (csf_ret.csf_val_ != csf_not_allowed){
				if (csf_ret.main_count_ < 5){
					csf_ret = csf_result();
				}
				break;
			}
		} while (0);
	}
	
	return csf_ret;
}

//牌能不能出
int is_greater_pattern(csf_result& plan, csf_result& refer)
{
	//牌型是炸弹,判断第一张大小就行
	if (plan.csf_val_ == csf_jet){
		return true;
	}
	else if (plan.csf_val_ >= csf_quadruple){
		if (refer.csf_val_ == csf_quadruple){
			return plan.weight_ > refer.weight_;
		}
		else if (refer.csf_val_ == csf_jet){
			return false;
		}
		else {
			return true;
		}
	}
	//如果牌型一样
	else if (plan.csf_val_ == refer.csf_val_){
		//要出的牌型必须和出过的牌型一样，且要大于出过的牌型
		if (plan.main_count_ == refer.main_count_ && 
			plan.attachment_count_ == refer.attachment_count_ && 
			plan.weight_ > refer.weight_){
				return true;
		}
	}
	//牌型不一样，肯定不可以出
	return false;
}

//探测主牌型
void		probe_combinations(std::vector<ddz_card>& cards, std::vector<csf_result>& results)
{
	std::vector<ddz_card> thisp = cards;
	
	//优先配单连(顺子)
	do{
		csf_result ret;
		analyze_single(thisp, ret, false, 1000, -1);
		if (ret.csf_val_ != csf_not_allowed){
			results.push_back(ret);
			thisp = remove_cards(thisp, ret);
		}
		else{
			break;
		}
	}while (1);

	//配4张炸
	do
	{
		csf_result ret;
		analyze_quadruple(thisp, ret, 1000, -1);
		if (ret.csf_val_ != csf_not_allowed){
			results.push_back(ret);
			thisp = remove_cards(thisp, ret);
		}
		else{
			break;
		}
	}while (1);

	//配连3张(飞机)
	do
	{
		csf_result ret;
		analyze_triple(thisp, ret, false, 1000, -1);
		if (ret.csf_val_ != csf_not_allowed){
			results.push_back(ret);
			thisp = remove_cards(thisp, ret);
		}
		else{
			break;
		}
	}while (1);

	//配单3张
	do
	{
		csf_result ret;
		analyze_triple(thisp, ret, false, 1, -1);
		if (ret.csf_val_ != csf_not_allowed){
			results.push_back(ret);
			thisp = remove_cards(thisp, ret);
		}
		else{
			break;
		}
	}while (1);

	//配连对
	do
	{
		csf_result ret;
		analyze_dual(thisp, ret, false, 1000, -1);
		if (ret.csf_val_ != csf_not_allowed){
			results.push_back(ret);
			thisp = remove_cards(thisp, ret);
		}
		else{
			break;
		}
	}while (1);


	//配单对
	do
	{
		csf_result ret;
		analyze_dual(thisp, ret, false, 1, -1);
		if (ret.csf_val_ != csf_not_allowed){
			results.push_back(ret);
			thisp = remove_cards(thisp, ret);
		}
		else{
			break;
		}
	}while (1);

	bool has_singles = false;
	//剩下的都是单牌
	if (thisp.size() > 0){
		csf_result ret;
		ret.csf_val_ = csf_single;
		ret.main_count_ = thisp.size();
		ret.main_cards_ = thisp;
		results.push_back(ret);
		has_singles = true;
	}

	if (results.size() > 1 && has_singles){
		std::sort(results.begin(), results.end() - 1, asc_sort_csf);
	}
	else if (results.size() > 1 ){
		std::sort(results.begin(), results.end(), asc_sort_csf);
	}
}

//先择带单牌还是带对牌，考虑以下几个因素:
//单牌已经比较大(权重>=J)
//对牌已经比较大
//如果是跟牌,to_play里的attachment_count要设置好,这里就会设置根据这个来提取
int		pickup_attachment(std::vector<csf_result>& results, csf_result& to_play, int is_freely, bool deadly, int attach_csf_to_pick)
{
	//所有单牌在结果的最后，并且在csf_result.main_cards_里;
	if (results.empty()){
		return 0;
	}

	csf_result singles;
	if (results[results.size() - 1].csf_val_ == csf_single){
		singles = results[results.size() - 1]; 
	}
	
	int		c = 0;//带的数量

	vector<ddz_card> attachment;//带的牌
	int attc = 0;//实际带的数量
	int tp = -1;
	//如果是先手牌,
	if (is_freely){
		if (to_play.csf_val_ == csf_triple){ //三张带主牌型数目张
			c = to_play.main_count_;
		}
		else if (to_play.csf_val_ == csf_quadruple){//四张带两张
			c = 2;
		}

		int singc = 0; //单牌数量
		int dualc = 0; //单对数量
		if (!singles.main_cards_.empty())	{
			for(unsigned i = 0; i < singles.main_cards_.size(); i++)
			{
				//如果牌面不是很大,或者已经只剩单牌了(如果只剩下一张单的大牌，会带掉)
				if (singles.main_cards_[i].card_weight_ < 13 /*||	singles.main_cards_.size() == 1*/){
					singc++;
				}
			}
		}

		auto it = results.begin();
		while (it != results.end())
		{
			if ((*it).csf_val_  == csf_dual && (*it).weight_ < 13 && (*it).main_count_ == 1){
				dualc++;
			}
			it++;
		}

		//只允许带单牌
		if (attach_csf_to_pick == csf_single && singc >= c){//带单牌
			attachment.insert(attachment.end(), singles.main_cards_.begin(), singles.main_cards_.begin() + c);
			if (to_play.csf_val_ == csf_triple){
				to_play.csf_val_ = csf_triple;
			}
			else if (to_play.csf_val_ == csf_quadruple){
				to_play.csf_val_ = csf_quadruple_single;
			}
			to_play.attachment_count_ = c;
			to_play.attachments_ = attachment;
			return c;
		}
		else if (attach_csf_to_pick == csf_dual && dualc >= c){//带对子
			auto it = results.begin();
			attc = 0;
			while (it != results.end() && attc < c)
			{
				if ((*it).csf_val_  == csf_dual && (*it).main_count_ == 1){
					attc++;
					attachment.insert(attachment.end(), (*it).main_cards_.begin(), (*it).main_cards_.end());
				}
				it++;
			}

			if (to_play.csf_val_ == csf_triple){
				to_play.csf_val_ = csf_triple_dual;
			}
			else if (to_play.csf_val_ == csf_quadruple){
				to_play.csf_val_ = csf_quadruple_dual;
			}
			to_play.attachment_count_ = c;
			to_play.attachments_ = attachment;
			return c;
		}
	}// end of if (is_freely){
	else{//后手跟牌
		c = to_play.attachment_count_;
		//跟牌需要带单
		if (to_play.csf_val_ == csf_triple || to_play.csf_val_ == csf_quadruple_single){//跟单张
			if (deadly){
				//如果单牌不够,拆单对
				if ((int)singles.main_cards_.size() < c)	{
					auto it = results.begin();
					while (it != results.end())
					{
						if ((*it).csf_val_  == csf_dual && (*it).main_count_ == 1){
							singles.main_cards_.insert(singles.main_cards_.end(), (*it).main_cards_.begin(), (*it).main_cards_.end());
						}
						it++;
					}
				}
			}
			
			for (int i = 0; i < c && i < (int)singles.main_cards_.size(); i++)
			{
				attachment.push_back(singles.main_cards_[i]);
				attc++;
			}
		}
		else if (to_play.csf_val_ == csf_triple_dual || to_play.csf_val_ == csf_quadruple_dual){//跟对子
			auto it = results.begin();
			while (it != results.end() && attc < c)
			{
				if ((*it).csf_val_  == csf_dual && (*it).main_count_ == 1){
					attc++;
					attachment.insert(attachment.end(), (*it).main_cards_.begin(), (*it).main_cards_.end());
				}
				it++;
			}
		}
		if (attc >= c){
			to_play.attachments_ = attachment;
			return c;
		}
	}
	return 0;
}


int		remap_pos(int ori_pos,
											 std::vector<ddz_card>& lead_cards,
											 std::vector<ddz_card>& next_cards,
											 std::vector<ddz_card>& next_next_cards,
											 std::vector<ddz_card>* v1,
											 std::vector<ddz_card>* v2,
											 std::vector<ddz_card>* v3)
{
	std::vector<ddz_card>* banker = nullptr;
	if (ori_pos == 0){
		banker = &lead_cards;
	}
	else if (ori_pos == 1){
		banker = &next_cards;
	}
	else{
		banker = &next_next_cards;
	}
	
	if (banker == v1){
		return 0;
	}
	else if (banker == v2){
		return 1;
	}
	else 
		return 2;
}

//模拟打牌.
//这个函数需要优化,目前太不优雅,实现功能先
//这个函数用来决定出牌策略的,
//其中如果放弃牌的时候,如果返回的结是不输,则会放弃牌,这种情况下一定要考虑好是不是正的可以放弃牌.
int		simulate_play(int banker_pos,
										int	leadpos,
										bool	isdeadly,
										csf_result& lead,
										std::vector<ddz_card>& my_cards,
										std::vector<ddz_card>& next_cards,
										std::vector<ddz_card>& next_next_cards,
										bool& is_over)
{
	int		winner = -1;
	csf_result csf_sim_lead = lead;
	auto bak_0 = my_cards;
	auto bak_1 = next_cards;
	auto bak_2 = next_next_cards;
	while (winner < 0)
	{
		csf_result ret; bool pass1 = true, pass2 = true;
		int bankp = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
			&bak_1, &bak_2, &bak_0);

		int leadp = remap_pos(leadpos, bak_0, bak_1, bak_2, 
			&bak_1, &bak_2, &bak_0);

		follow_card(bankp, leadp, csf_sim_lead, ret, bak_1, bak_2, bak_0, isdeadly, true); //如果
		if (ret.csf_val_ != csf_not_allowed){
			csf_sim_lead = ret;
			pass1 = false;
			bak_1 = remove_cards(bak_1, ret);
			leadpos = 1; 
			if (bak_1.empty()){
				winner = 1;
				is_over = true;
				break;
			}
		}

		ret = csf_result();
		bankp = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
			&bak_2, &bak_0, &bak_1);

		leadp = remap_pos(leadpos, bak_0, bak_1, bak_2, 
			&bak_2, &bak_0, &bak_1);

		follow_card(bankp, leadp, csf_sim_lead, ret, bak_2, bak_0, bak_1, isdeadly, true);
		if (ret.csf_val_ != csf_not_allowed){
			csf_sim_lead = ret;
			pass2 = false;
			leadpos = 2;
			bak_2 = remove_cards(bak_2, ret);
			if (bak_2.empty()){
				winner = 2;
				is_over = true;
				break;
			}
		}

		//说明没有人要得起
		if (pass1 && pass2){
			winner = leadpos;
			break;
		}
		else{
			ret = csf_result();
			bankp = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
				&bak_0, &bak_1, &bak_2);

			leadp = remap_pos(leadpos, bak_0, bak_1, bak_2, 
				&bak_0, &bak_1, &bak_2);
			//到了第二轮,可以用死亡模式跟牌,因为第二轮,玩家是有机会出牌的,只要保证第二轮能跟上牌,就说明暂时不会输
			follow_card(bankp, leadpos, csf_sim_lead, ret, bak_0, bak_1, bak_2, true, true);
			if (ret.csf_val_ != csf_not_allowed){
				csf_sim_lead = ret;
				bak_0 = remove_cards(bak_0, ret);	
				leadpos = (leadp + 1) % 3;
				winner = 0;
				leadpos = 0;
				if (bak_0.empty()){
					is_over = true;
					break;
				}
			}
			else{
				if (!pass2){
					winner = 2;
					break;
				}
				else{
					winner = 1;
					break;
				}
			}
		}
	}
	
	if (is_over){
		return winner;
	}
	else{
		//这一回合自己赢了
		if (winner == 0){
			return 0;
		}
		//一回合其它玩家赢了
		else if(winner == 1){
			int bankp = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
				&bak_1, &bak_2, &bak_0);
			csf_result tcsf;

			lead_card(bankp, tcsf, bak_1, bak_2, bak_0, true);
			bak_1 = remove_cards(bak_1, tcsf);
			//让其它玩家出牌,只要牌没有结束,就算自己赢
			if (!bak_1.empty()){
				//再下一张牌
				int bankp2 = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
					&bak_2, &bak_0, &bak_1);
				int leadp2 = remap_pos(1, bak_0, bak_1, bak_2, 
					&bak_2, &bak_0, &bak_1);
				csf_result tcsf_follow;
				follow_card(bankp2, leadp2, tcsf, tcsf_follow, bak_2, bak_0, bak_1, true, true);
				if (tcsf_follow.csf_val_ != csf_not_allowed){
					bak_2 = remove_cards(bak_2, tcsf_follow);
					if (bak_2.empty()){
						is_over = true;
						winner = 2;
					}
					else{
						winner = 0;
					}
				}
				else
					winner = 0;
			}
			else{
				is_over = true;
				winner = 1;
			}
		}
		//一回合其它玩家赢了
		else if (winner == 2){
			int bankp = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
				&bak_2, &bak_0, &bak_1);

			csf_result tcsf;
			lead_card(bankp, tcsf, bak_2, bak_0, bak_1, true);
			bak_2 = remove_cards(bak_2, tcsf);
			//让其它玩家出牌,结果还是自己赢,只要对手牌没出完,就算自己赢
			if (!bak_2.empty()){
				//再下一张牌
				int bankp2 = remap_pos(banker_pos, bak_0, bak_1, bak_2, 
					&bak_0, &bak_1, &bak_2);
				int leadp2 = remap_pos(2, bak_0, bak_1, bak_2, 
					&bak_0, &bak_1, &bak_2);
				csf_result tcsf_follow;
				follow_card(bankp2, leadp2, tcsf, tcsf_follow, bak_0, bak_1, bak_2, true, true);
				if (tcsf_follow.csf_val_ != csf_not_allowed){
					bak_0 = remove_cards(bak_0, tcsf_follow);
					if (bak_0.empty()){
						is_over = true;
						winner = 0;
					}
					else{
						winner = 0;
					}
				}
				else
					winner = 0;
			}
			else{
				is_over = true;
				winner = 2;
			}
		}
		return winner;
	}
}

//出这手牌合不合适
bool		is_proper_pattern(csf_result& pat, std::vector<int>& exclusive_csf, int llv)
{
	auto	it = std::find(exclusive_csf.begin(), exclusive_csf.end(), pat.csf_val_);
	bool	is_exclude = it != exclusive_csf.end();
	if (pat.csf_val_ == csf_single){
		//单牌出的优先度要适当提高
		if ((pat.main_cards_[0].card_weight_ < (llv + 2) && !is_exclude) || llv < 0){
			return true;
		}
	}
	else{
		if ((pat.weight_ < llv && !is_exclude) || llv < 0){
			return true;
		}
	}
	return false;
}


bool	is_max_csf_of_all(int bankp,
												csf_result& toplay,
												vector<ddz_card>& next,
												vector<ddz_card>& next_next)
{
	csf_result ret;
	//如果我是庄家,看两个闲家是不是能接上牌
	if (bankp == 0){
		do_follow_card(next, toplay, ret, true, toplay.weight_);
		if (ret.csf_val_ != csf_not_allowed){
			return false;
		}

		ret = csf_result();
		do_follow_card(next_next, toplay, ret, true, toplay.weight_);
		if (ret.csf_val_ != csf_not_allowed){
			return false;
		}
	}
	//如果我是闲家,看庄家是不是能接上牌
	else{
		if (bankp == 1){
			do_follow_card(next, toplay, ret, true, toplay.weight_);
			if (ret.csf_val_ != csf_not_allowed){
				return false;
			}
		}
		else{
			do_follow_card(next_next, toplay, ret, true, toplay.weight_);
		}
		if (ret.csf_val_ != csf_not_allowed){
			return false;
		}
	}
	return true;
}

//必定要赢
bool	sure_to_win(
									int bankp,
									csf_result& to_sure, 
									vector<ddz_card>& mycard, 
									vector<ddz_card>& next,
									vector<ddz_card>& next_next)
{
	std::vector<csf_result>other_remains;
	probe_combinations(mycard, other_remains);
	vector<ddz_card> thisp = remove_cards(mycard, to_sure);
	//如果只剩自己
	if (other_remains.size() == 0){
		//如果不是单牌,就赢定了
		if (to_sure.csf_val_ != csf_single){
			return true;
		}
		//如果还有其它单牌
		else{
			if (to_sure.main_cards_.size() > 2){
				return false;
			}
			else if (to_sure.main_cards_.size() < 2){
				return true;
			}
			//=2
			else{
				csf_result csf;
				csf.csf_val_ = csf_single;
				csf.main_cards_.push_back(to_sure.main_cards_[1]);
				return is_max_csf_of_all(bankp, csf, next, next_next);
			}
		}
	}
	else if(other_remains.size() != 1)
		return false;
	else{
		//如果当前牌面是最大的
		if (is_max_csf_of_all(bankp, to_sure, next, next_next)){
			csf_result& last = other_remains[0];
			if (last.csf_val_ == csf_single){
				return last.main_cards_.size() < 2;
			}
			else{
				return true;
			}
		}
		else{
			return false;
		}
	}
}

//有大牌型
bool	has_bigger_csf(csf_result& to_sure, std::vector<csf_result>& results)
{
	//如果要测试的牌型本身是大牌.则不先出
	if(results.size() == 0) return false;

	if (to_sure.csf_val_ == csf_quadruple || 
		to_sure.csf_val_ == csf_quadruple_dual ||
		to_sure.csf_val_ == csf_quadruple_single){
			 return false;
	}

	if (to_sure.csf_val_ == csf_single){
		csf_result sgl = results[results.size() - 1];
		if (sgl.csf_val_ == csf_single){
			if (sgl.main_cards_.size() > 1){
				int wmax = sgl.main_cards_[sgl.main_cards_.size() - 1].card_weight_;
				int wmin = sgl.main_cards_[0].card_weight_;
				if (wmax >= 14 && wmin < 14)	{
					return true;
				}
			}
			else{
				return false;
			}
		}
	}

	else{
		if (to_sure.csf_val_ == csf_triple || to_sure.csf_val_ == csf_triple_dual){
			if (to_sure.weight_ >= 10) return false;
		}
		else
			if (to_sure.weight_ >= 13) return false;

		for (unsigned i = 0; i < results.size(); i++)
		{
			if (to_sure.csf_val_ != results[i].csf_val_ ||
				to_sure.main_count_ != results[i].main_count_) continue;
			if (results[i].weight_ >= 13){
				return true;
			}
		}
	}	
	return false;
}

//判断是否会输
bool will_lose(int	banker_pos,
							 int	leadpos,
							 bool isdeadly,
							 csf_result& toplay, 
							 std::vector<ddz_card>& my_cards,			//我手上的牌
							 std::vector<ddz_card>& next_cards,		//对手1牌
							 std::vector<ddz_card>& next_next_cards//对手2的牌
							 )
{
	bool isover = false;
	int winner = simulate_play(banker_pos, leadpos, isdeadly, toplay, my_cards, next_cards, next_next_cards, isover);
	//如果我是庄家
	if (banker_pos == 0){
		//如果不是庄家赢
		if (winner != banker_pos){
			return isover;
		}
		else{
			return false;
		}
	}
	//如果我是闲家
	else{
		//如果赢的不是庄家
		if (winner != banker_pos){
			return false;
		}
		else{
			return isover;
		}
	}
}

//参数results故意不是引用,
//评分越高,出牌优先级越高.
int		calc_csf_score(int bankp,
										 int i,
										 std::vector<csf_result> results, 
										 std::vector<ddz_card>& mycard,
										 std::vector<ddz_card>& next,
										 std::vector<ddz_card>& next_next
										 )
{
	csf_result csf = results[i];
	//如果必会赢,则首先要出这个
	if (sure_to_win(bankp, csf, mycard, next, next_next)){
		return 0x7FFFFFFF;
	}
	else{
		bool hasbig		= has_bigger_csf(csf, results);
		int	big_score = 0;//hasbig ? 100000 : 0;
		int	csf_score = 0;

		//小牌面优先出
		int	weight_score = (16 - csf.min_weight()) * 1000;

		//单连牌
		if (csf.csf_val_ == csf_multi_single){
			//小牌,优先级高
			csf_score = 999;
		}

		//三张牌
		if ((csf.csf_val_ == csf_triple || csf.csf_val_ == csf_triple_dual) && csf.main_count_ == 1){
			////小牌,优先级高
			csf_score = 900;
			int min_att = 0;
			if (csf.attachments_.size() > 0){
				min_att = csf.attachments_[0].card_weight_;
			}
			//三张牌的牌面太大了
			if (csf.weight_ - min_att > 6 && csf.weight_ > 8){
				weight_score = 8000;
			}
		}

		//单牌
		if (csf.csf_val_ == csf_single){
			if(csf.main_cards_.size() > 0){
				csf_score = 800;
			}
			else{
				weight_score = 0;
			}
		}

		//单对牌
		if (csf.csf_val_ == csf_dual && csf.main_count_ == 1){
			csf_score = 700;
		}

		//三张牌
		if ((csf.csf_val_ == csf_triple || csf.csf_val_ == csf_triple_dual) && csf.main_count_ > 1){
			////小牌,优先级高
			csf_score = 600;
			int min_att = 0;
			if (csf.attachments_.size() > 0){
				min_att = csf.attachments_[0].card_weight_;
			}
			//三张牌的牌面太大了
			if (csf.weight_ - min_att > 6 && csf.weight_ > 8){
				weight_score = 8000;
			}
		}
		//对连牌
		if (csf.csf_val_ == csf_multi_dual && csf.main_count_ > 1){
			csf_score = 500;
		}
			
		//炸弹
		if (csf.csf_val_ == csf_quadruple || csf.csf_val_ == csf_quadruple_single ||
			csf.csf_val_ == csf_quadruple_dual){
			csf_score = 400;
			weight_score = 0;
		}
			
		//大小王
		if (results[i].csf_val_ == csf_jet){
			csf_score = 300;
			weight_score = 0;
		}
		return big_score + csf_score + weight_score;
	}
}

void	choose_lead_cards_deadly(std::vector<ddz_card>& results, csf_result& ret)
{
	if (results.empty()) return;

	ret.csf_val_ = csf_single;
	ret.main_count_ = 1;
	ret.weight_ = results[results.size() - 1].card_weight_;
	ret.main_cards_.push_back(results[results.size() - 1]);
}

extern std::string		to_string(std::vector<ddz_card> cards);
void	enum_combinations(std::vector<ddz_card>& thisp, std::vector<csf_result>& results)
{
	std::vector<csf_result> tmp_results;
	probe_combinations(thisp, tmp_results);

	//先处理3带或4带的情况,这个情况比较复杂,先带上附件
	for (unsigned i = 0; i < tmp_results.size(); i++)
	{
		if (tmp_results[i].csf_val_ == csf_triple || tmp_results[i].csf_val_ == csf_quadruple){
			pickup_attachment(tmp_results, tmp_results[i], 1, false, csf_single);
			results.push_back(tmp_results[i]);
			thisp = remove_cards(thisp, tmp_results[i]);
		}
	}
	//重新组织一下
	tmp_results.clear();
	probe_combinations(thisp, tmp_results);
	results.insert(results.end(), tmp_results.begin(), tmp_results.end());

	//牌面从小到大排下序
	if (results.size() > 1){
		std::sort(results.begin(), results.end() - 1, asc_sort_csf);
	}
}

//出先手牌
//1.检查牌是否可以出，如果出了这手牌，地主就要赢了，就不出，则换出牌策略。
//2.如果自己手上有同牌型的大牌，则可以考虑出该牌型的小牌。
//n.出牌策略需要继续完善。
//三带大对了,
//4个不会带2对
int		lead_card(int banker_pos,
								csf_result& to_play,									//我要出的牌
								std::vector<ddz_card>& my_cards,			//我手上的牌
								std::vector<ddz_card>& next_cards,		//对手1牌
								std::vector<ddz_card>& next_next_cards,//对手2的牌
								bool is_test)
{
	std::vector<ddz_card> thisp = my_cards;
	std::vector<csf_result> results;//出牌结果
	{
		std::vector<csf_result> tmp_results;//分析出的牌型
		probe_combinations(thisp, tmp_results);

		//先处理3带或4带的情况,这个情况比较复杂,先带上附件
		for (unsigned i = 0; i < tmp_results.size(); i++)
		{
			if (tmp_results[i].csf_val_ == csf_triple || tmp_results[i].csf_val_ == csf_quadruple){
				csf_result tcsf = tmp_results[i];
				//带单牌的情况
				int c1 = pickup_attachment(tmp_results, tcsf, 1, false, csf_single);
				if (c1 > 0){
					results.push_back(tcsf);
				}
				
				//带对子的情况
				tcsf.attachments_.clear();
				tcsf.attachment_count_ = 0;
				int c2 = pickup_attachment(tmp_results, tcsf, 1, false, csf_dual);
				if (c2 > 0){
					results.push_back(tcsf);
				}

				if (c1 == 0 && c2 == 0){
					results.push_back(tmp_results[i]);
				}
			}
			else{
				results.push_back(tmp_results[i]);
			}
		}
		//牌面从小到大排下序
		if (results.size() > 1){
			std::sort(results.begin(), results.end() - 1, asc_sort_csf);
		}
	}

	csf_result csf_ret;
	int max_score = -1, max_index = 0;
	
	csf_result _csf_ret_single;
	for (std::vector<csf_result>::iterator it = results.begin();
		it != results.end();++it){
		if(it->csf_val_ == csf_single){
			_csf_ret_single = *it;
			results.erase(it);
			break;
		}
	}
	if (_csf_ret_single.csf_val_ != csf_not_allowed){
		for (unsigned j = 0; j < _csf_ret_single.main_cards_.size(); j++)
		{
			csf_result csf_test;
			csf_test.csf_val_ = csf_single;
			csf_test.main_count_ = 1;
			csf_test.main_cards_.push_back(_csf_ret_single.main_cards_[j]);
			csf_test.weight_ = _csf_ret_single.main_cards_[j].card_weight_;
			results.push_back(csf_test);
		}
	}
	
	for (unsigned i = 0; i < results.size(); i++)
	{
		std::string str_cards = to_string(results[i].main_cards_);
		thisp = remove_cards(my_cards, results[i]);
		csf_result tcsf = results[i];
		//如果打完这个牌就没了.则表示赢了,
		if (!is_test && thisp.size() > 0){
			if (will_lose(banker_pos, 0, true,	tcsf, thisp, next_cards, next_next_cards)){
				debug_log.write_log("test type:%d cards:%s will lose, continue next test!", results[i].csf_val_, str_cards.c_str());
				continue;
			}
		};

		thisp = remove_cards(my_cards, results[i]);
		int score = calc_csf_score(banker_pos, i, results, thisp, next_cards, next_next_cards);
		if (max_score < score){
			max_score = score;
			max_index = i;
		}
	} // end of for (unsigned i = 0; i < results.size(); i++)
	//正常出牌要输,拆牌出
	if (max_score == -1){
		debug_log.write_log("all type is invailable, deadly choose cards!");
		csf_ret = csf_result();
		choose_lead_cards_deadly(my_cards, csf_ret);
	}
	else{
		csf_ret = results[max_index];
	}
	to_play = csf_ret;
	return true;
}

//
bool	is_right_csf(csf_result& lead, csf_result& follow)
{
	if (lead.csf_val_ == csf_triple || lead.csf_val_ == csf_triple_dual){
		return follow.csf_val_ == csf_triple;
	}
	else if (lead.csf_val_ == csf_quadruple_single || lead.csf_val_ == csf_quadruple_dual 
		||lead.csf_val_ == csf_quadruple)	{
			return follow.csf_val_ == csf_quadruple;
	}
	else
		return lead.csf_val_ == follow.csf_val_;
}

void	do_follow_card(std::vector<ddz_card>& my_cards, csf_result& lead, csf_result& ret, bool isdead, int start_weight)
{
	if (start_weight == -1){
		start_weight = lead.weight_ + 1;
	}

	if (lead.csf_val_ == csf_multi_single){
		analyze_single(my_cards, ret, isdead, lead.main_count_, start_weight);
	}
	else if (lead.csf_val_ == csf_single){
		//跟单牌
		if (!isdead){
			std::vector<csf_result> v;
			probe_combinations(my_cards, v);
			if (!v.empty()){
				auto singles = v[v.size() - 1].main_cards_;
				for (unsigned i = 0; i < singles.size(); i++)
				{
					if (singles[i].card_weight_ > start_weight){
						ret.csf_val_ = csf_single;
						ret.main_cards_.push_back(singles[i]);
						ret.main_count_ = 1;
						ret.weight_ = singles[i].card_weight_;
						break;
					}
				}
			}
		}
		else{
			for (unsigned i = 0; i < my_cards.size(); i++)
			{
				if (my_cards[i].card_weight_ > start_weight){
					ret.csf_val_ = csf_single;
					ret.main_cards_.push_back(my_cards[i]);
					ret.main_count_ = 1;
					ret.weight_ = my_cards[i].card_weight_;
					break;
				}
			}
		}
	}
	else if (lead.csf_val_ == csf_dual){
		analyze_dual(my_cards, ret, isdead, lead.main_count_, start_weight);
	}
	else{
		if(lead.csf_val_ == csf_triple ||
			lead.csf_val_ == csf_triple_dual){
			analyze_triple(my_cards, ret, isdead, lead.main_count_, start_weight);
			if(csf_triple == ret.csf_val_){
				ret.csf_val_ = lead.csf_val_;
			}
		}
		else if (lead.csf_val_ == csf_quadruple_single || 
			lead.csf_val_ == csf_quadruple_dual){
			analyze_quadruple(my_cards, ret, lead.main_count_, start_weight);
			if(csf_quadruple == ret.csf_val_){
				ret.csf_val_ = lead.csf_val_;
			}
		}

		if (ret.csf_val_ != csf_not_allowed){
			std::vector<ddz_card> thisp = remove_cards(my_cards, ret);
			std::vector<csf_result> v;
			probe_combinations(my_cards, v);

			ret.attachment_count_ = lead.attachment_count_;
			int att = pickup_attachment(v, ret, false, isdead);
			//如果没有取到同样数目的附件,则表示跟不上牌
			if (att != lead.attachment_count_)	{
				ret.csf_val_ = csf_not_allowed;
			}
		}
	}
}

void		deadly_follow_card(int banker_pos,
													 int lead_pos,
											csf_result& lead,
											csf_result& ret,
											std::vector<ddz_card>& my_cards,
											std::vector<ddz_card>& next_cards,				//下个玩家手上的牌
											std::vector<ddz_card>& next_next_cards		//下下个玩家手上的牌
											)
{
	//拆牌也跟不上,
	if (ret.csf_val_ == csf_not_allowed && lead.csf_val_ != csf_jet){
		int test_weight = lead.weight_;
		//先炸他妈的
		if (lead.csf_val_ != csf_quadruple){
			test_weight = 0;
		}
		analyze_quadruple(my_cards, ret, 1, test_weight);
		bool this_is_bomb				= ret.csf_val_ == csf_quadruple;
		bool lead_is_small_bomb = (lead.csf_val_ == csf_quadruple && ret.weight_ > lead.weight_);
		bool lead_isnot_bomb		= lead.csf_val_ < csf_quadruple;

		if ((this_is_bomb && (lead_is_small_bomb || lead_isnot_bomb)) || ret.csf_val_ == csf_jet)	{
			debug_log.write_log("fire in the hole.");
			return;
		}
		else{
			debug_log.write_log("fire in the hole, but no bomb.");
		}
		//用大小王炸
		analyze_jet(my_cards, ret);
		if (ret.csf_val_ != csf_not_allowed){
			debug_log.write_log("fire in the hole with jet!");
			return;
		}
		else{
			debug_log.write_log("fire in the hole, but no jet.");
		}
	}
	debug_log.write_log("call follow_card with deadly.");
	follow_card(banker_pos, lead_pos, lead, ret, my_cards, next_cards, next_next_cards, 1);
}


//************************************
// Method:    follow_card															//跟牌
// FullName:  follow_card
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int banker_pos													//庄位置
// Parameter: int leadpos															//当前轮上一个出牌者
// Parameter: csf_result & lead												//当前轮上一个出牌这出的牌
// Parameter: csf_result & ret												//跟的牌
// Parameter: std::vector<ddz_card> & my_cards				//我的牌
// Parameter: std::vector<ddz_card> & next_cards			//下一个人的牌
// Parameter: std::vector<ddz_card> & next_next_cards	//下下一个人的牌
// Parameter: bool deadly_follow											//死亡跟牌(不计代价跟牌)
// Parameter: bool is_test													  //
//************************************
void		follow_card(int		banker_pos,
										int		leadpos,
										csf_result& lead, 
										csf_result& ret,
										std::vector<ddz_card>& my_cards, 
										std::vector<ddz_card>& next_cards,				//下个玩家手上的牌
										std::vector<ddz_card>& next_next_cards,		//下下个玩家手上的牌
										bool deadly_follow,												//死亡跟牌(不计代价跟牌)
										bool is_test)
{
	bool can_grace_follow = false;

	std::vector<ddz_card> thisp = my_cards;
	int test_weight = lead.weight_;	
	csf_result csf_sim;
	do_follow_card(thisp, lead, csf_sim, deadly_follow, test_weight);

	//如果可以自然接上
	while (csf_sim.csf_val_ != csf_not_allowed && test_weight <= 15){
		//看出的牌是不是要输.
		//如果要输,则加大牌相同的自然牌型跟牌
		if (is_test){
			can_grace_follow = true;
			break;
		}
		else if(!deadly_follow){
			bool isover = false;
			thisp = remove_cards(thisp, csf_sim);
			
			//如果打完了,则表示已赢了.
			if (thisp.size() == 0){
				can_grace_follow = true;
				break;
			}

			int winner = simulate_play(banker_pos, 0, true, csf_sim, thisp, next_cards, next_next_cards, isover);
			//如果我是庄家
			if (banker_pos == 0){
				//如果赢家不是我,说明我输了,则提升牌型大小,看是否要输
				if (winner != banker_pos){
					csf_sim = csf_result();
					test_weight++;
					debug_log.write_log("will lose game, test next weight:%d", test_weight);
					do_follow_card(thisp, lead, csf_sim, deadly_follow, test_weight);
				}
				else{
					can_grace_follow = true;
					break;
				}
			}
			//如果我不是庄家
			else {
				//如果庄家赢了,说明我输了,则提升牌型大小,看是否要输
				if (winner == banker_pos)	{
					csf_sim = csf_result();
					test_weight++;
					debug_log.write_log("will losse game, test next weight:%d", test_weight);
					do_follow_card(thisp, lead, csf_sim, deadly_follow, test_weight);
				}
				else{
					can_grace_follow = true;
					break;
				}
			}
		}
		else{
			break;
		}
	}
	//如果不能自然接上
	//则看如果放弃这次出牌会不会输,如果会输，则进入死亡跟牌模式
	if (!is_test && !can_grace_follow && !deadly_follow){
		//跟不上,先看下炸了之后是否必赢
		{
			csf_result tmp_csf;
			int test_weight = lead.weight_;
			if (lead.csf_val_ != csf_quadruple && lead.csf_val_ != csf_jet){
				test_weight = 0;
			}
			analyze_quadruple(my_cards, tmp_csf, 1, test_weight);

			if (tmp_csf.csf_val_ == csf_quadruple)	{
				std::vector<ddz_card> tmpv = remove_cards(my_cards, tmp_csf);
				if(sure_to_win(banker_pos, tmp_csf, my_cards, next_cards, next_next_cards)){
					debug_log.write_log("fire in the hole.");
					ret = tmp_csf;
					return;
				}
			}
		}
		
		//用大小王炸
		{
			csf_result tmp_csf;
			analyze_jet(my_cards, tmp_csf);
			if (ret.csf_val_ == csf_jet){
				std::vector<ddz_card> tmpv = remove_cards(my_cards, tmp_csf);
				if(sure_to_win(banker_pos, tmp_csf, my_cards, next_cards, next_next_cards)){
					debug_log.write_log("fire in the hole.");
					ret = tmp_csf;
					return;
				}
			}
		}

		//炸了不是必赢,先看放弃牌..
		debug_log.write_log("can't follow any cards gracely, test if pass will cause losing game.");
		bool isover = false;
		int winner = simulate_play(banker_pos, leadpos, false, lead, thisp, next_cards, next_next_cards, isover);
		//如果我是庄家,
		if (banker_pos == 0){
			//如果赢的不是我,说明我输了
			if (winner != 0){
				debug_log.write_log("will lose game, enter dealy follow mode.");
				deadly_follow_card(banker_pos, leadpos, lead, ret, my_cards, next_cards, next_next_cards);
			}
			else{
				debug_log.write_log("will not lose game, pass accept.");
			}
		}
		//如果我不是庄家
		else{
			//如果庄家赢了,说明我输了
			if (winner == banker_pos){
				debug_log.write_log("will lose game, enter dealy follow mode.");
				deadly_follow_card(banker_pos, leadpos, lead, ret, my_cards, next_cards, next_next_cards);
			}
			else{
				debug_log.write_log("will not lose game, pass accept.");
			}
		}
	}
	else if (is_test && !can_grace_follow && deadly_follow){
		{
			csf_result tmp_csf;
			int test_weight = lead.weight_;
			if (lead.csf_val_ != csf_quadruple && lead.csf_val_ != csf_jet){
				test_weight = 0;
			}
			analyze_quadruple(my_cards, tmp_csf, 1, test_weight);
			if (tmp_csf.csf_val_ == csf_quadruple)	{
				ret = tmp_csf;
				return;
			}
		}

		//用大小王炸
		{
			csf_result tmp_csf;
			analyze_jet(my_cards, tmp_csf);
			if (ret.csf_val_ == csf_jet){
				ret = tmp_csf;
				return;
			}
		}
	}
	else if (can_grace_follow){
		ret = csf_sim;
	}
}

vector<ddz_card> ddz_card::generate()
{
	vector<ddz_card> ret;
	for (int i = 0; i < 54; i++)
	{
		ddz_card c = generate(i);
		ret.push_back(c);
	}
	return ret;
}

ddz_card ddz_card::generate(int card_id)
{
	ddz_card c;
	c.card_id_ = card_id;
	//小王
	if(card_id == 52){
		c.card_weight_ = 14;
		c.face_[0] = '*';
	}
	//大王
	else if (card_id == 53){
		c.card_weight_ = 15;
		c.face_[0] = '@';
	}
	else{
		c.card_weight_ = (card_id % 13) + 1;
		//如果是2
		if (c.card_weight_ == 2){
			c.card_weight_ = 13;
			c.face_[0] = '2';
		}
		else if (c.card_weight_ == 1){
			c.card_weight_ = 12;
			c.face_[0] = 'A';
		}
		else{
			c.card_weight_ -= 2;
			if (c.card_weight_ == 9){
				c.face_[0] = 'J';
			}
			else if (c.card_weight_ == 10){
				c.face_[0] = 'Q';
			}
			else if (c.card_weight_ == 11){
				c.face_[0] = 'K';
			}
			else
				_ltoa(c.card_weight_ + 2, c.face_, 10);
		}
	}
	return c;
}

ddz_card ddz_card::generate(std::string strc)
{
	std::string poker_number[] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K","*","@"};
	bool bret = std::any_of(poker_number,poker_number + 15,[&](const std::string &s)->bool{ return s == strc;});
	BOOST_ASSERT(bret);
	ddz_card c;
	strcpy(c.face_, strc.c_str());
	if (strc == "@"){
		c.card_weight_ = 15;
		c.card_id_ = 53;
	}
	else if (strc == "*"){
		c.card_weight_ = 14;
		c.card_id_ = 52;
	}
	else if (strc == "A"){
		c.card_weight_ = 12;
		c.card_id_ = 0;
	}
	else if (strc == "2"){
		c.card_weight_ = 13;
		c.card_id_ = 1;
	}
	else if (strc == "K"){
		c.card_weight_ = 11;
		c.card_id_ = 12;
	}
	else if (strc == "Q"){
		c.card_weight_ = 10;
		c.card_id_ = 11;
	}
	else if (strc == "J"){
		c.card_weight_ = 9;
		c.card_id_ = 10;
	}
	else if(strc == "10"){
		c.card_weight_ = 8;
		c.card_id_ = 9;
	}
	else {
		char a = strc[0];
		int w = 1 + (a - '3');
		c.card_weight_ = w;
		c.card_id_ = w + 1;
	}
	return c;
}

bool ddz_card::operator==(const ddz_card& other)
{
	return other.card_id_ == card_id_;
}

void ddz_card::reset_and_shuffle(vector<ddz_card>& vcards)
{
	random_shuffle(vcards.begin(), vcards.end());
}

int csf_result::min_weight()
{
	int m1 = 10000, m2 = -1;
	if (!main_cards_.empty()){
		m1 = main_cards_[0].card_weight_;
	}
	if (!attachments_.empty()){
		m2 = attachments_[0].card_weight_;
	}

	if (m1 > 11){
		return m1;
	}
	else {
		if (m2 == -1){
			return m1;
		}
		else
			return std::min<int>(m1, m2);
	}
}
