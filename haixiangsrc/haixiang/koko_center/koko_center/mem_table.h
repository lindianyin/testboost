//内存表实现
#pragma once
#include <string>
#include <unordered_map>
#include <map>

struct variant
{
	char	type_;
	union 
	{
		double  value_;			
		char		str_value_[64];
		char*		big_data_;	//大数据存这里
	};
};

typedef std::unordered_map<std::string, variant> row_t;
typedef std::unordered_map<std::string, row_t>	
