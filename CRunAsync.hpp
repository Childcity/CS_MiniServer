#pragma once
#define _WIN32_WINNT 0x0501

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <vector>

using namespace boost::asio;

struct CRunAsync: boost::enable_shared_from_this<CRunAsync>
	, private boost::noncopyable{

	typedef boost::shared_ptr<CRunAsync> ptr;

	static ptr new_() { return ptr(new CRunAsync); }

private:
	typedef boost::function<void(boost::system::error_code)> completion_func;
	typedef boost::function<boost::system::error_code()> op_func;

	CRunAsync(): started_(false){}

	struct operation{
		operation(io_service& service, op_func op, completion_func completion)
			: service(&service), op(op), completion(completion)
			, work(new io_service::work(service))
		{}

		operation()
			: service(0)
		{}

		typedef boost::shared_ptr<io_service::work> work_ptr;
		completion_func completion;
		io_service *service;
		work_ptr work;
		op_func op;
	};

	void start();

	void run();

public:
	void add(op_func op, completion_func completion, io_service& service);

	void stop();

private:
	boost::recursive_mutex cs_;
	std::vector<operation> ops_;
	bool started_;
	ptr self_;
};
