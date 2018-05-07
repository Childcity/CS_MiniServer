#include "CConfig.h"
#include "Utils.h"

using samilton::INIWriter;

CConfig::KeyBindings::KeyBindings()
{
	connectionString = L"Driver={SQL Server};Server=MAXWELL;Database=StopNet4; Uid=sa; Pwd=111111;";
	ipAdress = "127.0.0.1";
	port = 65042;
	threads = 10;

	logDir = GetCurrentFolderPath() + L"\\logs";
	logToStdErr = false;
	stopLoggingIfFullDisk = true;
	verbousLog = 0;
	minLogLevel = 0;

	serviceName = L"CS_MiniServerSvc";
};

CConfig::CConfig(){}

void CConfig::Load()
{
	setLocale();
	updateKeyBindings();
	initGlog();
	checkOrRewrightStartScript();
	checkOrRewrightStopScript();
}

CConfig::Status CConfig::getStatus() const
{
	return status;
}

void CConfig::setLocale() const
{
	//std::locale cp1251_locale("ru_RU.CP866");
	//std::locale::global(cp1251_locale);
	setlocale(LC_CTYPE, "");
}

void CConfig::setStatusOk()
{
	status = LOADED_OK;
}

void CConfig::setStatusError()
{
	status = ERROR;
}

wstring CConfig::getConstructedNameOfLogDir() const
{
	std::time_t t = std::time(0);   // get time now
	std::tm *now = std::localtime(&t);

	std::wostringstream nameStream;
	nameStream << (now->tm_year + 1900)
		<< (now->tm_mon + 1)
		<< now->tm_mday << "-"
		<< now->tm_hour <<now->tm_min <<now->tm_sec <<'.';

	return wstring(nameStream.str());
}

void CConfig::initGlog()
{
	//Init Glog
	if( LOADED_OK == getStatus() )
	{
		wstring newFolder = keyBindings.logDir + L"\\" + getConstructedNameOfLogDir();

		fLS::FLAGS_log_dir = ConverterUTF8_UTF16<wstring, string>(newFolder);
		FLAGS_logtostderr = keyBindings.logToStdErr;
		FLAGS_stop_logging_if_full_disk = keyBindings.stopLoggingIfFullDisk;
		FLAGS_v = keyBindings.verbousLog;
		FLAGS_minloglevel = keyBindings.minLogLevel;

		CreateDirectoryW(keyBindings.logDir.c_str(), NULL);
		CreateDirectoryW(newFolder.c_str(), NULL);
	} else {
		wstring newFolder = defaultKeyBindings.logDir + L"\\" + getConstructedNameOfLogDir();

		fLS::FLAGS_log_dir = ConverterUTF8_UTF16<wstring, string>(newFolder);
		FLAGS_logtostderr = defaultKeyBindings.logToStdErr;
		FLAGS_stop_logging_if_full_disk = defaultKeyBindings.stopLoggingIfFullDisk;
		FLAGS_v = defaultKeyBindings.verbousLog;
		FLAGS_minloglevel = defaultKeyBindings.minLogLevel;

		CreateDirectoryW(defaultKeyBindings.logDir.c_str(), NULL);
		CreateDirectoryW(newFolder.c_str(), NULL);
	}


	google::InitGoogleLogging(GetExeName());
}

