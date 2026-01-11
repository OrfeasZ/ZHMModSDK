#include "TitaniumBullets.h"

#include <Hooks.h>
#include <Logging.h>
#include <Globals.h>

#include <Glacier/CompileReflection.h>
#include <Glacier/EUpdateMode.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZModule.h>

#include <IconsMaterialDesign.h>

#include <algorithm>
#include <array>
#include <string_view>

/**
 * TitaniumBullets - Repository Patch
 *
 * The SMF TitaniumBullets mod works by patching entries in `pro.repo` so that ammo uses an
 * ammo config with penetration enabled. This plugin replicates that behaviour at runtime by
 * editing the in-memory repository objects (and restoring them when disabled).
 */

namespace {
static constexpr const char kPenetrationAmmoConfigIdStr[] = "87ae0524-2f22-4fe0-82e1-84a050b43cf0";
static const ZRepositoryID kPenetrationAmmoConfigId = ZRepositoryID(kPenetrationAmmoConfigIdStr);

// Entries patched by SMF's TitaniumBullets.repository.json.
static const std::array<ZRepositoryID, 20> kTargets = {
    ZRepositoryID("06f7302c-60f5-41d6-9c5f-6f0659efeea4"),
    ZRepositoryID("1064bfbe-61d0-40e6-ac05-4a2c210a6e13"),
    ZRepositoryID("1d43a4aa-1bdb-4318-b97f-ebb2427d63cf"),
    ZRepositoryID("270de950-858a-46cb-9c84-4007a5f914fc"),
    ZRepositoryID("298cc6ab-41fc-475c-a9a7-afdb79d69017"),
    ZRepositoryID("2d9d14aa-ca9d-4393-a793-8b0412eb0176"),
    ZRepositoryID("357e6077-6ed0-4f5a-955a-bdf1331c3ecf"),
    ZRepositoryID("38b41bbb-2bb3-4e83-9ccd-86a806f8cc4c"),
    ZRepositoryID("57564829-af0a-4369-af40-f5943d3c1b6a"),
    ZRepositoryID("a25320e6-fd52-4c74-92f6-61f9fc157fc9"),
    ZRepositoryID("a29c35c2-526f-447e-b6ae-b15ad64bba67"),
    ZRepositoryID("c86f2315-ac67-4df1-bdc2-d18e45e0506e"),
    ZRepositoryID("d6ad5fd8-e673-4852-8062-3d790fb2b1d8"),
    ZRepositoryID("de4a2ec0-8fdf-488e-a564-2610383cf2c3"),
    ZRepositoryID("e1f41c79-bbd6-4fd1-b2df-30628d39c767"),
    ZRepositoryID("e5e70578-d49b-446c-900b-5ccbf3c1985f"),
    ZRepositoryID("e6dfcbaa-10bc-454f-a793-4a72e7db4241"),
    ZRepositoryID("eb374e6a-5ebc-4452-8b07-65516139f3e8"),
    ZRepositoryID("f9886fb6-8b04-4e23-b7e9-0eb16aa34057"),
    ZRepositoryID("fb1ed921-817d-4ced-9e0d-85743fb23aaa"),
};

bool IsTargetId(const ZRepositoryID& p_Id) {
    return std::find(kTargets.begin(), kTargets.end(), p_Id) != kTargets.end();
}

static const ZString kIdKey = ZString(std::string_view("ID_"));
static const ZString kAmmoConfigKey = ZString(std::string_view("AmmoConfig"));

ZRepositoryID GetRepoIdFromDynamicObjectOrDefault(const ZDynamicObject& p_Value) {
    if (const auto s_Repo = p_Value.As<ZRepositoryID>()) {
        return *s_Repo;
    }

    if (const auto s_Str = p_Value.As<ZString>()) {
        return ZRepositoryID(*s_Str);
    }

    return ZRepositoryID("00000000-0000-0000-0000-000000000000");
}

SDynamicObjectKeyValuePair* FindPair(TArray<SDynamicObjectKeyValuePair>* p_Entries, const ZString& p_Key) {
    if (!p_Entries) {
        return nullptr;
    }

    for (auto& entry : *p_Entries) {
        if (entry.sKey == p_Key) {
            return &entry;
        }
    }

    return nullptr;
}
} // namespace

