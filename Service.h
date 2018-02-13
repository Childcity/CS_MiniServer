#pragma once

struct IpAddresses{
	std::list<std::string> mIpv4, mIpv6;
};

void GetIpAddresses(IpAddresses & ipAddrs);

void GetODBCDrivers(std::list<std::wstring> & lst);
