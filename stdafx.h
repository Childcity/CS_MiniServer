#ifndef STDAFX_
#define STDAFX_
//lib\\libglog_x64_release.lib;lib\\INIReader_x64_Release.lib;
//lib\\INIReader_x64_Debug.lib;lib\\libglog_x64_debug.lib;
#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
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

#include "INIReaderWriter\cpp\INIReader.h"
#include "INIReaderWriter\INIWriter.hpp"

#define QUICKLOG(str) std::ofstream log("D:\\log.log", std::ios::ate);log << (str) << std::endl;log.close();
#define QUICKLOGW(str) std::wofstream log(L"D:\\log.log", std::ios::ate);log << (str) << std::endl;log.close();

// to define the length of array before compilation
template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

//template <typename T, size_t N>
//char(&ArraySizeHelper(T(&array)[N]))[N];
//#define arraysize(array) (sizeof(ArraySizeHelper(array)))

template<class A, class B>
static B ConverterUTF8_UTF16(A str1)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converterUTF8_UTF16;

	if constexpr (std::is_same_v<A, std::wstring>)
	{
		return converterUTF8_UTF16.to_bytes(str1);
	} else
	{
		return converterUTF8_UTF16.from_bytes(str1);
	}
}
#pragma warning(pop)

#endif // !STDAFX_