#ifndef _CCONFIG_
#define _CCONFIG_
#pragma once

using std::wstring;
using std::string;

class CConfig {
private:

	struct KeyBindings {

		KeyBindings();

		wstring connectionString;
		string ipAdress;
		int port;
		short threads;

		wstring logDir;
		bool logToStdErr;
		bool stopLoggingIfFullDisk;
		int verbousLog;
		int minLogLevel;

		wstring serviceName;
	};

	void setLocale() const;

	void setStatusOk();

	void setStatusError();

	void updateKeyBindings();

	void saveKeyBindings();

	wstring CConfig::getConstructedNameOfLogDir() const;

	CConfig(CConfig const&) = delete;
	CConfig operator=(CConfig const&) = delete;

public:
	const char * const SETTINGS_FILE_NAME = "CS_MiniServer.ini";
	const char * const START_BATCH_FILE_NAME = "Start.bat";
	const char * const STOP_BATCH_FILE_NAME = "Stop.bat";

	enum Status{ LOADED_OK = 0, ERROR = -1 };

	KeyBindings keyBindings, defaultKeyBindings;

	CConfig();

	void Load();

	Status getStatus() const;

	void initGlog();

	void checkOrRewrightStartScript();

	void checkOrRewrightStopScript();

private:
	Status status = ERROR;
};
#endif