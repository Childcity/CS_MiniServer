#include "CClientSession.h"

boost::recursive_mutex clients_cs;
typedef boost::shared_ptr<CClientSession> client_ptr;
typedef std::vector<client_ptr> cli_ptr_vector;
cli_ptr_vector clients;

void CClientSession::start()
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

CClientSession::ptr CClientSession::new_(io_context& io_context)
{
	ptr new_(new CClientSession(io_context));
	return new_;
}

void CClientSession::stop()
{
	{
		boost::recursive_mutex::scoped_lock lk(cs_);

		VLOG(1) << "DEBUG: stop client: " << username() << std::endl;

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

bool CClientSession::started() const
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	return started_;
}

ip::tcp::socket& CClientSession::sock()
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	return sock_;
}

string CClientSession::username() const
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	return username_;
}

void CClientSession::set_clients_changed()
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	clients_changed_ = true;
}

void CClientSession::on_read(const error_code & err, size_t bytes)
{
	if( err )
		stop();

	if( !started() )
		return;

	// the static variable and them size will be one for ALL INSTANCES OF THE CLASS that are created
	static constexpr const char login[] = "login ";
	static constexpr const char fibo[] = "fibo ";
	static constexpr const char query[] = "query ";
	static constexpr const char ping[] = "ping";
	static constexpr const char who[] = "who";
	static constexpr const size_t login_size = countof(login);
	static constexpr const size_t fibo_size = countof(fibo);
	static constexpr const size_t query_size = countof(query);
	static constexpr const size_t ping_size = countof(ping);
	static constexpr const size_t who_size = countof(who);

	// process the msg

	// we must make copy of read_buffer_, for quick unlock sc_ mutex
	boost::scoped_array<char> tmp_read_buffer_{new char[bytes + sizeof(char)]}; //+ sizeof(char) for '\0'
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		memcpy_s(tmp_read_buffer_.get(), bytes + sizeof(char), read_buffer_, bytes/sizeof(char));
		tmp_read_buffer_[bytes] = 0;
	}

	VLOG(1) << "DEBUG: received msg: '" << tmp_read_buffer_.get() << '\''
			<< "DEBUG: received bytes: " << bytes <<' ' << memcmp(tmp_read_buffer_.get(), who, who_size - 1)
			<< std::endl;

	if( bytes > login_size && memcmp(tmp_read_buffer_.get(), login, login_size - 1) == 0 )
	{
		on_login(move(string(tmp_read_buffer_.get(), bytes)));
	} else if( bytes == ping_size/*+1*/ && memcmp(tmp_read_buffer_.get(), ping, ping_size - 1) == 0 )
	{
		on_ping();
	} else if( bytes == who_size/*+1*/ && memcmp(tmp_read_buffer_.get(), who, who_size - 1) == 0 )
	{
		on_clients();
	}  else if( bytes > fibo_size && memcmp(tmp_read_buffer_.get(), fibo, fibo_size - 1) == 0 )
	{
		on_fibo(move(string(tmp_read_buffer_.get(), bytes)));
	} else if( bytes > query_size && memcmp(tmp_read_buffer_.get(), query, query_size - 1) == 0 )
	{
		on_query(move(string(tmp_read_buffer_.get(), bytes)));
	} else
	{
		do_write(move(string("command undefined\n")));
		LOG(WARNING) << "Invalid msg from client " << username() << ": '" << tmp_read_buffer_.get() << '\'' <<std::endl;
	}
}

void CClientSession::on_login(const string && msg)
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	std::istringstream in(msg);

	in >> username_ >> username_;

	VLOG(1) << "DEBUG: logged in: " << username_ << std::endl;

	do_write(move(string("login ok\n")));
	update_clients_changed();
}

void CClientSession::on_ping()
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	do_write(clients_changed_ ? move(string("ping client_list_changed\n")) : move(string("ping OK\n")));

	// we have notified client, that clients list was changed yet,
	// so clients_changed_ should be false 
	clients_changed_ = false;
}

void CClientSession::on_clients()
{
	cli_ptr_vector clients_copy;
	{
		boost::recursive_mutex::scoped_lock lk(clients_cs);
		clients_copy = clients;
	}

	string msg;

	for( auto it : clients_copy )
		msg += it->username() + " ";

	//for( cli_ptr_vector::const_iterator b = copy.begin(), e = copy.end(); b != e; ++b )
	//	msg += (*b)->username() + " ";

	do_write(move(string("clients: " + msg + "\n")));
}

//void do_ping()
//{
//	do_write("ping\n");
//}

//void do_ask_clients()
//{
//	do_write("ask_clients\n");
//}

void CClientSession::on_check_ping()
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
	if( (now - last_ping_).total_milliseconds() > max_timeout )
	{
		VLOG(1) << "DEBUG: stopping: " << username_ << " - no ping in time" << std::endl;
		stop();
	}
	last_ping_ = boost::posix_time::microsec_clock::local_time();
}

