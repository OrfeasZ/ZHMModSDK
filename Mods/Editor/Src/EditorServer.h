#pragma once

#include <expected>
#include <string>
#include <cstdint>

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
    void OnSceneLoading(const std::string& p_Scene, const std::vector<std::string>& p_Bricks);
    void OnSceneClearing(bool p_ForReload);
    void OnEntityTreeRebuilt();
    void SetEnabled(bool p_Enabled);
    bool GetEnabled();

private:
    static void OnMessage(WebSocket* p_Socket, std::string_view p_Message) noexcept(false);

    static void SendWelcome(WebSocket* p_Socket);
    static void SendHitmanEntity(WebSocket* p_Socket, std::optional<int64_t> p_MessageId);
    static void SendCameraEntity(WebSocket* p_Socket, std::optional<int64_t> p_MessageId);
    static void SendError(WebSocket* p_Socket, std::string p_Message, std::optional<int64_t> p_MessageId);
    static void SendEntityList(WebSocket* p_Socket, std::shared_ptr<EntityTreeNode> p_Tree, std::optional<int64_t> p_MessageId);
    static void SendEntityDetails(WebSocket* p_Socket, ZEntityRef p_Entity, std::optional<int64_t> p_MessageId);
    static void SendEntitiesDetails(WebSocket* p_Socket, const std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>& p_Entities, bool p_Done);
    static void SendDoneLoadingNavpMessage(WebSocket* p_Socket);
    static void WriteEntityTransforms(std::ostream& p_Stream, Quat p_Quat, ZEntityRef p_Entity);
    static void WriteEntityDetails(std::ostream& p_Stream, ZEntityRef p_Entity);
    static void WriteVector3(std::ostream& p_Stream, double p_X, double p_Y, double p_Z);
    static void WriteRotation(std::ostream& p_Stream, double p_Yaw, double p_Pitch, double p_Roll);
    static void WriteQuat(std::ostream& p_Stream, double p_x, double p_y, double p_z, double p_w);
    static void WriteTransform(std::ostream& p_Stream, SMatrix p_Transform);
    static void WritePropertyName(std::ostream& p_Stream, ZEntityProperty* p_Property);
    static void WriteProperty(std::ostream& p_Stream, ZEntityRef p_Entity, ZEntityProperty* p_Property);

public:
    static EntitySelector ReadEntitySelector(simdjson::ondemand::value p_Selector);
    static std::vector<EntitySelector> ReadPrimEntitySelectors(simdjson::ondemand::array p_Selector);
    static SVector3 ReadVector3(simdjson::ondemand::value p_Vector);
    static EulerAngles ReadRotation(simdjson::ondemand::value p_Rotation);
    static SMatrix ReadTransform(simdjson::ondemand::value p_Transform);
    static ZRuntimeResourceID ReadResourceId(simdjson::ondemand::value p_ResourceId);
    static uint64_t ReadEntityId(simdjson::ondemand::value p_EntityId);

private:
    void PublishEvent(const std::string& p_Event, std::optional<std::string> p_IgnoreClient);
    static bool IsPropertyValueTrue(const ZEntityProperty* s_Property, const ZEntityRef& p_Entity);
    static bool IsExcludedFromNavMeshExport(const ZEntityRef& p_Entity);

private:
    uint64_t m_LastClientId = 0;
    uWS::App* m_App;
    uWS::Loop* m_Loop;
    std::vector<SocketUserData*> m_SocketUserDatas;
    std::jthread m_ServerThread;
    static bool m_Enabled;
};
