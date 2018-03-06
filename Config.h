#ifndef _CONFIG_
#define _CONFIG_
#pragma once

class Config {
private:
	struct KeyBindings {
		std::string connectionString = "Driver={SQL Server};Server=MAXWELL;Database=StopNet4; Uid=sa; Pwd=111111;";
		int port = 65042;
		short threads = 10;
		std::string ipAdress = "127.0.0.1";
	};
public:
	KeyBindings keyBindings;

	Config();

	void updateKeyBindings();

	void saveKeyBindings() const;
};
#endif