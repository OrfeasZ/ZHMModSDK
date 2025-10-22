#include "EditorServer.h"

#include <Logging.h>
#include <simdjson.h>
#include <queue>
#include <numbers>

#include "Editor.h"
#include "JsonHelpers.h"

#include <Glacier/ZEntity.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZCameraEntity.h>

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
                        Logger::Debug("New editor connection established.");

                        const auto s_ClientId = m_LastClientId++;
                        const auto s_ClientIdStr = std::to_string(s_ClientId);

                        p_Socket->getUserData()->ClientId = s_ClientIdStr;
                        m_SocketUserDatas.push_back(p_Socket->getUserData());
                    },
                    .message = [&](WebSocket* p_Socket, std::string_view p_Message, uWS::OpCode p_OpCode) {
                        Logger::Trace("Socket message received: {}", p_Message);

                        try {
                            if (m_Enabled) {
                                OnMessage(p_Socket, p_Message);
                            }
                            else {
                                Logger::Info("EditorServer disabled, ignoring message.");
                            }
                        }
                        catch (const std::exception& e) {
                            Logger::Error(
                                "Failed to handle editor message with error: {}\nMessage was: {}", e.what(), p_Message
                            );
                            SendError(p_Socket, e.what(), std::nullopt);
                        }
                    },
                    .close = [this](WebSocket* p_Socket, int p_Code, std::string_view p_Message) {
                        Logger::Debug("Editor connection closed with code '{}' and message: {}", p_Code, p_Message);

                        auto s_Data = p_Socket->getUserData();
                        m_SocketUserDatas.erase(
                            std::remove(m_SocketUserDatas.begin(), m_SocketUserDatas.end(), s_Data),
                            m_SocketUserDatas.end()
                        );
                    }
                }
            );

            s_App->listen(
                c_EditorHost, c_EditorPort, [](auto* p_Socket) {
                    if (p_Socket) {
                        Logger::Info("Editor server listening on {}:{}", c_EditorHost, c_EditorPort);
                    }
                    else {
                        Logger::Error(
                            "Editor server failed to listen on {}:{}. Is another program using this port?",
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
    (void) m_ServerThread;
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

bool EditorServer::m_Enabled = true;

void EditorServer::OnMessage(WebSocket* p_Socket, std::string_view p_Message) noexcept(false) {
    simdjson::ondemand::parser s_Parser;
    const auto s_Json = simdjson::padded_string(p_Message);
    simdjson::ondemand::document s_JsonMsg = s_Parser.iterate(s_Json);

    const std::string_view s_Type = s_JsonMsg["type"];
    Logger::Trace("Editor message type: {}", s_Type);

    std::optional<int64_t> s_MessageId;

    if (s_JsonMsg.find_field("msgId").error() == simdjson::SUCCESS) {
        if (s_JsonMsg["msgId"].type() == simdjson::ondemand::json_type::number) {
            s_MessageId = s_JsonMsg["msgId"];
        }
    }

    if (s_Type == "hello") {
        p_Socket->getUserData()->Identifier = std::string_view(s_JsonMsg["identifier"]);
        SendWelcome(p_Socket);
    }
    else if (s_Type == "selectEntity") {
        Plugin()->SelectEntity(ReadEntitySelector(s_JsonMsg["entity"]), p_Socket->getUserData()->ClientId);
    }
    else if (s_Type == "setEntityTransform") {
        Plugin()->SetEntityTransform(
            ReadEntitySelector(s_JsonMsg["entity"]),
            ReadTransform(s_JsonMsg["transform"]),
            s_JsonMsg["relative"],
            p_Socket->getUserData()->ClientId
        );
    }
    else if (s_Type == "spawnQnEntity") {
        Plugin()->SpawnQnEntity(
            std::string(std::string_view(s_JsonMsg["qnJson"])),
            ReadEntityId(s_JsonMsg["entityId"]),
            std::string(std::string_view(s_JsonMsg["name"])),
            p_Socket->getUserData()->ClientId
        );
    }
    else if (s_Type == "createEntityResources") {
        Plugin()->CreateEntityResources(
            std::string(std::string_view(s_JsonMsg["qnJson"])),
            p_Socket->getUserData()->ClientId
        );
    }
    else if (s_Type == "destroyEntity") {
        Plugin()->DestroyEntity(ReadEntitySelector(s_JsonMsg["entity"]), p_Socket->getUserData()->ClientId);
    }
    else if (s_Type == "setEntityName") {
        Plugin()->SetEntityName(
            ReadEntitySelector(s_JsonMsg["entity"]),
            std::string(std::string_view(s_JsonMsg["name"])),
            p_Socket->getUserData()->ClientId
        );
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

        Plugin()->SetEntityProperty(
            ReadEntitySelector(s_JsonMsg["entity"]),
            s_PropertyId,
            simdjson::to_json_string(s_JsonMsg["value"]),
            p_Socket->getUserData()->ClientId
        );
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

        Plugin()->SignalEntityPin(
            ReadEntitySelector(s_JsonMsg["entity"]),
            s_PinId,
            s_JsonMsg["output"]
        );
    }
    else if (s_Type == "listEntities") {
        SendEntityList(p_Socket, Plugin()->GetEntityTree(), s_MessageId);
    }
    else if (s_Type == "listAlocPfBoxAndSeedPointEntities") {
        SendAlocPfBoxesAndSeedPointEntityList(p_Socket);
    }
    else if (s_Type == "loadNavp") {
        const uint64_t s_ChunkIndex = s_JsonMsg["chunk"];
        const uint64_t s_ChunkCount = s_JsonMsg["chunkCount"];
        const simdjson::ondemand::array s_Areas = s_JsonMsg["areas"];
        Plugin()->LoadNavpAreas(s_Areas, s_ChunkIndex);
        if (s_ChunkIndex == s_ChunkCount - 1) {
            SendDoneLoadingNavpMessage(p_Socket);
        }
    }
    else if (s_Type == "getEntityDetails") {
        const auto s_Selector = ReadEntitySelector(s_JsonMsg["entity"]);
        const auto s_Entity = Plugin()->FindEntity(s_Selector);
        SendEntityDetails(p_Socket, s_Entity, s_MessageId);
    }
    else if (s_Type == "getHitmanEntity") {
        SendHitmanEntity(p_Socket, s_MessageId);
    }
    else if (s_Type == "getCameraEntity") {
        SendCameraEntity(p_Socket, s_MessageId);
    }
    else if (s_Type == "rebuildEntityTree") {
        Plugin()->RebuildEntityTree();
    }
    else {
        throw std::runtime_error(std::format("Unknown editor message type: {}", s_Type));
    }
}

void EditorServer::SendWelcome(EditorServer::WebSocket* p_Socket) {
    Logger::Info(
        "Client with identifier '{}' connected to the editor server. Sending welcome message.",
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

void EditorServer::SendHitmanEntity(WebSocket* p_Socket, std::optional<int64_t> p_MessageId) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping SendHitmanEntity.");
        return;
    }

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman || !s_LocalHitman.m_ref) {
        SendError(p_Socket, "Failed to get local hitman entity.", p_MessageId);
        return;
    }

    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("hitmanEntity") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, s_LocalHitman.m_ref);
    s_Event << "}";

    p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
}

void EditorServer::SendCameraEntity(WebSocket* p_Socket, std::optional<int64_t> p_MessageId) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping SendCameraEntity.");
        return;
    }
    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera) {
        SendError(p_Socket, "Failed to get active camera entity.", p_MessageId);
        return;
    }

    ZEntityRef s_Ref;
    s_CurrentCamera->GetID(&s_Ref);

    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("cameraEntity") << ",";
    s_Event << write_json("entity") << ":";
    WriteEntityDetails(s_Event, s_Ref);
    s_Event << "}";

    p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
}

