#include "Config.h"

using INIWriter = samilton::INIWriter;

Config::Config() {
	updateKeyBindings();
}

void Config::updateKeyBindings() {
	INIReader settings("CS_MiniServer.ini");

	if (settings.ParseError() < 0) {
		LOG(INFO) << "Can't load 'settings.ini', creating and using default bindings" << std::endl;
		saveKeyBindings();
	}
	else {
		keyBindings.port = settings.GetInteger("ServerSettings", "Port", -1L);
		keyBindings.threads = static_cast<short>(settings.GetInteger("ServerSettings", "Threads", -1L));
		keyBindings.ipAdress = settings.Get("ServerSettings", "IpAdress", "0");
		keyBindings.connectionString = settings.Get("DatabaseSettings", "ConnectionString", "a");

		if (keyBindings.port == -1 || keyBindings.threads == -1 || keyBindings.ipAdress == "a" || keyBindings.connectionString == "0" ) {
			LOG(WARNING) << "Bad bindings, creating and using default bindings" << std::endl;
			saveKeyBindings();
		}
	}
}

void Config::saveKeyBindings() const {
	INIWriter settings(INIWriter::INIcommentType::windowsType, true);

	settings["ServerSettings"]["Port"] = keyBindings.port;
	settings["ServerSettings"]["Threads"] = keyBindings.threads;
	settings["ServerSettings"]["IpAdress"] = keyBindings.ipAdress;
	settings["DatabaseSettings"]["ConnectionString"] = keyBindings.connectionString;

	std::ofstream file("CS_MiniServer.ini", std::ios::trunc);

	file << settings << std::endl
		<< "; Standard security\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerAddress; Database = myDataBase; Uid = myUsername; Pwd = myPassword;\n\n"
		"; Trusted Connection\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerAddress; Database = myDataBase; Trusted_Connection = yes;\n\n"
		"; Connecting to an SQL Server instance\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerName\\theInstanceName; Database = myDataBase; Trusted_Connection = yes;\n\n"
		"; Enable MARS\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerAddress; Database = myDataBase; Trusted_Connection = yes; MARS_Connection = yes;\n\n"
		"; Encrypt data sent over network\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerAddress; Database = myDataBase; Trusted_Connection = yes; Encrypt = yes;\n\n"
		"; Attach a database file on connect to a local SQL Server Express instance\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = .\\SQLExpress; AttachDbFilename = c:\\asd\\qwe\\mydbfile.mdf; Database = dbname; Trusted_Connection = Yes;\n\n"
		"; Attach a database file, located in the data directory, on connect to a local SQL Server Express instance\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = .\\SQLExpress; AttachDbFilename = | DataDirectory | mydbfile.mdf; Database = dbname; Trusted_Connection = Yes;\n\n"
		"; Database mirroring\n"
		"; If you connect with ADO.NET or the SQL Native Client to a database that is being mirrored,\n"
		"; your application can take advantage of the drivers ability to automatically redirect\n"
		"; connections when a database mirroring failover occurs.\n"
		"; You must specify the initial principal server and database\n"
		"; in the connection string and the failover partner server.\n\n"
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerAddress; Failover_Partner = myMirrorServerAddress; Database = myDataBase; Trusted_Connection = yes;\n\n";

	LOG(INFO) << "Saved 'settings.ini'" << std::endl;

	file.close();
}

void Config::initBatFile() {
	std::ifstream checkedFile("Start.bat");
	if (checkedFile.good())
		return;

	LOG(INFO) << "'Start.bat' doesn't exist or broken, creating and using default version of it" << std::endl;
	std::ofstream file("Start.bat", std::ios::trunc);

	file << "@echo off\n"
		"::Log lines have this form:\n"
		"::\n"
		"::Lmmdd hh : mm:ss.uuuuuu threadid file : line] msg...\n"
		"::\n"
		"::where the fields are defined as follows :\n"
		"::\n"
		"::L                A single character, representing the log level\n"
		"::                    (eg 'I' for INFO)\n"
		"::mm               The month(zero padded; ie May is '05')\n"
		"::dd               The day(zero padded)\n"
		"::hh:mm:ss.uuuuuu  Time in hours, minutes and fractional seconds\n"
		"::threadid         The space - padded thread ID as returned by GetTID()\n"
		"::                    (this matches the PID on Linux)\n"
		"::file             The file name\n"
		"::line             The line number\n"
		"::msg              The user - supplied message\n"
		"::\n"
		"::Example:\n"
		"::\n"
		"::I1103 11 : 57 : 31.739339 24395 google.cc : 2341] Command line : . / some_prog\n"
		"::I1103 11 : 57 : 31.739403 24395 google.cc : 2342] Process id 24395\n"
		"::\n"
		"::NOTE: although the microseconds are useful for comparing events on\n"
		"::a single machine, clocks on different machines may not be well\n"
		"::synchronized.Hence, use caution when comparing the low bits of\n"
		"::timestamps from different machines.\n\n"
		"::Log messages to stderr(console)	 instead of logfiles\n"
		"set GLOG_logtostderr=false\n\n"
		"::FATAL<-WARNINGS<-INFO\n"
		"::Log message at or above this level.Default 0, that represent INFO and above\n"
		"set GLOG_minloglevel=0\n\n"
		"::Deep log for debuging. 0 = off, 1 = on\n"
		"set GLOG_v=0\n\n"
		"::Log folder.Default windows tmp directory\n"
		"set GLOG_log_dir=logs\n\n"
		"if not exist %GLOG_log_dir% mkdir %GLOG_log_dir%\n"
		"if %errorlevel% NEQ 0 pause & exit /b 1\n\n"
		"::Start server\n"
		"start /wait /b CS_MiniServer.exe\n"
		"exit /b 0\n";

	LOG(INFO) << "Saved 'Start.bat'" << std::endl;

	file.close();
}