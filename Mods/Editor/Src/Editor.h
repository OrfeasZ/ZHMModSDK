#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"

#include "Components/Qne.h"

#include "ImGuizmo.h"

class Editor : public IPluginInterface
{
public:
    Editor();
    ~Editor() override;

    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnDraw3D(IRenderer* p_Renderer) override;
    void OnEngineInitialized() override;

private:
    void SpawnCameras();
    void CopyToClipboard(const std::string& p_String) const;
    void OnMouseDown(SVector2 p_Pos, bool p_FirstClick);

    void DrawEntityManipulator(bool p_HasFocus);
    void DrawEntityAABB(IRenderer* p_Renderer);

    void DrawEntityProperties();

    void RenderBrick(ZEntityRef p_Entity);
    void RenderEntity(int p_Index, ZEntityRef p_Entity, uint64_t p_EntityId, IEntityBlueprintFactory* p_Factory, ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity);
    void DrawEntityTree();
    bool SearchForEntityById(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, uint64_t p_EntityId);
    bool SearchForEntityByType(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, const std::string& p_TypeName);
    bool SearchForEntityByName(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, const std::string& p_EntityName);

    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void CheckQneConnection(float p_DeltaTime);
    void ReceiveQneMessages();
    void SendQneMessage(const Qne::SdkToQnMessage& p_Message);

    void DrawPinTracer();

    static bool ImGuiCopyWidget(const std::string& p_Id);

private:
    DEFINE_PLUGIN_DETOUR(Editor, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DEFINE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);
    DEFINE_PLUGIN_DETOUR(Editor, ZTemplateEntityBlueprintFactory*, ZTemplateEntityBlueprintFactory_ctor, ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint, ZResourcePending& ResourcePending);
    DEFINE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);
    DEFINE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);

private:
    ZEntityRef m_Camera;
    ZEntityRef m_CameraRT;

    bool m_HoldingMouse = false;
    bool m_UseSnap = false;
    float m_SnapValue[3] = { 1.0f, 1.0f, 1.0f };

    float4 m_From;
    float4 m_To;
    float4 m_Hit;
    float4 m_Normal;

    size_t m_SelectedBrickIndex = 0;
    ZEntityRef m_SelectedEntity;
    bool m_ShouldScrollToEntity = false;

    ImGuizmo::OPERATION m_GizmoMode = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE m_GizmoSpace = ImGuizmo::MODE::WORLD;

    ZInputAction m_RaycastAction = "SkipLoadingAction"; // space
    ZInputAction m_DeleteAction = "TemporaryCamSpeedMultiplier0"; // shift

    struct PinFireInfo
    {
        std::chrono::time_point<std::chrono::system_clock> m_FireTime;
    };

    std::unordered_map<uint32_t, PinFireInfo> m_FiredInputPins = {};
    std::unordered_map<uint32_t, PinFireInfo> m_FiredOutputPins = {};

    SOCKET m_QneSocket = INVALID_SOCKET;
    bool m_ConnectedToQne = false;
    float m_QneConnectionTimer = 999.f; // Set to a high number so we connect on startup.
    sockaddr_in m_QneAddress = {};

};

DEFINE_ZHM_PLUGIN(Editor)
