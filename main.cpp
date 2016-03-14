// test_asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "net.h"
#include <exception>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>

using namespace std;
using namespace boost;
int main(int argc, char * argv[])
{

	boost::asio::io_service io;
	server s1(io,10241);
	while(true)
	{
		io.poll();
	}
	return 0;
}

