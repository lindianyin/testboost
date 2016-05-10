#include "boost/interprocess/managed_shared_memory.hpp"
#include "boost/interprocess/sync/named_mutex.hpp"
#include "boost/interprocess/containers/map.hpp"
#include "boost/interprocess/containers/string.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <string>
using namespace boost;

//模板参数不能是指针, key 和value都必须是简单数据类型合集
template<class value_type>
class shared_object
{
public:
	shared_object(std::string identify, unsigned int element_count)
	{
		identify_ = identify;
		element_count_ = element_count;
	}
	
	bool		open_shared_object()
	{
		try{
			shm_.reset(new interprocess::managed_shared_memory(interprocess::open_or_create, identify_.c_str(), element_count_ * sizeof(value_type)));
			alloc_inst_.reset(new void_allocator(shm_->get_segment_manager()));
			container_ = shm_->find_or_construct<map_type>(identify_.c_str())(std::less<char_string>(), *alloc_inst_);
		}
		catch(...){
			return false;
		}
		return true;
	}
	
	void		set(std::string key, value_type val)
	{
		char_string ckey(*alloc_inst_);
		ckey = key.c_str();
		auto itf = container_->find(ckey);
		if (itf == container_->end()){
			container_->insert(std::make_pair(ckey, val));
		}
		else{
			itf->second = val;
		}
	}

	void		remove(std::string key)
	{
		char_string ckey(*alloc_inst_);
		ckey = key.c_str();
		auto itf = container_->find(ckey);
		if (itf != container_->end()){
			container_->erase(itf);
		}
	}

	bool		get(std::string key, value_type& ret)
	{
		char_string ckey(*alloc_inst_);
		ckey = key.c_str();
		auto itf = container_->find(ckey);
		if (itf != container_->end()){
			ret = itf->second;
			return true;
		}
		return false;
	}

	template<class iterator_func>
	void		visit(iterator_func itfunc)
	{
		auto itf = container_->begin();
		while (itf != container_->end()){
			char_string ckey = itf->first; 
			std::string key = ckey.c_str();
			value_type& val = itf->second;
			itfunc(key, val);
			itf++;
		}
	}
	
	void		clean()
	{
		container_->clear();
	}

private:
	typedef interprocess::managed_shared_memory::segment_manager	segment_manager_t;

	typedef interprocess::allocator<char, segment_manager_t> char_allocator;
	typedef interprocess::basic_string<char, std::char_traits<char>, char_allocator> char_string;

	typedef	interprocess::allocator<std::pair<const char_string, value_type>, segment_manager_t> map_allocator;
	typedef interprocess::map<char_string, value_type, std::less<char_string>, map_allocator> map_type;

	typedef interprocess::allocator<void, segment_manager_t>  void_allocator;

	std::string		identify_;
	unsigned int	element_count_;

	boost::shared_ptr<interprocess::managed_shared_memory> shm_;
	boost::shared_ptr<void_allocator>							alloc_inst_;
	map_type*	container_;
};

