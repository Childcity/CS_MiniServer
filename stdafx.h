#pragma warning(push)
#pragma warning(disable : 4005) //C4005	_WIN32_WINNT : изменение макроопределения

#pragma comment(lib, "ws2_32.lib") // Winsock library file
#pragma comment(lib, "IPHLPAPI.lib")

#include <assert.h>
#include <WS2tcpip.h> // For Winsock
#include <iphlpapi.h>

#define _WIN32_WINNT 0x0501

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>


#include <windows.h>
#include <string>
#include <sqlext.h>
#include <sql.h>


#include <iostream>
#include <string>
#include <locale>
#include <vector>
#include <list>

#include "glog\logging.h"
#include "CRunAsync.h"

#pragma warning(pop)