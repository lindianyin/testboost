﻿// test_asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "net.h"
#include <exception>
#include <algorithm> 

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/assert.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/function.hpp>
#include <boost/log/trivial.hpp>

#include <boost/format.hpp>

#include <boost/algorithm/string.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/typeof/typeof.hpp>

#include <boost/any.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/variant.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/assert.hpp>

#include <unordered_map>
#include <unordered_set>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/unordered_map.hpp>
#include <boost/assign.hpp>

#include <boost/container/string.hpp>


#include <list>




#include "solution.h"
//#include "poker.h"
#include "threadpool.h"

#include "boolinq/boolinq.h"

using namespace std;
using namespace boost;


#include "singleton.h"


template<typename K, typename V>
inline bool TryGetValue(std::map<K,V>& m,K& k,V& v)
{
	bool bRet = false;
	std::map<K,V>::iterator it = m.find(k);
	if(it != m.end())
	{
		bRet = true;
		v = it->second;
	}
	return bRet;
}

std::string GetSameBirthDayProbility(const int nNum)
{
	// 1 -      (365 * (365 -1) * (365 -n +1 )) / 365 * 365 * ... 
	boost::multiprecision::cpp_dec_float_100 a(1);
	for (int i=0;i<nNum;i++)
	{
		a *= 365;
	}
	boost::multiprecision::cpp_dec_float_100 b(1);
	for (int i=365;i >= (365- nNum + 1);i--)
	{
		b *= i;
	}
	std::string str = boost::lexical_cast<std::string>( 1 - b / a);
	return str;
}




