#include "DiscordClient.h"
#include "Logging.h"

static const char* APPLICATION_ID = "852754886197379103";

void DiscordClient::Initialize()
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

void DiscordClient::Update(const char* state, const char* details, const char* imageKey)
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.state = state;
	discordPresence.details = details;
	discordPresence.largeImageKey = imageKey;
	Discord_UpdatePresence(&discordPresence);
	Logger::Info("Updated Discord presence: {}, {}, {}", state, details, imageKey);
}

void DiscordClient::Teardown()
{
	Discord_ClearPresence();
	Discord_Shutdown();
}
