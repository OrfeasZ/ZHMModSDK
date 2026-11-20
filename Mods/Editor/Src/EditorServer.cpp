#include "EditorServer.h"

#include <Logging.h>
#include <simdjson.h>
#include <queue>
#include <numbers>
#include <ranges>

#include "Editor.h"
#include "JsonHelpers.h"

#include <Glacier/ZEntity.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZRoom.h>
#include <Glacier/ZAIAreaEntity.h>
#include <Glacier/Enums.h>
#include <Glacier/TArray.h>

#include <ResourceLib_HM3.h>

// 46735 is a phoneword for HMSDK
constinit const char* c_EditorHost = "127.0.0.1";
constinit const uint16_t c_EditorPort = 46735;

EditorServer::EditorServer() {
    m_ServerThread = std::jthread(
        [this]() {
            auto* s_App = new uWS::App();
            auto* s_Loop = uWS::Loop::get();

            m_App = s_App;
            m_Loop = s_Loop;

            s_App->ws<SocketUserData>(
                "/*", {
                    .compression = uWS::DISABLED,
                    .maxPayloadLength = 100 * 1024 * 1024,
                    .open = [this](WebSocket* p_Socket) {
                        Logger::Debug("[Editor] New editor connection established.");

                        const auto s_ClientId = m_LastClientId++;
                        const auto s_ClientIdStr = std::to_string(s_ClientId);

                        p_Socket->getUserData()->ClientId = s_ClientIdStr;
                        m_Sockets[s_ClientIdStr] = p_Socket;
                    },
                    .message = [&](WebSocket* p_Socket, std::string_view p_Message, uWS::OpCode p_OpCode) {
                        Logger::Trace("[Editor] Socket message received: {}", p_Message);

                        try {
                            if (m_Enabled) {
                                OnMessage(p_Socket, p_Message);
                            }
                            else {
                                Logger::Info("[Editor] EditorServer disabled, ignoring message.");
                            }
                        }
                        catch (const std::exception& e) {
                            Logger::Error(
                                "[Editor] Failed to handle editor message with error: {}\nMessage was: {}", e.what(), p_Message
                            );
                            SendError(p_Socket->getUserData()->ClientId, e.what(), std::nullopt);
                        }
                    },
                    .close = [this](WebSocket* p_Socket, int p_Code, std::string_view p_Message) {
                        Logger::Debug("[Editor] Editor connection closed with code '{}' and message: {}", p_Code, p_Message);

                        m_Sockets.erase(p_Socket->getUserData()->ClientId);
                    }
                }
            );

            s_App->listen(
                c_EditorHost, c_EditorPort, [](auto* p_Socket) {
                    if (p_Socket) {
                        Logger::Info("[Editor] Editor server listening on {}:{}", c_EditorHost, c_EditorPort);
                    }
                    else {
                        Logger::Error(
                            "[Editor] Editor server failed to listen on {}:{}. Is another program using this port?",
                            c_EditorHost,
                            c_EditorPort
                        );
                    }
                }
            );

            s_App->run();

            // Run returns after the server has stopped.
            m_App = nullptr;
            m_Loop = nullptr;

            delete s_App;
            s_Loop->free();
        }
    );

    // This is here so the IDE doesn't think the thread function is unused.
    (void)m_ServerThread;
}

EditorServer::~EditorServer() {
    if (m_App && m_Loop) {
        m_Loop->defer(
            [this]() {
                m_App->close();
            }
        );
    }
}

std::atomic<bool> EditorServer::m_Enabled = true;

void EditorServer::OnMessage(WebSocket* p_Socket, std::string_view p_Message) noexcept(false) {
    simdjson::ondemand::parser s_Parser;
    const auto s_Json = simdjson::padded_string(p_Message);
    simdjson::ondemand::document s_JsonMsg = s_Parser.iterate(s_Json);

    const std::string_view s_Type = s_JsonMsg["type"];
    Logger::Trace("[Editor] Editor message type: {}", s_Type);

    std::optional<int64_t> s_MessageId;

    if (s_JsonMsg.find_field("msgId").error() == simdjson::SUCCESS) {
        if (s_JsonMsg["msgId"].type() == simdjson::ondemand::json_type::number) {
            s_MessageId = s_JsonMsg["msgId"];
        }
    }

    const std::string s_ClientId = p_Socket->getUserData()->ClientId;

    if (s_Type == "hello") {
        p_Socket->getUserData()->Identifier = std::string_view(s_JsonMsg["identifier"]);
        SendWelcome(p_Socket);
    }
    else if (s_Type == "selectEntity") {
        QueueRequest(s_ClientId, s_MessageId, [s_ClientId, s_Selector = ReadEntitySelector(s_JsonMsg["entity"])] {
            Plugin()->SelectEntity(s_Selector, s_ClientId);
        });
    }
    else if (s_Type == "setEntityTransform") {
        const auto s_Selector = ReadEntitySelector(s_JsonMsg["entity"]);
        const auto s_Transform = ReadTransform(s_JsonMsg["transform"]);
        const bool s_Relative = s_JsonMsg["relative"];

        QueueRequest(s_ClientId, s_MessageId, [s_ClientId, s_Selector, s_Transform, s_Relative] {
            Plugin()->SetEntityTransform(s_Selector, s_Transform, s_Relative, s_ClientId);
        });
    }
    else if (s_Type == "spawnQnEntity") {
        auto s_QnJson = std::string(std::string_view(s_JsonMsg["qnJson"]));
        const auto s_EntityId = ReadEntityId(s_JsonMsg["entityId"]);
        auto s_Name = std::string(std::string_view(s_JsonMsg["name"]));

        QueueRequest(
            s_ClientId,
            s_MessageId,
            [s_ClientId, s_QnJson = std::move(s_QnJson), s_EntityId, s_Name = std::move(s_Name)] {
                Plugin()->SpawnQnEntity(s_QnJson, s_EntityId, s_Name, s_ClientId);
            }
        );
    }
    else if (s_Type == "createEntityResources") {
        QueueRequest(
            s_ClientId,
            s_MessageId,
            [s_ClientId, s_QnJson = std::string(std::string_view(s_JsonMsg["qnJson"]))] {
                Plugin()->CreateEntityResources(s_QnJson, s_ClientId);
            }
        );
    }
    else if (s_Type == "destroyEntity") {
        QueueRequest(s_ClientId, s_MessageId, [s_ClientId, s_Selector = ReadEntitySelector(s_JsonMsg["entity"])] {
            Plugin()->DestroyEntity(s_Selector, s_ClientId);
        });
    }
    else if (s_Type == "setEntityName") {
        const auto s_Selector = ReadEntitySelector(s_JsonMsg["entity"]);
        auto s_Name = std::string(std::string_view(s_JsonMsg["name"]));

        QueueRequest(s_ClientId, s_MessageId, [s_ClientId, s_Selector, s_Name = std::move(s_Name)] {
            Plugin()->SetEntityName(s_Selector, s_Name, s_ClientId);
        });
    }
    else if (s_Type == "setEntityProperty") {
        int32_t s_PropertyId;

        if (s_JsonMsg["property"].type() == simdjson::ondemand::json_type::number) {
            s_PropertyId = s_JsonMsg["property"];
        }
        else {
            std::string_view s_PropertyName = s_JsonMsg["property"];
            s_PropertyId = Hash::Crc32(s_PropertyName.data(), s_PropertyName.size());
        }

        const auto s_Selector = ReadEntitySelector(s_JsonMsg["entity"]);
        std::string s_Value(simdjson::to_json_string(s_JsonMsg["value"]).value());

        QueueRequest(s_ClientId, s_MessageId, [s_ClientId, s_Selector, s_PropertyId, s_Value = std::move(s_Value)] {
            Plugin()->SetEntityProperty(s_Selector, s_PropertyId, s_Value, s_ClientId);
        });
    }
    else if (s_Type == "signalEntityPin") {
        int32_t s_PinId;

        if (s_JsonMsg["pin"].type() == simdjson::ondemand::json_type::number) {
            s_PinId = s_JsonMsg["pin"];
        }
        else {
            std::string_view s_PinName = s_JsonMsg["pin"];
            s_PinId = Hash::Crc32(s_PinName.data(), s_PinName.size());
        }

        const auto s_Selector = ReadEntitySelector(s_JsonMsg["entity"]);
        const bool s_Output = s_JsonMsg["output"];

        QueueRequest(s_ClientId, s_MessageId, [s_Selector, s_PinId, s_Output] {
            Plugin()->SignalEntityPin(s_Selector, s_PinId, s_Output);
        });
    }
    else if (s_Type == "listEntities") {
        QueueRequest(s_ClientId, s_MessageId, [this, s_ClientId, s_MessageId] {
            SendEntityList(s_ClientId, Plugin()->GetEntityTree(), s_MessageId);
        });
    }
    else if (s_Type == "listAlocPfBoxAndSeedPointEntities") {

        bool s_OnlyCollidableMeshes = true;
        if (auto result = s_JsonMsg["onlyCollidable"]; result.error() == simdjson::SUCCESS) {
            if (result.type() == simdjson::ondemand::json_type::boolean) {
                s_OnlyCollidableMeshes = result.get_bool();
            }
        }
        QueueRequest(s_ClientId, s_MessageId, [this, s_ClientId, s_OnlyCollidableMeshes] {
            SendNavKitScene(s_ClientId, s_OnlyCollidableMeshes);
        });
    }
    else if (s_Type == "getEntityDetails") {
        QueueRequest(
            s_ClientId,
            s_MessageId,
            [this, s_ClientId, s_MessageId, s_Selector = ReadEntitySelector(s_JsonMsg["entity"])] {
                SendEntityDetails(s_ClientId, Plugin()->FindEntity(s_Selector), s_MessageId);
            }
        );
    }
    else if (s_Type == "getHitmanEntity") {
        QueueRequest(s_ClientId, s_MessageId, [this, s_ClientId, s_MessageId] {
            SendHitmanEntity(s_ClientId, s_MessageId);
        });
    }
    else if (s_Type == "getCameraEntity") {
        QueueRequest(s_ClientId, s_MessageId, [this, s_ClientId, s_MessageId] {
            SendCameraEntity(s_ClientId, s_MessageId);
        });
    }
    else if (s_Type == "rebuildEntityTree") {
        QueueRequest(s_ClientId, s_MessageId, [] {
            Plugin()->RebuildEntityTree();
        });
    }
    else {
        throw std::runtime_error(std::format("Unknown editor message type: {}", s_Type));
    }
}