void EditorServer::SendError(
    EditorServer::WebSocket* p_Socket, std::string p_Message, std::optional<int64_t> p_MessageId
) {
    std::ostringstream s_Event;

    s_Event << "{";

    if (p_MessageId) {
        s_Event << write_json("msgId") << ":" << write_json(*p_MessageId) << ",";
    }

    s_Event << write_json("type") << ":" << write_json("error") << ",";
    s_Event << write_json("message") << ":" << write_json(p_Message);
    s_Event << "}";

    p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
}

void EditorServer::OnEntitySelected(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntitySelected.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_Entity, p_ByClient]() {
            if (!m_App) {
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
    );
}

void EditorServer::OnEntityTransformChanged(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntityTransformChanged.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_Entity, p_ByClient]() {
            if (!m_App) {
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
    );
}

void EditorServer::OnEntityNameChanged(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntityNameChanged.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_Entity, p_ByClient]() {
            if (!m_App) {
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
    );
}

void EditorServer::OnEntityPropertySet(
    ZEntityRef p_Entity, uint32_t p_PropertyId, std::optional<std::string> p_ByClient
) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntityPropertySet.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_Entity, p_PropertyId, p_ByClient]() {
            if (!m_App) {
                return;
            }

            const auto s_EntityType = p_Entity->GetType();

            if (!s_EntityType) {
                return;
            }

            const auto s_Property = s_EntityType->FindProperty(p_PropertyId);

            if (!s_Property || !s_Property->m_pType) {
                return;
            }

            const auto s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

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

            if (s_PropertyInfo->m_pType->typeInfo()->isResource() || s_PropertyInfo->m_nPropertyID != s_Property->
                m_nPropertyId) {
                // Some properties don't have a name for some reason. Try to find using RL.
                const auto s_PropertyName = HM3_GetPropertyName(s_Property->m_nPropertyId);

                if (s_PropertyName.Size > 0) {
                    s_Event << write_json(std::string_view(s_PropertyName.Data, s_PropertyName.Size)) << ",";
                }
                else {
                    s_Event << write_json(s_Property->m_nPropertyId) << ",";
                }
            }
            else {
                s_Event << write_json(s_PropertyInfo->m_pName) << ",";
            }

            s_Event << write_json("value") << ":";
            WriteProperty(s_Event, p_Entity, s_Property);

            s_Event << "}";

            PublishEvent(s_Event.str(), p_ByClient);
        }
    );
}

