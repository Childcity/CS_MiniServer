#pragma once
#define _WIN32_WINNT 0x0501

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <vector>

#include "glog\logging.h"
#include "CRunAsync.hpp"

using namespace boost::asio;
using namespace boost::posix_time;

// to define the length of array before comilation
template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

void update_clients_changed();

class CClientSession : public boost::enable_shared_from_this<CClientSession>
		, boost::noncopyable{
private:
	CClientSession(io_context& io_context)
		: sock_(io_context)
		, started_(false)
		, timer_(io_context)
		, clients_changed_(false)
		, username_("user")
		, io_context_(io_context)
	{}

public:
	typedef boost::system::error_code error_code;
	typedef boost::shared_ptr<CClientSession> ptr;

	// init and start do_read()
	void start();

	// class factory. Return ptr to this class
	static ptr new_(io_context& io_context);

	// stop working with current client and remove it from clients
	void stop();

	// return started
	bool started() const;

	// return link to socket of current client
	ip::tcp::socket& sock();

	// get user name
	std::string username() const;
	
	// set flag clients changed to true, then client session notify	
	// its client, that clients list was changed
	void set_clients_changed();

private:
	void on_read(const error_code & err, size_t bytes);

	void on_login(const std::string msg);

	void on_ping();

	void on_clients();

	void on_check_ping();

	void post_check_ping();

	void on_write(const error_code& err, size_t bytes);

		error_code do_get_fibo(size_t n);

		void on_get_fibo(const size_t n, error_code& err);

		void on_fibo(const std::string msg);

	void do_read();

	void do_write(const std::string& msg);

	size_t read_complete(const error_code& err, size_t bytes);


	mutable boost::recursive_mutex cs_;
	enum{ max_msg = 1024, max_timeout = 10000 };
	char read_buffer_[max_msg];
	char write_buffer_[max_msg];
	io_context& io_context_;
	ip::tcp::socket sock_;
	bool started_;

	boost::posix_time::ptime last_ping_;
	deadline_timer timer_;

	std::vector<std::pair<size_t,size_t>> res;
	std::string username_;
	bool clients_changed_;
};