TitaniumBullets::TitaniumBullets() {
}

TitaniumBullets::~TitaniumBullets() {
    if (Globals::GameLoopManager) {
        const ZMemberDelegate<TitaniumBullets, void(const SGameUpdateEvent&)> s_Delegate(
            this, &TitaniumBullets::OnFrameUpdate
        );

        Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
    }
}

void TitaniumBullets::Init() {
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &TitaniumBullets::OnClearScene);
}

void TitaniumBullets::OnEngineInitialized() {
    Logger::Info("[TitaniumBullets] Build: {} {}", __DATE__, __TIME__);

    // Load saved settings
    m_Enabled = GetSettingBool("TitaniumBullets", "Enabled", true);

    // Register an update callback so we can apply the patch once the repository is loaded.
    if (Globals::GameLoopManager) {
        const ZMemberDelegate<TitaniumBullets, void(const SGameUpdateEvent&)> s_Delegate(
            this, &TitaniumBullets::OnFrameUpdate
        );

        Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
    }

    Logger::Info(
        "[TitaniumBullets] Ready (enabled={}). Will apply repository patch when available.",
        m_Enabled
    );

    if (m_Enabled) {
        ApplyRepositoryPatch();
    }
}

void TitaniumBullets::OnFrameUpdate(const SGameUpdateEvent&) {
    if (!m_Enabled || m_PatchApplied || m_AutoApplyDisabled) {
        return;
    }

    ApplyRepositoryPatch();
}

bool TitaniumBullets::EnsureRepositoryLoaded() {
    if (!Globals::ResourceManager) {
        return false;
    }

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_Id = ResId<"[assembly:/repository/pro.repo].pc_repo">;
        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_Id, 0);
    }

    return m_RepositoryResource && m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID;
}

