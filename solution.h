#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <stack>


#define LOG_CHECK(e) 	  \
if(!(e))								\
{												\
	goto EXIT0;						\
}												\


#define CHECK_BREAK(e)  \
if(!(e))								\
{												\
	break;								\
}												\



using namespace std;
class Solution {
public:
	//1. Two Sum
	vector<int> twoSum(vector<int> &numbers, int target)
	{
		 std::unordered_map<int, int> hash;// val idx
		 vector<int> result;
		 for (int i=0;i<numbers.size();i++)
		 {
			 int val = target - numbers[i];
			 if(hash.find(numbers[i]) != hash.end())
			 {
				 result.push_back(i);
				 result.push_back(hash[val]);
				 return result;
			 }
			 hash[val] = i;
		 }
	}
	//2. Add Two Numbers
	list<int> addTwoNumbers(list<int> a, list<int> b) 
	{
		a.reverse();
		b.reverse();
		int nMaxSize = std::max(a.size(),b.size());
		list<int> result(nMaxSize+1,0);
		for (int i=0;i<(nMaxSize-a.size());i++)
		{
			a.push_back(0);
		}
		for (int i=0;i<(nMaxSize - b.size());i++)
		{
			b.push_back(0);
		}

		int nSetp = 0;
		list<int>::iterator itr = result.begin();
		for (list<int>::iterator ita = a.begin(),itb = b.begin();
				ita != a.end()&& itb != b.end();++ita,++itb)
		{
			int nRest = (*ita + *itb + nSetp) % 10;
			*itr = nRest;
			nSetp = (*ita + *itb + nSetp) / 10;
			++itr;
		}
		*itr = nSetp;
		result.reverse();
		return result;
	}
	//3. Longest Substring Without Repeating Characters
	//map的作用 每个字母最后出现的位置
	int lengthOfLongestSubstring(std::string s) 
	{
		if (s.size() == 0) 
		{
			return 0;
		}
		map<char, int> map;
		int max=0;
		for (int i=0, j=0; i<s.length(); ++i){
			if (map.count(s[i]) != 0){
				j = std::max(j,map[s[i]]+1);
			}
			map[s[i]] = i;
			max = std::max(max,i-j+1);
		}
		return max;
	}
	//4. Median of Two Sorted Arrays
	// 1 3 5  2 4 6 7
	// 4 / 2 == 2   2-1 2 	  
	//	  
	double findMedianSortedArrays(vector<int>& nums1, vector<int>& nums2)
	{
		vector<int> vec(nums1.size() + nums2.size());
		int idx1=0,idx2 = 0;
		int idx12 = 0;
		while (idx1 < nums1.size() && idx2 < nums2.size())
		{
			vec[idx12++] = nums1[idx1] < nums2[idx2] ? nums1[idx1++] : nums2[idx2++];
		}
		while (idx1 < nums1.size())
		{
			vec[idx12++] = nums1[idx1++];
		}
		while (idx2 < nums2.size())
		{
			vec[idx12++] = nums2[idx2++];
		}

		if(vec.size() % 2 == 1)
		{
			return (double)vec[vec.size() / 2];
		}
		else
		{
			return ((double)vec[vec.size() / 2 - 1] + (double)vec[vec.size() / 2] ) / 2;
		}
	}

	bool isPalindrome(string s, int startIndex, int endIndex)
	{
		for(int i = startIndex, j = endIndex; i <= j; i++, j--)
		{
			if (s[i] != s[j]) 
			{
				return false;
			}
		}
		return true;
	}


	string longestPalindrome(string s) {
		if(s.size() < 2)
		{
			return s;
		}
		int max = 0;
		int start = 0;
		for (int i=0;i<s.size();i++)
		{
			for (int j=s.size() -1;j>=i;j--)
			{
				if(isPalindrome(s,i,j))
				{
					int _max = std::max(max,j-i+1);
					if(_max > max)
					{
						max = _max;
						start = i;
					}
					break;
				}
				else
				{
					continue;
				}
			}
		}
		return s.substr(start,max);
	}

