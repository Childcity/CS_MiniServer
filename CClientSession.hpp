#pragma once
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

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

class CClientSession;
typedef boost::shared_ptr<CClientSession> client_ptr;

		typedef std::vector<client_ptr> cli_ptr_vector;
		cli_ptr_vector clients;	
		boost::recursive_mutex clients_cs;

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
	void start()
	{
		{
			boost::recursive_mutex::scoped_lock lk(clients_cs);
			clients.push_back(shared_from_this());
		}

		boost::recursive_mutex::scoped_lock lk(cs_);
		started_ = true;
		last_ping_ = boost::posix_time::microsec_clock::local_time();

		// first, we wait for client to login
		on_ping();
		//do_read();
	}

	// class factory. Return ptr to this class
	static ptr new_(io_context& io_context)
	{
		ptr new_(new CClientSession(io_context));
		return new_;
	}

	// stop working with current client and remove it from clients
	void stop()
	{
		{
			boost::recursive_mutex::scoped_lock lk(cs_);

			VLOG(1)<< "DEBUG: stop client: " << username() << std::endl;

			if( !started_ ) 
				return;

			started_ = false;
			sock_.close();
		}

		ptr self = shared_from_this();

		{
			boost::recursive_mutex::scoped_lock lk(clients_cs);
			cli_ptr_vector::iterator it = std::find(clients.begin(), clients.end(), self);
			clients.erase(it);
		}
		update_clients_changed();
	}

	// return started
	bool started() const
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		return started_;
	}

	// return link to socket of current client
	ip::tcp::socket& sock()
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		return sock_;
	}

	std::string username() const
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		return username_;
	}

	void set_clients_changed()
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		clients_changed_ = true;
	}

