#include<iostream>
#include<string>

#include "TcpListener.hpp"

using namespace std;

void Listener_MessageReceived(CTcpListener *listener, int client, string msg);

void main( int arc, char **argv )
{
	CTcpListener server("127.0.0.1", 53521, Listener_MessageReceived);

	if( server.Init() )
	{
		server.Run();
	}


	system("pause>nul");
}

void Listener_MessageReceived(CTcpListener * listener, int client, string msg)
{ 
	listener->Send(client, msg);
}