	int reverse(int x) {
		long long res = 0;
		while(x) {
			res = res*10 + x%10;
			x /= 10;
		}
		return (res<INT_MIN || res>INT_MAX) ? 0 : res;
	}
	//123
	int myAtoi(string str) {
		int sign = 1;
		int base = 0;
		int i = 0;
		while (str[i] == ' ') 
		{ 
			i++; 
		}
		if (str[i] == '-' || str[i] == '+') 
		{
			sign = 1 - 2 * (str[i++] == '-'); 
		}
		while (str[i] >= '0' && str[i] <= '9')
		{
			if (base >  INT_MAX / 10 || (base == INT_MAX / 10 && str[i] - '0' > 7)) 
			{
				if (sign == 1) 
				{
					return INT_MAX;
				}
				else 
				{
					return INT_MIN;
				}
			}
			base  = 10 * base + (str[i++] - '0');
		}
		return base * sign;
	}

	int romanToInt(string s) {
		std::map<char,int> T;
		T['I'] = 1;
		T['V'] = 5;
		T['X'] = 10;
		T['L'] = 50;
		T['C'] = 100;
		T['D'] = 500;
		T['M'] = 1000;

		int sum = T[s.back()];
		for (int i = s.length() - 2; i >= 0; --i) 
		{
			if (T[s[i]] < T[s[i + 1]])
			{
				sum -= T[s[i]];
			}
			else
			{
				sum += T[s[i]];
			}
		}

		return sum;
	}

	//14. Longest Common Prefix
	string longestCommonPrefix(vector<string>& strs) {
		string prefix = "";
		for(int idx=0; strs.size()>0; prefix+=strs[0][idx], idx++)
			for(int i=0; i<strs.size(); i++)
				if(idx >= strs[i].size() ||(i > 0 && strs[i][idx] != strs[i-1][idx]))
					return prefix;
		return prefix;

	}
	//20. Valid Parentheses
	bool isValid(string s) {
		unsigned char T[256];
		T['('] = ')';
		T[')'] = '(';
		T['['] = ']';
		T[']'] = '[';
		T['{'] = '}';
		T['}'] = '{';
		std::stack<char> stack;
		for (int i=0;i<s.size();i++)
		{
			char ch = s[i];
			if('(' == ch || '[' == ch || '{' == ch)
			{
				stack.push(ch);
			}
			else
			{
				if(stack.top() == T[ch])
				{
					stack.pop();
				}
				else
				{
					return false;
				}
			}
		}
		return stack.size() == 0;
	}
	//26. Remove Duplicates from Sorted Array
	int removeDuplicates(vector<int>& nums){
		int count = 0;
		int n = nums.size();
		for(int i = 1; i < n; i++){
			if(nums[i] == nums[i-1]) 
			{
				count++;
			}
			else 
			{
				nums[i-count] = nums[i];
			}
		}
		return n - count;
	}
	//27. Remove Element
	int removeElement(vector<int>& nums, int val) {
// 		for (vector<int>::iterator it = nums.begin();
// 			it != nums.end();)
// 		{
// 			if(*it == val)
// 			{
// 				it = nums.erase(it);
// 			}
// 			else
// 			{
// 				++it;
// 			}
// 		}
// 		return nums.size();

		int cnt = 0;
		for(int i = 0 ; i < nums.size() ; ++i) {
			if(nums[i] == val)
				cnt++;
			else
				nums[i-cnt] = nums[i];
		}
		return nums.size()-cnt;
	}

	//28. Implement strStr()
	int strStr(string str, string substr) {
		for (int i = 0; i < str.size(); i++)
		{
			int count = 0;
			for (int j = 0; j < substr.size(); j++)
			{
				if(substr[j] == str[i+j])
				{
					count ++;
					if(count == substr.size())
					{
						return i;
					}
				}
			}
		}
		return -1;
	}

