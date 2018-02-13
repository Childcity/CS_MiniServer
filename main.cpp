#pragma once
#include "main.h"
#include "CDatabase.h"
#include "Service.h"
#include "CServer.h"

using std::endl;
using std::exception;

void ShowUsage(const char *argv0);

WCHAR ConectionString[512];
HWND hWnd;

void main(int argc, char **argv)
{
	//std::locale cp1251_locale("ru_RU.CP866");
	//std::locale::global(cp1251_locale);
	setlocale(LC_CTYPE, "");

	//Init Glog
	//fLS::FLAGS_log_dir = "logs\\";
	google::InitGoogleLogging(argv[0]);

	try 
	{
		boost::asio::io_context io_context ;

		//wifstream file("user.cfg");
		//ZeroMemory(ConectionString, sizeof(ConectionString));
		//file.getline(ConectionString, 512);

		WCHAR Conection[150] = L"Driver={SQL Server};Server=MAXWELL;Database=StopNet4; Uid=sa; Pwd=111111;";
		wmemcpy_s(ConectionString, sizeof(ConectionString), Conection, sizeof(Conection)/sizeof(WCHAR));
		


		hWnd = GetDesktopWindow(); // need for connection to ODBC driver

		// try connect to db, to see if everything is ok with connection string
		// and db is well configured

		// try to connect to ODBC driver
		ODBCDatabase::CDatabase db;

		// if connected, send query to db
		if( db.ConnectedOk() )
		{
			LOG(INFO) << "Connection to db was success" << endl;
			db.~CDatabase();
		} else
		{
			LOG(FATAL) << "Can't connect to db. Check connection string in configuration file" << endl;
		}

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
	FLAGS_alsologtostderr = true; //to make logging both on stderr and logfile 
	LOG(INFO) << "Usage:" << argv0 << " [ipAddress] [port] [threds_number]" << endl << endl;
	//return;

	// I do not know if this is necessary, but I think that in the phase of development it will not be unnecessary
	IpAddresses ips; // Declare structure, that consists of list of Ipv4 and Ipv6 ip addresses
	std::list<std::wstring> posDrivers; // List of possible ODBC drivers on this machine
	GetIpAddresses(ips); // Get list of ipv4 and ipv6 from possible interfaces

	LOG(INFO) << "Possible ipV4 addresses on this machine:" << endl;

	int i = 1;
	LOG(INFO) << i++ << ". 127.0.0.1" << endl;
	for( auto & var : ips.mIpv4 )
		LOG(INFO) << i++ << ". " << var << endl;
	
	LOG(INFO) << "List of possible ODBC drivers on this machine:"  << endl;

	GetODBCDrivers(posDrivers);

	i = 1;
	for( auto & var : posDrivers )
		LOG(INFO) << i++ << ". " << std::string(var.begin(), var.end()) <<endl;

	LOG(INFO) << "Exiting...";
	//LOG(INFO) << "\nPress ENTER to exit..." <<endl;
	//cin.get();
}