//�ڴ��ʵ��
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
		char*		big_data_;	//�����ݴ�����
	};
};

typedef std::unordered_map<std::string, variant> row_t;
typedef std::unordered_map<std::string, row_t>;