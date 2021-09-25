#include "DiscordRpc.h"

#include "Hooks.h"
#include "Logging.h"
#include "DiscordClient.h"
#include "Glacier/ZGameLoopManager.h"

#include <Glacier/ZScene.h>
#include <regex>


DiscordRpc::~DiscordRpc()
{
	m_DiscordClient.Teardown();

	const ZMemberDelegate<DiscordRpc, void(const SGameUpdateEvent&)> s_Delegate(this, &DiscordRpc::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdatePlayMode);
}

void DiscordRpc::PreInit()
{
	m_DiscordClient.Initialize();
	PopulateScenes();
	PopulateGameModes();
	PopulateCodenameHints();
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &DiscordRpc::OnLoadScene);
}

void DiscordRpc::OnEngineInitialized()
{
	const ZMemberDelegate<DiscordRpc, void(const SGameUpdateEvent&)> s_Delegate(this, &DiscordRpc::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdatePlayMode);
}

void DiscordRpc::PopulateScenes()
{
	m_Scenes = {
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
		{ "coastaltown", "Sapienza" },
		{ "marrakesh", "Marrakesh" },
		{ "bangkok", "Bangkok" },
		{ "colorado", "Colorado" },
		{ "hokkaido", "Hokkaido" },
		// HITMAN 2
		{ "sheep", "Hawke's Bay" },
		{ "miami", "Miami" },
		{ "colombia", "Santa Fortuna" },
		{ "mumbai", "Mumbai" },
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
	Logger::Trace("Finished populating scene map");
}

void DiscordRpc::PopulateGameModes()
{
	m_GameModes = {
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
	Logger::Trace("Finished populating game modes");
}

void DiscordRpc::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	m_DiscordClient.Callback();
}

void DiscordRpc::PopulateCodenameHints()
{
	m_CodenameHints = {
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
	Logger::Trace("Finished populating codename hints");
}

std::string DiscordRpc::LowercaseString(const std::string& p_In)
{
	std::string s_Copy = p_In;
	std::transform(s_Copy.begin(), s_Copy.end(), s_Copy.begin(), [](unsigned char p_C) { return std::tolower(p_C); });

	return s_Copy;
}

std::string DiscordRpc::FindLocationForScene(ZString p_Scene)
{
	std::string s_LowercaseScene = LowercaseString(p_Scene.c_str());
	for (auto& s_It : m_Scenes)
	{
		if (s_LowercaseScene.find(s_It.first) != std::string::npos)
		{
			return s_It.second;
		}
	}

	return "ERR_UNKNOWN_LOCATION";
}

DECLARE_PLUGIN_DETOUR(DiscordRpc, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& sceneData)
{
	Logger::Trace("Scene: {}", sceneData.m_sceneName);
	Logger::Trace("Codename: {}", sceneData.m_codeNameHint);
	Logger::Trace("Type: {}", sceneData.m_type);
	std::string s_Action = "";
	std::string s_Details = "";
	std::string s_Location = "";
	std::string s_ImageKey = "logo";

	s_Location = FindLocationForScene(sceneData.m_sceneName);

	if (s_Location == "In Startup Screen" || s_Location == "In Menu")
	{
		s_Action = s_Location;
	}
	else
	{
		auto s_GameModeIt = m_GameModes.find(sceneData.m_type.ToStringView());
		std::string s_GameMode = s_GameModeIt == m_GameModes.end() ? "ERR_UNKNOWN_GAMEMODE" : s_GameModeIt->second;

		s_Details = "Playing " + s_GameMode + " in " + s_Location;

		// Discord image key
		std::string s_LocationKey = std::regex_replace(s_Location, std::regex(" "), "-");
		s_LocationKey = std::regex_replace(s_LocationKey, std::regex("à"), "a");
		s_LocationKey = LowercaseString(s_LocationKey);

		s_ImageKey = "location-" + s_LocationKey;
		if (s_GameMode == "Mission" || s_GameMode == "Sniper Assassin")
		{
			auto s_MissionIt = m_CodenameHints.find(sceneData.m_codeNameHint.ToStringView());
			s_Action = s_MissionIt == m_CodenameHints.end() ? "ERR_UNKNOWN_MISSION" : s_MissionIt->second;
			std::string s_MissionName = s_Action;

			std::string s_MissionKey = std::regex_replace(s_MissionName, std::regex(" "), "-");
			s_MissionKey = LowercaseString(s_MissionKey);
			s_ImageKey = "mission-" + s_MissionKey;
		}
		else
		{
			s_Details = "Playing " + s_GameMode;
			s_Action = s_Location;
		}
	}
	

	m_DiscordClient.Update(s_Action, s_Details, s_ImageKey);

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(DiscordRpc);
