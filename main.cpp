#pragma once
//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING // for asio debuging
//#define GOOGLE_STRIP_LOG 0 // cut all glog strings from .exe
#include <iostream>
#include <string>

#include "GetIp.hpp"
#include "CServer.h"

#include "glog\logging.h"
#include <boost\asio.hpp> 

using namespace std;

void ShowUsage(const char *argv0);

void main(int argc, char **argv)
{
	system("chcp 65001>nul");

	//Init Glog
	//fLS::FLAGS_log_dir = "logs\\";
	google::InitGoogleLogging(argv[0]);

	try
	{
		boost::asio::io_context io_context ;

		if( argc == 3 )
			CServer Server(io_context, std::atoi(argv[1]), std::atoi(argv[2]));
		else if( argc == 4 )
			CServer Server(io_context, argv[1], std::atoi(argv[2]), std::atoi(argv[3])); 
		else
			ShowUsage(argv[0]);

	} catch(exception& e)
	{
		LOG(FATAL) << "Server has been crashed: " << e.what() << std::endl;
	}
}

void ShowUsage( const char * argv0 )
{
	cout << "Usage:" << argv0 << " [ipAddress] [port] [threds_number]" << endl << endl;
	//return;

	// I do not know if this is necessary, but I think that in the phase of development it will not be unnecessary
	IpAddresses ips; // Declare structure, that consists of list of Ipv4 and Ipv6 ip addresses
	GetIpAddresses(ips); // Get list of ipv4 and ipv6 from possible interfaces

	cout << "Possible ipV4 addresses on this machine:" << endl;

	int i = 1;
	cout << i++ << ". 127.0.0.1" << endl;
	for( auto var : ips.mIpv4 )
		cout << i++ << ". " << var << endl;

	cout << "\nPress ENTER to exit..." <<endl;
	cin.get();
}