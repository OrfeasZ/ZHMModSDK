#include "DiscordRichPresence.h"

#include "Hooks.h"
#include "Logging.h"
#include "Glacier/ZGameLoopManager.h"

#include <Glacier/ZScene.h>
#include <regex>

#include <discord.h>

#include "simdjson.h"
#include "Functions.h"
#include "Util/StringUtils.h"
#include "Glacier/ZModule.h"

static constexpr discord::ClientId APPLICATION_ID = 852754886197379103;

DiscordRichPresence::DiscordRichPresence() :
    m_DiscordCore(nullptr) {}

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
        Logger::Error("Discord init failed with result: {}", static_cast<int>(s_DiscordCreateResult));
        m_DiscordCore = nullptr;
        return;
    }

    BuildGameModeMappings();

    Hooks::ZLevelManager_StartGame->AddDetour(this, &DiscordRichPresence::ZLevelManager_StartGame);
}

void DiscordRichPresence::OnEngineInitialized() {
    const ZMemberDelegate<DiscordRichPresence, void(const SGameUpdateEvent&)> s_Delegate(
        this, &DiscordRichPresence::OnFrameUpdate
    );
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 99999, EUpdateMode::eUpdateAlways);
}

void DiscordRichPresence::BuildSceneMappings() {
    m_CodeNameHintToSceneName.insert(std::make_pair("Boot.entity", "In Startup Screen"));
    m_CodeNameHintToSceneName.insert(std::make_pair("MainMenu.entity", "In Main Menu"));
    m_CodeNameHintToSceneName.insert(std::make_pair("Vanilla", "Safehouse"));

    const ZRuntimeResourceID s_ConfigRuntimeResourceID = ResId<"[assembly:/_pro/online/default/offlineconfig/config.contracts].pc_contracts">;
    ZResourcePtr s_ConfigResourcePtr;

    Globals::ResourceManager->GetResourcePtr(s_ConfigResourcePtr, s_ConfigRuntimeResourceID, 0);

    const ZResourceContainer::SResourceInfo& s_ConfigResourceInfo = s_ConfigResourcePtr.GetResourceInfo();

    for (size_t i = 0; i < s_ConfigResourceInfo.numReferences; ++i) {
        const uint32_t s_JsonReferenceIndex = (*Globals::ResourceContainer)->m_references[s_ConfigResourceInfo.firstReferenceIndex + i].index;
        const ZResourceContainer::SResourceInfo& s_JsonReferenceInfo = (*Globals::ResourceContainer)->m_resources[s_JsonReferenceIndex];

        // Scene path and location don't match in this json
        if (s_JsonReferenceInfo.rid == ResId<
            "[assembly:/_pro/online/default/contracts/seed/whitespider/"
            "c_ws_group_3d407b2b-e2f2-4204-9c08-7da67baa78fd.contract.json]"
            "([assembly:/_pro/online/default/offlineconfig/config.unlockables]"
            ".pc_unlockables).pc_json">) {
            continue;
        }

        ZResourcePtr s_JsonResourcePtr;

        Globals::ResourceManager->LoadResource(s_JsonResourcePtr, s_JsonReferenceInfo.rid);

        ZResourceReader* s_JsonResourceReader = *reinterpret_cast<ZResourceReader**>(s_JsonReferenceInfo.resourceData);
        ZResourceDataBuffer* s_DataBuffer = s_JsonResourceReader->m_pResourceData.m_pObject;

        if (!s_DataBuffer || !s_DataBuffer->m_pData) {
            Logger::Error("{:016x} JSON resource has no data buffer!", s_JsonReferenceInfo.rid.GetID());

            continue;
        }

        const char* s_JsonData = static_cast<const char*>(s_DataBuffer->m_pData);
        size_t s_JsonSize = s_DataBuffer->m_nSize;

        simdjson::padded_string s_PaddedJson(s_JsonData, s_JsonSize);

        simdjson::ondemand::parser s_Parser;
        auto s_Document = s_Parser.iterate(s_PaddedJson);

        auto s_ParseErrorCode = s_Document.error();

        if (s_ParseErrorCode) {
            Logger::Error("Failed to parse JSON: {}!", simdjson::error_message(s_ParseErrorCode));

            continue;
        }

        simdjson::ondemand::object s_Metadata = s_Document["Metadata"];
        const std::string_view s_CodeNameHint = s_Metadata["CodeName_Hint"];
        const std::string_view s_LocationKey = s_Metadata["Location"];
        const std::string_view s_TitleKey = s_Metadata["Title"];

        std::string s_LocationKey2 = std::format("UI_{}_CITY", s_LocationKey);
        const uint32_t s_LocationHash = Hash::Crc32(s_LocationKey2.data(), s_LocationKey2.size());

        const uint32_t s_TitleHash = Hash::Crc32(s_TitleKey.data(), s_TitleKey.size());

        ZString s_SceneName;
        int s_OutMarkupResult;

        bool s_TextFound = Hooks::ZUIText_TryGetTextFromNameHash->Call(
            Globals::UIText,
            s_LocationHash,
            s_SceneName,
            s_OutMarkupResult
        );

        if (!s_TextFound) {
            Logger::Error(
                "Missing UI text for location key: {} (Runtime Resource ID: {:016x})!",
                s_LocationKey2,
                s_JsonReferenceInfo.rid.GetID()
            );

            continue;
        }

        ZString s_Title;

        s_TextFound = Hooks::ZUIText_TryGetTextFromNameHash->Call(
            Globals::UIText,
            s_TitleHash,
            s_Title,
            s_OutMarkupResult
        );

        if (!s_TextFound) {
            Logger::Error(
                "Missing UI text for title key: {} (Runtime Resource ID: {:016x})!",
                s_TitleKey,
                s_JsonReferenceInfo.rid.GetID()
            );

            continue;
        }

        const std::string s_CodeNameHint2 { s_CodeNameHint };

        m_CodeNameHintToSceneName[s_CodeNameHint2] = std::string(s_SceneName);
        m_CodeNameHintToTitle[s_CodeNameHint2] = std::string(s_Title);
    }

    Logger::Trace("Finished building scene mappings.");
}