private:
	void on_read(const error_code & err, size_t bytes)
	{
		if( err ) 
			stop();

		if( !started() )
			return;

		static const char login[] = "login ";
		static const char fibo[] = "fibo ";
		static const char ping[] = "ping";
		static const char who[] = "who";
		static const size_t login_size = countof(login);
		static const size_t fibo_size = countof(fibo);
		static const size_t ping_size = countof(ping);
		static const size_t who_size = countof(who);

		// process the msg
		boost::recursive_mutex::scoped_lock lk(cs_);
		std::string msg(read_buffer_, bytes);

		VLOG(1) << "DEBUG: received msg: '" << msg << '\'' << std::endl;

		if( bytes > login_size && memcmp(read_buffer_, login, login_size - 1) == 0 )
		{
			on_login(std::move(msg));
		}
		else if( bytes == ping_size && memcmp(read_buffer_, ping, ping_size - 1) == 0 )
		{
			on_ping();
		} 
		else if( bytes == who_size && memcmp(read_buffer_, who, who_size - 1) == 0 )
		{
			on_clients();
		}
		else if( bytes > fibo_size && memcmp(read_buffer_, fibo, fibo_size - 1) == 0 )
		{
			on_fibo(std::move(msg));
		}
		else
		{
			do_write("command undefined\n");
			LOG(INFO) << "Invalid msg from client: " << username() << " - " << msg;
		}

		/*if( msg.find("login ") == 0 ) 
			on_login(msg);
		else if( msg.find("ping") == 0 ) 
			on_ping();
		else if( msg.find("who") == 0 ) 
			on_clients();
		else if( msg.find("fibo ") == 0 )
			do_fibo(msg);
		else 
		{
			do_write("command undefined\n");
			LOG(INFO) << "Invalid msg from client: " << username()<<" - " << msg;
		}*/
	}

	void on_login(const std::string msg)
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		std::istringstream in(msg);

		in >> username_ >> username_;

		VLOG(1)<<"debug: logged in: " << username_ << std::endl;

		do_write("login ok\n");
		update_clients_changed();
	}

	void on_ping()
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		do_write(clients_changed_ ? "ping client_list_changed\n" : "ping OK\n");

		// we have notified client, that clients list was changed yet,
		// so clients_changed_ should be false 
		clients_changed_ = false;
	}

	void on_clients()
	{
		cli_ptr_vector copy;

		{
			boost::recursive_mutex::scoped_lock lk(clients_cs);
			copy = clients;
		}

		std::string msg;

		for( auto it : clients )
			msg += it->username() + " ";

		//for( cli_ptr_vector::const_iterator b = copy.begin(), e = copy.end(); b != e; ++b )
		//	msg += (*b)->username() + " ";

		do_write("clients: " + msg + "\n");
	}

	//void do_ping()
	//{
	//	do_write("ping\n");
	//}

	//void do_ask_clients()
	//{
	//	do_write("ask_clients\n");
	//}

	void on_check_ping()
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
		if( (now - last_ping_).total_milliseconds() > max_timeout )
		{
			VLOG(1)<< "DEBUG: stopping: " << username_ << " - no ping in time" << std::endl;
			stop();
		}
		last_ping_ = boost::posix_time::microsec_clock::local_time();
	}

	void post_check_ping()
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		timer_.expires_from_now(boost::posix_time::millisec(max_timeout));
		timer_.async_wait( boost::bind(&CClientSession::on_check_ping, shared_from_this()) );
	}

	void on_write(const error_code& err, size_t bytes)
	{
		do_read();
	}

	error_code do_get_fibo(size_t n)
	{
		//return n<=2 ? n: get_fibo(n-1) + get_fibo(n-2);
		size_t a = 1, b = 1;
		for( int i = 3; i <= n; i++ )
		{
			size_t c = a + b;
			a = b; b = c;
		}
		
		boost::recursive_mutex::scoped_lock cs_;
		res.push_back(std::make_pair(n,b));

		return boost::system::error_code(0, boost::system::generic_category());
	}

	void on_get_fibo(const size_t n, error_code& err)
	{
		if( err )
			return;

		boost::recursive_mutex::scoped_lock cs_;
		for(auto it : res)
			if(it.first == n)
			{
				VLOG(1) << "DEBUG: fibo for: " << n << " = " << it.second << std::endl;
				return;
			}

		//do_write("RESULT\n");
	}

	void on_fibo(const std::string msg)
	{
		std::istringstream in(msg);
		in.ignore(5);
		short n; in >> n;
		CRunAsync::new_()->add(boost::bind(&CClientSession::do_get_fibo, shared_from_this(), n)
							  , boost::bind(&CClientSession::on_get_fibo, shared_from_this(), n, _1)
							  , io_context_);

		do_read();
	}

	void do_read()
	{
		VLOG(1)<< "DEBUG: do read" << std::endl;
		async_read(sock_, buffer(read_buffer_),
				   boost::bind(&CClientSession::read_complete, shared_from_this(), _1, _2),
				   boost::bind(&CClientSession::on_read, shared_from_this(), _1, _2)
				   );

		//post_check_ping();
	}

	void do_write(const std::string& msg)
	{
		if( !started() ) 
			return;

		boost::recursive_mutex::scoped_lock lk(cs_);
		std::copy(msg.begin(), msg.end(), write_buffer_);
		sock_.async_write_some(buffer(write_buffer_, msg.size()),
							   boost::bind(&CClientSession::on_write, shared_from_this(), _1, _2)
							   );
	}

	size_t read_complete(const error_code& err, size_t bytes)
	{
		if( err ) 
			return 0;

		bool found = std::find(read_buffer_, read_buffer_ + bytes, '\n') < read_buffer_ + bytes;
		// we read one-by-one until we get to enter, no buffering
		return found ? 0 : 1;
	}


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

void update_clients_changed()
{
	cli_ptr_vector copy;
	{
		boost::recursive_mutex::scoped_lock lk(clients_cs);
		copy = clients;
	}

	for( auto it : clients )
		it->set_clients_changed();

	//old variant
	//for( cli_ptr_vector::iterator b = copy.begin(), e = copy.end(); b != e; ++b )
	//	(*b)->set_clients_changed();
}
