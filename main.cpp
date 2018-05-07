#pragma once
#include "main.h"
#include "CDatabase.h"
#include "Service.h"
#include "Utils.h"
#include "CServer.h"
#include "CConfig.h"

using std::exception;

WCHAR ConectionString[512];
HWND hWnd;
static int running_from_service = 0;

int wmain()
{
	static CConfig cfg;

	if( ! running_from_service )
	{
		cfg.Load();
		LOG_IF(FATAL, CConfig::Status::ERROR == cfg.getStatus()) <<"An error was, while reading settings or start/stop script does not exist." ;

		running_from_service = 1;
		if( service_register((LPWSTR)cfg.keyBindings.serviceName.c_str()) )
		{
			VLOG(1) << "DEBUG: We've been called as a service. Register service and exit this thread.";
			/* We've been called as a service. Register service
			* and exit this thread. main() would be called from
			* service.c next time.
			*
			* Note that if service_register() succeedes it does
			* not return until the service is stopped.
			* That is why we should set running_from_service
			* before calling service_register and unset it
			* afterwards.
			*/
			return 0;
		}

		LOG(INFO) <<"Started as console application.";

		running_from_service = 0;
	}

	try
	{
		boost::asio::io_context io_context;

		ZeroMemory(ConectionString, sizeof(ConectionString));

		wmemcpy_s(ConectionString, 
			sizeof(ConectionString), cfg.keyBindings.connectionString.c_str(), 
			cfg.keyBindings.connectionString.size());


		hWnd = GetDesktopWindow(); // need for connection to ODBC driver

		// try connect to db, to see if everything is ok with connection string
		// and db is well configured

		// try to connect to ODBC driver
		auto db = new ODBCDatabase::CDatabase();

		// if connected, ok, if not - exit with exeption
		if( db->ConnectedOk() )
		{
			LOG(INFO) << "Connection to db was success";
			delete db;
		} else {
			LOG(FATAL) << "Can't connect to db. Check connection string in configuration file";
		}

		if(cfg.keyBindings.ipAdress.empty())
			CServer Server(io_context, cfg.keyBindings.port, cfg.keyBindings.threads);
		else
			CServer Server(io_context, cfg.keyBindings.ipAdress, cfg.keyBindings.port, cfg.keyBindings.threads);

	} catch(exception& e) {
		LOG(FATAL) << "Server has been crashed: " << e.what() << std::endl;
	}

	return 0;
}

void SafeExit()
{
	LOG(INFO) <<"Server stopped safely.";

	//We just exit from program. All connections wrapped in shared_ptr, so they will be closed soon
	//We don't need to watch them

	google::ShutdownGoogleLogging();
}