void EditorServer::OnEntitySpawned(ZEntityRef p_Entity, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntitySpawned.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_Entity, p_ByClient]() {
            if (!m_App) {
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
    );
}

void EditorServer::OnEntityDestroying(uint64_t p_EntityId, std::optional<std::string> p_ByClient) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntityDestroying.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_EntityId, p_ByClient]() {
            if (!m_App) {
                return;
            }

            std::ostringstream s_Event;

            s_Event << "{";

            s_Event << write_json("type") << ":" << write_json("entityDestroying") << ",";
            s_Event << write_json("entityId") << ":" << write_json(std::format("{:016x}", p_EntityId));

            s_Event << "}";

            PublishEvent(s_Event.str(), std::nullopt);
        }
    );
}

void EditorServer::OnSceneLoading(const std::string& p_Scene, const std::vector<std::string>& p_Bricks) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnSceneLoading.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_Scene, p_Bricks]() {
            if (!m_App) {
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

            m_App->publish("all", s_Event.str(), uWS::OpCode::TEXT);
        }
    );
}

void EditorServer::OnSceneClearing(bool p_FullyUnloadScene) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnSceneClearing.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this, p_FullyUnloadScene]() {
            if (!m_App) {
                return;
            }

            std::ostringstream s_Event;

            s_Event << "{";

            s_Event << write_json("type") << ":" << write_json("sceneClearing") << ",";
            s_Event << write_json("p_FullyUnloadScene") << ":" << write_json(p_FullyUnloadScene);

            s_Event << "}";

            m_App->publish("all", s_Event.str(), uWS::OpCode::TEXT);
        }
    );
}

void EditorServer::OnEntityTreeRebuilt() {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping OnEntityTreeRebuilt.");
        return;
    }
    if (!m_Loop) {
        return;
    }

    m_Loop->defer(
        [this]() {
            if (!m_App) {
                return;
            }

            std::ostringstream s_Event;

            s_Event << "{";
            s_Event << write_json("type") << ":" << write_json("entityTreeRebuilt");
            s_Event << "}";

            m_App->publish("all", s_Event.str(), uWS::OpCode::TEXT);
        }
    );
}

void EditorServer::SetEnabled(bool p_Enabled) {
    m_Enabled = p_Enabled;
    if (p_Enabled) {
        Logger::Info("Enabling EditorServer");
    }
    else {
        Logger::Info("Disabling EditorServer");
    }
}

