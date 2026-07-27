#include "Noclip.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/SGameUpdateEvent.h>

#include "IconsMaterialDesign.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZKnowledge.h"
#include "Glacier/ZModule.h"
#include "Glacier/ZInputActionManager.h"
#include "Glacier/ZHM5CrippleBox.h"
#include "Glacier/SExternalReferences.h"

Noclip::Noclip() :
    m_ToggleNoclipAction("ToggleNoclip"),
    m_ForwardAction("Forward"),
    m_BackwardAction("Backward"),
    m_LeftAction("Left"),
    m_RightAction("Right"),
    m_FastAction("Fast")
{
}

Noclip::~Noclip() {
    const ZMemberDelegate<Noclip, void(const SGameUpdateEvent&)> s_Delegate(this, &Noclip::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void Noclip::OnEngineInitialized() {
    const ZMemberDelegate<Noclip, void(const SGameUpdateEvent&)> s_Delegate(this, &Noclip::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    const char* binds = "NoclipInput={"
        "ToggleNoclip=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,n);"
        "Forward=hold(kb,w);"
        "Backward=hold(kb,s);"
        "Left=hold(kb,a);"
        "Right=hold(kb,d);"
        "Fast=hold(kb,lshift) | hold(kb,rshift);};";

    if (ZInputActionManager::AddBindings(binds)) {
        Logger::Debug("Successfully added bindings.");
    }
    else {
        Logger::Debug("Failed to add bindings.");
    }
}

void Noclip::Init() {
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Noclip::OnClearScene);
}

void Noclip::OnDrawMenu() {
    if (ImGui::Checkbox(ICON_MD_SELF_IMPROVEMENT " Noclip", &m_NoclipEnabled)) {
        if (m_NoclipEnabled) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                if (const auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>())
                    m_PlayerPosition = s_HitmanSpatial->GetObjectToWorldMatrix();
            }
        }

        TogglePlayerMovement();
    }
}

void Noclip::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman)
        return;

    const auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

    if (!s_HitmanSpatial)
        return;

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera)
        return;

    if (Functions::ZInputAction_Digital->Call(&m_ToggleNoclipAction, -1)) {
        m_NoclipEnabled = !m_NoclipEnabled;

        if (m_NoclipEnabled) {
            m_PlayerPosition = s_HitmanSpatial->GetObjectToWorldMatrix();
        }

        TogglePlayerMovement();
    }

    if (!m_NoclipEnabled)
        return;

    auto s_CameraTrans = s_CurrentCamera->GetObjectToWorldMatrix();

    // Meters per second.
    float s_MoveSpeed = 5.f;

    if (Functions::ZInputAction_Digital->Call(&m_FastAction, -1))
        s_MoveSpeed = 20.f;

    if (Functions::ZInputAction_Digital->Call(&m_ForwardAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Up * -s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (Functions::ZInputAction_Digital->Call(&m_BackwardAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Up * s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (Functions::ZInputAction_Digital->Call(&m_LeftAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Right * -s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (Functions::ZInputAction_Digital->Call(&m_RightAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Right * s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    s_HitmanSpatial->SetObjectToWorldMatrixFromEditor(m_PlayerPosition);
}

void Noclip::TogglePlayerMovement() {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("Local player is not alive.");

        return;
    }

    bool s_IsHM5CrippleBoxEntityCreated = m_HM5CrippleBoxEntity;

    if (!s_IsHM5CrippleBoxEntityCreated) {
        s_IsHM5CrippleBoxEntityCreated = CreateHM5CrippleBoxEntity();

        ZHM5CrippleBox* s_HM5CrippleBox = m_HM5CrippleBoxEntity.QueryInterface<ZHM5CrippleBox>();

        s_HM5CrippleBox->m_bActivateOnStart = true;
        s_HM5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
        s_HM5CrippleBox->m_bMovementAllowed = false;

        s_HM5CrippleBox->Activate(0);
    }
    else {
        ZHM5CrippleBox* s_HM5CrippleBox = m_HM5CrippleBoxEntity.QueryInterface<ZHM5CrippleBox>();

        s_HM5CrippleBox->m_bMovementAllowed = !m_NoclipEnabled;

        Functions::ZHM5CrippleBox_UpdateFlags->Call(s_HM5CrippleBox);
        Functions::ZHM5CrippleBox_SetDataOnHitman->Call(s_HM5CrippleBox->m_rHitmanCharacter, false);
    }
}

bool Noclip::CreateHM5CrippleBoxEntity() {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");

        return false;
    }

    constexpr auto s_CrippleBoxFactoryId = ResId<"[modules:/zhm5cripplebox.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_CrippleBoxFactory;
    Globals::ResourceManager->GetResourcePtr(s_CrippleBoxFactory, s_CrippleBoxFactoryId, 0);

    if (!s_CrippleBoxFactory) {
        Logger::Debug("Resource is not loaded.");

        return false;
    }

    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_HM5CrippleBoxEntity,
        "",
        s_CrippleBoxFactory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        -1
    );

    if (!m_HM5CrippleBoxEntity) {
        Logger::Debug("Failed to spawn entity.");

        return false;
    }

    return true;
}

DEFINE_PLUGIN_DETOUR(Noclip, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    m_NoclipEnabled = false;

    if (m_HM5CrippleBoxEntity) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_HM5CrippleBoxEntity, {});

        m_HM5CrippleBoxEntity = {};
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Noclip);