// Requests change and read game state, so they run on the game's update thread.
void EditorServer::QueueRequest(
    const std::string& p_ClientId, std::optional<int64_t> p_MessageId, std::function<void()> p_Request
) {
    Plugin()->QueueTask([this, p_ClientId, p_MessageId, p_Request = std::move(p_Request)] {
        try {
            p_Request();
        }
        catch (const std::exception& e) {
            Logger::Error("[Editor] Failed to handle editor message with error: {}", e.what());
            SendError(p_ClientId, e.what(), p_MessageId);
        }
    });
}

// The socket can close before the loop runs this, so we find it again by client ID.
void EditorServer::DeferToClient(const std::string& p_ClientId, std::function<void(WebSocket*)> p_Send) {
    if (!m_Loop) {
        return;
    }

    m_Loop->defer([this, p_ClientId, p_Send = std::move(p_Send)] {
        if (const auto s_It = m_Sockets.find(p_ClientId); s_It != m_Sockets.end()) {
            p_Send(s_It->second);
        }
    });
}

void EditorServer::SendToClient(const std::string& p_ClientId, std::string p_Message) {
    DeferToClient(p_ClientId, [p_Message = std::move(p_Message)](WebSocket* p_Socket) {
        p_Socket->send(p_Message, uWS::OpCode::TEXT);
    });
}

void EditorServer::SendWelcome(EditorServer::WebSocket* p_Socket) {
    Logger::Info(
        "[Editor] Client with identifier '{}' connected to the editor server. Sending welcome message.",
        p_Socket->getUserData()->Identifier
    );

    // Subscribe the client to messages.
    p_Socket->subscribe("all");
    p_Socket->subscribe(p_Socket->getUserData()->ClientId);

    // TODO: Ideally these json events would be streamed directly
    // into the socket, but don't care at the moment.
    std::ostringstream s_Event;

    s_Event << "{";
    s_Event << write_json("type") << ":" << write_json("welcome");
    s_Event << "}";

    p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
}

void EditorServer::SendHitmanEntity(const std::string& p_ClientId, std::optional<int64_t> p_MessageId) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping SendHitmanEntity.");
        return;
    }

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman || !s_LocalHitman.m_entityRef) {
        SendError(p_ClientId, "Failed to get local hitman entity.", p_MessageId);
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("hitmanEntity") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, s_LocalHitman.m_entityRef);
    s_Event << "}";

    SendToClient(p_ClientId, s_Event.str());
}

void EditorServer::SendCameraEntity(const std::string& p_ClientId, std::optional<int64_t> p_MessageId) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping SendCameraEntity.");
        return;
    }
    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera) {
        SendError(p_ClientId, "Failed to get active camera entity.", p_MessageId);
        return;
    }

    ZEntityRef s_Ref;
    s_CurrentCamera->GetID(s_Ref);

    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("cameraEntity") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, s_Ref);
    s_Event << "}";

    SendToClient(p_ClientId, s_Event.str());
}

void EditorServer::SendError(
    const std::string& p_ClientId, std::string p_Message, std::optional<int64_t> p_MessageId
) {
    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("error") << ",";
    s_Event << write_json("message") << ":" << write_json(p_Message);
    s_Event << "}";

    SendToClient(p_ClientId, s_Event.str());
}

void EditorServer::OnEntitySelected(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntitySelected.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    if (!p_Entity) {
        s_Event << write_json("type") << ":" << write_json("entityDeselected");
    }
    else {
        s_Event << write_json("type") << ":" << write_json("entitySelected") << ",";
        s_Event << write_json("entity") << ":";
        WriteEntityDetails(s_Event, p_Entity);
    }

    s_Event << "}";

    PublishEvent(s_Event.str(), p_ByClient);
}

void EditorServer::OnEntityTransformChanged(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntityTransformChanged.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("entityTransformUpdated") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, p_Entity);

    s_Event << "}";

    PublishEvent(s_Event.str(), p_ByClient);
}

void EditorServer::OnEntityNameChanged(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntityNameChanged.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("entityNameUpdated") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, p_Entity);

    s_Event << "}";

    PublishEvent(s_Event.str(), p_ByClient);
}

void EditorServer::OnEntityPropertySet(
    ZEntityRef p_Entity, uint32_t p_PropertyId, std::optional<std::string> p_ByClient
) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntityPropertySet.");
        return;
    }
    const auto s_EntityType = p_Entity->GetType();

    if (!s_EntityType) {
        return;
    }

    const auto s_Property = s_EntityType->FindProperty(p_PropertyId);

    if (!s_Property || !s_Property->m_pPropertyInfo->m_Type) {
        return;
    }

    const auto s_PropertyInfo = s_Property->GetPropertyInfo();

    if (!s_PropertyInfo) {
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("entityPropertyChanged") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, p_Entity);
    s_Event << ",";
    s_Event << write_json("property") << ":";

    if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsResource() ||
        s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyID) {
        // Some properties don't have a name for some reason. Try to find using RL.
        const auto s_PropertyName = HM3_GetPropertyName(s_Property->m_nPropertyID);

        if (s_PropertyName.Size > 0) {
            s_Event << write_json(std::string_view(s_PropertyName.Data, s_PropertyName.Size)) << ",";
        }
        else {
            s_Event << write_json(s_Property->m_nPropertyID) << ",";
        }
    }
    else {
        s_Event << write_json(s_PropertyInfo->m_pszPropertyName) << ",";
    }

    s_Event << write_json("value") << ":";
    WriteProperty(s_Event, p_Entity, s_Property);

    s_Event << "}";

    PublishEvent(s_Event.str(), p_ByClient);
}

void EditorServer::OnEntitySpawned(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntitySpawned.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("entitySpawned") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, p_Entity);

    s_Event << "}";

    PublishEvent(s_Event.str(), std::nullopt);
}

void EditorServer::OnEntityDestroying(uint64_t p_EntityId, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntityDestroying.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("entityDestroying") << ",";
    s_Event << write_json("entityId") << ":" << write_json(std::format("{:016x}", p_EntityId));

    s_Event << "}";

    PublishEvent(s_Event.str(), std::nullopt);
}

void EditorServer::OnSceneLoading(const std::string& p_Scene, const std::vector<std::string>& p_Bricks) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnSceneLoading.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("sceneLoading") << ",";
    s_Event << write_json("scene") << ":" << write_json(p_Scene) << ",";
    s_Event << write_json("bricks") << ":" << "[";

    bool s_First = true;

    for (const auto& s_Brick : p_Bricks) {
        if (!s_First) {
            s_Event << ",";
        }

        s_Event << write_json(s_Brick);

        s_First = false;
    }

    s_Event << "]";

    s_Event << "}";

    PublishEvent(s_Event.str(), std::nullopt);
}

void EditorServer::OnSceneClearing(bool p_FullyUnloadScene) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnSceneClearing.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    s_Event << write_json("type") << ":" << write_json("sceneClearing") << ",";
    s_Event << write_json("forReload") << ":" << write_json(p_FullyUnloadScene);

    s_Event << "}";

    PublishEvent(s_Event.str(), std::nullopt);
}

void EditorServer::OnEntityTreeRebuilt() {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping OnEntityTreeRebuilt.");
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";
    s_Event << write_json("type") << ":" << write_json("entityTreeRebuilt");
    s_Event << "}";

    PublishEvent(s_Event.str(), std::nullopt);
}

void EditorServer::SetEnabled(bool p_Enabled) {
    m_Enabled = p_Enabled;
    if (p_Enabled) {
        Logger::Info("[Editor] Enabling EditorServer");
    }
    else {
        Logger::Info("[Editor] Disabling EditorServer");
    }
}

bool EditorServer::GetEnabled() {
    return m_Enabled;
}

