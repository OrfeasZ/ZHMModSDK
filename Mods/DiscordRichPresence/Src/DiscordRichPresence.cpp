#include "DiscordRichPresence.h"

#include <regex>

#include <discord.h>

#include <Glacier/ZModule.h>
#include <Glacier/ZScene.h>
#include <Glacier/ZContract.h>
#include <Glacier/ZGameLoopManager.h>

#include "Hooks.h"
#include "Logging.h"
#include "Util/StringUtils.h"

const std::unordered_map<std::string, std::string> DiscordRichPresence::m_TypeToGameMode = {
    {"sniper", "Sniper assassin"},
    {"usercreated", "Contracts mode"},
    {"creation", "Contracts mode"},
    {"featured", "Featured contract"},
    {"mission", "Mission"},
    {"flashback", "Mission"},
    {"tutorial", "Mission"},
    {"campaign", "Mission"},
    {"escalation", "Escalation"},
    {"elusive", "Elusive target"},
    {"arcade", "Elusive target arcade"},
    {"evergreen", "Freelancer"},
};

static constexpr discord::ClientId APPLICATION_ID = 852754886197379103;

DiscordRichPresence::DiscordRichPresence() :
    m_DiscordCore(nullptr) {
}

DiscordRichPresence::~DiscordRichPresence() {
    const ZMemberDelegate<DiscordRichPresence, void(const SGameUpdateEvent&)> s_Delegate(
        this, &DiscordRichPresence::OnFrameUpdate
    );
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdateAlways);

    if (m_DiscordCore)
        delete m_DiscordCore;
}

void DiscordRichPresence::Init() {
    const auto s_DiscordCreateResult = discord::Core::Create(
        APPLICATION_ID, DiscordCreateFlags_NoRequireDiscord, &m_DiscordCore
    );

    if (s_DiscordCreateResult != discord::Result::Ok) {
        Logger::Error("[DiscordRichPresence] Discord init failed with result: {}", static_cast<int>(s_DiscordCreateResult));
        m_DiscordCore = nullptr;
        return;
    }

    Hooks::ZLevelManager_StartGame->AddDetour(this, &DiscordRichPresence::ZLevelManager_StartGame);
}

void DiscordRichPresence::OnEngineInitialized() {
    const ZMemberDelegate<DiscordRichPresence, void(const SGameUpdateEvent&)> s_Delegate(
        this, &DiscordRichPresence::OnFrameUpdate
    );
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdateAlways);
}

void DiscordRichPresence::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (m_DiscordCore)
        m_DiscordCore->RunCallbacks();
}

std::string DiscordRichPresence::NormalizeAssetKey(std::string p_Name) {
    Util::StringUtils::ReplaceAll(p_Name, "Ã ", "a");

    Util::StringUtils::ReplaceAll(p_Name, " ", "-");

    return Util::StringUtils::ToLowerCase(p_Name);
}

std::string DiscordRichPresence::GetActivityImageKey(
    const std::string& p_GameMode,
    std::string p_Location,
    std::string p_Title
) {
    if (p_GameMode == "Mission" || p_GameMode == "Sniper assassin" ||
        p_GameMode == "Elusive target" || p_GameMode == "Elusive target arcade") {
        if (const size_t s_Position = p_Title.rfind(" - Level"); s_Position != std::string::npos) {
            p_Title.erase(s_Position);
        }

        if (const size_t s_Position = p_Title.find(" ("); s_Position != std::string::npos) {
            p_Title.erase(s_Position);
        }

        if (const size_t s_Position = p_Title.find(" - Year"); s_Position != std::string::npos) {
            p_Title.erase(s_Position);
        }

        const char* s_Prefix =
            p_GameMode == "Elusive target" ? "elusive-" :
            p_GameMode == "Elusive target arcade" ? "arcade-" :
            "mission-";

        return std::string(s_Prefix) + NormalizeAssetKey(std::move(p_Title));
    }

    return "location-" + NormalizeAssetKey(std::move(p_Location));
}

