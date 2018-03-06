#ifndef STDAFX_
#define STDAFX_

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push)
#pragma warning(disable : 4005) //C4005	_WIN32_WINNT : изменение макроопределения
#pragma warning(disable : 4996)

#pragma comment(lib, "ws2_32.lib") // Winsock library file
#pragma comment(lib, "IPHLPAPI.lib")

//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING // for asio debuging
//#define GOOGLE_STRIP_LOG 0 // cut all glog strings from .exe

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
#undef ERROR // for correct work with GLOG
#include <string>
#include <sqlext.h>
#include <sql.h>


#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <vector>
#include <list>

#include "glog\logging.h"
#include "CRunAsync.h"

#include "INIReaderWriter\INIReader.h"
#include "INIReaderWriter\INIWriter.hpp"

// to define the length of array before compilation
template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

#pragma warning(pop)

#endif // !STDAFX_