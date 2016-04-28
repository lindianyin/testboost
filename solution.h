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



//using namespace std;
class Solution {
public:
	//1. Two Sum
	std::vector<int> twoSum(std::vector<int> &numbers, int target)
	{
		 std::unordered_map<int, int> hash;// val idx
		 std::vector<int> result;
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
	std::list<int> addTwoNumbers(std::list<int> a, std::list<int> b) 
	{
		a.reverse();
		b.reverse();
		int nMaxSize = std::max(a.size(),b.size());
		std::list<int> result(nMaxSize+1,0);
		for (int i=0;i<(nMaxSize-a.size());i++)
		{
			a.push_back(0);
		}
		for (int i=0;i<(nMaxSize - b.size());i++)
		{
			b.push_back(0);
		}

		int nSetp = 0;
		std::list<int>::iterator itr = result.begin();
		for (std::list<int>::iterator ita = a.begin(),itb = b.begin();
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
		std::map<char, int> map;
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
	double findMedianSortedArrays(std::vector<int>& nums1, std::vector<int>& nums2)
	{
		std::vector<int> vec(nums1.size() + nums2.size());
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

	bool isPalindrome(std::string s, int startIndex, int endIndex)
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


	std::string longestPalindrome(std::string s) {
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
	int myAtoi(std::string str) {
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

	int romanToInt(std::string s) {
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
	std::string longestCommonPrefix(std::vector<std::string>& strs) {
		std::string prefix = "";
		for(int idx=0; strs.size()>0; prefix+=strs[0][idx], idx++)
			for(int i=0; i<strs.size(); i++)
				if(idx >= strs[i].size() ||(i > 0 && strs[i][idx] != strs[i-1][idx]))
					return prefix;
		return prefix;

	}
	//20. Valid Parentheses
	bool isValid(std::string s) {
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
	int removeDuplicates(std::vector<int>& nums){
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
	int removeElement(std::vector<int>& nums, int val) {
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
	int strStr(std::string str, std::string substr) {
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
	bool isMatch(std::string s, std::string p) {
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
	int lengthOfLastWord(std::string s) {
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
	std::vector<std::vector<int>> threeSum(std::vector<int>& num) {
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
		std::vector<std::vector<int> > ans;
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
					std::vector<int> tmp;
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

	//4Sum
	std::vector<std::vector<int>> fourSum(std::vector<int>& nums, int target) {

	}



	//16. 3Sum Closest
	int threeSumClosest(std::vector<int>& num, int target) {
		int ans = 0;
		int i, j, k, n = num.size();
		std::sort(num.begin(), num.begin() + n);
		ans = num[0] + num[1] + num[2];
		for (i = 0; i < n; i++){
			if (i > 0 && num[i] == num[i - 1]) continue;
			k = n - 1;
			j = i + 1;
			while (j < k){
				int sum = num[i] + num[j] + num[k];
				if (std::abs(target - ans) > abs(target - sum)) {
					ans = sum;
					if (ans == target) return ans;
				}
				(sum > target) ? k-- : j++;
			}
		}
		return ans;
	}

	//17. Letter Combinations of a Phone Number
	std::vector<std::string> letterCombinations(std::string digits) {
		std::vector<std::string> result;
		if(digits.empty()) return std::vector<std::string>();
		static const std::vector<std::string> v = 
		boost::assign::list_of("")("")("abc")("def")("ghi")("jkl")("mno")("pqrs")("tuv")("wxyz");
		result.push_back("");   // add a seed for the initial case
		for(int i = 0 ; i < digits.size(); ++i) {
			int num = digits[i]-'0';
			if(num < 0 || num > 9) break;
			const std::string& candidate = v[num];
			if(candidate.empty()) continue;
			std::vector<std::string> tmp;
			for(int j = 0 ; j < candidate.size() ; ++j) {
				for(int k = 0 ; k < result.size() ; ++k) {
					tmp.push_back(result[k] + candidate[j]);
				}
			}
			result.swap(tmp);
		}
		return result;
	}

	//22. Generate Parentheses
	std::vector<std::string> generateParenthesis(int n) {
		std::vector<std::string> res;
		addingpar(res, "", n, 0);
		return res;
	}
	void addingpar(std::vector<std::string> &v, std::string str, int n, int m){
		if(n==0 && m==0) {
			v.push_back(str);
			return;
		}
		if(m > 0){ addingpar(v, str+")", n, m-1); }
		if(n > 0){ addingpar(v, str+"(", n-1, m+1); }
	}


	//334. Increasing Triplet Subsequence
	bool increasingTriplet(std::vector<int>& nums) {
// 		int n = x.size();
// 		for (int i=0;i<n;i++)
// 		{
// 			for (int j=i+1;j<n;j++)
// 			{
// 				for (int k=j+1;k<n;k++)
// 				{
// 					if(x[i] < x[j] && x[j] < x[k])
// 					{
// 						return true;
// 					}
// 				}
// 			}
// 		}
// 		return false;

		int c1 = INT_MAX, c2 = INT_MAX;
		for (int x : nums) {
			if (x <= c1) {
				c1 = x;           // c1 is min seen so far (it's a candidate for 1st element)
			} else if (x <= c2) { // here when x > c1, i.e. x might be either c2 or c3
				c2 = x;           // x is better than the current c2, store it
			} else {              // here when we have/had c1 < c2 already and x > c2
				return true;      // the increasing subsequence of 3 elements exists
			}
		}
		return false;


	}

	std::vector<std::vector<int> > permute(std::vector<int> &num) {
		std::vector<std::vector<int> > result;
		permuteRecursive(num, 0, result);
		return result;
	}

	// permute num[begin..end]
	// invariant: num[0..begin-1] have been fixed/permuted
	void permuteRecursive(std::vector<int> &num, int begin, std::vector<std::vector<int> > &result)    {
		if (begin >= num.size()) {
			// one permutation instance
			result.push_back(num);
			return;
		}

		for (int i = begin; i < num.size(); i++) {
			std::swap(num[begin], num[i]);
			permuteRecursive(num, begin + 1, result);
			// reset
			std::swap(num[begin], num[i]);
		}
	}

	int fib(int n){
		if(n == 1 || n == 2){
			return 1;
		}else{
			return fac(n-1) + fac(n - 2);
		}
	}
	int fac(int n)
	{
		if(n == 1){
			return 1;
		}else{
			return n * fac(n-1);
		}
	}

	void insert_sort(std::vector<int>& vec){
		for (int j= 1;j< vec.size();j++)
		{
			int key = vec[j];
			int i = j-1;
			while(i > 0 && vec[i] > key){
				vec[i+1] = vec[i];
				i = i -1;
			}
			vec[i+1] = key;
		}
	}
	void Merge(std::vector<int>& a,int p,int q,int r){
		int n1 = q-p+1;  
		int n2 = r-q;  
		int *L = new int[n1+1];  
		int *R = new int[n2+1];  
		int i, j, k;  

		for (i=0; i<n1; i++){  
			L[i] = a[p+i];  
		}  
		for (j=0; j<n2; j++){  
			R[j] = a[q+j+1];  
		}  
		L[n1] = INT_MAX;  
		R[n2] = INT_MAX;  

		for (i=0, j=0, k=p; k<=r; k++) {  
			if (L[i]<=R[j]){  
				a[k] = L[i];  
				i++;  
			}else{  
				a[k] = R[j];  
				j++;  
			}  
		}  

		delete []L;  
		delete []R; 
	}
	void MergeSort1(std::vector<int>& a, int p, int r)  
	{  
		if (p<r)  
		{  
			int q = (p+r)/2;  
			MergeSort1(a, p, q);  
			MergeSort1(a, q+1, r);  
			Merge(a, p, q, r);  
		}  
	}
	//55. Jump Game 贪心
	bool canJump(std::vector<int>& nums) {
		int n=nums.size(),reachablesofar=0;
		for(int i=0;i<n;i++){
			if(reachablesofar<i) return false;
			reachablesofar=std::max(reachablesofar, i+nums[i]);
			if(reachablesofar>=n-1) return true;
		}
	}


	//45. Jump Game II
	int jump(std::vector<int>& nums) {
		int n = nums.size(), step = 0, start = 0, end = 0;
		while (end < n - 1) {
			step++; 
			int maxend = end + 1;
			for (int i = start; i <= end; i++) {
				if (i + nums[i] >= n - 1) return step;
				maxend = std::max(maxend, i + nums[i]);
			}
			start = end + 1;
			end = maxend;
		}
		return step;
	}
	//343. Integer Break 
	int integerBreak(int n) {
    if(n == 2) return 1;
    if(n == 3) return 2;
    std::vector<int> dp(n+1, 0);
    dp[2] = 2;
    dp[3] = 3;
    for(int i = 4; i <= n; i++){
        dp[i] = std::max(dp[i-2] * 2, dp[i-3] * 3);
    }
    return dp[n];
   }
//爬楼梯问题
//一个人每次只能走一层楼梯或者两层楼梯，问走到第80层楼梯一共有多少种方法。
//设DP[i]为走到第i层一共有多少种方法，那么DP[80]即为所求。很显然DP[1]=1, DP[2]=2（走到第一层只有一种方法：就是走一层楼梯；走到第二层有两种方法：走两次一层楼梯或者走一次两层楼梯）。同理，走到第i层楼梯，可以从i-1层走一层，或者从i-2走两层。很容易得到：
//递推公式：DP[i]=DP[i-1]+DP[i-2]
//边界条件：DP[1]=1   DP[2]=2

	long long DP(int n)  
	{
		static long long dp[81] = {0};
		if(dp[n])  
				return dp[n];  
		if(n == 1)  
				return 1;  
		if(n == 2)  
				return 2;  
		dp[n] = DP(n-1) + DP(n-2);  
		return dp[n];     
	}  
	//322. Coin Change
	int coinChange(std::vector<int>& coins, int amount) {
		      
  }

	int fibb(int n){
		if(0 == n || 1 == n){
			return 1;
		}
		return fibb(n -1) + fibb(n - 2);
	}

	int fibb1(int n){
		static std::map<int,int> _map;
		if(_map.count(n) == 0){
			_map[n] = fibb1(n-1) + fibb1(n-2);
		}
		return _map[n];
	}

	void calc(/*std::vector<int> &vec,std::vector<int> res*/){
		boost::mt19937 rng;
		boost::uniform_int<int> uniint(0,99);
		int nRand = uniint(rng);
	}

	//xorshift 生成器
	uint32_t rand(){
		static uint32_t x =1, y=2, z = 3, w = 4;
		uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
	}
	
	//使用nullptr的作用就是这样的
	void overload(char *p){
		std::cout << "char *" << std::endl;
	}

	void overload(int p){
		std::cout << "ssss" << std::endl;

	}

	int numSquares(int n) 
  {
      if (n <= 0)
      {
          return 0;
      }

      // cntPerfectSquares[i] = the least number of perfect square numbers 
      // which sum to i. Note that cntPerfectSquares[0] is 0.
      std::vector<int> cntPerfectSquares(n + 1, INT_MAX);
      cntPerfectSquares[0] = 0;
      for (int i = 1; i <= n; i++)
      {
          // For each i, it must be the sum of some number (i - j*j) and 
          // a perfect square number (j*j).
          for (int j = 1; j*j <= i; j++)
          {
              cntPerfectSquares[i] = 
                  std::min(cntPerfectSquares[i], cntPerfectSquares[i - j*j] + 1);
          }
      }

      return cntPerfectSquares.back();
  }





};