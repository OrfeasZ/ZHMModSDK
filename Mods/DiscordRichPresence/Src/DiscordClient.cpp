#include "DiscordClient.h"

static const discord::ClientId APPLICATION_ID = 852754886197379103;

void DiscordClient::Initialize()
{
	m_DiscordInitResult = discord::Core::Create(APPLICATION_ID, DiscordCreateFlags_NoRequireDiscord, &m_Core);
	if (m_DiscordInitResult != discord::Result::Ok)
	{
		Logger::Error("Discord init failed with result: {}", m_DiscordInitResult);
	}
}

void DiscordClient::Update(const std::string& p_State, const std::string& p_Details, const std::string& p_ImageKey)
{
	if (m_DiscordInitResult != discord::Result::Ok)
	{
		// Don't attempt to update Discord if we couldn't connect before
		return;
	}

	discord::Activity activity;
	activity.SetType(discord::ActivityType::Playing);
	activity.SetState(p_State.c_str());
	activity.SetDetails(p_Details.c_str());
	activity.GetAssets().SetLargeImage(p_ImageKey.c_str());

	m_Core->ActivityManager().UpdateActivity(activity, [](discord::Result p_Result)
	{
		Logger::Trace("Activity Manager push completed with result: {}", p_Result);
	});
}

void DiscordClient::Teardown()
{
	// no-op
}
