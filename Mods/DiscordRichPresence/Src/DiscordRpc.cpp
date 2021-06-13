#include "DiscordRpc.h"

#include "Hooks.h"
#include "Logging.h"
#include "DiscordClient.h"

#include <Glacier/ZScene.h>
#include <regex>

DiscordClient discordClient;

DiscordRpc::~DiscordRpc()
{
	discordClient.Teardown();
}

void DiscordRpc::PreInit()
{
	discordClient.Initialize();
	PopulateScenes();
	PopulateGameModes();
	PopulateCodenameHints();
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &DiscordRpc::OnLoadScene);
}

void DiscordRpc::PopulateScenes()
{
	m_scenes = {
		// Menu
		{ "boot.entity", "In Startup Screen" },
		{ "mainmenu.entity", "In Menu" },
		// Sniper Assassin
		{ "hawk", "Himmelstein" },
		{ "salty", "Hantu Port" },
		{ "caged", "Siberia" },
		// Prologue
		{ "thefacility", "ICA Facility" },
		// HITMAN
		{ "paris", "Paris" },
		{ "coastalTown", "Sapienza" },
		{ "marrakesh", "Marrakesh" },
		{ "bangkok", "Bangkok" },
		{ "colorado", "Colorado" },
		{ "hokkaido", "Hokkaido" },
		// HITMAN 2
		{ "sheep", "Hawke's Bay" },
		{ "miami", "Miami" },
		{ "colombia", "Santa Fortuna" },
		{ "Mumbai", "Mumbai" },
		{ "skunk", "Whittleton Creek" },
		{ "theark", "Isle of Sgàil" },
		{ "greedy", "New York" },
		{ "opulent", "Haven Island" },
		// HITMAN 3
		{ "golden", "Dubai" },
		{ "ancestral", "Dartmoor" },
		{ "edgy", "Berlin" },
		{ "wet", "Chongqing" },
		{ "elegant", "Mendoza" },
		{ "trapped", "Carpathian Mountains" }
	};
	Logger::Info("Finished populating scene map");
}

void DiscordRpc::PopulateGameModes()
{
	m_gameModes = {
		{ "sniper", "Sniper Assassin" },
		{ "usercreated", "Contracts Mode" },
		{ "creation", "Contracts Mode" },
		{ "featured", "Featured Contract" },
		{ "mission", "Mission" },
		{ "flashback", "Mission" },
		{ "tutorial", "Mission" },
		{ "campaign", "Mission" },
		{ "escalation", "Escalation" },
		{ "elusive", "Elusive Target" }
	};
	Logger::Info("Finished populating game modes");
}

void DiscordRpc::PopulateCodenameHints()
{
	m_codenameHints = {
		{ "GarterSnake", "A Bitter Pill" },
		{ "Spider", "A Gilded Cage" },
		{ "Python", "A House Built On Sand" },
		{ "Cottonmouth", "A Silver Tongue" },
		{ "Skunk", "Another Life" },
		{ "Edgy Fox 4", "Apex Predator" },
		{ "Polarbear Arrival FSP", "Arrival" },
		{ "Magpie", "The Ark Society" },
		{ "Mongoose", "Chasing A Ghost" },
		{ "Tiger", "Club 27" },
		{ "Bulldog", "Death In The Family" },
		{ "Anaconda", "Embrace Of The Serpent" },
		{ "Rat", "End Of An Era" },
		{ "Flamingo", "The Finish Line" },
		{ "Bull", "Freedom Fighters" },
		{ "Polarbear Module 002_B", "Freeform Training" },
		{ "Greedy Raccoon 16", "Golden Handshake" },
		{ "Polarbear Module 002 FSP", "Guided Training" },
		{ "Mamushi", "Hokkaido Snow Festival" },
		{ "Paris Noel", "Holiday Hoarders" },
		{ "KingCobra", "Illusions Of Grandeur" },
		{ "Mamba", "Landslide" },
		{ "Sheep 9", "Nightcall" },
		{ "Golden Gecko", "On Top Of The World" },
		{ "Flu", "Patient Zero" },
		{ "Snow Crane", "Situs Inversus" },
		{ "Ebola", "The Author" },
		{ "Llama", "The Farewell" },
		{ "Polarbear Graduation", "The Final Test" },
		{ "Copperhead", "The Icon" },
		{ "Stingray", "The Last Resort" },
		{ "Peacock", "The Showstopper" },
		{ "Zika", "The Source" },
		{ "Hippo", "Three-Headed Serpent" },
		{ "Wolverine", "Untouchable" },
		{ "Rabies", "The Vector" },
		{ "Octopus", "World Of Tomorrow" },
		// Sniper Assassin
		{ "SC_Hawk", "The Last Yardbird" },
		{ "SC_Seagull", "The Pen And The Sword" },
		{ "SC_Falcon", "Crime And Punishment" }
	};
}

std::string DiscordRpc::LowercaseString(std::string in)
{
	std::string copy = in;
	std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::tolower(c); });

	return copy;
}

std::string DiscordRpc::FindLocationForScene(ZString scene)
{
	std::string lowercaseScene = LowercaseString(scene.c_str());
	for (auto& it : m_scenes)
	{
		if (lowercaseScene.find(it.first) != std::string::npos)
		{
			return it.second;
		}
	}

	return "ERR_UNKNOWN_LOCATION";
}

DECLARE_PLUGIN_DETOUR(DiscordRpc, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& sceneData)
{
	Logger::Info("Scene: {}", sceneData.m_sceneName);
	Logger::Info("Codename: {}", sceneData.m_codeNameHint);
	Logger::Info("Type: {}", sceneData.m_type);
	std::string action = "";
	std::string details = "";
	std::string location = "";
	std::string imageKey = "logo";

	location = FindLocationForScene(sceneData.m_sceneName);

	if (location == "In Startup Screen" || location == "In Menu")
	{
		action = location;
	}
	else
	{
		auto gameModeIt = m_gameModes.find(sceneData.m_type.ToStringView());
		std::string gameMode = gameModeIt == m_gameModes.end() ? "ERR_UNKNOWN_GAMEMODE" : gameModeIt->second;

		details = "Playing " + gameMode + " in " + location;

		// Discord image key
		std::string locationKey = std::regex_replace(location, std::regex(" "), "-");
		locationKey = std::regex_replace(locationKey, std::regex("à"), "a");
		locationKey = LowercaseString(locationKey);

		imageKey = "location-" + locationKey;
		if (gameMode == "Mission" || gameMode == "Sniper Assassin")
		{
			auto missionIt = m_codenameHints.find(sceneData.m_codeNameHint.ToStringView());
			action = missionIt == m_codenameHints.end() ? "ERR_UNKNOWN_MISSION" : missionIt->second;
			std::string missionName = action;

			std::string missionKey = std::regex_replace(missionName, std::regex(" "), "-");
			missionKey = LowercaseString(missionKey);
			imageKey = "mission-" + missionKey;
		}
		else
		{
			details = "Playing " + gameMode;
			action = location;
		}
	}
	

	discordClient.Update(action.c_str(), details.c_str(), imageKey.c_str());

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(DiscordRpc);
