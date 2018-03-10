#include "CConfig.h"

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
}