void EditorServer::SendNavKitScene(const std::string& p_ClientId, bool s_OnlyCollidableMeshes) {
    DeferToClient(
        p_ClientId,
        [](WebSocket* p_Socket) {
            p_Socket->send(R"({"version":1,"meshes":[)", uWS::OpCode::TEXT);
        }
    );

    auto s_AnyMeshSentOverall = std::make_shared<bool>(false);
    auto s_TotalMeshesSent = std::make_shared<int>(0);
    auto s_LastLoggedMilestone = std::make_shared<int>(0);

    Logger::Info("[Editor] Sending Meshes...");

    Plugin()->FindMeshes(
        s_OnlyCollidableMeshes,
        [this, p_ClientId, s_AnyMeshSentOverall, s_TotalMeshesSent, s_LastLoggedMilestone](
            const std::vector<NavKitMeshEntity>& p_Entities,
            const std::map<std::string, NavKitMatiTextures>& p_MatiTextures,
            const std::map<std::string, std::vector<std::string>>& p_PrimMatis,
            const bool p_IsLastMeshBatch
            ) -> void {
                std::ostringstream s_BatchJson;
                bool s_DidPrepareDataThisBatch = false;

                if (!p_Entities.empty()) {
                    bool s_IsFirstItemInBatch = true;
                    *s_TotalMeshesSent += p_Entities.size();
                    const int currentMilestone = *s_TotalMeshesSent / 1000;

                    if (currentMilestone > *s_LastLoggedMilestone) {
                        Logger::Info("[Editor] Meshes sent: {}", *s_TotalMeshesSent);

                        *s_LastLoggedMilestone = currentMilestone;
                    }
                    for (auto& [s_AlocHash, s_PrimHash, s_Quat, s_RoomName, s_FolderName, s_Entity] : p_Entities) {
                        if (IsExcludedFromNavMeshExport(s_Entity)) continue;

                        if (!s_IsFirstItemInBatch) {
                            s_BatchJson << ",";
                        }

                        s_IsFirstItemInBatch = false;
                        s_DidPrepareDataThisBatch = true;
                        s_BatchJson << "{";
                        s_BatchJson << write_json("alocHash") << ":" << write_json(s_AlocHash) << ",";
                        s_BatchJson << write_json("primHash") << ":" << write_json(s_PrimHash) << ",";
                        s_BatchJson << write_json("roomName") << ":" << write_json(s_RoomName) << ",";
                        s_BatchJson << write_json("roomFolderName") << ":" << write_json(s_FolderName) << ",";
                        s_BatchJson << write_json("entity") << ":";

                        WriteEntityTransforms(s_BatchJson, s_Quat, s_Entity);
                        s_BatchJson << "}";
                    }
                }
                if (s_DidPrepareDataThisBatch) {
                    std::string data_to_send = s_BatchJson.str();

                    DeferToClient(
                        p_ClientId,
                        [data_to_send, s_AnyMeshSentOverall](WebSocket* p_Socket) {
                            if (*s_AnyMeshSentOverall) {
                                p_Socket->send(",", uWS::OpCode::TEXT);
                            }

                            p_Socket->send(data_to_send, uWS::OpCode::TEXT);
                            *s_AnyMeshSentOverall = true;
                        }
                    );
                }
                if (p_IsLastMeshBatch) {
                    const auto s_PfBoxEntities = Plugin()->FindEntitiesByType("ZPFBoxEntity", "00724CDE424AFE76");
                    const auto s_PfSeedPointEntities = Plugin()->FindEntitiesByType("ZPFSeedPoint", "00280B8C4462FAC8");
                    const auto s_GateEntities = Plugin()->FindEntitiesByType("ZGateEntity", "00D78DDF8301DF97");
                    const auto s_RoomEntities = Plugin()->FindEntitiesByType("ZRoomEntity", "0071E63EC98496FE");
                    const auto s_AIAreaWorldEntities = Plugin()->FindEntitiesByType(
                        "ZAIAreaWorldEntity", "00D23EE76CC1735F"
                    );
                    const auto s_AIAreaEntities = Plugin()->FindEntitiesByType("ZAIAreaEntity", "000F13E2D42C882E");
                    const auto s_VolumeBoxEntities = Plugin()->FindEntitiesByType("ZBoxVolumeEntity", "0054667393764C74");
                    const auto s_VolumeSphereEntities = Plugin()->FindEntitiesByType(
                        "ZSphereVolumeEntity", "00B86A9EE991EFB2"
                    );
                    const auto s_MapSpaceConfigEntities = Plugin()->FindEntitiesByType("ZUIMapSpaceConfig", "00972D765AF6B272");
                    const auto s_CrowdsJson = WriteCrowdsJson(s_VolumeBoxEntities, s_VolumeSphereEntities);

                    DeferToClient(
                        p_ClientId,
                        [s_PfBoxEntities, s_PfSeedPointEntities, s_GateEntities, s_RoomEntities,
                            s_AIAreaWorldEntities, s_AIAreaEntities, s_VolumeBoxEntities, s_VolumeSphereEntities,
                            s_MapSpaceConfigEntities, s_CrowdsJson, p_PrimMatis, p_MatiTextures](WebSocket* p_Socket) {
                                p_Socket->send("],\"pfBoxes\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_PfBoxEntities);

                                p_Socket->send("],\"pfSeedPoints\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_PfSeedPointEntities);

                                p_Socket->send("],\"gates\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_GateEntities);

                                p_Socket->send("],\"rooms\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_RoomEntities);

                                p_Socket->send("],\"aiAreaWorld\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_AIAreaWorldEntities);

                                p_Socket->send("],\"aiArea\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_AIAreaEntities);

                                p_Socket->send("],\"volumeBoxes\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_VolumeBoxEntities);

                                p_Socket->send("],\"volumeSpheres\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_VolumeSphereEntities);

                                p_Socket->send("],\"volumeShapes\":[", uWS::OpCode::TEXT);
                                SendJsonList(p_Socket, s_CrowdsJson.VolumeShapes);

                                p_Socket->send("],\"mapSpaceConfigs\":[", uWS::OpCode::TEXT);
                                SendEntitiesDetails(p_Socket, s_MapSpaceConfigEntities);

                                p_Socket->send("],\"crowds\":[", uWS::OpCode::TEXT);
                                SendJsonList(p_Socket, s_CrowdsJson.Crowds);

                                p_Socket->send("],\"matis\":[", uWS::OpCode::TEXT);

                                bool s_FirstMati = true;

                                for (const auto& [s_MatiHash, s_MatiTextureHashes] : p_MatiTextures) {
                                    if (!s_FirstMati) {
                                        p_Socket->send(",", uWS::OpCode::TEXT);
                                    }
                                    else {
                                        s_FirstMati = false;
                                    }

                                    p_Socket->send("{", uWS::OpCode::TEXT);
                                    p_Socket->send(write_json("hash") + ":" + write_json(s_MatiHash) + ",", uWS::OpCode::TEXT);
                                    p_Socket->send(write_json("diffuse") + ":" + write_json(s_MatiTextureHashes.m_DiffuseTextureHash) + ",", uWS::OpCode::TEXT);
                                    p_Socket->send(write_json("normal") + ":" + write_json(s_MatiTextureHashes.m_NormalTextureHash) + ",", uWS::OpCode::TEXT);
                                    p_Socket->send(write_json("specular") + ":" + write_json(s_MatiTextureHashes.m_SpecularTextureHash), uWS::OpCode::TEXT);
                                    p_Socket->send("}", uWS::OpCode::TEXT);
                                }

                                p_Socket->send("],\"primMatis\":[", uWS::OpCode::TEXT);

                                bool s_FirstPrim = true;

                                for (const auto& [s_PrimHash, s_Matis] : p_PrimMatis) {
                                    if (!s_FirstPrim) {
                                        p_Socket->send(",", uWS::OpCode::TEXT);
                                    }
                                    else {
                                        s_FirstPrim = false;
                                    }

                                    p_Socket->send("{", uWS::OpCode::TEXT);
                                    p_Socket->send(write_json("primHash") + ":" + write_json(s_PrimHash) + ",", uWS::OpCode::TEXT);
                                    p_Socket->send(write_json("matiHashes") + ":[", uWS::OpCode::TEXT);

                                    s_FirstMati = true;

                                    for (const auto& s_MatiHash : s_Matis) {
                                        if (!s_FirstMati) {
                                            p_Socket->send(",", uWS::OpCode::TEXT);
                                        }
                                        else {
                                            s_FirstMati = false;
                                        }

                                        p_Socket->send(write_json(s_MatiHash), uWS::OpCode::TEXT);
                                    }

                                    p_Socket->send("]}", uWS::OpCode::TEXT);
                                }

                                p_Socket->send("]}", uWS::OpCode::TEXT);
                                p_Socket->send("Done sending entities.", uWS::OpCode::TEXT);
                                Logger::Info("[Editor] Done sending scene.");
                        }
                    );
                }
        },
        [this, p_ClientId]() -> void {
            DeferToClient(
                p_ClientId,
                [](WebSocket* p_Socket) {
                    p_Socket->send("Rebuilding tree.", uWS::OpCode::TEXT);
                }
            );
        }
    );
}


namespace {
    struct TemplateSubEntity {
        ZTemplateEntityBlueprintFactory* Factory;
        uint32_t Index;

        const ZString& Name() const { return Factory->m_pTemplateEntityBlueprint->subEntities[Index].entityName; }
    };

    IEntityBlueprintFactory* GetDefiningBlueprintFactory(ZEntityRef p_Entity) {
        if (const auto s_Owner = p_Entity.GetOwningEntity()) {
            return s_Owner.GetBlueprintFactory();
        }

        return p_Entity.GetBlueprintFactory();
    }

    std::optional<TemplateSubEntity> FindTemplateSubEntity(IEntityBlueprintFactory* p_Factory, uint64_t p_EntityId) {
        if (!p_Factory) {
            return std::nullopt;
        }

        const auto s_Index = p_Factory->GetSubEntityIndex(p_EntityId);

        if (s_Index < 0) {
            return std::nullopt;
        }

        ZTemplateEntityBlueprintFactory* s_TemplateFactory = nullptr;
        uint32_t s_TemplateIndex = s_Index;

        if (p_Factory->IsTemplateEntityBlueprintFactory()) {
            s_TemplateFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Factory);
        }
        else if (p_Factory->IsAspectEntityBlueprintFactory()) {
            const auto s_AspectFactory = reinterpret_cast<ZAspectEntityBlueprintFactory*>(p_Factory);

            if (static_cast<size_t>(s_Index) >= s_AspectFactory->m_aSubEntitiesLookUp.size()) {
                return std::nullopt;
            }

            const auto& s_Lookup = s_AspectFactory->m_aSubEntitiesLookUp[s_Index];

            if (s_Lookup.m_nAspectIdx >= s_AspectFactory->m_aBlueprintFactories.size()) {
                return std::nullopt;
            }

            const auto s_AspectBlueprint = s_AspectFactory->m_aBlueprintFactories[s_Lookup.m_nAspectIdx];

            if (!s_AspectBlueprint || !s_AspectBlueprint->IsTemplateEntityBlueprintFactory()) {
                return std::nullopt;
            }

            s_TemplateFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_AspectBlueprint);
            s_TemplateIndex = s_Lookup.m_nSubentityIdx;
        }

        if (!s_TemplateFactory || !s_TemplateFactory->m_pTemplateEntityBlueprint ||
            s_TemplateIndex >= s_TemplateFactory->m_pTemplateEntityBlueprint->subEntities.size()) {
            return std::nullopt;
        }

        return TemplateSubEntity { s_TemplateFactory, s_TemplateIndex };
    }
}

void EditorServer::SendEntityList(
    const std::string& p_ClientId, std::shared_ptr<EntityTreeNode> p_Tree, std::optional<int64_t> p_MessageId
) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping SendEntityList.");
        return;
    }
    std::ostringstream s_EventStream;

    s_EventStream << "{";

    if (p_MessageId) {
        s_EventStream << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_EventStream << write_json("type") << ":" << write_json("entityList") << ",";
    s_EventStream << write_json("entities") << ":" << "[";

    std::queue<std::shared_ptr<EntityTreeNode>> s_NodeQueue;

    if (p_Tree) {
        s_NodeQueue.push(p_Tree);
    }

    bool s_First = true;

    while (!s_NodeQueue.empty()) {
        auto s_Node = s_NodeQueue.front();
        s_NodeQueue.pop();

        for (auto& childPair : s_Node->Children) {
            s_NodeQueue.push(childPair.second);
        }

        // The special nodes only group their children and have no real entity.
        if (!s_Node->Entity || Plugin()->IsSpecialEntityTreeNode(s_Node->Entity)) {
            continue;
        }

        if (!s_First) {
            s_EventStream << ",";
        }

        s_First = false;

        s_EventStream << "{";
        s_EventStream << write_json("id") << ":" << write_json(std::format("{:016x}", s_Node->EntityId)) << ",";
        s_EventStream << write_json("source") << ":" << write_json("game") << ",";
        s_EventStream << write_json("tblu") << ":" << write_json(
            std::format("{:016X}", s_Node->BlueprintFactory.GetID())
        ) << ",";
        s_EventStream << write_json("type") << ":" << write_json(
            (*s_Node->Entity->GetType()->m_pInterfaceData)[0].m_Type->GetTypeInfo()->pszTypeName
        );

        if (const auto s_SubEntity = FindTemplateSubEntity(
            GetDefiningBlueprintFactory(s_Node->Entity), s_Node->Entity->GetType()->m_nEntityID
        )) {
            s_EventStream << "," << write_json("name") << ":" << write_json(s_SubEntity->Name());
        }

        s_EventStream << "}";
    }

    s_EventStream << "]";

    s_EventStream << "}";

    SendToClient(p_ClientId, s_EventStream.str());
}

void EditorServer::SendEntityDetails(
    const std::string& p_ClientId, ZEntityRef p_Entity, std::optional<int64_t> p_MessageId
) {
    if (!m_Enabled) {
        Logger::Info("[Editor] EditorServer disabled. Skipping SendEntityDetails.");
        return;
    }
    if (!p_Entity) {
        throw std::runtime_error("Could not find entity for the given selector.");
    }

    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("entityDetails") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, p_Entity);

    s_Event << "}";

    SendToClient(p_ClientId, s_Event.str());
}

bool EditorServer::IsPropertyValueTrue(const SPropertyData* s_Property, const ZEntityRef& p_Entity) {
    const auto* s_PropertyInfo = s_Property->GetPropertyInfo();

    if (!s_PropertyInfo || !s_PropertyInfo->m_propertyInfo.m_Type) {
        return true;
    }

    const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(p_Entity.m_pObj) + s_Property->m_nPropertyOffset;
    const uint16_t s_TypeSize = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeSize;
    const uint16_t s_TypeAlignment = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeAlignment;
    const std::string s_TypeName = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->pszTypeName;

    // Get the value of the property.
    auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

    if (s_PropertyInfo->m_propertyInfo.m_Flags & E_HAS_GETTER_SETTER)
        s_PropertyInfo->m_propertyInfo.m_PropertyGetter(
            reinterpret_cast<void*>(s_PropertyAddress),
            s_Data,
            s_PropertyInfo->m_propertyInfo.m_nExtraData
        );
    else
        s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(
            s_Data, reinterpret_cast<void*>(s_PropertyAddress)
        );

    const auto s_JsonProperty = HM3_GameStructToJson(s_TypeName.c_str(), s_Data, s_TypeSize);

    s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->destruct(s_Data);
    (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);

    if (!s_JsonProperty) {
        return true;
    }

    const auto s_PropertyValue = std::string_view(s_JsonProperty->JsonData, s_JsonProperty->StrSize);
    const bool value = s_PropertyValue == "true";
    HM3_FreeJsonString(s_JsonProperty);
    return value;
}

bool EditorServer::IsExcludedFromNavMeshExport(const ZEntityRef& p_Entity) {
    // Check if m_bRemovePhysics is true. If it is true, skip this entity.
    const auto s_EntityType = p_Entity->GetType();
    const std::string s_RemovePhysicsPropertyName = "m_bRemovePhysics";
    const std::string s_DisableNavmeshExportPropertyName = "m_bDisableNavmeshExport";
    if (s_EntityType && s_EntityType->m_pPropertyData) {
        for (uint32_t i = 0; i < s_EntityType->m_pPropertyData->size(); ++i) {
            const SPropertyData* s_Property = &(*s_EntityType->m_pPropertyData)[i];
            const auto* s_PropertyInfo = s_Property->GetPropertyInfo();

            if (!s_PropertyInfo ||
                !s_PropertyInfo->m_propertyInfo.m_Type ||
                !s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()) {
                continue;
            }

            if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsResource() ||
                s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyID) {
                // Some properties don't have a name for some reason. Try to find using RL.

                if (const auto [s_data, s_size] =
                        HM3_GetPropertyName(s_Property->m_nPropertyID); s_size > 0) {
                    if (const auto s_PropertyNameView = std::string_view(s_data, s_size);
                        s_PropertyNameView == s_RemovePhysicsPropertyName) {
                        if (IsPropertyValueTrue(s_Property, p_Entity)) {
                            return true;
                        }
                    }
                    else if (s_PropertyNameView == s_DisableNavmeshExportPropertyName) {
                        if (IsPropertyValueTrue(s_Property, p_Entity)) {
                            return true;
                        }
                    }
                }
            }
            else if (s_PropertyInfo->m_pszPropertyName) {
                if (s_PropertyInfo->m_pszPropertyName == s_RemovePhysicsPropertyName) {
                    if (IsPropertyValueTrue(s_Property, p_Entity)) {
                        return true;
                    }
                }
                else if (s_PropertyInfo->m_pszPropertyName == s_DisableNavmeshExportPropertyName) {
                    if (IsPropertyValueTrue(s_Property, p_Entity)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool EditorServer::SendEntitiesDetails(
    WebSocket* p_Socket, const std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>& p_Entities
) {
    bool s_IsFirstItemActuallySentInThisCall = true;
    bool s_DidSendAnything = false;

    for (const auto& [s_Hashes, s_Quat, s_Entity] : p_Entities) {
        for (const auto& s_Hash : s_Hashes) {
            if (!s_IsFirstItemActuallySentInThisCall) {
                p_Socket->send(",", uWS::OpCode::TEXT);
            }
            s_IsFirstItemActuallySentInThisCall = false;
            s_DidSendAnything = true;

            std::ostringstream s_Event;
            WriteEntityTransforms(s_Event, s_Quat, s_Entity);
            p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
        }
    }
    return s_DidSendAnything;
}

void EditorServer::SendJsonList(WebSocket* p_Socket, const std::vector<std::string>& p_Items) {
    for (size_t i = 0; i < p_Items.size(); ++i) {
        if (i) {
            p_Socket->send(",", uWS::OpCode::TEXT);
        }

        p_Socket->send(p_Items[i], uWS::OpCode::TEXT);
    }
}

void EditorServer::WriteEntityIdentity(std::ostream& p_Stream, ZEntityRef p_Entity) {
    p_Stream << write_json("id") << ":" << write_json(
        std::format("{:016x}", p_Entity->GetType()->m_nEntityID)
    ) << ",";

    const auto s_Factory = GetDefiningBlueprintFactory(p_Entity);

    if (s_Factory) {
        if (const auto s_SubEntity = FindTemplateSubEntity(s_Factory, p_Entity->GetType()->m_nEntityID)) {
            p_Stream << write_json("name") << ":" << write_json(s_SubEntity->Name()) << ",";
            p_Stream << write_json("tblu") << ":" << write_json(
                std::format("{:016X}", static_cast<ZEntityBlueprintFactoryBase*>(s_Factory)->m_ridResource.GetID())
            ) << ",";
        }
    }
    else {
        p_Stream << write_json("name") << ":" << write_json(Plugin()->GetEntityName(p_Entity, false)) << ",";
    }
}

namespace {
    // GetProperty<T>().Get() null-derefs unless T is the exact type, so reads check As<T>() or read entity refs raw.
    bool IsEntityRefType(const STypeID* p_Type) {
        const auto* s_TypeInfo = p_Type->GetTypeInfo();
        return s_TypeInfo->IsEntity() && s_TypeInfo->m_nTypeSize == sizeof(TEntityRef<ZSpatialEntity>);
    }

    template <typename T, typename F>
    void WriteTypedProperty(std::ostream& p_Stream, ZEntityRef p_Entity, std::string_view p_Name, F&& p_Write) {
        const auto s_Value = p_Entity.GetProperty<T>(ZString(p_Name));

        if (const T* s_Data = s_Value.template As<T>()) {
            p_Write(*s_Data);
            return;
        }

        Logger::Warn(
            "[Editor] Crowd export: '{}'.{} is {}, expected {}.",
            Plugin()->GetEntityName(p_Entity), p_Name, s_Value.GetTypeID()->GetTypeInfo()->pszTypeName, ZHMTypeName<T>
        );
        p_Stream << "null";
    }

    template <typename T>
    void WriteScalarProperty(std::ostream& p_Stream, ZEntityRef p_Entity, std::string_view p_Name) {
        WriteTypedProperty<T>(p_Stream, p_Entity, p_Name, [&](const T& p_Value) { p_Stream << write_json(p_Value); });
    }

    ZEntityRef ReadEntityRefProperty(ZEntityRef p_Entity, const ZString& p_Name) {
        const auto s_Value = p_Entity.GetProperty<TEntityRef<ZSpatialEntity>>(p_Name);

        if (s_Value.IsEmpty() || !IsEntityRefType(s_Value.GetTypeID())) {
            return {};
        }

        return static_cast<const TEntityRef<ZSpatialEntity>*>(s_Value.GetData())->m_entityRef;
    }

    std::vector<ZEntityRef> ReadEntityRefArrayProperty(ZEntityRef p_Entity, const ZString& p_Name) {
        const auto s_Value = p_Entity.GetProperty<TArray<TEntityRef<ZSpatialEntity>>>(p_Name);
        std::vector<ZEntityRef> s_Refs;

        if (s_Value.IsEmpty() || !s_Value.GetTypeID()->GetTypeInfo()->IsArray()) {
            return s_Refs;
        }

        const auto* s_ArrayType = reinterpret_cast<const IArrayType*>(s_Value.GetTypeID()->GetTypeInfo());

        if (!IsEntityRefType(s_ArrayType->containerType.elementType)) {
            return s_Refs;
        }

        for (const auto& s_Ref : *static_cast<const TArray<TEntityRef<ZSpatialEntity>>*>(s_Value.GetData())) {
            s_Refs.push_back(s_Ref.m_entityRef);
        }

        return s_Refs;
    }

    std::string_view TeleportActionName(TeleportActionType p_Action) {
        switch (p_Action) {
            case TeleportActionType::STAY: return "STAY";
            case TeleportActionType::TELEPORT: return "TELEPORT";
            case TeleportActionType::LEAVE_CROWD: return "LEAVE_CROWD";
            default: return "UNKNOWN";
        }
    }

    std::string_view CrowdFlowCandidatesName(ECrowdFlowCandidates p_Value) {
        switch (p_Value) {
            case ECrowdFlowCandidates::NONE: return "NONE";
            case ECrowdFlowCandidates::EVERYONE: return "EVERYONE";
            case ECrowdFlowCandidates::SPAWNED_ON_FLOW: return "SPAWNED_ON_FLOW";
            default: return "UNKNOWN";
        }
    }

    void WriteCrowdEntityArray(
        std::ostream& p_Stream,
        ZEntityRef p_Crowd,
        std::string_view p_Key,
        std::string_view p_PropertyName,
        void (*p_WriteEntity)(std::ostream&, ZEntityRef)
    ) {
        p_Stream << "," << write_json(p_Key) << ":[";
        bool s_First = true;

        for (const auto& s_Ref : ReadEntityRefArrayProperty(p_Crowd, ZString(p_PropertyName))) {
            if (!s_Ref) {
                Logger::Debug(
                    "[Editor] Crowd export: '{}'.{} has an unresolved entry, skipping it.",
                    Plugin()->GetEntityName(p_Crowd), p_PropertyName
                );
                continue;
            }

            if (!s_First) {
                p_Stream << ",";
            }

            s_First = false;
            p_WriteEntity(p_Stream, s_Ref);
        }

        p_Stream << "]";
    }
}

void EditorServer::WriteSVector3(std::ostream& p_Stream, const SVector3& p_Vector) {
    WriteVector3(p_Stream, p_Vector.x, p_Vector.y, p_Vector.z);
}

void EditorServer::WriteWorldMatrix(std::ostream& p_Stream, ZEntityRef p_Entity) {
    const auto s_Spatial = p_Entity.QueryInterface<ZSpatialEntity>();

    if (!s_Spatial) {
        p_Stream << "null";
        return;
    }

    const SMatrix s_Matrix = s_Spatial->GetObjectToWorldMatrix();
    p_Stream << "{" << write_json("x") << ":";
    WriteVector3(p_Stream, s_Matrix.XAxis.x, s_Matrix.XAxis.y, s_Matrix.XAxis.z);
    p_Stream << "," << write_json("y") << ":";
    WriteVector3(p_Stream, s_Matrix.YAxis.x, s_Matrix.YAxis.y, s_Matrix.YAxis.z);
    p_Stream << "," << write_json("z") << ":";
    WriteVector3(p_Stream, s_Matrix.ZAxis.x, s_Matrix.ZAxis.y, s_Matrix.ZAxis.z);
    p_Stream << "," << write_json("t") << ":";
    WriteVector3(p_Stream, s_Matrix.Trans.x, s_Matrix.Trans.y, s_Matrix.Trans.z);
    p_Stream << "}";
}

void EditorServer::WriteVolume(std::ostream& p_Stream, ZEntityRef p_Volume) {
    Quat s_Quat;

    if (p_Volume) {
        try {
            s_Quat = Editor::GetWorldQuat(p_Volume);
        }
        catch (const std::runtime_error&) {
            Logger::Error("[Editor] Circular reference found! Sending crowd volume without rotation.");
        }
    }

    WriteEntityTransforms(p_Stream, s_Quat, p_Volume);
}

void EditorServer::WriteControlPoints(std::ostream& p_Stream, ZEntityRef p_Entity) {
    p_Stream << write_json("points") << ":";

    WriteTypedProperty<TArray<SVector3>>(p_Stream, p_Entity, "m_aControlPoints", [&](const TArray<SVector3>& p_Points) {
        p_Stream << "[";
        bool s_First = true;

        for (const auto& s_Point : p_Points) {
            if (!s_First) {
                p_Stream << ",";
            }

            s_First = false;
            WriteSVector3(p_Stream, s_Point);
        }

        p_Stream << "]";
    });
}

void EditorServer::WriteCrowdTeleport(std::ostream& p_Stream, ZEntityRef p_Teleport) {
    p_Stream << "{";
    WriteEntityIdentity(p_Stream, p_Teleport);
    p_Stream << write_json("exitZone") << ":";
    WriteVolume(p_Stream, ReadEntityRefProperty(p_Teleport, "m_rExitZone"));
    p_Stream << "," << write_json("enterZone") << ":";
    WriteVolume(p_Stream, ReadEntityRefProperty(p_Teleport, "m_rEnterZone"));
    p_Stream << "," << write_json("exitActionAmbient") << ":";
    WriteTypedProperty<TeleportActionType>(
        p_Stream, p_Teleport, "m_eExitActionAmbient", [&](TeleportActionType p_Action) {
            p_Stream << write_json(TeleportActionName(p_Action));
        }
    );
    p_Stream << "," << write_json("exitActionScared") << ":";
    WriteTypedProperty<TeleportActionType>(
        p_Stream, p_Teleport, "m_eExitActionScared", [&](TeleportActionType p_Action) {
            p_Stream << write_json(TeleportActionName(p_Action));
        }
    );
    p_Stream << "}";
}

void EditorServer::WriteSplineFlow(std::ostream& p_Stream, ZEntityRef p_Spline) {
    p_Stream << "{";
    WriteEntityIdentity(p_Stream, p_Spline);
    p_Stream << write_json("transform") << ":";
    WriteWorldMatrix(p_Stream, p_Spline);
    p_Stream << "," << write_json("flowWidth") << ":";
    WriteScalarProperty<float32>(p_Stream, p_Spline, "m_nFlowWidth");
    p_Stream << "," << write_json("lengthTessellation") << ":";
    WriteScalarProperty<float32>(p_Stream, p_Spline, "m_nLengthTessellation");
    p_Stream << "," << write_json("widthTessellation") << ":";
    WriteScalarProperty<float32>(p_Stream, p_Spline, "m_nWidthTessellation");
    p_Stream << "," << write_json("extrusionHeight") << ":";
    WriteScalarProperty<float32>(p_Stream, p_Spline, "m_fExtrusionHeight");
    p_Stream << "," << write_json("closed") << ":";
    WriteScalarProperty<bool>(p_Stream, p_Spline, "m_bIsControlPointsPathClosed");
    p_Stream << ",";
    WriteControlPoints(p_Stream, p_Spline);
    p_Stream << "}";
}

void EditorServer::WriteCrowdVolumeIndexArray(
    std::ostream& p_Stream,
    ZEntityRef p_Crowd,
    std::string_view p_Key,
    std::string_view p_PropertyName,
    const EntityIndexMap& p_Indices
) {
    p_Stream << "," << write_json(p_Key) << ":[";
    bool s_First = true;

    for (const auto& s_Ref : ReadEntityRefArrayProperty(p_Crowd, ZString(p_PropertyName))) {
        const auto s_Index = s_Ref ? p_Indices.find(s_Ref.m_pObj) : p_Indices.end();

        if (s_Index == p_Indices.end()) {
            Logger::Debug(
                "[Editor] Crowd export: '{}'.{} references '{}', which isn't in the exported volumes, skipping it.",
                Plugin()->GetEntityName(p_Crowd), p_PropertyName, s_Ref ? Plugin()->GetEntityName(s_Ref) : "null"
            );
            continue;
        }

        if (!s_First) {
            p_Stream << ",";
        }

        s_First = false;
        p_Stream << s_Index->second;
    }

    p_Stream << "]";
}

EditorServer::CrowdsJson EditorServer::WriteCrowdsJson(
    const SceneEntities& p_VolumeBoxEntities, const SceneEntities& p_VolumeSphereEntities
) {
    const auto s_VolumeShapeEntities = Plugin()->FindEntitiesByType("ZVolumeShapeEntity", "00A69EDD9DF220A8");
    const auto s_CrowdEntities = Plugin()->FindEntitiesByType("ZCrowdEntity", "00D97E3F727E82CB");

    const auto s_BuildIndexMap = [](const SceneEntities& p_Entities) {
        EntityIndexMap s_Indices;

        for (size_t i = 0; i < p_Entities.size(); ++i) {
            s_Indices.emplace(std::get<2>(p_Entities[i]).m_pObj, i);
        }

        return s_Indices;
    };

    const CrowdVolumeIndices s_VolumeIndices {
        s_BuildIndexMap(p_VolumeBoxEntities),
        s_BuildIndexMap(p_VolumeSphereEntities),
        s_BuildIndexMap(s_VolumeShapeEntities)
    };

    CrowdsJson s_Json;

    for (const auto& s_Shape : s_VolumeShapeEntities | std::views::elements<2>) {
        s_Json.VolumeShapes.push_back(WriteVolumeShape(s_Shape));
    }

    for (const auto& s_Crowd : s_CrowdEntities | std::views::elements<2>) {
        s_Json.Crowds.push_back(WriteCrowdEntity(s_Crowd, s_VolumeIndices));
    }

    return s_Json;
}

std::string EditorServer::WriteVolumeShape(ZEntityRef p_Shape) {
    std::ostringstream s_Stream;
    s_Stream << "{";
    WriteEntityIdentity(s_Stream, p_Shape);
    s_Stream << write_json("transform") << ":";
    WriteWorldMatrix(s_Stream, p_Shape);
    s_Stream << ",";
    WriteControlPoints(s_Stream, p_Shape);
    s_Stream << "," << write_json("extrusionHeight") << ":";
    WriteScalarProperty<float32>(s_Stream, p_Shape, "m_fExtrusionHeight");
    s_Stream << "," << write_json("flipSurfaceNormal") << ":";
    WriteScalarProperty<bool>(s_Stream, p_Shape, "m_bFlipSurfaceNormal");
    s_Stream << "," << write_json("closed") << ":";
    WriteScalarProperty<bool>(s_Stream, p_Shape, "m_bIsControlPointsPathClosed");
    s_Stream << "}";
    return s_Stream.str();
}

std::string EditorServer::WriteCrowdEntity(ZEntityRef p_Crowd, const CrowdVolumeIndices& p_VolumeIndices) {
    std::ostringstream s_Stream;
    s_Stream << "{";
    const auto s_Instance = p_Crowd.GetOwningEntity() ? p_Crowd.GetOwningEntity() : p_Crowd;
    WriteEntityIdentity(s_Stream, s_Instance);

    s_Stream << write_json("transform") << ":";
    WriteWorldMatrix(s_Stream, p_Crowd);
    s_Stream << "," << write_json("globalSize") << ":";
    WriteTypedProperty<SVector3>(s_Stream, p_Crowd, "m_vGlobalSize", [&](const SVector3& p_Size) {
        WriteSVector3(s_Stream, p_Size);
    });
    s_Stream << "," << write_json("cellSize") << ":";
    WriteScalarProperty<float32>(s_Stream, p_Crowd, "m_nGridCellSize");
    s_Stream << "," << write_json("navMeshShrink") << ":";
    WriteScalarProperty<float32>(s_Stream, p_Crowd, "m_nGridCellNavMeshShrink");
    s_Stream << "," << write_json("generatorOffset") << ":";
    WriteScalarProperty<float32>(s_Stream, p_Crowd, "m_nGridGeneratorOffset");
    s_Stream << "," << write_json("smoothenExitFlows") << ":";
    WriteScalarProperty<bool>(s_Stream, p_Crowd, "m_bSmoothenExitFlows");
    s_Stream << "," << write_json("flowCandidates") << ":";
    WriteTypedProperty<ECrowdFlowCandidates>(s_Stream, p_Crowd, "m_eFlowCandidates", [&](ECrowdFlowCandidates p_Value) {
        s_Stream << write_json(CrowdFlowCandidatesName(p_Value));
    });

    const auto& [s_Boxes, s_Spheres, s_Shapes] = p_VolumeIndices;
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "exclusionBoxes", "m_aCrowdExclusionBoxes", s_Boxes);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "inclusionBoxes", "m_aCrowdInclusionBoxes", s_Boxes);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "panicOnlyBoxes", "m_aCrowdPanicOnlyBoxes", s_Boxes);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "exclusionSpheres", "m_aCrowdExclusionSpheres", s_Spheres);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "exclusionShapes", "m_aCrowdExclusionVolumeShapes", s_Shapes);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "inclusionShapes", "m_aCrowdInclusionVolumeShapes", s_Shapes);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "panicOnlyShapes", "m_aCrowdPanicOnlyVolumeShapes", s_Shapes);
    WriteCrowdEntityArray(s_Stream, p_Crowd, "teleports", "m_aCrowdTeleports", WriteCrowdTeleport);
    WriteCrowdEntityArray(s_Stream, p_Crowd, "groupBoxes", "m_aCrowdGroupBoxes", WriteVolume);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "flowBoxes", "m_aFlowBoxes", s_Boxes);
    WriteCrowdVolumeIndexArray(s_Stream, p_Crowd, "flowBoxesTwoWay", "m_aFlowBoxesTwoWay", s_Boxes);
    WriteCrowdEntityArray(s_Stream, p_Crowd, "splineFlows", "m_aSplineFlows", WriteSplineFlow);

    s_Stream << "}";
    return s_Stream.str();
}

void EditorServer::WriteEntityTransforms(std::ostream& p_Stream, Quat p_Quat, ZEntityRef p_Entity) {
    if (!p_Entity) {
        p_Stream << "null";
        return;
    }

    p_Stream << "{";

    WriteEntityIdentity(p_Stream, p_Entity);

    // Write transform.
    if (const auto s_Spatial = p_Entity.QueryInterface<ZSpatialEntity>()) {
        const auto s_Trans = s_Spatial->GetObjectToWorldMatrix();

        SMatrix p_Transform = s_Spatial->GetObjectToWorldMatrix();
        const auto s_Decomposed = p_Transform.Decompose();
        p_Stream << write_json("position") << ":";
        WriteVector3(p_Stream, s_Decomposed.Position.x, s_Decomposed.Position.y, s_Decomposed.Position.z);
        p_Stream << ",";
    }
    p_Stream << write_json("rotation") << ":";
    WriteQuat(p_Stream, p_Quat.m.x, p_Quat.m.y, p_Quat.m.z, p_Quat.m.w);

    //Write the BBox info for the gates
    auto* s_Gate = p_Entity.QueryInterface<ZGateEntity>();
    if (s_Gate) {
        p_Stream << ",";
        p_Stream << write_json("bboxCenter") << ":";
        WriteVector3(p_Stream, s_Gate->m_vCenter.x, s_Gate->m_vCenter.y, s_Gate->m_vCenter.z);
        p_Stream << ",";
        p_Stream << write_json("bboxHalfSize") << ":";
        WriteVector3(p_Stream, s_Gate->m_vHalfSize.x, s_Gate->m_vHalfSize.y, s_Gate->m_vHalfSize.z);
    }
    //Same for the AIAreas
    auto* s_AiArea = p_Entity.QueryInterface<ZAIAreaEntity>();
    if (s_AiArea) {
        //We go up the logical parent chain until we reach a ZAIAreaEntity or a ZAIAreaWorldEntity, as some have ZEntity as logical parents
        std::vector<std::string> parents = std::vector<std::string>();
        ZEntityRef parentRef = p_Entity.GetLogicalParent();

        while (parentRef.m_pObj) {
            parents.push_back(Plugin()->GetEntityName(parentRef));
            if (parentRef.QueryInterface<ZAIAreaEntityBase>())
                break;
            parentRef = parentRef.GetLogicalParent();
        }

        p_Stream << ",";
        p_Stream << write_json("logicalParent") << ":[";
        for (auto i = 0; i < parents.size(); i++) {
            if (i)
                p_Stream << ",";
            p_Stream << write_json(parents[i]);
        }
        p_Stream << "]";

        p_Stream << ",";
        p_Stream << write_json("areaVolumeNames") << ":[";
        for (auto i = 0; i < s_AiArea->m_aAreaVolumes.size(); i++) {
            if (i)
                p_Stream << ",";
            p_Stream << write_json(Plugin()->GetEntityName(s_AiArea->m_aAreaVolumes[i].m_entityRef));
        }
        p_Stream << "]";
    }

    std::unordered_map<std::string_view, std::string> propNameToFieldName = {
        {"m_PrimitiveScale", "scale"},
        {"m_eType", "type"},
        {"m_vGlobalSize", "scale"},
        {"m_vRoomMin", "roomExtentMin"},
        {"m_vRoomMax", "roomExtentMax"},
        {"m_rParentArea", "parent"},
        {"m_fRadius", "radius"},
        {"m_aInclusions", "inclusions"},
        {"m_nGroupSize", "groupSize"}
    };

    if (const auto s_EntityType = p_Entity->GetType(); s_EntityType && s_EntityType->m_pPropertyData) {
        for (uint32_t i = 0; i < s_EntityType->m_pPropertyData->size(); ++i) {
            SPropertyData* s_Property = &(*s_EntityType->m_pPropertyData)[i];
            const auto* s_PropertyInfo = s_Property->GetPropertyInfo();

            if (!s_PropertyInfo ||
                !s_PropertyInfo->m_propertyInfo.m_Type ||
                !s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()) {
                continue;
            }

            if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsResource() ||
                s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyID) {
                // Some properties don't have a name for some reason. Try to find using RL.

                if (const auto [s_data, s_size] = HM3_GetPropertyName(s_Property->m_nPropertyID); s_size > 0) {
                    auto s_PropertyNameView = std::string_view(s_data, s_size);
                    if (propNameToFieldName.count(s_PropertyNameView)) {
                        p_Stream << ",";
                        p_Stream << write_json(propNameToFieldName.at(s_PropertyNameView)) << ":";
                        WriteProperty(p_Stream, p_Entity, s_Property);
                    }
                }
            }
            else if (s_PropertyInfo->m_pszPropertyName) {
                if (propNameToFieldName.count(s_PropertyInfo->m_pszPropertyName)) {
                    p_Stream << ",";
                    p_Stream << write_json(propNameToFieldName.at(s_PropertyInfo->m_pszPropertyName)) << ":";
                    WriteProperty(p_Stream, p_Entity, s_Property);
                }
            }
        }
    }

    p_Stream << "}";
}

