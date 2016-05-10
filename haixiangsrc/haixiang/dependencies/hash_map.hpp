//
// detail/hash_map.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Modify by hjt, make it more convenience to use

#ifndef BOOST_ASIO_DETAIL_HASH_MAP_HPP1
#define BOOST_ASIO_DETAIL_HASH_MAP_HPP1

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)
#include <boost/noncopyable.hpp>
#include <cassert>
#include <list>
#include <utility>
#include <string>
using namespace std;

template<class K> extern	std::size_t calculate_hash_value (K i);
template<>				extern	std::size_t calculate_hash_value<std::string>(std::string i);

template <typename K, typename V>
class hash_map
  : private boost::noncopyable
{
public:
  // The type of a value in the map.
  typedef std::pair<K, V> value_type;

  // The type of a non-const iterator over the hash map.
  typedef typename std::list<value_type>::iterator iterator;

  // The type of a const iterator over the hash map.
  typedef typename std::list<value_type>::const_iterator const_iterator;
  inline std::size_t calculate_hash_value(K i)
  {
		return ::calculate_hash_value(i);
  }

  // Constructor.
  hash_map()
    : size_(0),
      buckets_(0),
      num_buckets_(0)
  {
  }

  // Destructor.
  ~hash_map()
  {
    delete[] buckets_;
  }

  // Get an iterator for the beginning of the map.
  iterator begin()
  {
    return values_.begin();
  }

  // Get an iterator for the beginning of the map.
  const_iterator begin() const
  {
    return values_.begin();
  }

  // Get an iterator for the end of the map.
  iterator end()
  {
    return values_.end();
  }

  // Get an iterator for the end of the map.
  const_iterator end() const
  {
    return values_.end();
  }

  // Check whether the map is empty.
  bool empty() const
  {
    return values_.empty();
  }
  std::size_t size() const
  {
	  return size_;
  }

	V&	operator [](const K& k)
	{
		iterator it = find(k);
		if (it != end()){
			return it->second;
		}
		else{
			V v;
			insert(std::make_pair(k, v));
			return find(k)->second;
		}
	}

  // Find an entry in the map.
  iterator find(const K& k)
  {
    if (num_buckets_)
    {
      size_t bucket = calculate_hash_value(k) % num_buckets_;
      iterator it = buckets_[bucket].first;
      if (it == values_.end())
        return values_.end();
      iterator end = buckets_[bucket].last;
      ++end;
      while (it != end)
      {
        if (it->first == k)
          return it;
        ++it;
      }
    }
    return values_.end();
  }

  // Find an entry in the map.
  const_iterator find(const K& k) const
  {
    if (num_buckets_)
    {
      size_t bucket = calculate_hash_value(k) % num_buckets_;
      const_iterator it = buckets_[bucket].first;
      if (it == values_.end())
        return it;
      const_iterator end = buckets_[bucket].last;
      ++end;
      while (it != end)
      {
        if (it->first == k)
          return it;
        ++it;
      }
    }
    return values_.end();
  }

  // Insert a new entry into the map.
  std::pair<iterator, bool> insert(const value_type& v)
  {
    if (size_ + 1 >= num_buckets_)
      rehash(hash_size(size_ + 1));
    size_t bucket = calculate_hash_value(v.first) % num_buckets_;
    iterator it = buckets_[bucket].first;
    if (it == values_.end())
    {
      buckets_[bucket].first = buckets_[bucket].last =
        values_insert(values_.end(), v);
      ++size_;
      return std::pair<iterator, bool>(buckets_[bucket].last, true);
    }
    iterator end = buckets_[bucket].last;
    ++end;
    while (it != end)
    {
      if (it->first == v.first){
				it->second = v.second;//Ìæ»»value
        return std::pair<iterator, bool>(it, false);
			}
      ++it;
    }
    buckets_[bucket].last = values_insert(end, v);
    ++size_;
    return std::pair<iterator, bool>(buckets_[bucket].last, true);
  }

  // Erase an entry from the map.
  iterator erase(iterator it)
  {
    assert(it != values_.end());
	iterator ret;
    size_t bucket = calculate_hash_value(it->first) % num_buckets_;
    bool is_first = (it == buckets_[bucket].first);
    bool is_last = (it == buckets_[bucket].last);
    if (is_first && is_last)
	  ret = buckets_[bucket].first = buckets_[bucket].last = values_.end();
    else if (is_first)
      ret = ++buckets_[bucket].first;
    else if (is_last)
      ret = --buckets_[bucket].last;

    values_erase(it);
    --size_;
	return ret;
  }

  // Erase a key from the map.
  iterator erase(const K& k)
  {
    iterator it = find(k);
    if (it != values_.end())
     return erase(it);
	return values_.end();
  }

  // Remove all entries from the map.
  void clear()
  {
    // Clear the values.
    values_.clear();
    size_ = 0;

    // Initialise all buckets to empty.
    iterator end = values_.end();
    for (size_t i = 0; i < num_buckets_; ++i)
      buckets_[i].first = buckets_[i].last = end;
  }

  hash_map<K,V>& operator = (const hash_map<K,V>& rhs)
  {
	if (&rhs != this)
	{
		typename hash_map<K,V>::const_iterator it = rhs.begin();
		while ( it != rhs.end())
		{
			insert(std::make_pair(it->first, it->second));
			it++;
		}
	}
	return *this;
  }

private:
  // Calculate the hash size for the specified number of elements.
  static std::size_t hash_size(std::size_t num_elems)
  {
    static std::size_t sizes[] =
    {

      3, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
      49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
      12582917, 25165843
    };
    const std::size_t nth_size = sizeof(sizes) / sizeof(std::size_t) - 1;
    for (std::size_t i = 0; i < nth_size; ++i)
      if (num_elems < sizes[i])
        return sizes[i];
    return sizes[nth_size];
  }

  // Re-initialise the hash from the values already contained in the list.
  void rehash(std::size_t num_buckets)
  {
    if (num_buckets == num_buckets_)
      return;
    num_buckets_ = num_buckets;

    iterator values_end = values_.end();

    // Update number of buckets and initialise all buckets to empty.
    bucket_type* tmp = new bucket_type[num_buckets_];
    delete[] buckets_;
    buckets_ = tmp;
    for (std::size_t i = 0; i < num_buckets_; ++i)
      buckets_[i].first = buckets_[i].last = values_end;

    // Put all values back into the hash.
    iterator iter = values_.begin();
    while (iter != values_end)
    {
      std::size_t bucket = calculate_hash_value(iter->first) % num_buckets_;
      if (buckets_[bucket].last == values_end)
      {
        buckets_[bucket].first = buckets_[bucket].last = iter++;
      }
      else if (++buckets_[bucket].last == iter)
      {
        ++iter;
      }
      else
      {
        values_.splice(buckets_[bucket].last, values_, iter++);
        --buckets_[bucket].last;
      }
    }
  }

  // Insert an element into the values list by splicing from the spares list,
  // if a spare is available, and otherwise by inserting a new element.
  iterator values_insert(iterator it, const value_type& v)
  {
    if (spares_.empty())
    {
      return values_.insert(it, v);
    }
    else
    {
      spares_.front() = v;
      values_.splice(it, spares_, spares_.begin());
      return --it;
    }
  }

  // Erase an element from the values list by splicing it to the spares list.
  void values_erase(iterator it)
  {
    *it = value_type();
    spares_.splice(spares_.begin(), values_, it);
  }

  // The number of elements in the hash.
  std::size_t size_;

  // The list of all values in the hash map.
  std::list<value_type> values_;

  // The list of spare nodes waiting to be recycled. Assumes that POD types only
  // are stored in the hash map.
  std::list<value_type> spares_;

  // The type for a bucket in the hash table.
  struct bucket_type
  {
    iterator first;
    iterator last;
  };

  // The buckets in the hash.
  bucket_type* buckets_;

  // The number of buckets in the hash.
  std::size_t num_buckets_;
};
template<class K>
static std::size_t calculate_hash_value (K i)
{
	return static_cast<std::size_t>(i);
}

template<>
static std::size_t calculate_hash_value<std::string>(std::string i)
{
	std::size_t	hash_val	= 2166136261U, 
		ufirst		= 0,
		ulast		= i.length();

	std::size_t	ustride = 1 + ulast / 10;

	for(; ufirst < ulast; ufirst += ustride)
	{
		hash_val = 16777619U * hash_val ^ (std::size_t)i[ufirst];
	}
	return(hash_val);
}


#endif // BOOST_ASIO_DETAIL_HASH_MAP_HPP
