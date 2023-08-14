#pragma once

#include <WinSock2.h>

#include <random>
#include <unordered_map>
#include <map>
#include <shared_mutex>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"

#include "ImGuizmo.h"
#include "EditorServer.h"
#include "EntityTreeNode.h"

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

public:
	void SelectEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId);
	void SetEntityTransform(EntitySelector p_Selector, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId);
	void SpawnEntity(ZRuntimeResourceID p_Template, uint64_t p_EntityId, std::string p_Name, std::optional<std::string> p_ClientId);
	void DestroyEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId);
	void SetEntityName(EntitySelector p_Selector, std::string p_Name, std::optional<std::string> p_ClientId);
	void SetEntityProperty(EntitySelector p_Selector, uint32_t p_PropertyId, std::string_view p_JsonValue, std::optional<std::string> p_ClientId);
	void SignalEntityPin(EntitySelector p_Selector, uint32_t p_PinId, bool p_Output);
	void LockEntityTree() { m_CachedEntityTreeMutex.lock_shared(); }
	std::shared_ptr<EntityTreeNode> GetEntityTree() { return m_CachedEntityTree; }
	void UnlockEntityTree() { m_CachedEntityTreeMutex.unlock_shared(); }
	ZEntityRef FindEntity(EntitySelector p_Selector);
	void RebuildEntityTree();

private:
    void SpawnCameras();
    void ActivateCamera(ZEntityRef* m_CameraEntity);
    void DeactivateCamera();
    void CopyToClipboard(const std::string& p_String) const;
    void OnMouseDown(SVector2 p_Pos, bool p_FirstClick);

    void DrawEntityManipulator(bool p_HasFocus);
    void DrawEntityAABB(IRenderer* p_Renderer);

    void DrawEntityProperties();

    void RenderEntity(std::shared_ptr<EntityTreeNode> p_Node);
    void DrawEntityTree();
    bool SearchForEntityById(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, uint64_t p_EntityId);
    bool SearchForEntityByType(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, const std::string& p_TypeName);
    bool SearchForEntityByName(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, const std::string& p_EntityName);
	void UpdateEntities();

	void OnSelectEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId);
	void OnEntityTransformChange(ZEntityRef p_Entity, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId);
	void OnEntityNameChange(ZEntityRef p_Entity, const std::string& p_Name, std::optional<std::string> p_ClientId);
	void OnSetPropertyValue(ZEntityRef p_Entity, uint32_t p_PropertyId, const ZObjectRef& p_Value, std::optional<std::string> p_ClientId);
	void OnSignalEntityPin(ZEntityRef p_Entity, const std::string& p_Pin, bool p_Output);
	void OnSignalEntityPin(ZEntityRef p_Entity, uint32_t p_PinId, bool p_Output);

    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    void DrawPinTracer();

    static bool ImGuiCopyWidget(const std::string& p_Id);

	// Properties
	void UnsupportedProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

	// Primitive properties.
	void StringProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void BoolProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Uint8Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Uint16Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Uint32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Uint64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Int8Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Int16Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Int32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Int64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Float32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void Float64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void EnumProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

	// Vector properties.
	void SVector2Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void SVector3Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
	void SVector4Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

	void SMatrix43Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

	void ResourceProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

private:
    DECLARE_PLUGIN_DETOUR(Editor, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool forReload);
    DECLARE_PLUGIN_DETOUR(Editor, ZTemplateEntityBlueprintFactory*, ZTemplateEntityBlueprintFactory_ctor, ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint, ZResourcePending& ResourcePending);
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);

private:
    ZEntityRef m_Camera;
    ZEntityRef m_CameraRT;

    bool m_CameraActive = false;
    ZEntityRef m_OriginalCam;

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

	std::shared_mutex m_CachedEntityTreeMutex;
	std::shared_ptr<EntityTreeNode> m_CachedEntityTree;

	std::unordered_map<uint64_t, ZEntityRef> m_SpawnedEntities;
	std::unordered_map<ZEntityRef, std::string> m_EntityNames;

	EditorServer m_Server;
};

DECLARE_ZHM_PLUGIN(Editor)