void EditorServer::WriteEntityDetails(std::ostream& p_Stream, ZEntityRef p_Entity) {
    if (!p_Entity) {
        p_Stream << "null";
        return;
    }
    Logger::Debug("[Editor] Sending entity details for entity id: '{}'", p_Entity->GetType()->m_nEntityID);

    p_Stream << "{";

    p_Stream << write_json("id") << ":" << write_json(std::format("{:016x}", p_Entity->GetType()->m_nEntityID)) << ",";

    const auto s_Factory = GetDefiningBlueprintFactory(p_Entity);

    if (s_Factory) {
        if (const auto s_SubEntity = FindTemplateSubEntity(s_Factory, p_Entity->GetType()->m_nEntityID)) {
            p_Stream << write_json("name") << ":" << write_json(s_SubEntity->Name()) << ",";
        }

        p_Stream << write_json("source") << ":" << write_json("game") << ",";

        p_Stream << write_json("tblu") << ":" << write_json(
            std::format("{:016X}", static_cast<ZEntityBlueprintFactoryBase*>(s_Factory)->m_ridResource.GetID())
        ) << ",";
    }
    else {
        // TODO: Name.
        p_Stream << write_json("source") << ":" << write_json("editor") << ",";
    }

    // Write type and interfaces.
    auto s_Interfaces = (*p_Entity->GetType()->m_pInterfaceData);

    p_Stream << write_json("type") << ":" << write_json(s_Interfaces[0].m_Type->GetTypeInfo()->pszTypeName) << ",";

    p_Stream << write_json("interfaces") << ":" << "[";
    bool s_FirstInterface = true;

    for (auto& s_Interface : s_Interfaces) {
        if (!s_Interface.m_Type) {
            continue;
        }

        const auto* s_TypeInfo = s_Interface.m_Type->GetTypeInfo();

        if (!s_TypeInfo || !s_TypeInfo->pszTypeName) {
            continue;
        }

        if (!s_FirstInterface) {
            p_Stream << ",";
        }

        s_FirstInterface = false;

        p_Stream << write_json(s_TypeInfo->pszTypeName);
    }

    p_Stream << "],";

    // Write transform.
    if (const auto s_Spatial = p_Entity.QueryInterface<ZSpatialEntity>()) {
        const auto s_Trans = s_Spatial->GetObjectToWorldMatrix();

        p_Stream << write_json("transform") << ":";
        WriteTransform(p_Stream, s_Spatial->GetObjectToWorldMatrix());
        p_Stream << ",";

        SMatrix s_ParentTrans;

        bool s_DoesntHaveParent = false;

        // Get parent entity.
        if (s_Spatial->m_eidParent.m_pInterfaceRef) {
            s_ParentTrans = s_Spatial->m_eidParent.m_pInterfaceRef->GetObjectToWorldMatrix();
        }
        else if (p_Entity.GetLogicalParent() && p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()) {
            s_ParentTrans = p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
        }
        else if (p_Entity.GetOwningEntity() && p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()) {
            s_ParentTrans = p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
        }
        else {
            s_DoesntHaveParent = true;
        }

        if (!s_DoesntHaveParent) {
            const auto s_ParentTransInv = s_ParentTrans.Inverse();

            auto s_LocalTrans = s_ParentTransInv * s_Trans;
            s_LocalTrans.Trans = s_Trans.Trans - s_ParentTrans.Trans;
            s_LocalTrans.Trans.w = 1.f;

            p_Stream << write_json("relativeTransform") << ":";
            WriteTransform(p_Stream, s_LocalTrans);
            p_Stream << ",";
        }
    }

    // Write properties.
    p_Stream << write_json("properties") << ":" << "{";

    const auto s_EntityType = p_Entity->GetType();
    bool s_FirstProperty = true;

    if (s_EntityType && s_EntityType->m_pPropertyData) {
        for (uint32_t i = 0; i < s_EntityType->m_pPropertyData->size(); ++i) {
            SPropertyData* s_Property = &(*s_EntityType->m_pPropertyData)[i];
            const auto* s_PropertyInfo = s_Property->GetPropertyInfo();

            if (!s_PropertyInfo ||
                !s_PropertyInfo->m_propertyInfo.m_Type ||
                !s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()) {
                continue;
            }

            if (!s_FirstProperty) {
                p_Stream << ",";
            }

            s_FirstProperty = false;

            WritePropertyName(p_Stream, s_Property);
            p_Stream << ":";
            WriteProperty(p_Stream, p_Entity, s_Property);
        }
    }

    p_Stream << "}";

    p_Stream << "}";
}