bool TitaniumBullets::ApplyRepositoryPatch() {
    if (m_PatchApplied) {
        return true;
    }

    if (!EnsureRepositoryLoaded()) {
        if (!m_LogRepoNotReadyOnce) {
            Logger::Debug("[TitaniumBullets] pro.repo not ready yet; waiting...");
            m_LogRepoNotReadyOnce = true;
        }

        return false;
    }

    m_LogRepoNotReadyOnce = false;

    const auto s_RepositoryData = static_cast<THashMap<
        ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.GetResourceData());

    if (!s_RepositoryData) {
        Logger::Warn("[TitaniumBullets] pro.repo resource data is null");
        m_AutoApplyDisabled = true;
        return false;
    }

    // Clear any stale state and re-capture originals for this scene/load.
    m_OriginalAmmoConfigs.clear();
    m_OriginalAmmoConfigs.reserve(kTargets.size());
    m_RepoEntriesPatched = 0;

    // 1) Fast-path: patch by hashmap key (if keys match SMF IDs)
    for (const auto& s_TargetId : kTargets) {
        auto s_It = s_RepositoryData->find(s_TargetId);
        if (s_It == s_RepositoryData->end()) {
            continue;
        }

        auto& s_DynamicObject = s_It->second;
        auto* s_Entries = s_DynamicObject.As<TArray<SDynamicObjectKeyValuePair>>();

        auto* s_AmmoConfigPair = FindPair(s_Entries, kAmmoConfigKey);
        if (!s_AmmoConfigPair) {
            continue;
        }

        if (s_AmmoConfigPair->value.As<ZRepositoryID>()) {
            m_OriginalAmmoConfigs.emplace_back(s_TargetId, s_AmmoConfigPair->value);
            s_AmmoConfigPair->value = kPenetrationAmmoConfigId;
            ++m_RepoEntriesPatched;
        } else if (s_AmmoConfigPair->value.As<ZString>()) {
            m_OriginalAmmoConfigs.emplace_back(s_TargetId, s_AmmoConfigPair->value);
            s_AmmoConfigPair->value = ZString(std::string_view(kPenetrationAmmoConfigIdStr));
            ++m_RepoEntriesPatched;
        }
    }

    // 2) Fallback: some versions may not use ID_ as the hashmap key; scan by ID_ field.
    if (m_RepoEntriesPatched == 0) {
        for (auto& [s_MapKey, s_DynamicObject] : *s_RepositoryData) {
            auto* s_Entries = s_DynamicObject.As<TArray<SDynamicObjectKeyValuePair>>();
            if (!s_Entries) {
                continue;
            }

            auto* s_IdPair = FindPair(s_Entries, kIdKey);
            if (!s_IdPair) {
                continue;
            }

            const auto s_RepoId = GetRepoIdFromDynamicObjectOrDefault(s_IdPair->value);
            if (!IsTargetId(s_RepoId)) {
                continue;
            }

            auto* s_AmmoConfigPair = FindPair(s_Entries, kAmmoConfigKey);
            if (!s_AmmoConfigPair) {
                Logger::Warn("[TitaniumBullets] Target entry missing AmmoConfig (ID={})", s_RepoId.ToString());
                continue;
            }

            if (s_AmmoConfigPair->value.As<ZRepositoryID>()) {
                m_OriginalAmmoConfigs.emplace_back(s_MapKey, s_AmmoConfigPair->value);
                s_AmmoConfigPair->value = kPenetrationAmmoConfigId;
                ++m_RepoEntriesPatched;
            } else if (s_AmmoConfigPair->value.As<ZString>()) {
                m_OriginalAmmoConfigs.emplace_back(s_MapKey, s_AmmoConfigPair->value);
                s_AmmoConfigPair->value = ZString(std::string_view(kPenetrationAmmoConfigIdStr));
                ++m_RepoEntriesPatched;
            } else {
                const auto* s_TypeInfo = s_AmmoConfigPair->value.GetTypeID()
                    ? s_AmmoConfigPair->value.GetTypeID()->typeInfo()
                    : nullptr;
                Logger::Warn(
                    "[TitaniumBullets] AmmoConfig has unexpected type '{}' (ID={})",
                    s_TypeInfo && s_TypeInfo->m_pTypeName ? s_TypeInfo->m_pTypeName : "<null>",
                    s_RepoId.ToString()
                );
            }
        }
    }

    if (m_RepoEntriesPatched == 0) {
        Logger::Warn(
            "[TitaniumBullets] No matching repository entries found to patch (game update?). Disabling automatic retries until next scene load."
        );
        m_OriginalAmmoConfigs.clear();
        m_AutoApplyDisabled = true;
        return false;
    }

    m_PatchApplied = true;
    m_AutoApplyDisabled = false;

    Logger::Info("[TitaniumBullets] Patched {} repository entries (AmmoConfig -> penetration)", m_RepoEntriesPatched);

    return true;
}

