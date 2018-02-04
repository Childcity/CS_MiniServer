#pragma once
/*
#define _WIN32_WINNT 0x0501

#include <boost/bind.hpp>
#include <boost/asio.hpp> 
#include <boost/thread.hpp>*/

#include "CClientSession.h"

using namespace boost::asio;
using boost::asio::ip::tcp;

class CServer{
public:
	explicit CServer(io_context& io_context, short port, short thread_num)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
		, io_context(io_context)
		, thread_num_(thread_num)
	{ Start(); }

	explicit CServer(io_context& io_context, std::string ipAddress, short port, short thread_num)
		: acceptor_(io_context, tcp::endpoint(ip::address::from_string(std::move(ipAddress)), port))
		, io_context(io_context)
		, thread_num_(thread_num)
	{ Start(); }
	
	CServer(CServer const&) = delete;
	CServer operator=(CServer const&) = delete;

private:
	void Start();

	void do_accept(CClientSession::ptr client, const boost::system::error_code& err);

	void start_listen();

	io_context& io_context;
	tcp::acceptor acceptor_;
	boost::thread_group threads;
	short thread_num_;
};