void EditorServer::WriteVector3(std::ostream& p_Stream, double p_X, double p_Y, double p_Z) {
    p_Stream << "{";
    p_Stream << write_json("x") << ":" << write_json(p_X) << ",";
    p_Stream << write_json("y") << ":" << write_json(p_Y) << ",";
    p_Stream << write_json("z") << ":" << write_json(p_Z);
    p_Stream << "}";
}

void EditorServer::WriteRotation(std::ostream& p_Stream, double p_Yaw, double p_Pitch, double p_Roll) {
    p_Stream << "{";
    p_Stream << write_json("yaw") << ":" << write_json(p_Yaw) << ",";
    p_Stream << write_json("pitch") << ":" << write_json(p_Pitch) << ",";
    p_Stream << write_json("roll") << ":" << write_json(p_Roll);
    p_Stream << "}";
}

void EditorServer::WriteQuat(std::ostream& p_Stream, double p_x, double p_y, double p_z, double p_w) {
    p_Stream << "{";
    p_Stream << write_json("x") << ":" << write_json(p_x) << ",";
    p_Stream << write_json("y") << ":" << write_json(p_y) << ",";
    p_Stream << write_json("z") << ":" << write_json(p_z) << ",";
    p_Stream << write_json("w") << ":" << write_json(p_w);
    p_Stream << "}";
}

