#pragma once

#include <string>

#include <WS2tcpip.h> // For Winsock
#pragma comment(lib, "ws2_32.lib") // Winsock library file

#define MAX_BUFFER_SIZE (4096)
// Forward declaration of class
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

		//Cleanup
		void Cleanup();

	private:
		// Create a cocket
		SOCKET CreateSocket();

		// Wait for a connection
		SOCKET WaitForConnection(SOCKET listening);


	std::string				m_ipAddress;
	int						m_port;
	MessageRecievedHandler  MessageReceived;
};
