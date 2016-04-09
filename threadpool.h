#include <vector>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/smart_ptr.hpp>


class thread_pool :public boost::enable_shared_from_this<thread_pool>
{
public:
	bool init(int nCount)
	{
		count_ = 0;
		for (int i=0;i<nCount;i++)
		{
			boost::shared_ptr<boost::thread> th(new boost::thread(boost::bind(&thread_pool::loop,shared_from_this())));
			thread_pool_.push_back(th);
		}
		return true;
	}
	void loop()
	{
		while (true)
		{
			boost::unique_lock<boost::mutex> uq(mutex_);
			condition_.wait(uq);
			std::cout << "cout_=" << count_ << std::endl;
		}
	}
	void add()
	{
		count_++;
		condition_.notify_one();
	}
	void join()
	{
		for (int i=0;i<thread_pool_.size();i++)
		{
			thread_pool_[i]->join();
		}
	}
protected:
private:
	std::vector<boost::shared_ptr<boost::thread>> thread_pool_;
	boost::condition_variable  condition_;
	boost::mutex               mutex_;
	int                        count_;
};