void EditorServer::WriteTransform(std::ostream& p_Stream, SMatrix p_Transform) {
    const auto s_Decomposed = p_Transform.Decompose();
    const auto s_Euler = s_Decomposed.Quaternion.ToEuler();

    p_Stream << "{";

    p_Stream << write_json("position") << ":";
    WriteVector3(p_Stream, s_Decomposed.Position.x, s_Decomposed.Position.y, s_Decomposed.Position.z);
    p_Stream << ",";

    p_Stream << write_json("rotation") << ":";
    WriteRotation(p_Stream, s_Euler.yaw, s_Euler.pitch, s_Euler.roll);
    p_Stream << ",";

    p_Stream << write_json("scale") << ":";
    WriteVector3(p_Stream, s_Decomposed.Scale.x, s_Decomposed.Scale.y, s_Decomposed.Scale.z);

    p_Stream << "}";
}

void EditorServer::WritePropertyName(std::ostream& p_Stream, SPropertyData* p_Property) {
    const auto* s_PropertyInfo = p_Property->GetPropertyInfo();

    if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsResource() ||
        s_PropertyInfo->m_nPropertyID != p_Property->m_nPropertyID) {
        // Some properties don't have a name for some reason. Try to find using RL.
        const auto s_PropertyName = HM3_GetPropertyName(p_Property->m_nPropertyID);

        if (s_PropertyName.Size > 0) {
            p_Stream << write_json(std::string_view(s_PropertyName.Data, s_PropertyName.Size));
        }
        else {
            p_Stream << write_json(std::format("~{:08x}", p_Property->m_nPropertyID));
        }
    }
    else if (s_PropertyInfo->m_pszPropertyName) {
        p_Stream << write_json(s_PropertyInfo->m_pszPropertyName);
    }
    else {
        p_Stream << write_json(std::format("~{:08x}", p_Property->m_nPropertyID));
    }
}