void DiscordRichPresence::BuildGameModeMappings() {
    m_TypeToGameMode = {
        {"sniper", "Sniper Assassin"},
        {"usercreated", "Contracts Mode"},
        {"creation", "Contracts Mode"},
        {"featured", "Featured Contract"},
        {"mission", "Mission"},
        {"flashback", "Mission"},
        {"tutorial", "Mission"},
        {"campaign", "Mission"},
        {"escalation", "Escalation"},
        {"elusive", "Elusive Target"},
        {"arcade", "Elusive Target Arcade"},
        {"evergreen", "Freelancer"},
    };

    Logger::Trace("Finished building game mode mappings.");
}

void DiscordRichPresence::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (m_DiscordCore)
        m_DiscordCore->RunCallbacks();
}

DEFINE_PLUGIN_DETOUR(DiscordRichPresence, void, ZLevelManager_StartGame, ZLevelManager* th) {
    if (!m_DiscordCore) {
        return HookResult<void>(HookAction::Continue());
    }

    SSceneInitParameters& s_SceneInitParameters = Globals::Hitman5Module->m_pEntitySceneContext->m_SceneInitParameters;

    Logger::Trace("Scene: {}", s_SceneInitParameters.m_SceneResource);
    Logger::Trace("Codename: {}", s_SceneInitParameters.m_CodeNameHint);
    Logger::Trace("Type: {}", s_SceneInitParameters.m_Type);

    std::string s_Action = "";
    std::string s_Details = "";
    std::string s_Location = "";
    std::string s_ImageKey = "logo";

    if (m_CodeNameHintToSceneName.empty()) {
        if (s_SceneInitParameters.m_SceneResource == "assembly:/_PRO/Scenes/Frontend/Boot.entity") {
            s_Location = "In Startup Screen";
        }
        else if (s_SceneInitParameters.m_SceneResource == "assembly:/_PRO/Scenes/Frontend/MainMenu.entity") {
            s_Location = "In Main Menu";

            BuildSceneMappings();
        }
    }
    else {
        std::string s_CodeNameHint = s_SceneInitParameters.m_CodeNameHint.c_str();

        if (s_CodeNameHint.empty()) {
            s_CodeNameHint = s_SceneInitParameters.m_SceneResource;
            s_CodeNameHint = s_CodeNameHint.substr(s_CodeNameHint.find_last_of("/") + 1);
        }

        s_Location = m_CodeNameHintToSceneName[s_CodeNameHint];
    }

    if (s_Location == "In Startup Screen" || s_Location == "In Main Menu") {
        s_Action = s_Location;
    }
    else {
        auto s_GameModeIt = m_TypeToGameMode.find(s_SceneInitParameters.m_Type.c_str());
        std::string s_GameMode = s_GameModeIt == m_TypeToGameMode.end() ? "ERR_UNKNOWN_GAMEMODE" : s_GameModeIt->second;

        s_Details = "Playing " + s_GameMode + " in " + s_Location;

        // Discord image key
        std::string s_LocationKey = std::regex_replace(s_Location, std::regex(" "), "-");
        s_LocationKey = std::regex_replace(s_LocationKey, std::regex("à"), "a");
        s_LocationKey = Util::StringUtils::ToLowerCase(s_LocationKey);

        s_ImageKey = "location-" + s_LocationKey;

        if (s_GameMode == "Mission" || s_GameMode == "Sniper Assassin") {
            auto s_MissionIt = m_CodeNameHintToTitle.find(s_SceneInitParameters.m_CodeNameHint.c_str());
            s_Action = s_MissionIt == m_CodeNameHintToTitle.end() ? "ERR_UNKNOWN_MISSION" : s_MissionIt->second;
            std::string s_MissionName = s_Action;

            std::string s_MissionKey = std::regex_replace(s_MissionName, std::regex(" "), "-");
            s_MissionKey = Util::StringUtils::ToLowerCase(s_MissionKey);
            s_ImageKey = "mission-" + s_MissionKey;
        }
        else {
            s_Details = "Playing " + s_GameMode;
            s_Action = s_Location;
        }
    }

    discord::Activity activity {};
    activity.SetType(discord::ActivityType::Playing);
    activity.SetState(s_Action.c_str());
    activity.SetDetails(s_Details.c_str());
    activity.GetAssets().SetLargeImage(s_ImageKey.c_str());

    m_DiscordCore->ActivityManager().UpdateActivity(
        activity, [](discord::Result p_Result) {
            Logger::Trace("Activity Manager push completed with result: {}", static_cast<int>(p_Result));
        }
    );

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(DiscordRichPresence);
