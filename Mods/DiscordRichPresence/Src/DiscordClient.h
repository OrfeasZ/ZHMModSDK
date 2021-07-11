#pragma once
#include "discord.h"
#include "Logging.h"

class DiscordClient
{
public:
	void Initialize();
	void Update(std::string p_State, std::string p_Details, std::string p_ImageKey);
	void Teardown();
private:
	discord::Core* m_Core;
	discord::Result m_DiscordInitResult;
};
