#include "CRunAsync.hpp"


void CRunAsync::start()
{
	{
		boost::recursive_mutex::scoped_lock lk(cs_);
		if( started_ )
			return;
		started_ = true;
	}
	boost::thread t(boost::bind(&CRunAsync::run, this));
}

void CRunAsync::run()
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


void CRunAsync::add(op_func op, completion_func completion, io_service& service)
{
	// so that we're not destroyed while async-executing something
	self_ = shared_from_this();
	boost::recursive_mutex::scoped_lock lk(cs_);
	ops_.push_back(operation(service, op, completion));

	if( !started_ )
		start();
}

void CRunAsync::stop()
{
	boost::recursive_mutex::scoped_lock lk(cs_);
	started_ = false;
	ops_.clear();
}