void EditorServer::WriteProperty(std::ostream& p_Stream, ZEntityRef p_Entity, SPropertyData* p_Property) {
    p_Stream << "{" << write_json("type") << ":";

    const auto* s_PropertyInfo = p_Property->GetPropertyInfo();

    if (!s_PropertyInfo || !s_PropertyInfo->m_propertyInfo.m_Type) {
        p_Stream << write_json("unknown") << ",";
        p_Stream << write_json("data") << ":" << "null" << "}";
        return;
    }

    const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(p_Entity.m_pObj) + p_Property->m_nPropertyOffset;
    const uint16_t s_TypeSize = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeSize;
    const uint16_t s_TypeAlignment = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeAlignment;
    const std::string s_TypeName = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->pszTypeName;

    p_Stream << write_json(s_TypeName) << ",";
    p_Stream << write_json("data") << ":";

    // Get the value of the property.
    auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

    if (s_PropertyInfo->m_propertyInfo.m_Flags & EPropertyInfoFlags::E_HAS_GETTER_SETTER)
        s_PropertyInfo->m_propertyInfo.m_PropertyGetter(
            reinterpret_cast<void*>(s_PropertyAddress),
            s_Data,
            s_PropertyInfo->m_propertyInfo.m_nExtraData
        );
    else
        s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(
            s_Data, reinterpret_cast<void*>(s_PropertyAddress)
        );

    if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo() &&
        s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsEntity()) {
        auto* s_EntityData = reinterpret_cast<TEntityRef<ZEntityImpl>*>(s_Data);

        if (!s_EntityData || !*s_EntityData) {
            s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->destruct(s_Data);
            (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
            p_Stream << "null" << "}";
            return;
        }

        p_Stream << "{" << write_json("id") << ":" << write_json(
            std::format("{:016x}", s_EntityData->m_entityRef->GetType()->m_nEntityID)
        ) << ",";

        const auto s_Factory = GetDefiningBlueprintFactory(s_EntityData->m_entityRef);

        if (s_Factory) {
            const auto s_EntityId = s_EntityData->m_entityRef->GetType()->m_nEntityID;

            if (const auto s_SubEntity = FindTemplateSubEntity(s_Factory, s_EntityId)) {
                p_Stream << write_json("name") << ":" << write_json(s_SubEntity->Name()) << ",";
            }

            p_Stream << write_json("source") << ":" << write_json("game") << ",";

            p_Stream << write_json("tblu") << ":" << write_json(
                std::format("{:016X}", static_cast<ZEntityBlueprintFactoryBase*>(s_Factory)->m_ridResource.GetID())
            ) << ",";
        }
        else {
            // TODO: Name.
            p_Stream << write_json("source") << ":" << write_json("editor") << ",";
        }

        // Write type and interfaces.
        auto s_Interfaces = (*s_EntityData->m_entityRef->GetType()->m_pInterfaceData);

        p_Stream << write_json("type") << ":" << write_json(s_Interfaces[0].m_Type->GetTypeInfo()->pszTypeName) << "}}";

        s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->destruct(s_Data);
        (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
    }
    else if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsArray()) {
        const auto* s_ArrayType = reinterpret_cast<const IArrayType*>(
            s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()
        );
        const auto* s_ElementTypeID = s_ArrayType->containerType.elementType;
        const auto* s_ElementTypeInfo = s_ElementTypeID->GetTypeInfo();

        if (s_ElementTypeInfo->IsEntity()) {
            p_Stream << "[";

            void* s_Iterator = s_ArrayType->containerType.pVTab->begin(s_Data);
            void* s_End = s_ArrayType->containerType.pVTab->end(s_Data);

            bool s_IsFirst = true;

            for (; s_Iterator != s_End; s_Iterator = s_ArrayType->containerType.pVTab->next(s_Data, s_Iterator)) {
                if (!s_IsFirst) {
                    p_Stream << ",";
                }

                s_IsFirst = false;

                const auto* s_EntityRef = reinterpret_cast<const ZEntityRef*>(s_Iterator);

                if (*s_EntityRef) {
                    p_Stream << write_json(
                        std::format("{:016x}", (*s_EntityRef)->GetType()->m_nEntityID)
                    );
                }
                else {
                    p_Stream << "null";
                }
            }

            p_Stream << "]}";
        }
        else {
            auto s_JsonProperty = HM3_GameStructToJson(s_TypeName.c_str(), s_Data, s_TypeSize);

            s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->destruct(s_Data);
            (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);

            if (!s_JsonProperty) {
                p_Stream << "null" << "}";
                return;
            }

            p_Stream << std::string_view(s_JsonProperty->JsonData, s_JsonProperty->StrSize) << "}";
            HM3_FreeJsonString(s_JsonProperty);
        }
    }
    else {
        auto s_JsonProperty = HM3_GameStructToJson(s_TypeName.c_str(), s_Data, s_TypeSize);

        s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->destruct(s_Data);
        (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);

        if (!s_JsonProperty) {
            p_Stream << "null" << "}";
            return;
        }

        p_Stream << std::string_view(s_JsonProperty->JsonData, s_JsonProperty->StrSize) << "}";
        HM3_FreeJsonString(s_JsonProperty);
    }
}