	//326. Power of Three
	bool isPowerOfThree(int n) {

	}

	//10. Regular Expression Matching
// 		'.' Matches any single character.
// 		'*' Matches zero or more of the preceding element.
// 
// 		The matching should cover the entire input string (not partial).
// 
// 		The function prototype should be:
// 		bool isMatch(const char *s, const char *p)
// 
// 		Some examples:
// 		isMatch("aa","a") → false
// 		isMatch("aa","aa") → true
// 		isMatch("aaa","aa") → false
// 		isMatch("aa", "a*") → true
// 		isMatch("aa", ".*") → true
// 		isMatch("ab", ".*") → true
// 		isMatch("aab", "c*a*b") → true
	bool isMatch(string s, string p) {
		if (p.empty())
		{
			return s.empty();
		}
		if ('*' == p[1])
		{
			// x* matches empty string or at least one character: x* -> xx*
			// *s is to ensure s is non-empty
			return (isMatch(s, p.substr(2)) || !s.empty() && (s[0] == p[0] || '.' == p[0]) && isMatch(s.substr(1), p));
		}
		else
		{
			return !s.empty() && (s[0] == p[0] || '.' == p[0]) && isMatch(s.substr(1), p.substr(1));
		}
			
	}

	//21. Merge Two Sorted Lists
	std::list<int> mergeTwoLists(std::list<int> l1, std::list<int> l2) {
		std::list<int> list;
		std::list<int>::iterator it = l1.begin(),it1 = l2.begin();
		for (;it != l1.end() && it1 != l2.end();)
		{
			if(*it < *it1)
			{
				list.push_back(*it);
				++it;
			}
			else
			{
				list.push_back(*it1);
				++it1;
			}
		}
		if(it != l1.end())
		{
			list.insert(list.end(),it,l1.end());
		}
		else
		{
			list.insert(list.end(),it1,l2.end());
		}
		return list;
	}
	

	//24. Swap Nodes in Pairs
	std::list<int> swapPairs(std::list<int> l) {
		for (std::list<int>::iterator it = l.begin();
			it != l.end();++it)
		{

		}
	}

	//58. Length of Last Word
	int lengthOfLastWord(string s) {
		int j = s.size() - 1;
		while(s[j] == ' ')
		{
			j--;
		}
		int i = j;
		while (s[i] != ' ')
		{
			i--;
		}
		return j - i;
	}

	//15. 3Sum
	vector<vector<int>> threeSum(vector<int>& num) {
// 		vector<vector<int>> vec;
// 		for (int i=0;i<num.size();i++)
// 		{
// 			for (int j=i+1;j<num.size();j++)
// 			{
// 				for (int k=j+1;k<num.size();k++)
// 				{
// 					if((num[i] + num[j] + num[k]) == 0)
// 					{
// 						std::vector<int> _vec;
// 						_vec.push_back(num[i]);
// 						_vec.push_back(num[j]);
// 						_vec.push_back(num[k]);
// 						vec.push_back(_vec);
// 					}
// 				}
// 			}
// 		}
//    return vec;
		vector<vector<int> > ans;
		int i, j, k, n = num.size();
		sort(num.begin(), num.begin() + n);
		for (i = 0; i < n; i++){
			if (i > 0 && num[i] == num[i - 1]) continue;
			k = n - 1;
			j = i + 1;
			while (j < k){
				if (num[i] + num[j] + num[k] > 0) k--;
				else if (num[i] + num[j] + num[k] < 0) j++;
				else{
					vector<int> tmp;
					tmp.push_back(num[i]);
					tmp.push_back(num[j]);
					tmp.push_back(num[k]);
					ans.push_back(tmp);
					while (j < k && num[k] == num[k - 1]) k--;
					while (j < k && num[j] == num[j + 1]) j++;
					k--; j++;
				}
			}
		}
		return ans;
	}




};