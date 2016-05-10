#pragma once
#include "boost/lexical_cast.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "boost/uuid/random_generator.hpp"
#include "boost/thread/locks.hpp"
#include "boost/random.hpp"

#include <vector>
#include <string>

using namespace std;
template<class T>
std::string lex_cast_to_str( T to_cast )
{
	static boost::mutex mutx_;
	boost::lock_guard<boost::mutex> l(mutx_);
	return boost::lexical_cast<std::string>(to_cast);
}

//a必须为数组
#define COPY_STR(a, b)\
	strncpy_s(a, sizeof(a), b, _TRUNCATE)

static std::string get_guid()
{
	static boost::mutex mutx_;
	boost::lock_guard<boost::mutex> l(mutx_);
	boost::uuids::random_generator uuid_gen;
	boost::uuids::uuid uuid_(uuid_gen());
	return to_string(uuid_);
}
//随机出a <= x <= b的数

static unsigned int rand_r(unsigned int a, unsigned int b)
{
	static boost::mt19937 gen((unsigned int)time(NULL));                                     
	boost::uniform_int<unsigned int>dist(a, b);
	boost::variate_generator<boost::mt19937&,boost::uniform_int<unsigned int>>die(gen,dist);
	return die();
}

static unsigned int rand_r(unsigned int a)
{
	return rand_r(0, a);
}

template<class map_t, class key_t, class value_t>
void replace_map_v(map_t& m, std::pair<key_t, value_t>& val)
{
	auto it = m.find(val.first);
	if (it == m.end()){
		m.insert(val);
	}
	else{
		it->second = val.second;
	}
}

template<class T >
static bool		split_str(const char* src, unsigned int search_len, const char * sep, vector<T>& res, bool strong_match)
{
	if (search_len == 0) return true;

	char* start = (char*)src;
	char* last_pos = (char*)src;
	while (char* curpos = strstr(last_pos, sep))
	{
		string t = last_pos;
		t.erase(t.begin() + (curpos - last_pos), t.end());
		try	{
			res.push_back(boost::lexical_cast<T>(t));
		}
		catch (boost::bad_lexical_cast e){
			return false;
		}
		last_pos = curpos + 1;
		unsigned int tl = last_pos > start ? (last_pos - start) : 0;
		if(tl >= search_len)
			break;
	}
	if (!strong_match){
		T t = boost::lexical_cast<T>(last_pos);
		res.push_back(t);
	}
	return true;
}

template<>
static bool split_str<std::string>(const char* src, unsigned int search_len, const char * sep, vector<std::string>& res, bool strong_match)
{
	char* start = (char*)src;
	char* last_pos = (char*)src;
	while (char* curpos = strstr(last_pos, sep))
	{
		std::string t;
		t.append(last_pos, (curpos - last_pos));
		res.push_back(t);
		last_pos = curpos + 1;
		unsigned int tl = last_pos > start ? (last_pos - start) : 0;
		if(tl >= search_len)
			break;
	}
	if (!strong_match){
		std::string t;
		t.append(last_pos, search_len - (last_pos - src));
		res.push_back(t);
	}
	return true;
}

template<class T>
string		combin_str( T* to_combin, int count)
{
	string ret;
	for (int i = 0; i < count; i++)
	{
		ret +=boost::lexical_cast<string, T>(to_combin[i]) + ",";
	}
	return ret;
}

template<class T>
string		combin_str(vector<T> to_combin, string sep = ",", bool expand_end = true)
{
	string ret;
	for (unsigned int i = 0; i < to_combin.size(); i++)
	{
		if (i == (to_combin.size() - 1)){
			if (expand_end){
				ret += boost::lexical_cast<string, T>(to_combin[i]) + sep;
			}
			else
				ret +=boost::lexical_cast<string, T>(to_combin[i]);
		}
		else
			ret +=boost::lexical_cast<string, T>(to_combin[i]) + sep;
	}
	return ret;
}


