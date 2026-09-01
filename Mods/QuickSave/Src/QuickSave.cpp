#include "QuickSave.h"

#include <random>

#include "Events.h"
#include "Functions.h"
#include "Logging.h"

#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZInputActionManager.h>

#include "Glacier/SExternalReferences.h"
#include "Glacier/ZModule.h"

QuickSave::QuickSave() :
    m_QuickSave("QuickSave"),
    m_QuickLoad("QuickLoad") {
}

QuickSave::~QuickSave() {
    const ZMemberDelegate<QuickSave, void(const SGameUpdateEvent&)> s_Delegate(this, &QuickSave::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    m_LevelControlEntity = {};
}

void QuickSave::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &QuickSave::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &QuickSave::OnClearScene);
}

void QuickSave::OnEngineInitialized() {
    const ZMemberDelegate<QuickSave, void(const SGameUpdateEvent&)> s_Delegate(this, &QuickSave::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    const char* binds = "QuickSaveInput={"
        "QuickSave=tap(kb,f5);"
        "QuickLoad=tap(kb,f9);};";

    if (ZInputActionManager::AddBindings(binds)) {
        Logger::Debug("[QuickSave] Successfully added bindings.");
    }
    else {
        Logger::Debug("[QuickSave] Failed to add bindings.");
    }
}

void QuickSave::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (!*Globals::ApplicationEngineWin32)
        return;

    if (Functions::ZInputAction_Digital->Call(&m_QuickSave, -1)) {
        if (const auto s_Ent = GetLevelControlEntity()) {
            s_Ent.SignalInputPin("SaveQuick");
        }
    }
    else if (Functions::ZInputAction_Digital->Call(&m_QuickLoad, -1)) {
        if (const auto s_Ent = GetLevelControlEntity()) {
            s_Ent.SignalInputPin("LoadQuick");
        }
    }
}

ZEntityRef QuickSave::GetLevelControlEntity() {
    if (m_LevelControlEntity) {
        return m_LevelControlEntity;
    }

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Warn("[QuickSave] Scene not loaded. Cannot quicksave.");
        return {};
    }

    const auto s_ResId = ResId<"[modules:/zlevelcontrolentity.class].pc_entitytype">;
    TResourcePtr<ZTemplateEntityFactory> s_Resource;
    Globals::ResourceManager->GetResourcePtr(s_Resource, s_ResId, 0);

    if (!s_Resource) {
        Logger::Warn("[QuickSave] Level control entity is not loaded. Cannot quicksave.");
        return {};
    }

    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_LevelControlEntity,
        "",
        s_Resource,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        -1
    );

    return m_LevelControlEntity;
}

DEFINE_PLUGIN_DETOUR(QuickSave, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters&) {
    m_LevelControlEntity = {};
    return {HookAction::Continue()};
}

DEFINE_PLUGIN_DETOUR(QuickSave, void, OnClearScene, ZEntitySceneContext* th, bool) {
    m_LevelControlEntity = {};
    return {HookAction::Continue()};
}

DEFINE_ZHM_PLUGIN(QuickSave);