void CConfig::updateKeyBindings() {
	string pathToSettings = ConverterUTF8_UTF16<wstring, string>(GetCurrentFolderPath()) +"\\" + SETTINGS_FILE_NAME;
	INIReader settings(pathToSettings);

	if (settings.ParseError() < 0) {
		//!!! This log massage go to stderr ONLY, because GLOG is not initialized yet !
		LOG(WARNING) << "Can't load '" << pathToSettings << "', creating default bindings";
		saveKeyBindings();
	} else {
		//Server settings
		keyBindings.port = settings.GetInteger("ServerSettings", "Port", -1L);
		keyBindings.threads = static_cast<short>(settings.GetInteger("ServerSettings", "Threads", -1L));
		keyBindings.ipAdress = settings.Get("ServerSettings", "IpAdress", "0");	
		//DB settings
		keyBindings.connectionString = ConverterUTF8_UTF16<string, wstring>(settings.Get("DatabaseSettings", "ConnectionString", "_a"));
		//Log settings
		keyBindings.logDir = ConverterUTF8_UTF16<string, wstring>(settings.Get("LogSettings", "LogDir", "_a"));
		keyBindings.logToStdErr = settings.GetBoolean("LogSettings", "LogToStdErr", 0L);
		keyBindings.stopLoggingIfFullDisk = settings.GetBoolean("LogSettings", "StopLoggingIfFullDisk", 1L);
		keyBindings.verbousLog = settings.GetInteger("LogSettings", "DeepLogging", 0L);
		keyBindings.minLogLevel = settings.GetInteger("LogSettings", "MinLogLevel", 0L);
		keyBindings.serviceName = ConverterUTF8_UTF16<string, wstring>(settings.Get("LogSettings", "LogDir", "_a"));

		if (keyBindings.port == -1L || keyBindings.threads == -1L || keyBindings.ipAdress == "0" || keyBindings.connectionString == L"_a"
			|| keyBindings.logDir == L"_a" || keyBindings.serviceName == L"_a" ) {
			//!!! This log massage go to stderr ONLY, because GLOG is not initialized yet !
			LOG(WARNING) << "Format of settings is not correct. Creating settings by default";
			saveKeyBindings();
			return;
		}

		if( keyBindings.logDir == L"" )
		{
			keyBindings.logDir = defaultKeyBindings.logDir;
		}

		if( keyBindings.serviceName == L"" )
		{
			keyBindings.serviceName = defaultKeyBindings.serviceName;
		}

		//If we |here|, settings loaded correctly and we can continue
		setStatusOk();
	}
}

void CConfig::saveKeyBindings() {

	setStatusError();

	INIWriter settings(INIWriter::INIcommentType::windowsType, true);

	//Server settings
	settings["ServerSettings"]["Port"] = defaultKeyBindings.port;
	settings["ServerSettings"]["Threads"] = defaultKeyBindings.threads;
	settings["ServerSettings"]["IpAdress"] = defaultKeyBindings.ipAdress;
	//DB settings
	settings["DatabaseSettings"]["ConnectionString"] = ConverterUTF8_UTF16<wstring, string>(defaultKeyBindings.connectionString);
	//Log settings
	settings["LogSettings"]["LogDir"] = ConverterUTF8_UTF16<wstring, string>(defaultKeyBindings.logDir);
	settings["LogSettings"]["LogToStdErr"] = defaultKeyBindings.logToStdErr;
	settings["LogSettings"]["StopLoggingIfFullDisk "] = defaultKeyBindings.stopLoggingIfFullDisk;
	settings["LogSettings"]["DeepLogging"] = defaultKeyBindings.verbousLog;
	settings["LogSettings"]["MinLogLevel"] = defaultKeyBindings.minLogLevel;
	settings["LogSettings"]["ServiceName"] = ConverterUTF8_UTF16<wstring, string>(defaultKeyBindings.serviceName);


	string pathToSettings = ConverterUTF8_UTF16<wstring, string>(GetCurrentFolderPath()) + "\\" + SETTINGS_FILE_NAME;
	std::ofstream file(pathToSettings, std::ios::trunc);

	file << settings << std::endl
		<< "\n; *** Possible variants of connection string are: ***\n" 
		"; Standard security\n"
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
		"; Driver = { SQL Server Native Client 11.0 }; Server = myServerAddress; Failover_Partner = myMirrorServerAddress; Database = myDataBase; Trusted_Connection = yes;\n\n"
		"; *** Log parameters ***\n"
		"; Log lines have this form:\n"
		"; \n"
		"; Lmmdd hh : mm:ss.uuuuuu threadid file : line] msg...\n"
		"; \n"
		"; where the fields are defined as follows :\n"
		"; \n"
		"; L                A single character, representing the log level\n"
		";                     (eg 'I' for INFO)\n"
		"; mm               The month(zero padded; ie May is '05')\n"
		"; dd               The day(zero padded)\n"
		"; hh:mm:ss.uuuuuu  Time in hours, minutes and fractional seconds\n"
		"; threadid         The space - padded thread ID as returned by GetTID()\n"
		";                     (this matches the PID on Linux)\n"
		"; file             The file name\n"
		"; line             The line number\n"
		"; msg              The user - supplied message\n"
		"; \n"
		"; Example:\n"
		"; \n"
		"; I1103 11 : 57 : 31.739339 24395 google.cc : 2341] Command line : . / some_prog\n"
		"; I1103 11 : 57 : 31.739403 24395 google.cc : 2342] Process id 24395\n"
		"; \n"
		"; NOTE: although the microseconds are useful for comparing events on\n"
		"; a single machine, clocks on different machines may not be well\n"
		"; synchronized.Hence, use caution when comparing the low bits of\n"
		"; timestamps from different machines.\n"
		";\n"
		"; Log messages to stderr(console)\t instead of logfiles\n"
		"; LogToStdErr = false\n"
		";\n"
		"; This is ierarchy of errors: FATAL<-WARNINGS<-INFO\n"
		"; Log message at or above this level. Default 0, that represent INFO and above\n"
		"; MinLogLevel = 0\n"
		";\n"
		"; Deep log for debuging. 0 = off, 1 = on\n"
		"; DeepLogging = 0\n"
		"\n"
		"; Log folder. Default windows tmp directory\n"
		"; LogDir = D:\\logs\n";

	//!!! This log massage go to stderr ONLY, because GLOG is not initialized yet !
	LOG(WARNING) << "Default settings saved to'" << pathToSettings << "'";
	file.close();
}

