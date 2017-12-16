#include<iostream>
#include<string>
#include<thread>

#include"GetIp.hpp"
#include "TcpListener.hpp"

#include "glog\logging.h"

using namespace std;

void Listener_MessageReceived(CTcpListener *listener, int client, string msg);

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

	fLS::FLAGS_log_dir = "logs\\";
	google::InitGoogleLogging(argv[0]);
	LOG(WARNING) << "Test " << 123;

	CTcpListener server(argv[1], stoi(argv[2]), Listener_MessageReceived);

	if( server.Init() )
	{
		LOG(WARNING) << "Server is working on: " << argv[1] << ':' << argv[2] << endl;
		server.Run();
	}

}

void Listener_MessageReceived(CTcpListener *listener, int client, string msg)
{ 
	try
	{
		cout << "SOCKET #" << client << ": " << msg << "\r\n";

		// TODO: Do some work with DB


		//Send answer to client
		std::ostringstream ss;
		ss << "SOCKET #" << client << ": " << msg << "\r\n";
		string strOut = ss.str();
		listener->Send(client, strOut);
	} catch( ... )
	{
		listener->InformExeption(current_exception());
	}
	
}
