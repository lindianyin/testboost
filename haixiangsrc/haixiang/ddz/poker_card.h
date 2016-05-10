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
	int		card_id_;				//�����С, 0-51(), 52С����53������
	int		card_weight_;		//�����С, 3-K(1-11), A(12), 2(13) С��(14)������(15)
};


//ţţ��
class		ddz_card : public poker_card
{
public:
	//����һ�������
	static vector<ddz_card>		generate();
	static void					reset_and_shuffle(vector<ddz_card>& vcards);
	static ddz_card				generate(int card_id);
	static ddz_card				generate(std::string c);
	bool	operator == (const ddz_card& other);
};

//���ʹ�С��������,�򻯺�����߼�
enum card_stand_for
{
	csf_not_allowed,			//�����������

	csf_single,						//����
	csf_multi_single,			//����(˳��)

	csf_dual,							//����
	csf_multi_dual,				//����

	csf_triple,						//������(���ܴ���,����������)
	csf_triple_dual,			//������(����������)

	csf_quadruple_single,	//���Ŵ���(���2��),
	csf_quadruple_dual,		//���Ŵ���(���2��)

	csf_quadruple,				//����(ը��)
	csf_jet,							//��С��ը��
};

struct csf_result
{
	int			csf_val_;						//����ֵ
	int			main_count_;				//��������
	int			attachment_count_;	//������
	int			weight_;						//����Ȩ��ֵ,��һ�����е���С������������Ƶ�Ȩֵ
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
									std::vector<ddz_card>& next_cards = std::vector<ddz_card>(),	//����1��
									std::vector<ddz_card>& next_next_cards = std::vector<ddz_card>(),//����2����
									bool deadly_follow = 0,  //��������(���ƴ��۸���)
									bool is_test = false);

int		lead_card(int banker_pos,
								csf_result& to_play,											//��Ҫ������
								std::vector<ddz_card>& my_cards,					//�����ϵ���
								std::vector<ddz_card>& next_cards,				//����1���ϵ���
								std::vector<ddz_card>& next_next_cards,		//����2���ϵ���
								bool is_test = false);

bool	will_lose(int		banker_pos,
								int	leadpos,
								bool	isdeadly,
								csf_result& toplay, 
								std::vector<ddz_card>& my_cards,			//�����ϵ���
								std::vector<ddz_card>& next_cards,		//����1��
								std::vector<ddz_card>& next_next_cards//����2����
							 );

csf_result	analyze_csf(std::vector<ddz_card>& plan);
int is_greater_pattern(csf_result& plan, csf_result& refer);
int		next_same(int pos, std::vector<ddz_card>& thisp, int weight);
bool	should_pass_for_alliance(csf_result& current_csf_);
//�����Ƿ�Ҫ��ׯ
int		evalue_score_to_be_banker(std::vector<ddz_card>& mycards);
std::vector<ddz_card>	remove_cards(std::vector<ddz_card>& my_cards, csf_result& to_remove);
