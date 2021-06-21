#include "DiscordClient.h"

static const discord::ClientId APPLICATION_ID = 852754886197379103;

void DiscordClient::Initialize()
{
	m_discordInitResult = discord::Core::Create(APPLICATION_ID, DiscordCreateFlags_NoRequireDiscord, &m_core);
	if (m_discordInitResult != discord::Result::Ok)
	{
		Logger::Error("Discord init failed with result: {}", m_discordInitResult);
	}
}

void DiscordClient::Update(std::string state, std::string details, std::string imageKey)
{
	if (m_discordInitResult != discord::Result::Ok)
	{
		// Don't attempt to update Discord if we couldn't connect before
		return;
	}

	discord::Activity activity;
	activity.SetType(discord::ActivityType::Playing);
	activity.SetState(state.c_str());
	activity.SetDetails(details.c_str());
	activity.GetAssets().SetLargeImage(imageKey.c_str());

	m_core->ActivityManager().UpdateActivity(activity, [](discord::Result result)
	{
		//-- Don't care
		Logger::Debug("Activity Manager push completed with result: {}", result);
	});
}

void DiscordClient::Teardown()
{
	// no-op
}