EntitySelector EditorServer::ReadEntitySelector(simdjson::ondemand::value p_Selector) {
    const std::string_view s_IdString = p_Selector["id"];
    const auto s_Id64 = std::stoull(std::string(s_IdString), nullptr, 16);

    auto s_TbluField = p_Selector.find_field("tblu");

    if (s_TbluField.error() == simdjson::SUCCESS) {
        const std::string_view s_TbluString = s_TbluField;
        const auto s_Tblu64 = std::stoull(std::string(s_TbluString), nullptr, 16);

        return {
            .EntityId = s_Id64,
            .TbluHash = std::make_optional(s_Tblu64),
            .PrimHash = std::nullopt,
        };
    }

    return {
        .EntityId = s_Id64,
        .TbluHash = std::nullopt,
        .PrimHash = std::nullopt,
    };
}

std::vector<EntitySelector> EditorServer::ReadPrimEntitySelectors(simdjson::ondemand::array p_Selector) {
    std::vector<EntitySelector> s_EntitySelectors;
    for (const std::string_view s_PrimStringView : p_Selector) {
        const auto s_PrimString = std::string { s_PrimStringView };
        Logger::Info("[Editor] Reading PrimEntitySelector for hash: '{}'", s_PrimString);

        s_EntitySelectors.push_back(
            {
                .EntityId = 0,
                .TbluHash = std::nullopt,
                .PrimHash = std::make_optional(s_PrimString),
            }
            );
    }

    return s_EntitySelectors;
}

SVector3 EditorServer::ReadVector3(simdjson::ondemand::value p_Vector) {
    return {
        float(double(p_Vector["x"])),
        float(double(p_Vector["y"])),
        float(double(p_Vector["z"]))
    };
}


EulerAngles EditorServer::ReadRotation(simdjson::ondemand::value p_Rotation) {
    return {
        float(double(p_Rotation["yaw"])),
        float(double(p_Rotation["pitch"])),
        float(double(p_Rotation["roll"]))
    };
}

SMatrix EditorServer::ReadTransform(simdjson::ondemand::value p_Transform) {
    const auto s_Position = ReadVector3(p_Transform["position"]);
    const auto s_Rotation = ReadRotation(p_Transform["rotation"]);
    const auto s_Scale = ReadVector3(p_Transform["scale"]);

    const auto s_Quat = DirectX::XMQuaternionRotationRollPitchYaw(s_Rotation.yaw, s_Rotation.pitch, s_Rotation.roll);

    const auto s_Matrix = DirectX::XMMatrixAffineTransformation(
        DirectX::XMVectorSet(s_Scale.x, s_Scale.y, s_Scale.z, 0.0f),
        DirectX::XMVectorZero(),
        s_Quat,
        DirectX::XMVectorSet(s_Position.x, s_Position.y, s_Position.z, 1.0f)
    );

    return { s_Matrix };
}

ZRuntimeResourceID EditorServer::ReadResourceId(simdjson::ondemand::value p_ResourceId) {
    const std::string_view s_IdString = p_ResourceId;
    return { std::stoull(std::string(s_IdString), nullptr, 16) };
}

uint64_t EditorServer::ReadEntityId(simdjson::ondemand::value p_EntityId) {
    const std::string_view s_IdString = p_EntityId;
    return std::stoull(std::string(s_IdString), nullptr, 16);
}

// Events are built on the thread that changed the entity, and only published on the server thread.
void EditorServer::PublishEvent(std::string p_Event, std::optional<std::string> p_IgnoreClient) {
    if (!m_Loop) {
        return;
    }

    m_Loop->defer([this, p_Event = std::move(p_Event), p_IgnoreClient = std::move(p_IgnoreClient)] {
        if (!m_App) {
            return;
        }

        if (!p_IgnoreClient) {
            m_App->publish("all", p_Event, uWS::OpCode::TEXT);
        }
        else {
            // Send to all but the client that triggered the event.
            for (const auto& s_ClientId : m_Sockets | std::views::keys) {
                if (s_ClientId != *p_IgnoreClient) {
                    m_App->publish(s_ClientId, p_Event, uWS::OpCode::TEXT);
                }
            }
        }
    });
}