bool EditorServer::GetEnabled() {
    return m_Enabled;
}

void EditorServer::SendAlocPfBoxesAndSeedPointEntityList(WebSocket* p_Socket) {
    p_Socket->send("{\"alocs\":[", uWS::OpCode::TEXT);
    auto s_AnyAlocSentOverall = std::make_shared<bool>(false);
    Plugin()->FindAlocs(
        [p_Socket, s_AnyAlocSentOverall](
    const std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>& p_Entities,
    const bool p_IsLastAlocBatch
) -> void {
            bool s_SentAlocThisBatch = false;
            if (!p_Entities.empty()) {
                bool s_CurrentBatchWillSend = false;
                for (auto& [s_Hashes, s_Quat, s_Entity] : p_Entities) {
                    if (IsExcludedFromNavMeshExport(s_Entity)) continue;
                    if (!s_Hashes.empty()) {
                        s_CurrentBatchWillSend = true;
                        break;
                    }
                }
                if (s_CurrentBatchWillSend) {
                    if (*s_AnyAlocSentOverall) {
                        p_Socket->send(",", uWS::OpCode::TEXT);
                    }
                    s_SentAlocThisBatch = SendEntitiesDetails(p_Socket, p_Entities);
                }
            }
            if (s_SentAlocThisBatch) {
                *s_AnyAlocSentOverall = true;
            }
            if (p_IsLastAlocBatch) {
                p_Socket->send("],\"pfBoxes\":[", uWS::OpCode::TEXT);

                const auto s_PfBoxEntities =
                        Plugin()->FindEntitiesByType("ZPFBoxEntity", "00280B8C4462FAC8");
                SendEntitiesDetails(p_Socket, s_PfBoxEntities);

                p_Socket->send("],\"pfSeedPoints\":[", uWS::OpCode::TEXT);
                const auto s_PfSeedPointEntities =
                        Plugin()->FindEntitiesByType("ZPFSeedPoint", "00280B8C4462FAC8");
                SendEntitiesDetails(p_Socket, s_PfSeedPointEntities);

                p_Socket->send("]}", uWS::OpCode::TEXT);

                p_Socket->send("Done sending entities.", uWS::OpCode::TEXT);
            }
        },
        [p_Socket]() -> void {
            p_Socket->send("Rebuilding tree.", uWS::OpCode::TEXT);
        }
    );
}


void EditorServer::SendEntityList(
    EditorServer::WebSocket* p_Socket, std::shared_ptr<EntityTreeNode> p_Tree, std::optional<int64_t> p_MessageId
) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping SendEntityList.");
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

        if (!s_Node->Entity) {
            continue;
        }

        if (!s_First) {
            s_EventStream << ",";
        }

        s_First = false;

        auto s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Node->Entity.GetBlueprintFactory());

        if (s_Node->Entity.GetOwningEntity()) {
            s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Node->Entity.GetOwningEntity().
                GetBlueprintFactory());
        }

        s_EventStream << "{";
        s_EventStream << write_json("id") << ":" << write_json(std::format("{:016x}", s_Node->EntityId)) << ",";
        s_EventStream << write_json("source") << ":" << write_json("game") << ",";
        s_EventStream << write_json("tblu") << ":" << write_json(std::format("{:016X}", s_Node->TBLU.GetID())) << ",";
        s_EventStream << write_json("type") << ":" << write_json(
            (*s_Node->Entity->GetType()->m_pInterfaces)[0].m_pTypeId->typeInfo()->m_pTypeName
        );

        if (s_Factory) {
            // This is also probably wrong.
            auto s_Index = s_Factory->GetSubEntityIndex(s_Node->Entity->GetType()->m_nEntityId);

            if (s_Index != -1 && s_Factory->m_pTemplateEntityBlueprint) {
                const auto s_Name = s_Factory->m_pTemplateEntityBlueprint->subEntities[s_Index].entityName;
                s_EventStream << "," << write_json("name") << ":" << write_json(s_Name);
            }
        }

        s_EventStream << "}";

        for (auto& childPair : s_Node->Children) {
            s_NodeQueue.push(childPair.second);
        }
    }

    s_EventStream << "]";

    s_EventStream << "}";

    p_Socket->send(s_EventStream.str(), uWS::OpCode::TEXT);
}

