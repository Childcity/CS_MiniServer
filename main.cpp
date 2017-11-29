#include<iostream>
#include<string>
#include<thread>

#include "TcpListener.hpp"

using namespace std;

void Listener_MessageReceived(CTcpListener *listener, int client, string msg);

//127.0.0.1 5301
void main( int arc, char **argv )
{
	if( arc != 3 )
	{
		cout <<"Usage:" <<"csserver.exe [ipAdress] [port]" <<endl;
		return;
	}

	cout<<"iPAdress: " << argv[1] <<':' << argv[2] <<endl;

	CTcpListener server(argv[1], stoi(argv[2]), Listener_MessageReceived);

	if( server.Init() )
	{
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
