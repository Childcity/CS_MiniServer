#include "TcpListener.hpp"


CTcpListener::CTcpListener(const std::string ipAddress, int port, MessageRecievedHandler handler)
	: m_ipAddress(ipAddress), m_port(port), MessageReceived(handler)
{
	g_exceptions.clear();
}

CTcpListener::~CTcpListener()
{
	Cleanup();
}

//Send a massage to the specified client
void CTcpListener::Send(int clientSocket, std::string msg)
{
	int res = send((SOCKET)clientSocket, msg.c_str(), (int)msg.size()+1, 0);
	if( res == SOCKET_ERROR )
	{
		std::cout <<"send failed with error: " << WSAGetLastError() <<std::endl;
	}
}

// Initialize winsock
bool CTcpListener::Init()
{
	WSAData data;
	WORD ver = MAKEWORD(2, 0);

	int wsInit = WSAStartup(ver, &data);
	// TODO: Inform caller the error that occured
	return wsInit == 0;
}

// The main processing loop
void CTcpListener::Run()
{
	// Create a listening socket
	SOCKET listening = CreateSocket();
	if( listening == INVALID_SOCKET )
	{
		// Fatal error
		return;
	}

	// Create the master file descriptor set and zero it
	fd_set master;
	FD_ZERO(&master);

	// Add our first socket that we're interested in interacting with; the listening socket
	FD_SET(listening, &master);

	// this will be changed by the \quit command (see below, bonus not in video!)
	bool running = true;
	while( true )
	{
		fd_set copy = master;

		// See who's talking to us
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		// Loop for proccessing each connection
		for( int i = 0; i < socketCount; i++ )
		{
			// Makes things easy for us doing this assignment
			SOCKET sock = copy.fd_array[i];

			// Is it an inbound communication?
			if( sock == listening )
			{
				// Accept a new connection
				SOCKET client = WaitForConnection(listening);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);

				// Send a welcome message to the connected client
				std::string welcomeMsg = "Hello from Awesome Server\r\ndeveloped by Denis and Nikolas and Google Search!\r\n";
				Send(client, welcomeMsg);
			} else // It's an inbound message
			{
				char buf[MAX_BUFFER_SIZE];
				ZeroMemory(buf, MAX_BUFFER_SIZE);

				// Receive message
				int bytesIn = recv(sock, buf, MAX_BUFFER_SIZE, 0);
				if( bytesIn <= 0 )
				{
					// Drop the client
					std::cout << "SOCKET #" << sock << ": go away..." << "\r\n";
					closesocket(sock);
					FD_CLR(sock, &master);
				} else 
				{
					// Check to see if it's a command. \quit kills the server
					if( buf[0] == '\\' )
					{
						// Is the command quit? 
						std::string cmd = std::string(buf, bytesIn);
						if( cmd == "\\quit" || cmd == "\\q" )
						{
							running = false;
							break;
						} //else if(cmd == ...)
						{
							// TODO: Place for other commands for server
						}

						  // Unknown command
						continue;
					}

					if( sock != listening )
					{
						if( MessageReceived != NULL )
						{
							std::thread t( MessageReceived, this, sock, std::string(buf, 0, bytesIn) );
							//MessageReceived( this, sock, std::string(buf, 0, bytesIn));
							t.detach(); //Make thread separate from server
						}
					}

					// Check for Exeptions
					g_exceptions_mutex.lock();
					for( auto &e : g_exceptions )
					{
						try
						{
							if( e != nullptr )
								std::rethrow_exception(e);
						} catch( const std::exception &e )
						{
						///////////////// Should be changed to log !!! \\\\\\\\\\\\\\\\\/
							std::cout << e.what() << std::endl;
						}
					}
					g_exceptions_mutex.unlock();


					// TODO: This loop for that moment, when we need to sent message for ALL clients

					//// Send message to other clients, and definiately NOT the listening socket
					//for( int i = 0; i < master.fd_count; i++ )
					//{
					//	SOCKET outSock = master.fd_array[i];
					//	if( outSock != listening && outSock != sock )
					//	{
					//		if( MessageReceived != NULL )
					//		{
					//			MessageReceived(this, outSock, std::string(buf, 0, bytesIn));
					//		}
					//	}
					//}
				}
			}
		}

	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listening, &master);
	closesocket(listening);

	// Message to let users know what's happening.
	std::string msg = "Server is shutting down. Goodbye\r\n";

	while( master.fd_count > 0 )
	{
		// Get the socket number
		SOCKET sock = master.fd_array[0];

		// Send the goodbye message
		Send(sock, msg);

		// Remove it from the master file list and close the socket
		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// Cleanup winsock
	Cleanup();
}

void CTcpListener::Cleanup()
{
	WSACleanup();
}

void CTcpListener::InformExeption(std::exception_ptr & e)
{ 
	std::lock_guard<std::recursive_mutex> lock(g_exceptions_mutex);
	g_exceptions.push_back(std::current_exception());
}

// Create a cocket
SOCKET CTcpListener::CreateSocket()
{
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if( listening != INVALID_SOCKET )
	{
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(m_port);
		inet_pton(AF_INET, m_ipAddress.c_str(), &hint.sin_addr);

		int bindOk = bind(listening, (sockaddr*)&hint, sizeof(hint));
		if( bindOk != SOCKET_ERROR )
		{
			int listenOk = listen(listening, SOMAXCONN);
			if( listenOk == SOCKET_ERROR )
			{
				return -1;
			}
		} else
		{
			//Port can't be use!
			return -1;
		}
	}

	return listening;
}

// Wait for a connection
SOCKET CTcpListener::WaitForConnection(SOCKET listening)
{
	SOCKET client = accept(listening,  nullptr, nullptr);
	return client;
}

