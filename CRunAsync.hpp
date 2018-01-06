#pragma once
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

	CRunAsync()
		: started_(false) 
		{}

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

	void start()
	{
		{
			boost::recursive_mutex::scoped_lock lk(cs_);
			if( started_ )
				return;
			started_ = true;
		}
		boost::thread t(boost::bind(&CRunAsync::run, this));
	}

	void run()
	{
		while( true )
		{
			{
				boost::recursive_mutex::scoped_lock lk(cs_);
				if( !started_ ) 
					break;
			}
			boost::this_thread::sleep(boost::posix_time::millisec(10));
			operation cur;
			{
				boost::recursive_mutex::scoped_lock lk(cs_);
				if( !ops_.empty() )
				{
					cur = ops_[0];
					ops_.erase(ops_.begin());
				}
			}
			if( cur.service )
			{
				boost::system::error_code err = cur.op();
				cur.service->post(boost::bind(cur.completion, err));
			}
		}
		self_.reset();
	}

public:
	void add(op_func op, completion_func completion, io_service& service)
	{
		// so that we're not destroyed while async-executing something
		self_ = shared_from_this();
		boost::recursive_mutex::scoped_lock lk(cs_);
		ops_.push_back(operation(service, op, completion));

		if( !started_ ) 
			start();
	}

	void stop()
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		started_ = false;
		ops_.clear();
	}

private:
	boost::recursive_mutex cs_;
	std::vector<operation> ops_;
	bool started_;
	ptr self_;
};
