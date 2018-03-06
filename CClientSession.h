#pragma once

#include "CDatabase.h"
#include "main.h"

using namespace boost::asio;
using namespace boost::posix_time;
using std::string;
using std::wstring;
using std::move;

void update_clients_changed();

class CClientSession : public boost::enable_shared_from_this<CClientSession>
							, boost::noncopyable{
private:
	typedef boost::system::error_code error_code;

	CClientSession(io_context & io_context)
		: sock_(io_context)
		, started_(false)
		, timer_(io_context)
		, clients_changed_(false)
		, username_("user")
		, io_context_(io_context)
	{}

public:

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
	ip::tcp::socket & sock();

	// get user name
	string username() const;
	
	// set flag clients changed to true, then client session notify	
	// its client, that clients list was changed
	void set_clients_changed();

private:
	void on_read(const error_code & err, size_t bytes);

	void on_login(const string & msg);

	void on_ping();

	void on_clients();

	void on_check_ping();

	void post_check_ping();

	void on_write(const error_code & err, size_t bytes);

		error_code do_get_fibo(size_t n) ;

		void on_get_fibo(const size_t n, error_code & err);

		void on_fibo(const string & msg);

	error_code do_ask_db(const string query, size_t queryId);

	void on_answer_db(const size_t queryId, error_code & err);

	void on_query(const string & msg);

	void do_read();

	void do_write(const string & msg);

	size_t read_complete(const error_code & err, size_t bytes);


private:

	mutable boost::recursive_mutex cs_;
	enum{ max_msg = 20971520, max_timeout = 10000 };
	static constexpr const char endOfMsg[] = "!@e";
	static constexpr const size_t sizeEndOfMsg = countof(endOfMsg) - 1;
	char read_buffer_[max_msg];
	char write_buffer_[max_msg];
	io_context& io_context_;
	ip::tcp::socket sock_;
	bool started_;

	boost::posix_time::ptime last_ping_;
	deadline_timer timer_;

	std::vector<std::pair<size_t,string>> res;
	std::vector<std::pair<size_t, size_t>> fibo_res;
	string username_;
	bool clients_changed_;
};
