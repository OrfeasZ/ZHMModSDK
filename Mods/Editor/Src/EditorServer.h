#pragma once

#include <expected>
#include <string>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include "EntityTreeNode.h"

#include "uwebsockets/App.h"
#include <Glacier/ZMath.h>
#include <Glacier/ZResourceID.h>
#include <Glacier/ZEntity.h>

#include <simdjson.h>

struct EntitySelector {
    uint64_t EntityId;
    std::optional<ZRuntimeResourceID> TbluHash;
    std::optional<std::string> PrimHash;
};

class EditorServer {
public:
    struct SocketUserData {
        std::string ClientId;
        std::string Identifier;
    };

    using WebSocket = uWS::WebSocket<false, true, SocketUserData>;

    EditorServer();
    ~EditorServer();

    void OnEntitySelected(ZEntityRef p_Entity, std::optional<std::string> p_ByClient);
    void OnEntityTransformChanged(ZEntityRef p_Entity, std::optional<std::string> p_ByClient);
    void OnEntityNameChanged(ZEntityRef p_Entity, std::optional<std::string> p_ByClient);
    void OnEntityPropertySet(ZEntityRef p_Entity, uint32_t p_PropertyId, std::optional<std::string> p_ByClient);
    void OnEntitySpawned(ZEntityRef p_Entity, std::optional<std::string> p_ByClient);
    void OnEntityDestroying(uint64_t p_EntityId, std::optional<std::string> p_ByClient);
    void OnSceneLoading(const std::string& p_Scene, const std::vector<std::string>& p_Bricks);
    void OnSceneClearing(bool p_FullyUnloadScene);
    void OnEntityTreeRebuilt();
    static void SetEnabled(bool p_Enabled);
    static bool GetEnabled();

private:
    using EntityIndexMap = std::unordered_map<const void*, size_t>;

    using SceneEntities = std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>;

    struct CrowdVolumeIndices {
        EntityIndexMap Boxes;
        EntityIndexMap Spheres;
        EntityIndexMap Shapes;
    };

    struct CrowdsJson {
        std::vector<std::string> VolumeShapes;
        std::vector<std::string> Crowds;
    };

    void OnMessage(WebSocket* p_Socket, std::string_view p_Message) noexcept(false);
    void QueueRequest(const std::string& p_ClientId, std::optional<int64_t> p_MessageId, std::function<void()> p_Request);
    void DeferToClient(const std::string& p_ClientId, std::function<void(WebSocket*)> p_Send);
    void SendToClient(const std::string& p_ClientId, std::string p_Message);

    static void SendWelcome(WebSocket* p_Socket);
    void SendHitmanEntity(const std::string& p_ClientId, std::optional<int64_t> p_MessageId);
    void SendCameraEntity(const std::string& p_ClientId, std::optional<int64_t> p_MessageId);
    void SendError(const std::string& p_ClientId, std::string p_Message, std::optional<int64_t> p_MessageId);
    void SendEntityList(
        const std::string& p_ClientId, std::shared_ptr<EntityTreeNode> p_Tree, std::optional<int64_t> p_MessageId
    );
    void SendEntityDetails(const std::string& p_ClientId, ZEntityRef p_Entity, std::optional<int64_t> p_MessageId);
    void SendNavKitScene(const std::string& p_ClientId, bool s_OnlyCollidableMeshes);
    static bool SendEntitiesDetails(
        WebSocket* p_Socket, const std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>& p_Entities
    );
    static void SendJsonList(WebSocket* p_Socket, const std::vector<std::string>& p_Items);
    static CrowdsJson WriteCrowdsJson(
        const SceneEntities& p_VolumeBoxEntities, const SceneEntities& p_VolumeSphereEntities
    );
    static std::string WriteCrowdEntity(ZEntityRef p_Crowd, const CrowdVolumeIndices& p_VolumeIndices);
    static std::string WriteVolumeShape(ZEntityRef p_Shape);
    static void WriteCrowdTeleport(std::ostream& p_Stream, ZEntityRef p_Teleport);
    static void WriteSplineFlow(std::ostream& p_Stream, ZEntityRef p_Spline);
    static void WriteVolume(std::ostream& p_Stream, ZEntityRef p_Volume);
    static void WriteControlPoints(std::ostream& p_Stream, ZEntityRef p_Entity);
    static void WriteWorldMatrix(std::ostream& p_Stream, ZEntityRef p_Entity);
    static void WriteSVector3(std::ostream& p_Stream, const SVector3& p_Vector);
    static void WriteCrowdVolumeIndexArray(
        std::ostream& p_Stream,
        ZEntityRef p_Crowd,
        std::string_view p_Key,
        std::string_view p_PropertyName,
        const EntityIndexMap& p_Indices
    );
    static void WriteEntityTransforms(std::ostream& p_Stream, Quat p_Quat, ZEntityRef p_Entity);
    static void WriteEntityIdentity(std::ostream& p_Stream, ZEntityRef p_Entity);
    static void WriteEntityDetails(std::ostream& p_Stream, ZEntityRef p_Entity);
    static void WriteVector3(std::ostream& p_Stream, double p_X, double p_Y, double p_Z);
    static void WriteRotation(std::ostream& p_Stream, double p_Yaw, double p_Pitch, double p_Roll);
    static void WriteQuat(std::ostream& p_Stream, double p_x, double p_y, double p_z, double p_w);
    static void WriteTransform(std::ostream& p_Stream, SMatrix p_Transform);
    static void WritePropertyName(std::ostream& p_Stream, SPropertyData* p_Property);
    static void WriteProperty(std::ostream& p_Stream, ZEntityRef p_Entity, SPropertyData* p_Property);

public:
    static EntitySelector ReadEntitySelector(simdjson::ondemand::value p_Selector);
    static std::vector<EntitySelector> ReadPrimEntitySelectors(simdjson::ondemand::array p_Selector);
    static SVector3 ReadVector3(simdjson::ondemand::value p_Vector);
    static EulerAngles ReadRotation(simdjson::ondemand::value p_Rotation);
    static SMatrix ReadTransform(simdjson::ondemand::value p_Transform);
    static ZRuntimeResourceID ReadResourceId(simdjson::ondemand::value p_ResourceId);
    static uint64_t ReadEntityId(simdjson::ondemand::value p_EntityId);

private:
    void PublishEvent(std::string p_Event, std::optional<std::string> p_IgnoreClient);
    static bool IsPropertyValueTrue(const SPropertyData* s_Property, const ZEntityRef& p_Entity);
    static bool IsExcludedFromNavMeshExport(const ZEntityRef& p_Entity);

private:
    uint64_t m_LastClientId = 0;
    uWS::App* m_App;
    uWS::Loop* m_Loop;
    // Only used on the server thread.
    std::unordered_map<std::string, WebSocket*> m_Sockets;
    std::jthread m_ServerThread;
    static std::atomic<bool> m_Enabled;
};
