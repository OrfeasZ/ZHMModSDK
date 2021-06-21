#pragma once
#include "discord.h"
#include "Logging.h"

class DiscordClient
{
public:
	void Initialize();
	void Update(std::string state, std::string details, std::string imageKey);
	void Teardown();
private:
	discord::Core* m_core;
	discord::Result m_discordInitResult;
};