void EditorServer::SendEntityDetails(WebSocket* p_Socket, ZEntityRef p_Entity, std::optional<int64_t> p_MessageId) {
    if (!m_Enabled) {
        Logger::Info("EditorServer disabled. Skipping SendEntityDetails.");
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

    p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
}

void EditorServer::SendDoneLoadingNavpMessage(WebSocket* p_Socket) {
    p_Socket->send("Done loading Navp.", uWS::OpCode::TEXT);
}

bool EditorServer::IsPropertyValueTrue(const ZEntityProperty* s_Property, const ZEntityRef& p_Entity) {
    const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

    if (!s_PropertyInfo || !s_PropertyInfo->m_pType) {
        return true;
    }

    const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(p_Entity.m_pEntity) + s_Property->m_nOffset;
    const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
    const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;
    const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;

    // Get the value of the property.
    auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

    if (s_PropertyInfo->m_nFlags & E_HAS_GETTER_SETTER)
        s_PropertyInfo->get(reinterpret_cast<void*>(s_PropertyAddress), s_Data, s_PropertyInfo->m_nOffset);
    else
        s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
            s_Data, reinterpret_cast<void*>(s_PropertyAddress)
        );

    const auto s_JsonProperty = HM3_GameStructToJson(s_TypeName.c_str(), s_Data, s_TypeSize);
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
    const std::string s_VisiblePropertyName = "m_bVisible";
    if (s_EntityType && s_EntityType->m_pProperties01) {
        for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
            const ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);
            const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

            if (!s_PropertyInfo || !s_PropertyInfo->m_pType || !s_PropertyInfo->m_pType->typeInfo()) {
                continue;
            }

            if (s_PropertyInfo->m_pType->typeInfo()->isResource() ||
                s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyId) {
                // Some properties don't have a name for some reason. Try to find using RL.

                if (const auto [s_data, s_size] =
                        HM3_GetPropertyName(s_Property->m_nPropertyId); s_size > 0) {
                    if (const auto s_PropertyNameView = std::string_view(s_data, s_size);
                        s_PropertyNameView == s_RemovePhysicsPropertyName) {
                        if (IsPropertyValueTrue(s_Property, p_Entity)) {
                            return true;
                        }
                    }
                    else if (s_PropertyNameView == s_VisiblePropertyName) {
                        if (!IsPropertyValueTrue(s_Property, p_Entity)) {
                            return true;
                        }
                    }
                }
            }
            else if (s_PropertyInfo->m_pName) {
                if (s_PropertyInfo->m_pName == s_RemovePhysicsPropertyName) {
                    if (IsPropertyValueTrue(s_Property, p_Entity)) {
                        return true;
                    }
                }
                else if (s_PropertyInfo->m_pName == s_VisiblePropertyName) {
                    if (!IsPropertyValueTrue(s_Property, p_Entity)) {
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

    for (const auto& [s_AlocHashes, s_Quat, s_Entity] : p_Entities) {
        if (IsExcludedFromNavMeshExport(s_Entity)) {
            continue;
        }

        for (const auto& s_Hash : s_AlocHashes) {
            if (!s_IsFirstItemActuallySentInThisCall) {
                p_Socket->send(",", uWS::OpCode::TEXT);
            }
            s_IsFirstItemActuallySentInThisCall = false;
            s_DidSendAnything = true;

            std::ostringstream s_Event;
            s_Event << "{" << write_json("hash") << ":";
            s_Event << write_json(s_Hash) << ",";
            s_Event << write_json("entity") << ":";
            WriteEntityTransforms(s_Event, s_Quat, s_Entity);
            s_Event << "}";
            p_Socket->send(s_Event.str(), uWS::OpCode::TEXT);
        }
    }
    return s_DidSendAnything;
}

void EditorServer::WriteEntityTransforms(std::ostream& p_Stream, Quat p_Quat, ZEntityRef p_Entity) {
    if (!p_Entity) {
        p_Stream << "null";
        return;
    }

    p_Stream << "{";

    p_Stream << write_json("id") << ":" << write_json(
        std::format("{:016x}", p_Entity->GetType()->m_nEntityId)
    ) << ",";

    auto s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Entity.GetBlueprintFactory());

    if (p_Entity.GetOwningEntity()) {
        s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Entity.GetOwningEntity().
            GetBlueprintFactory());
    }

    if (s_Factory) {
        if (auto s_Index = s_Factory->GetSubEntityIndex(p_Entity->GetType()->m_nEntityId); s_Index != -1) {
            const auto s_Name = s_Factory->m_pTemplateEntityBlueprint->subEntities[s_Index].entityName;
            p_Stream << write_json("name") << ":" << write_json(s_Name) << ",";
            p_Stream << write_json("tblu") << ":" << write_json(
                fmt::format("{:016X}", s_Factory->m_ridResource.GetID()).c_str()
            ) << ",";
        }
    }

    // Write transform.
    if (const auto s_Spatial = p_Entity.QueryInterface<ZSpatialEntity>()) {
        const auto s_Trans = s_Spatial->GetWorldMatrix();

        SMatrix p_Transform = s_Spatial->GetWorldMatrix();
        const auto s_Decomposed = p_Transform.Decompose();
        p_Stream << write_json("position") << ":";
        WriteVector3(p_Stream, s_Decomposed.Position.x, s_Decomposed.Position.y, s_Decomposed.Position.z);
        p_Stream << ",";
    }
    p_Stream << write_json("rotation") << ":";
    WriteQuat(p_Stream, p_Quat.m.x, p_Quat.m.y, p_Quat.m.z, p_Quat.m.w);

    const std::string s_ScalePropertyName = "m_PrimitiveScale";
    const std::string s_TypePropertyName = "m_eType";
    const std::string s_GlobalSizePropertyName = "m_vGlobalSize";

    if (const auto s_EntityType = p_Entity->GetType(); s_EntityType && s_EntityType->m_pProperties01) {
        for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
            ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);
            const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

            if (!s_PropertyInfo || !s_PropertyInfo->m_pType || !s_PropertyInfo->m_pType->typeInfo()) {
                continue;
            }

            if (s_PropertyInfo->m_pType->typeInfo()->isResource() || s_PropertyInfo->m_nPropertyID != s_Property->
                m_nPropertyId) {
                // Some properties don't have a name for some reason. Try to find using RL.

                if (const auto [s_data, s_size] = HM3_GetPropertyName(s_Property->m_nPropertyId); s_size > 0) {
                    auto s_PropertyNameView = std::string_view(s_data, s_size);
                    if (s_PropertyNameView == s_ScalePropertyName) {
                        p_Stream << ",";
                        p_Stream << write_json("scale") << ":";
                        WriteProperty(p_Stream, p_Entity, s_Property);
                    }
                    if (s_PropertyNameView == s_TypePropertyName) {
                        p_Stream << ",";
                        p_Stream << write_json("type") << ":";
                        WriteProperty(p_Stream, p_Entity, s_Property);
                    }
                    else if (s_PropertyNameView == s_GlobalSizePropertyName) {
                        p_Stream << ",";
                        p_Stream << write_json("scale") << ":";
                        WriteProperty(p_Stream, p_Entity, s_Property);
                    }
                }
            }
            else if (s_PropertyInfo->m_pName) {
                if (s_PropertyInfo->m_pName == s_ScalePropertyName) {
                    p_Stream << ",";
                    p_Stream << write_json("scale");
                    p_Stream << ":";
                    WriteProperty(p_Stream, p_Entity, s_Property);
                }
                if (s_PropertyInfo->m_pName == s_TypePropertyName) {
                    p_Stream << ",";
                    p_Stream << write_json("type");
                    p_Stream << ":";
                    WriteProperty(p_Stream, p_Entity, s_Property);
                }
                else if (s_PropertyInfo->m_pName == s_GlobalSizePropertyName) {
                    p_Stream << ",";
                    p_Stream << write_json("scale");
                    p_Stream << ":";
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
    Logger::Info("Sending entity details for entity id: '{}'", p_Entity->GetType()->m_nEntityId);

    p_Stream << "{";

    p_Stream << write_json("id") << ":" << write_json(std::format("{:016x}", p_Entity->GetType()->m_nEntityId)) << ",";

    auto s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Entity.GetBlueprintFactory());

    if (p_Entity.GetOwningEntity()) {
        s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Entity.GetOwningEntity().
            GetBlueprintFactory());
    }

    if (s_Factory) {
        // This is also probably wrong.
        auto s_Index = s_Factory->GetSubEntityIndex(p_Entity->GetType()->m_nEntityId);

        if (s_Index != -1 && s_Factory->m_pTemplateEntityBlueprint) {
            const auto s_Name = s_Factory->m_pTemplateEntityBlueprint->subEntities[s_Index].entityName;
            p_Stream << write_json("name") << ":" << write_json(s_Name) << ",";
        }

        p_Stream << write_json("source") << ":" << write_json("game") << ",";

        p_Stream << write_json("tblu") << ":" << write_json(
            std::format("{:016X}", s_Factory->m_ridResource.GetID())
        ) << ",";
    }
    else {
        // TODO: Name.
        p_Stream << write_json("source") << ":" << write_json("editor") << ",";
    }

    // Write type and interfaces.
    auto s_Interfaces = (*p_Entity->GetType()->m_pInterfaces);

    p_Stream << write_json("type") << ":" << write_json(s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName) << ",";

    p_Stream << write_json("interfaces") << ":" << "[";
    bool s_FirstInterface = true;

    for (auto& s_Interface : s_Interfaces) {
        if (!s_Interface.m_pTypeId) {
            continue;
        }

        const auto* s_TypeInfo = s_Interface.m_pTypeId->typeInfo();

        if (!s_TypeInfo || !s_TypeInfo->m_pTypeName) {
            continue;
        }

        if (!s_FirstInterface) {
            p_Stream << ",";
        }

        s_FirstInterface = false;

        p_Stream << write_json(s_TypeInfo->m_pTypeName);
    }

    p_Stream << "],";

    // Write transform.
    if (const auto s_Spatial = p_Entity.QueryInterface<ZSpatialEntity>()) {
        const auto s_Trans = s_Spatial->GetWorldMatrix();

        p_Stream << write_json("transform") << ":";
        WriteTransform(p_Stream, s_Spatial->GetWorldMatrix());
        p_Stream << ",";

        SMatrix s_ParentTrans;

        bool s_DoesntHaveParent = false;

        // Get parent entity.
        if (s_Spatial->m_eidParent.m_pInterfaceRef) {
            s_ParentTrans = s_Spatial->m_eidParent.m_pInterfaceRef->GetWorldMatrix();
        }
        else if (p_Entity.GetLogicalParent() && p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()) {
            s_ParentTrans = p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
        }
        else if (p_Entity.GetOwningEntity() && p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()) {
            s_ParentTrans = p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
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

    if (s_EntityType && s_EntityType->m_pProperties01) {
        for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
            ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);
            const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

            if (!s_PropertyInfo || !s_PropertyInfo->m_pType || !s_PropertyInfo->m_pType->typeInfo()) {
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

void EditorServer::WritePropertyName(std::ostream& p_Stream, ZEntityProperty* p_Property) {
    const auto* s_PropertyInfo = p_Property->m_pType->getPropertyInfo();

    if (s_PropertyInfo->m_pType->typeInfo()->isResource() || s_PropertyInfo->m_nPropertyID != p_Property->
        m_nPropertyId) {
        // Some properties don't have a name for some reason. Try to find using RL.
        const auto s_PropertyName = HM3_GetPropertyName(p_Property->m_nPropertyId);

        if (s_PropertyName.Size > 0) {
            p_Stream << write_json(std::string_view(s_PropertyName.Data, s_PropertyName.Size));
        }
        else {
            p_Stream << write_json(std::format("~{:08x}", p_Property->m_nPropertyId));
        }
    }
    else if (s_PropertyInfo->m_pName) {
        p_Stream << write_json(s_PropertyInfo->m_pName);
    }
    else {
        p_Stream << write_json(std::format("~{:08x}", p_Property->m_nPropertyId));
    }
}

void EditorServer::WriteProperty(std::ostream& p_Stream, ZEntityRef p_Entity, ZEntityProperty* p_Property) {
    p_Stream << "{" << write_json("type") << ":";

    const auto* s_PropertyInfo = p_Property->m_pType->getPropertyInfo();

    if (!s_PropertyInfo || !s_PropertyInfo->m_pType) {
        p_Stream << write_json("unknown") << ",";
        p_Stream << write_json("data") << ":" << "null" << "}";
        return;
    }

    const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(p_Entity.m_pEntity) + p_Property->m_nOffset;
    const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
    const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;
    const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;

    p_Stream << write_json(s_TypeName) << ",";
    p_Stream << write_json("data") << ":";

    // Get the value of the property.
    auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

    if (s_PropertyInfo->m_nFlags & EPropertyInfoFlags::E_HAS_GETTER_SETTER)
        s_PropertyInfo->get(reinterpret_cast<void*>(s_PropertyAddress), s_Data, s_PropertyInfo->m_nOffset);
    else
        s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
            s_Data, reinterpret_cast<void*>(s_PropertyAddress)
        );

    if (s_PropertyInfo->m_pType->typeInfo() && s_PropertyInfo->m_pType->typeInfo()->isEntity()) {
        auto* s_EntityData = reinterpret_cast<TEntityRef<ZEntityImpl>*>(s_Data);

        if (!s_EntityData || !*s_EntityData) {
            (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
            p_Stream << "null" << "}";
            return;
        }

        p_Stream << "{" << write_json("id") << ":" << write_json(
            std::format("{:016x}", s_EntityData->m_ref->GetType()->m_nEntityId)
        ) << ",";

        auto s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_EntityData->m_ref.GetBlueprintFactory());

        if (s_EntityData->m_ref.GetOwningEntity()) {
            s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_EntityData->m_ref.GetOwningEntity().
                GetBlueprintFactory());
        }

        if (s_Factory) {
            // This is also probably wrong.
            auto s_Index = s_Factory->GetSubEntityIndex(s_EntityData->m_ref->GetType()->m_nEntityId);

            if (s_Index != -1 && s_Factory->m_pTemplateEntityBlueprint) {
                const auto s_Name = s_Factory->m_pTemplateEntityBlueprint->subEntities[s_Index].entityName;
                p_Stream << write_json("name") << ":" << write_json(s_Name) << ",";
            }

            p_Stream << write_json("source") << ":" << write_json("game") << ",";

            p_Stream << write_json("tblu") << ":" << write_json(
                std::format("{:016X}", s_Factory->m_ridResource.GetID())
            ) << ",";
        }
        else {
            // TODO: Name.
            p_Stream << write_json("source") << ":" << write_json("editor") << ",";
        }

        // Write type and interfaces.
        auto s_Interfaces = (*s_EntityData->m_ref->GetType()->m_pInterfaces);

        p_Stream << write_json("type") << ":" << write_json(s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName) << "}}";

        (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
    }
    else {
        auto s_JsonProperty = HM3_GameStructToJson(s_TypeName.c_str(), s_Data, s_TypeSize);
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
        const auto s_PrimString = std::string {s_PrimStringView};
        Logger::Info("Reading PrimEntitySelector for hash: '{}'", s_PrimString);

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

    return {s_Matrix};
}

ZRuntimeResourceID EditorServer::ReadResourceId(simdjson::ondemand::value p_ResourceId) {
    const std::string_view s_IdString = p_ResourceId;
    return {std::stoull(std::string(s_IdString), nullptr, 16)};
}

uint64_t EditorServer::ReadEntityId(simdjson::ondemand::value p_EntityId) {
    const std::string_view s_IdString = p_EntityId;
    return std::stoull(std::string(s_IdString), nullptr, 16);
}

void EditorServer::PublishEvent(const std::string& p_Event, std::optional<std::string> p_IgnoreClient) {
    if (!p_IgnoreClient) {
        m_App->publish("all", p_Event, uWS::OpCode::TEXT);
    }
    else {
        // Send to all but the client that triggered the event.
        for (auto& s_Socket : m_SocketUserDatas) {
            if (s_Socket->ClientId != *p_IgnoreClient) {
                m_App->publish(s_Socket->ClientId, p_Event, uWS::OpCode::TEXT);
            }
        }
    }
}