void CConfig::checkOrRewrightStartScript() {

	string pathToBatch = ConverterUTF8_UTF16<wstring, string>(GetCurrentFolderPath()) + "\\" + START_BATCH_FILE_NAME;
	std::ifstream checkedFile(pathToBatch);

	if( checkedFile.good() )
	{
		if(ERROR != getStatus())
			setStatusOk();

		checkedFile.close();
		return;
	}

	checkedFile.close();

	setStatusError();

	LOG(WARNING) << "'" << pathToBatch << "' doesn't exist or broken, creating default version of it" ;
	std::ofstream file(pathToBatch, std::ios::trunc);

	file << "@echo off\n\n"
		"::Start server as service\n"
		"sc create CS_MiniServerSvc binpath=%~dp0CS_MiniServer.exe\n"
		"sc description CS_MiniServerSvc \"Service for CS_MiniServer\"\n"
		"sc start CS_MiniServerSvc\n"
		"pause\n";

	file.close();

	LOG(WARNING) << "Saved '" << pathToBatch << "', Please, start server with '" << pathToBatch <<"'";
}

void CConfig::checkOrRewrightStopScript()
{

	string pathToBatch = ConverterUTF8_UTF16<wstring, string>(GetCurrentFolderPath()) + "\\" + STOP_BATCH_FILE_NAME;
	std::ifstream checkedFile(pathToBatch);

	if( checkedFile.good() )
	{
		if( ERROR != getStatus() )
			setStatusOk();

		checkedFile.close();
		return;
	}

	checkedFile.close();

	setStatusError();

	LOG(INFO) << "'" << pathToBatch << "' doesn't exist or broken, creating default version of it";
	std::ofstream file(pathToBatch, std::ios::trunc);

	file << "@echo off\n\n"
		"::Stop server\n"
		"sc stop CS_MiniServerSvc\n"
		"sc delete CS_MiniServerSvc\n";

	file.close();

	LOG(WARNING) << "Saved '" << pathToBatch << "', Please, start server with '" << "<start script>" << "'";
}