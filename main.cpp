#pragma once
//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING // for asio debuging
//#define GOOGLE_STRIP_LOG 0 // cut all glog strings from .exe
#include <iostream>
#include <string>

#include "GetIp.h"
#include "CServer.hpp"

#include "glog\logging.h"
#include <boost/asio.hpp> 

using namespace std;

void main( int arc, char **argv )
{
	if( arc != 3 )
	{
		cout <<"Usage:" <<"csserver.exe [ipAdress] [port]" <<endl <<endl;
		//return;

		// I do not know if this is necessary, but I think that in the phase of development it will not be unnecessary
		IpAddresses ips; // Declare structure, that consists of list of Ipv4 and Ipv6 ip addresses
		GetIpAddresses(ips); // Get list of ipv4 and ipv6 from possible interfaces

		cout <<"Possible ip addresses on this machine:" <<endl;

		int i = 1;
		cout << i++ << ". 127.0.0.1" << endl;
		for( auto var : ips.mIpv4 )
			cout <<i++ << ". " << var << endl;

		cin.get();
		return;
	}

	system("chcp 65001>nul");

	//Init Glog
	//fLS::FLAGS_log_dir = "logs\\";
	google::InitGoogleLogging(argv[0]);

	try
	{
		boost::asio::io_context io_context;
		CServer server(io_context, std::atoi(argv[1]), std::atoi(argv[2]));

	} catch(exception& e)
	{
		LOG(FATAL) << "Server has been crashed: " << e.what() << std::endl;
	}
}