DEFINE_PLUGIN_DETOUR(DiscordRichPresence, void, ZLevelManager_StartGame, ZLevelManager* th) {
    if (!m_DiscordCore) {
        return HookResult<void>(HookAction::Continue());
    }

    SSceneInitParameters& s_SceneInitParameters = Globals::Hitman5Module->m_pEntitySceneContext->m_SceneInitParameters;

    Logger::Trace("[DiscordRichPresence] Scene: {}", s_SceneInitParameters.m_SceneResource);
    Logger::Trace("[DiscordRichPresence] Codename: {}", s_SceneInitParameters.m_CodeNameHint);
    Logger::Trace("[DiscordRichPresence] Type: {}", s_SceneInitParameters.m_Type);

    ZString s_Location;
    ZString s_Title;

    if (!Globals::ContractsManager->m_contractContext.m_sLocationId.IsEmpty()) {
        std::string s_LocationKey = std::format("UI_{}_CITY", Globals::ContractsManager->m_contractContext.m_sLocationId.c_str());
        const uint32_t s_LocationHash = Hash::Crc32(s_LocationKey.data(), s_LocationKey.size());

        ZString s_SceneName;
        int s_OutMarkupResult;

        bool s_TextFound = Hooks::ZUIText_TryGetTextFromNameHash->Call(
            Globals::UIText,
            s_LocationHash,
            s_SceneName,
            s_OutMarkupResult
        );

        if (s_TextFound) {
            s_Location = s_SceneName;
        }
        else {
            s_Location = "ERR_UNKNOWN_LOCATION";

            Logger::Error("[DiscordRichPresence] Missing UI text for location key: {}!", s_LocationKey);
        }

        auto* s_Entries = Globals::ContractsManager->m_contractContext.m_contractData.As<TArray<SDynamicObjectKeyValuePair>>();

        if (s_Entries) {
            ZString s_SceneTitle;

            for (auto& s_Entry : *s_Entries) {
                if (s_Entry.sKey != "Metadata") {
                    continue;
                }

                auto* s_Metadata = s_Entry.value.As<TArray<SDynamicObjectKeyValuePair>>();

                if (!s_Metadata) {
                    break;
                }

                for (const auto& s_MetadataEntry : *s_Metadata) {
                    if (s_MetadataEntry.sKey == "Title") {
                        ZString* s_TitleKey = s_MetadataEntry.value.As<ZString>();
                        const uint32_t s_TitleHash = Hash::Crc32(s_TitleKey->c_str(), s_TitleKey->size());

                        s_TextFound = Hooks::ZUIText_TryGetTextFromNameHash->Call(
                            Globals::UIText,
                            s_TitleHash,
                            s_SceneTitle,
                            s_OutMarkupResult
                        );

                        if (s_TextFound) {
                            s_Title = s_SceneTitle;
                        }
                        else {
                            s_Title = "ERR_UNKNOWN_MISSION";

                            Logger::Error("[DiscordRichPresence] Missing UI text for title key: {}!", s_TitleKey->c_str());
                        }

                        break;
                    }
                }

                break;
            }
        }
    }
    else if (s_SceneInitParameters.m_SceneResource == "assembly:/_PRO/Scenes/Frontend/Boot.entity") {
        s_Location = "In startup screen";
    }
    else if (s_SceneInitParameters.m_SceneResource == "assembly:/_PRO/Scenes/Frontend/MainMenu.entity") {
        s_Location = "In main menu";
    }

    std::string s_Action;
    std::string s_Details;
    std::string s_ImageKey;

    if (s_Location == "In startup screen" || s_Location == "In main menu") {
        s_Action = s_Location;
        s_ImageKey = "logo";
    }
    else {
        auto s_GameModeIt = m_TypeToGameMode.find(s_SceneInitParameters.m_Type.c_str());
        std::string s_GameMode = s_GameModeIt == m_TypeToGameMode.end() ? "ERR_UNKNOWN_GAMEMODE" : s_GameModeIt->second;

        s_Action = s_Title.c_str();
        s_Details = "Playing " + s_GameMode + " in " + s_Location.c_str();

        s_ImageKey = GetActivityImageKey(
            s_GameMode,
            s_Location.c_str(),
            s_Title.c_str()
        );
    }

    discord::Activity activity{};
    activity.SetType(discord::ActivityType::Playing);
    activity.SetState(s_Action.c_str());
    activity.SetDetails(s_Details.c_str());
    activity.GetAssets().SetLargeImage(s_ImageKey.c_str());

    m_DiscordCore->ActivityManager().UpdateActivity(
        activity, [](discord::Result p_Result) {
        Logger::Trace("[DiscordRichPresence] Activity manager push completed with result: {}", static_cast<int>(p_Result));
    }
    );

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(DiscordRichPresence);
