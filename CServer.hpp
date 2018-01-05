#pragma once
#include <boost/bind.hpp>
#include <boost/asio.hpp> 
#include <boost/thread.hpp>

#include "CClientSession.hpp"

using namespace boost::asio;
using boost::asio::ip::tcp;

class CServer{
public:
	CServer(io_context& io_context, short port, short thread_num)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
		, io_context(io_context)
		, thread_num_(thread_num)
	{
		// init first client
		VLOG(1)<< "DEBUG: init first client" <<std::endl;
		CClientSession::ptr client = CClientSession::new_(io_context);

		// accept first client
		VLOG(1)<< "DEBUG: accept first client";
		acceptor_.async_accept(client->sock(), boost::bind(&CServer::do_accept, this, client, _1));

		// start listen
		VLOG(1)<< "DEBUG: start listening" << std::endl;
		start_listen();

		threads.join_all();
	}

private:
	void do_accept(CClientSession::ptr client, const boost::system::error_code& err)
	{
		client->start();
		CClientSession::ptr new_client = CClientSession::new_(io_context);
		VLOG(1)<< "DEBUG: accept NEXT client";
		acceptor_.async_accept(new_client->sock(), boost::bind(&CServer::do_accept, this, new_client, _1));
	}

	void start_listen()
	{
		// run io_context in thread_num_ of threads
		for( int i = 0; i < thread_num_; ++i )
		{
			threads.create_thread(
				[this]()
				{
					this->io_context.run();
				});
		}
	}

	io_context& io_context;
	tcp::acceptor acceptor_;
	boost::thread_group threads;
	short thread_num_;
};

