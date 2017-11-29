#pragma once

#include <sstream>
#include <iostream>
#include <string>
#include "thread"
#include <mutex>

#include <WS2tcpip.h> // For Winsock
#pragma comment(lib, "ws2_32.lib") // Winsock library file

#define MAX_BUFFER_SIZE (4096)
// Forward declaration of class for Callback
class CTcpListener;

// TODO: Callback to data received
typedef void (*MessageRecievedHandler)(CTcpListener *listener, int socketId, std::string msg);

class CTcpListener{

public:

	CTcpListener( const std::string ipAddress, int port, MessageRecievedHandler handler);

	~CTcpListener();

	//Send a massage to the specified client
	void Send(int clientSocket, std::string msg);

	// Initialize winsock
	bool Init();

	// The main processing loop
	void Run();

	// Cleanup
	void Cleanup();

	// Add exeption from threads
	void InformExeption( std::exception_ptr &e);

private:
	// Create a cocket
	SOCKET CreateSocket();

	// Wait for a connection
	SOCKET WaitForConnection(SOCKET listening);


	// For server
	std::string				m_ipAddress;
	int						m_port;
	MessageRecievedHandler  MessageReceived;


	// TODO: variables for service needs
	// Container for thread exeptins
	std::vector<std::exception_ptr>  g_exceptions;
	std::recursive_mutex                       g_exceptions_mutex;
};
