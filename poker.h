#include <algorithm>
#include <boost/assert.hpp>

#define N (1)

#define SMALL_JOKER (52)
#define BIG_JOKER	(53)

//
enum emPokerType
{
	emPokerTypeNull,
	emPokerTypeSingle,
	emPokerTypeMutiSingle,
	emPokerTypePair,
	emPokerTypeMutiPair,
	emPokerTypeThree,
	emPokerTypeThreeOne,
	emPokerTypeThreePair,
	emPokerTypeMutiThree,
	emPokerTypeMutiThreeOne,
	emPokerTypeMutiThreePair,
	emPokerTypeFour,
	emPokerTypeBigest,
};



class poker
{
public:
	poker()
	{
		for (int i=0;i< N;i++)
		{
			for (int j=0;j<54;j++)
			{
				deck[j + 54 * (N - 1)] = j;
			}
		}

	}

	static int GetWeight(const int nNum)
	{
		int nIdx = -1;
		int _weight[] = {2,3,4,5,6,7,8,9,10,11,12,0,1,13,14};
		int _nLen = sizeof(_weight)/sizeof(_weight[0]);
		if(SMALL_JOKER == nNum)
		{
			nIdx = _nLen - 1  -1;
		}
		else if(BIG_JOKER == nNum)
		{
			nIdx =  _nLen - 1 ;
		}
		else
		{
			for (int i=0;i< _nLen; i++)
			{
				if(_weight[i] == (nNum % 13))
				{
					nIdx = i;
					break;
				}
			}
		}
		BOOST_ASSERT(-1 != nIdx);
		return nIdx;
	}

	static int Sort(const int *pPoker,const int nCount)
	{
		 std::sort(pPoker,pPoker + nCount,[](int a,int b)->bool{ return poker::GetWeight(a) < poker::GetWeight(b);});
	}


	static int GetType(const int *pPoker,const int nCount)
	{
		
		emPokerType emType = emPokerTypeNull;
		if(1 == nCount )
		{
			emType = emPokerTypeSingle;
		}
		else if(nCount > 5)
		{

		}
	}
private:
	int deck[54 * N];
};