void CClientSession::post_check_ping()
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	timer_.expires_from_now(boost::posix_time::millisec(max_timeout));
	timer_.async_wait(boost::bind(&CClientSession::on_check_ping, shared_from_this()));
}

void CClientSession::on_write(const error_code & err, size_t bytes)
{
	do_read();
}

	CClientSession::error_code CClientSession::do_get_fibo(size_t n)
	{
		//return n<=2 ? n: get_fibo(n-1) + get_fibo(n-2);
		size_t a = 1, b = 1;
		for( int i = 3; i <= n; i++ )
		{
			size_t c = a + b;
			a = b; b = c;
		}

		boost::recursive_mutex::scoped_lock cs_;
		fibo_res.push_back(std::make_pair(n, b));

		 return boost::system::error_code(0, boost::system::generic_category());
	}

	void CClientSession::on_get_fibo(const size_t n, error_code & err)
	{
		if( err )
			return;

		if( !started() )
			return;

		boost::recursive_mutex::scoped_lock cs_;
		for( auto it : fibo_res )
		{
			if( it.first == n )
			{
				VLOG(1) << "DEBUG: fibo for: " << n << " = " << it.second << std::endl;
				do_write(move(string("fibo: " + std::to_string(it.second) + "\n")));
				break;
			}
		}

	}

	void CClientSession::on_fibo(const string && msg)
	{
		std::istringstream in(msg);
		in.ignore(5);
		size_t n; in >> n;
		CRunAsync::new_()->add(boost::bind(&CClientSession::do_get_fibo, shared_from_this(), n)
								, boost::bind(&CClientSession::on_get_fibo, shared_from_this(), n, _1)
								, io_context_);

		do_read();
	}



CClientSession::error_code CClientSession::do_ask_db(const string query, size_t queryId)
{
	wstring answer;

	// try to connect to ODBC driver
	ODBCDatabase::CDatabase db(L",");

	// if connected, send query to db
	if( db.ConnectedOk() )
		db << move(wstring(query.begin(), query.end()));

	// get answer from db
	db >> answer;


	std::wcout <<answer;

	// end answer with 'end symbol'
	answer += move(wstring(L"\n"));

	{
		boost::recursive_mutex::scoped_lock cs_;
		res.push_back(move(std::make_pair(queryId, move(answer))));
	}

	return boost::system::error_code(0, boost::system::generic_category());
}

void CClientSession::on_answer_db(const size_t queryId, error_code & err)
{
	if( err )
		return;

	if( !started() )
		return;
	
	{
		boost::recursive_mutex::scoped_lock cs_;
		for( auto & it : res )
			if( it.first == queryId )
			{
				//HERE WE SHOULD zip msg (it.second)
				do_write(move(string(it.second.begin(), it.second.end())));
				//VLOG(1) << "DEBUG: answer for: " << queryId << " = " << it.second << std::endl;
				break;
			}
	}
}

void CClientSession::on_query(const string && msg)
{
	// we must generate for each query Id to distinguish different query
	srand((unsigned)time(NULL));// I don't know, if this func threed safe!
	size_t queryId = rand();

	string query(msg.begin() + 6, msg.end());

	CRunAsync::new_()->add(boost::bind(&CClientSession::do_ask_db, shared_from_this(), query, queryId)
						   , boost::bind(&CClientSession::on_answer_db, shared_from_this(), queryId, _1)
						   , io_context_);

	do_read();
}



void CClientSession::do_read()
{
	VLOG(1) << "DEBUG: do read" << std::endl;
	async_read(sock_, buffer(read_buffer_),
				boost::bind(&CClientSession::read_complete, shared_from_this(), _1, _2),
				boost::bind(&CClientSession::on_read, shared_from_this(), _1, _2)
	);

	//post_check_ping();
}

void CClientSession::do_write(const string && msg)
{
	if( !started() )
		return;

	boost::recursive_mutex::scoped_lock lk(cs_);
	std::copy(msg.begin(), msg.end(), write_buffer_);
	sock_.async_write_some(buffer(write_buffer_, msg.size()),
							boost::bind(&CClientSession::on_write, shared_from_this(), _1, _2)
	);
}

size_t CClientSession::read_complete(const error_code & err, size_t bytes)
{
	if( err )
		return 0;

	bool found = strstr(read_buffer_, "\n") == read_buffer_ + bytes-1;
	//bool found = strstr(read_buffer_, "!e\n") == read_buffer_ + bytes - 3;
	//bool found = std::find(read_buffer_, read_buffer_ + bytes, '\n') < read_buffer_ + bytes;

	// we read one-by-one until we get to enter, no buffering
	return found ? 0 : 1;
}


void update_clients_changed()
{
	cli_ptr_vector clients_copy;
	{
		boost::recursive_mutex::scoped_lock lk(clients_cs);
		clients_copy = clients;
	}

	for( auto it : clients_copy )
		it->set_clients_changed();

	//old variant
	//for( cli_ptr_vector::iterator b = copy.begin(), e = copy.end(); b != e; ++b )
	//	(*b)->set_clients_changed();
}