#ifndef _UTILS_
#define _UTILS_
#pragma once
struct IpAddresses{
	std::list<std::string> mIpv4, mIpv6;
};

void GetIpAddresses(IpAddresses & ipAddrs);
void GetODBCDrivers(std::list<std::wstring> & lst);
const char *GetExeName(HMODULE hModule = NULL);
std::wstring GetCurrentFolderPath(HMODULE hModule = NULL);
#endif