int main(int argc, char * argv[])
{
	BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
	BOOST_LOG_TRIVIAL(fatal) << "An error severity message";

	//boost::shared_ptr<DbMgr> dbMgr = DbMgr::GetInstance();

	std::map<bool,std::shared_ptr<std::vector<int>>> group;
	auto f = [](int a) -> bool { return a % 2 == 0 ;};
	std::array<int,5> arr = {1,2,3,4,5};
	for (auto e : arr)
	{
		bool bRet = f(e);
		std::map<bool,std::shared_ptr<std::vector<int>>>::iterator it = group.find(bRet);
		if(it != group.end())
		{
			if(NULL == it->second)
			{
				std::shared_ptr<std::vector<int>> _spVec(new std::vector<int>());
				group[bRet] = _spVec;
			}
			it->second->push_back(e);
		}
		else
		{
			std::shared_ptr<std::vector<int>> _spVec(new std::vector<int>());
			_spVec->push_back(e);
			group[bRet] = _spVec;
		}

	}

	std::unordered_map<int,std::string> map;

	
	boost::variant<int,double,bool> v;
	v = false;

	bool b = boost::get<bool>(v);

	std::list<int> list;
	list.insert(list.begin(),0);
	list.insert(list.begin(),1);
	list.insert(list.begin(),2);

	{
		clock_t t1 = std::clock();
		Solution s;
		int arr[] = {2, 7, 11, 15,1,8};
		std::vector<int> vec(arr,arr+(sizeof(arr)/sizeof(arr[0])));
		//std::vector<int> vecRet = s.twoSum(vec,9);
		std::vector<int> vecRet = s.twoSum(vec,9);
		clock_t t2 = std::clock();
		std::cout << (t2 - t1) << std::endl;
	}
	{
		Solution s;
		int a[] = {2,4,3};
		int b[] = {5,6,4};
		std::list<int>		la(a,a+3);
		std::list<int>		lb(b,b+3);
		std::list<int> lc = s.addTwoNumbers(la,lb);

		int nRet = s.lengthOfLongestSubstring("abcabc");

	}

	{
		Solution s;
		int a[] = {1,3,5};
		int b[] = {2,4,6,7};
		// 1 2 ,3 4,5,6,7
		std::vector<int>		la(a,a+ sizeof(a) / sizeof(a[0]));
		std::vector<int>		lb(b,b + sizeof(b) / sizeof(b[0]));
		double dRet = s.findMedianSortedArrays(la,lb);
	}

	{
		Solution s;
		std::string str = "dabbaa";
		std::string strRet  = s.longestPalindrome(str);
	}

	{
		Solution s;
		int nRet   = s.reverse(-123);
	}

	{
		Solution s;
		int nRet   = s.myAtoi("123");
	}

	{
		Solution s;
		int nRet   = s.romanToInt("IV");
	}

	{
		Solution s;
		std::vector<std::string> vec;
		vec.push_back("hello");
		vec.push_back("hell");
		vec.push_back("hellow");

		std::string str  = s.longestCommonPrefix(vec);


	}

	{
		Solution s;
		std::string str = "[{()}]";
		bool bRet  = s.isValid(str);
	}


	{
		Solution s;
		int arr[5] = {1,1,2,2,3};
		std::vector<int> vec(arr,arr+5);
		int count  = s.removeDuplicates(vec);
	}


	{
		Solution s;
		int arr[5] = {1,1,2,2,3};
		std::vector<int> vec(arr,arr+5);
		int count  = s.removeElement(vec,2);
	}

	{
		Solution s;
		int idx  = s.strStr("abcde","cd");

	}
	//boost::shared_ptr<thread_pool> th(new thread_pool());
	//th->init(3);
// 	th->add();
// 	th->add();
// 	th->add();
// 	th->add();
// 	th->join();

	{
		std::vector<int> vec;
		boost::assign::push_back(vec) (1)(2)(3);
		std::map<int,int> map;
		boost::assign::insert(map) (1,1)(2,2)(3,3);

		std::vector<int> vec1 = boost::assign::list_of(1)(2)(3);

		std::map<int,int> map1 = boost::assign::map_list_of
			(1,2)
			(3,4)
			(1,2)
			(1,2)
			(3,4);
		std::set<int> set = boost::assign::list_of(1)(2)(3);

		int k = 1;
		int v = 0;
		bool bRet = TryGetValue(map1,k,v);

		const stack<string> names = boost::assign::list_of( "Mr. Foo" )( "Mr. Bar")( "Mrs. FooBar" ).to_adapter();



	}
	
	{
		int agg_int[] = {1,2,3,9,9,4,5,6,7,10};
		int size = sizeof(agg_int) / sizeof(agg_int[0]);
		std::sort(agg_int,agg_int + size,std::less<int>());
		std::map<int,int> ret;
		for (int i=0;i<size;i++)
		{
			if(ret.count(agg_int[i]) == 0)
			{
				ret[agg_int[i]] = 1;
			}
			else
			{
				ret[agg_int[i]]++;
			}
		}
		std::vector<int> keys;
		std::vector<int> values;
		for (std::map<int,int>::iterator it = ret.begin();it != ret.end();it++)
		{
			keys.push_back(it->first);
			values.push_back(it->second);
		}
		std::vector<int> vec(agg_int,agg_int+size);
		std::vector<int>::iterator it = std::unique(vec.begin(),vec.end(),[](int a,int b)->bool{ return ((a%2) - (b % 2)) == 0;});
		vec.resize(std::distance(vec.begin(),it));


	}

	{
		Solution s;
		std::list<int> l1 = boost::assign::list_of(1)(3)(4)(9);
		std::list<int> l2 = boost::assign::list_of(2)(3)(5)(7)(10);
		std::list<int> l3 = s.mergeTwoLists(l1,l2);


	}

	{
		Solution s;					
		bool bRet = s.isMatch("aa","a");			
		bRet = 	s.isMatch("aa","aa");					
		bRet = 	s.isMatch("aaa","aa");							
		bRet = 	s.isMatch("aa", "a*");							
		bRet = 	s.isMatch("aa", ".*");							
		bRet = 	s.isMatch("ab", ".*");							
		bRet = 	s.isMatch("aab", "c*a*b");		 		

	}



	{
		Solution s;
		std::vector<int> vec = boost::assign::list_of(-1)(0)(1)(2)(-1)(-4);
		std::vector<std::vector<int>> vec1 = s.threeSum(vec); 		
	}

	{
		Solution s;
		std::vector<int> vec = boost::assign::list_of(-1)(2)(1)(-4);
		int nRet = s.threeSumClosest(vec,1); 		
	}

	{
		Solution s;
		std::vector<std::string> vec = s.letterCombinations("23");
	}

	{
		Solution s;
		std::vector<std::string> vec = s.generateParenthesis(3);
	}

	{
		Solution s;
		std::vector<int> vec = boost::assign::list_of(4)(5)(1)(2)(3);
		bool bRet = s.increasingTriplet(vec);
	}



	boost::asio::io_service io;
	server s1(io,10241);
	while(true)
	{
		io.poll_one();
	}

	return 0;
}

