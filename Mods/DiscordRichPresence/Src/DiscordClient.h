#pragma once
#include "discord.h"
#include "Logging.h"

class DiscordClient
{
public:
	void Initialize();
	void Update(const std::string& p_State, const std::string& p_Details, const std::string& p_ImageKey);
	void Teardown();
	void Callback();
private:
	discord::Core* m_Core;
	discord::Result m_DiscordInitResult;
};