void TitaniumBullets::RestoreRepositoryPatch() {
    if (!m_PatchApplied) {
        return;
    }

    if (!EnsureRepositoryLoaded()) {
        Logger::Warn("[TitaniumBullets] Cannot restore; pro.repo not available");
        m_PatchApplied = false;
        m_OriginalAmmoConfigs.clear();
        m_AutoApplyDisabled = false;
        return;
    }

    const auto s_RepositoryData = static_cast<THashMap<
        ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.GetResourceData());

    if (!s_RepositoryData) {
        Logger::Warn("[TitaniumBullets] Cannot restore; pro.repo resource data is null");
        m_PatchApplied = false;
        m_OriginalAmmoConfigs.clear();
        m_AutoApplyDisabled = false;
        return;
    }

    m_RepoEntriesRestored = 0;

    // Restore only the entries we originally patched.
    for (const auto& [s_MapKey, s_OriginalAmmoConfig] : m_OriginalAmmoConfigs) {
        auto s_It = s_RepositoryData->find(s_MapKey);
        if (s_It == s_RepositoryData->end()) {
            continue;
        }

        auto* s_Entries = s_It->second.As<TArray<SDynamicObjectKeyValuePair>>();
        if (!s_Entries) {
            continue;
        }

        if (auto* s_AmmoConfigPair = FindPair(s_Entries, kAmmoConfigKey)) {
            s_AmmoConfigPair->value = s_OriginalAmmoConfig;
            ++m_RepoEntriesRestored;
        }
    }

    Logger::Info("[TitaniumBullets] Restored {} repository entries", m_RepoEntriesRestored);

    m_PatchApplied = false;
    m_OriginalAmmoConfigs.clear();
    m_AutoApplyDisabled = false;
}

void TitaniumBullets::OnDrawMenu() {
    // Add menu checkbox
    if (ImGui::Checkbox(ICON_MD_SHIELD " Titanium Bullets", &m_Enabled)) {
        SetSettingBool("TitaniumBullets", "Enabled", m_Enabled);

        if (m_Enabled) {
            m_AutoApplyDisabled = false;
            // Apply immediately if possible; otherwise the frame update will apply later.
            ApplyRepositoryPatch();
            Logger::Info("[TitaniumBullets] ENABLED");
        } else {
            RestoreRepositoryPatch();
            Logger::Info("[TitaniumBullets] DISABLED");
        }
    }
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Replicates SMF TitaniumBullets by patching AmmoConfig in pro.repo");
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Debug")) {
        m_DebugWindowActive = !m_DebugWindowActive;
    }
}

void TitaniumBullets::OnDrawUI(const bool p_HasFocus) {
    if (!m_DebugWindowActive || !p_HasFocus) {
        return;
    }
    
    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("Titanium Bullets Debug", &m_DebugWindowActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());
    
    if (s_Showing) {
        ImGui::Text("Enabled: %s", m_Enabled ? "YES" : "NO");
        ImGui::Text("Patch Applied: %s", m_PatchApplied ? "YES" : "NO");
        ImGui::Text("Repo Patched: %u", m_RepoEntriesPatched);
        ImGui::Text("Repo Restored: %u", m_RepoEntriesRestored);

        ImGui::Separator();

        if (ImGui::Button("Apply Now")) {
            m_AutoApplyDisabled = false;
            ApplyRepositoryPatch();
        }

        ImGui::SameLine();

        if (ImGui::Button("Restore Now")) {
            m_Enabled = false;
            SetSettingBool("TitaniumBullets", "Enabled", m_Enabled);
            RestoreRepositoryPatch();
        }

        ImGui::Separator();

        ImGui::TextWrapped(
            "This mod edits the in-memory repository (pro.repo) so ammo uses a penetration AmmoConfig. "
            "If bullets do not penetrate, the repository IDs may have changed with a game update."
        );
    }
    
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

/**
 * Called when the scene is cleared (level unload)
 * Reset our state here
 */
DEFINE_PLUGIN_DETOUR(TitaniumBullets, void, OnClearScene, 
    ZEntitySceneContext* th, bool p_FullyUnloadScene) 
{
    // Scene unload resets the repository state; clear our cached handle and patch state.
    m_RepositoryResource = {};
    m_OriginalAmmoConfigs.clear();
    m_PatchApplied = false;
    m_RepoEntriesPatched = 0;
    m_RepoEntriesRestored = 0;
    m_LogRepoNotReadyOnce = false;
    m_AutoApplyDisabled = false;
    
    return HookResult<void>(HookAction::Continue());
}

// Register the plugin with ZHMModSDK
DEFINE_ZHM_PLUGIN(TitaniumBullets);
