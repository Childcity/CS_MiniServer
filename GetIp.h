#pragma once

//#include "stdafx.h"

struct IpAddresses{
	std::list<std::string> mIpv4, mIpv6;
};

void GetIpAddresses(IpAddresses & ipAddrs);