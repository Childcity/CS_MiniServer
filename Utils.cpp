#include "Utils.h"

void GetIpAddresses(IpAddresses & ipAddrs)
{
	IP_ADAPTER_ADDRESSES* adapter_addresses(NULL);
	IP_ADAPTER_ADDRESSES* adapter(NULL);

	// Start with a 16 KB buffer and resize if needed -
	// multiple attempts in case interfaces change while
	// we are in the middle of querying them.
	DWORD adapter_addresses_buffer_size = 16 * 1024;
	for( int attempts = 0; attempts != 3; ++attempts )
	{
		adapter_addresses = (IP_ADAPTER_ADDRESSES*)malloc(adapter_addresses_buffer_size);
		assert(adapter_addresses);//if the adapter_addresses is false, a message is written to the standard error device and abort is called, terminating the program execution.

		DWORD error = ::GetAdaptersAddresses(
			AF_UNSPEC,
			GAA_FLAG_SKIP_ANYCAST |
			GAA_FLAG_SKIP_MULTICAST |
			GAA_FLAG_SKIP_DNS_SERVER |
			GAA_FLAG_SKIP_FRIENDLY_NAME,
			NULL,
			adapter_addresses,
			&adapter_addresses_buffer_size);

		if( ERROR_SUCCESS == error )
		{
			// We're done here, people!
			break;
		} else if( ERROR_BUFFER_OVERFLOW == error )
		{
			// Try again with the new size
			free(adapter_addresses);
			adapter_addresses = NULL;

			continue;
		} else
		{
			// Unexpected error code - log and throw
			free(adapter_addresses);
			adapter_addresses = NULL;

			// @todo
			//LOG_AND_THROW_HERE();
		}
	}

	// Iterate through all of the adapters
	for( adapter = adapter_addresses; NULL != adapter; adapter = adapter->Next )
	{
		// Skip loopback adapters
		if( IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType )
		{
			continue;
		}

		// Parse all IPv4 and IPv6 addresses
		for(
			IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress;
			NULL != address;
			address = address->Next )
		{
			auto family = address->Address.lpSockaddr->sa_family;
			if( AF_INET == family )
			{
				// IPv4
				SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);

				char str_buffer[INET_ADDRSTRLEN] = { 0 };
				inet_ntop(AF_INET, &(ipv4->sin_addr), str_buffer, INET_ADDRSTRLEN);
				ipAddrs.mIpv4.push_back(str_buffer);
			} else if( AF_INET6 == family )
			{
				// IPv6
				SOCKADDR_IN6* ipv6 = reinterpret_cast<SOCKADDR_IN6*>(address->Address.lpSockaddr);

				char str_buffer[INET6_ADDRSTRLEN] = { 0 };
				inet_ntop(AF_INET6, &(ipv6->sin6_addr), str_buffer, INET6_ADDRSTRLEN);

				std::string ipv6_str(str_buffer);

				// Detect and skip non-external addresses
				bool is_link_local(false);
				bool is_special_use(false);

				if( 0 == ipv6_str.find("fe") )
				{
					char c = ipv6_str[2];
					if( c == '8' || c == '9' || c == 'a' || c == 'b' )
					{
						is_link_local = true;
					}
				} else if( 0 == ipv6_str.find("2001:0:") )
				{
					is_special_use = true;
				}

				if( !(is_link_local || is_special_use) )
				{
					ipAddrs.mIpv6.push_back(ipv6_str);
				}
			} else
			{
				// Skip all other types of addresses
				continue;
			}
		}
	}

	// Cleanup
	free(adapter_addresses);
	adapter_addresses = NULL;

	// Cheers!
}

void GetODBCDrivers(std::list<std::wstring> & lst)
{
	HENV hEnv = NULL;
	WCHAR driver[512], attr[512];
	SQLSMALLINT driver_ret, attr_ret;
	SQLUSMALLINT direction = SQL_FETCH_FIRST;

	if( SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR || 
		SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0) )
	{
		LOG(FATAL) << "ODBC: Unable to allocate an environment handle or set atribute (can't connect to database)!" << std::endl;
		return;
	}

	while( SQL_SUCCEEDED(SQLDriversW(hEnv, direction, driver, static_cast<SQLSMALLINT>(countof(attr)),
		  &driver_ret,
		  attr, static_cast<SQLSMALLINT>(countof(attr)),
		  &attr_ret)) )
	{
		direction = SQL_FETCH_NEXT;
		lst.push_back( driver + std::wstring(L"		- ") + attr);
	}

}

const char *GetExeName(HMODULE hModule)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converterUTF8_UTF16;

	size_t size = 64;
	wchar_t *buffer;
	for( ; ; )
	{
		buffer = new wchar_t[size + 1];
		DWORD r = GetModuleFileNameW(hModule, buffer, (DWORD)size);
		if( r < size && r != 0 ) break;
		if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
		{
			delete[] buffer;
			size *= 2;
		} else
		{
			return NULL;
		}
	}

	std::wstring exePath = std::wstring(buffer);
	std::size_t found = exePath.find_last_of(L"/\\");
	std::string exeName = converterUTF8_UTF16.to_bytes(exePath.substr(found + 1));

	char *c_exeName = new char[exeName.size()+1];
	memcpy_s(c_exeName, exeName.size()+1, exeName.c_str(),exeName.size());
	c_exeName[exeName.size()] = 0;

	return c_exeName;
}

std::wstring GetCurrentFolderPath(HMODULE hModule)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converterUTF8_UTF16;

	size_t size = 64;
	wchar_t *buffer;
	for( ; ; )
	{
		buffer = new wchar_t[size + 1];
		DWORD r = GetModuleFileNameW(hModule, buffer, (DWORD)size);
		if( r < size && r != 0 ) break;
		if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
		{
			delete[] buffer;
			size *= 2;
		} else
		{
			return NULL;
		}
	}

	std::wstring exePath = std::wstring(buffer);
	size_t exeNameSize = std::string(GetExeName()).size();
	size_t folderPathSize = exePath.size() - exeNameSize - 1; // -1 because we have '\' inside 

	return exePath.substr(0, folderPathSize);
}

