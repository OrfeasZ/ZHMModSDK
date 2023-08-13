#include "Hitmen.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>

#include "IconsMaterialDesign.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZModule.h"
#include "Glacier/ZSpatialEntity.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZRender.h"
#include "Glacier/EntityFactory.h"
#include <ranges>

#include "backends/imgui_impl_dx12.h"
#include "Glacier/SGameUpdateEvent.h"
#include "Glacier/ZCollision.h"
#include "Glacier/ZPhysics.h"
#include "Glacier/ZGameLoopManager.h"
#include "Glacier/ZHitman5.h"
#include "Glacier/ZApplicationEngineWin32.h"
#include "Glacier/ZEngineAppCommon.h"
#include "Glacier/ZPlayerRegistry.h"
#include "steam/steamnetworkingsockets.h"

#include "BinaryStreamReader.h"
#include "BinaryStreamWriter.h"

static Hitmen* g_HitmenInstance = nullptr;

Hitmen::Hitmen()
{
    g_HitmenInstance = this;
}

Hitmen::~Hitmen()
{
    const ZMemberDelegate<Hitmen, void(const SGameUpdateEvent&)> s_Delegate(this, &Hitmen::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);

    GameNetworkingSockets_Kill();
}

void Hitmen::OnEngineInitialized()
{
    SteamDatagramErrMsg s_ErrorMessage;
    if (!GameNetworkingSockets_Init(nullptr, s_ErrorMessage))
    {
        Logger::Error("Could not initialize game networking sockets. Error: {}", s_ErrorMessage);
        return;
    }

    m_Sockets = SteamNetworkingSockets();

    const ZMemberDelegate<Hitmen, void(const SGameUpdateEvent&)> s_Delegate(this, &Hitmen::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);

    m_Initialized = true;
}

void Hitmen::Init()
{
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Hitmen::OnClearScene);
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &Hitmen::OnLoadScene);
    Hooks::ZPlayerRegistry_GetLocalPlayer->AddDetour(this, &Hitmen::GetLocalPlayer);
}

static void ServerCallback(SteamNetConnectionStatusChangedCallback_t* p_Info)
{
    g_HitmenInstance->OnServerStatus(p_Info);
}

void Hitmen::StartServer(uint16_t p_Port)
{
    SteamNetworkingIPAddr s_BindAddr {};
    s_BindAddr.Clear();
    s_BindAddr.m_port = p_Port;

    SteamNetworkingConfigValue_t s_Config {};
    s_Config.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ServerCallback);

    m_ServerSocket = m_Sockets->CreateListenSocketIP(s_BindAddr, 1, &s_Config);

    if (m_ServerSocket == k_HSteamListenSocket_Invalid)
    {
        Logger::Error("Failed to start server.");
        return;
    }

    m_PollGroup = m_Sockets->CreatePollGroup();

    if (m_PollGroup == k_HSteamNetPollGroup_Invalid)
    {
        Logger::Error("Failed to create poll group.");
        return;
    }

    Logger::Info("Hitmen server started on port {}.", p_Port);
    m_IsServer = true;
}

static void ClientCallback(SteamNetConnectionStatusChangedCallback_t* p_Info)
{
    g_HitmenInstance->OnClientStatus(p_Info);
}

void Hitmen::Connect(const std::string& p_Address, uint16_t p_Port)
{
    SteamNetworkingIPAddr s_Addr {};
    s_Addr.Clear();

    if (!s_Addr.ParseString((p_Address + ":" + std::to_string(p_Port)).c_str()))
    {
        Logger::Error("Invalid address specified.");
        return;
    }

    SteamNetworkingConfigValue_t s_Config {};
    s_Config.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ClientCallback);

    m_ClientConnection = m_Sockets->ConnectByIPAddress(s_Addr, 1, &s_Config);

    if (m_ClientConnection == k_HSteamNetConnection_Invalid)
    {
        Logger::Error("Could not create client connection.");
        return;
    }
}

void Hitmen::OnServerStatus(SteamNetConnectionStatusChangedCallback_t* p_Info)
{
    Logger::Debug("Server connection status changed: {}", p_Info->m_info.m_eState);

    switch (p_Info->m_info.m_eState)
    {
        case k_ESteamNetworkingConnectionState_Connecting:
        {
            if (m_Sockets->AcceptConnection(p_Info->m_hConn) != k_EResultOK)
            {
                m_Sockets->CloseConnection(p_Info->m_hConn, 0, nullptr, false);
                Logger::Warn("Can't accept connection.  (It was already closed?)");
                break;
            }

            if (!m_Sockets->SetConnectionPollGroup(p_Info->m_hConn, m_PollGroup))
            {
                m_Sockets->CloseConnection(p_Info->m_hConn, 0, nullptr, false);
                Logger::Warn("Failed to set client connection poll group.");
                break;
            }

            // TODO: Multiple clients.
            m_ClientConnection = p_Info->m_hConn;
            m_Connected = true;
            break;
        }

    }
}

void Hitmen::OnClientStatus(SteamNetConnectionStatusChangedCallback_t* p_Info)
{
    Logger::Debug("Client connection status changed: {}", p_Info->m_info.m_eState);

    switch (p_Info->m_info.m_eState)
    {
        case k_ESteamNetworkingConnectionState_Connected:
        {
            m_Connected = true;
            m_IsClient = true;
            break;
        }
    }
}

enum MessageId
{
    InputsAndPositions,
    NpcPositions,
};


void Hitmen::UpdateServer()
{
    ISteamNetworkingMessage* s_Msgs[100];
    const int s_MsgCount = m_Sockets->ReceiveMessagesOnPollGroup(m_PollGroup, s_Msgs, _countof(s_Msgs));

    if (s_MsgCount < 0)
    {
        Logger::Error("Server error.");
        return;
    }

    for (int i = 0; i < s_MsgCount; ++i)
    {
        const auto s_Msg = s_Msgs[i];

        //Logger::Debug("Got message with {} bytes.", s_Msg->m_cbSize);

        // TODO: This is extremely incredibly unsafe
        BinaryStreamReader s_Reader(s_Msg->m_pData, s_Msg->m_cbSize);

        switch (s_Reader.Read<MessageId>())
        {
            case InputsAndPositions:
                OnInputsAndPosition(s_Reader);
                break;

            case NpcPositions:
                OnNpcPositions(s_Reader);
                break;
        }

        s_Msg->Release();
    }

}

void Hitmen::UpdateClient()
{
    ISteamNetworkingMessage* s_Msgs[100];
    const int s_MsgCount = m_Sockets->ReceiveMessagesOnConnection(m_ClientConnection, s_Msgs, _countof(s_Msgs));

    if (s_MsgCount < 0)
    {
        Logger::Error("Client error.");
        return;
    }

    for (int i = 0; i < s_MsgCount; ++i)
    {
        const auto s_Msg = s_Msgs[i];

        //Logger::Debug("Got message with {} bytes.", s_Msg->m_cbSize);

        // TODO: This is extremely incredibly unsafe
        BinaryStreamReader s_Reader(s_Msg->m_pData, s_Msg->m_cbSize);

        switch (s_Reader.Read<MessageId>())
        {
            case InputsAndPositions:
                OnInputsAndPosition(s_Reader);
                break;

            case NpcPositions:
                OnNpcPositions(s_Reader);
                break;
        }

        s_Msg->Release();
    }
}

void Hitmen::SendInputsAndPosition(HSteamNetConnection p_Connection)
{
    BinaryStreamWriter s_Writer(sizeof(SMatrix) + 0x148);

    // TODO: This is extremely incredibly unsafe
    s_Writer.Write(InputsAndPositions);
    s_Writer.Write(m_OurHitman.QueryInterface<ZSpatialEntity>()->GetWorldMatrix());
    s_Writer.WriteBinary(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_OurHitman.QueryInterface<ZHitman5>()->m_pCharacterInputProcessor->m_pInput) + sizeof(uintptr_t)), 0x148);

    m_Sockets->SendMessageToConnection(p_Connection, s_Writer.Buffer(), s_Writer.WrittenBytes(), k_nSteamNetworkingSend_UnreliableNoNagle, nullptr);
}

void Hitmen::SendNpcPositions(HSteamNetConnection p_Connection)
{
    BinaryStreamWriter s_Writer(8192);

    uint32_t s_AliveActorCount = 0;

    for (int i = 0; i < *Globals::NextActorId; ++i)
    {
        auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

        if (s_Actor->IsAlive())
            ++s_AliveActorCount;
    }

    s_Writer.Write(NpcPositions);
    s_Writer.Write(s_AliveActorCount);

    for (int i = 0; i < *Globals::NextActorId; ++i)
    {
        const auto& s_Actor = Globals::ActorManager->m_aActiveActors[i];

        if (s_Actor.m_pInterfaceRef->IsAlive())
        {
            s_Writer.Write(i);
            s_Writer.Write(s_Actor.m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix());
        }
    }

    m_Sockets->SendMessageToConnection(p_Connection, s_Writer.Buffer(), s_Writer.WrittenBytes(), k_nSteamNetworkingSend_UnreliableNoNagle, nullptr);
}

void Hitmen::OnInputsAndPosition(BinaryStreamReader& p_Reader)
{
    const auto& s_Position = p_Reader.Read<SMatrix>();

    if (m_OtherHitman)
    {
        m_OtherHitman.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

        const auto s_SpatialHitman = m_OtherHitman.QueryInterface<ZSpatialEntity>();

        /*auto s_CurrentPos = s_SpatialHitman->GetWorldMatrix();

        if (float4::Distance(s_CurrentPos.Pos, s_Position.Pos) >= 0.3f)
        {
            s_CurrentPos.Pos = s_Position.Pos;
            s_SpatialHitman->SetWorldMatrix(s_CurrentPos);
        }*/

        s_SpatialHitman->SetWorldMatrix(s_Position);

        const auto s_OtherHitman = m_OtherHitman.QueryInterface<ZHitman5>();

        if (s_OtherHitman->m_pCharacterInputProcessor && s_OtherHitman->m_pCharacterInputProcessor->m_pInput)
        {
            // TODO: This is extremely incredibly unsafe
            p_Reader.ReadBytes(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(s_OtherHitman->m_pCharacterInputProcessor->m_pInput) + sizeof(uintptr_t)), 0x148);
        }
    }
}

void Hitmen::OnNpcPositions(BinaryStreamReader& p_Reader)
{
    const auto s_Actors = p_Reader.Read<uint32_t>();

    for (int i = 0; i < s_Actors; ++i)
    {
        // TODO: This is extremely incredibly unsafe
        const auto& s_ActorIndex = p_Reader.Read<int>();
        const auto& s_ActorPos = p_Reader.Read<SMatrix>();

        if (s_ActorIndex < *Globals::NextActorId)
        {
            const auto& s_Actor = Globals::ActorManager->m_aActiveActors[s_ActorIndex];

            if (s_Actor.m_pInterfaceRef->IsAlive())
            {
                s_Actor.m_ref.QueryInterface<ZSpatialEntity>()->SetWorldMatrix(s_ActorPos);
            }
        }
    }
}

void Hitmen::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    /*m_UpdateTimer += p_UpdateEvent.m_RealTimeDelta.ToSeconds();
    m_NpcUpdateTimer += p_UpdateEvent.m_RealTimeDelta.ToSeconds();

    if (!m_Initialized)
        return;

    if (m_IsServer)
    {
        UpdateServer();
    }
    else if (m_IsClient)
    {
        UpdateClient();
    }

    m_Sockets->RunCallbacks();

    if (!m_Connected)
        return;*/

    auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene || !(*Globals::ApplicationEngineWin32)->m_bSceneLoaded)
        return;

    if (!m_SceneLoaded)
    {
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

        if (!s_HitmanSpatial)
            return;

        m_OurHitman = s_LocalHitman.m_ref;

        for (auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks)
        {
            if (s_Brick.runtimeResourceID != ResId<"[assembly:/_sdk/hitmen.brick].pc_entitytype">)
                continue;

            const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Brick.entityRef.GetBlueprintFactory());

            if (!s_BpFactory)
                continue;

            if (const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeede715906f747f); s_Index != -1)
            {
                m_OtherHitman = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);
            }

            if (m_OtherHitman)
            {
                Logger::Debug(
        "Found other hitman {} (base {}) and our hitman {} (base {}).",
                    fmt::ptr(m_OtherHitman.QueryInterface<ZHitman5>()),
                    fmt::ptr(m_OtherHitman.GetEntity()),
                    fmt::ptr(m_OurHitman.QueryInterface<ZHitman5>()),
                    fmt::ptr(m_OurHitman.GetEntity())
                );
                m_SceneLoaded = true;
            }

            break;
        }

        return;
    }

    //if (m_UpdateTimer >= 1.f / 30.f)
    /*{
        m_UpdateTimer = 0.f;
        SendInputsAndPosition(m_ClientConnection);
    }

    if (m_NpcUpdateTimer >= 1.f / 10.f)
    {
        m_NpcUpdateTimer = 0.f;

        if (m_IsServer)
            SendNpcPositions(m_ClientConnection);
    }*/
}

void Hitmen::OnDrawMenu()
{
    if (ImGui::Button("Player Registry"))
    {
        for (int i = 0; i < 4; ++i)
        {
            auto& s_Data = Globals::PlayerRegistry->m_aPlayerData[i];

            /*
             * class ZNetPlayerController :
                public ZBaseReplica
            {
            public:
                void* m_unk0x8; // 0x10 (-8)
                uint32_t m_nFlags0x10; // 0x18 (-8)
                void* m_unk0x18; // 0x20 (-8)
                uint32_t m_nFlags0x20; // 0x28 (-8)
                void* m_nFlags0x28; // 0x30 (-8)
                bool m_bUnk0x30; // 0x38 (-8)
                uint32_t m_nFlags0x34; // 0x3C (-8)
                uint32_t m_nFlags0x38; // 0x40 (-8)
                uint32_t m_nFlags0x3C; // 0x44 (-8)
                uint16_t m_nFlags0x40; // 0x48 (-8)
                bool m_bUnk0x42; // 0x4A (-8)
                void* m_unk0x48; // 0x50 (-8)
                void* m_unk0x50; // 0x58 (-8)
                void* m_unk0x58; // 0x60 (-8)
                ZString m_sUnk0x60; // 0x68 (-8)
                uint32_t m_nFlags0x70; // 0x78 (-8)
                void* m_unk0x78; // 0x80 (-8)
                void* m_unk0x80; // 0x88 (-8)
                ZString m_sPlayerOnlineId; // 0x90 (-8)
                uint32_t m_nFlags0x98; // 0xA0 (-8)
                ZEntityRef m_HitmanEntity; // 0xA8 (-8)
                void* m_unk0xA8; // 0xB0 (-8) a pointer to the entity vtables, probably something related to the aspect dummy
                void* m_unk0xB0; // 0xB8 (-8)
                void* m_unk0xB8; // 0xC0 (-8)
                void* m_unk0xC0; // 0xC8 (-8)
            };
             */

            Logger::Debug(">>>> [{}] {}", i, fmt::ptr(&s_Data));
            Logger::Debug("[{}] player id = {}", i, s_Data.m_nPlayerId);
            Logger::Debug("[{}] counter = {}", i, s_Data.m_Controller.m_nUnkCounter);
            Logger::Debug("[{}] flags 0x18 = {:08X}", i, s_Data.m_Controller.m_nFlags0x10);
            Logger::Debug("[{}] raknet replica = {}", i, fmt::ptr(s_Data.m_Controller.m_pRakNetReplica));
            Logger::Debug("[{}] flags 0x28 = {:08X}", i, s_Data.m_Controller.m_nFlags0x20);
            Logger::Debug("[{}] flags 0x30 = {}", i, fmt::ptr(s_Data.m_Controller.m_nFlags0x28));
            Logger::Debug("[{}] is local player = {}", i, s_Data.m_Controller.m_bLocalPlayer);
            Logger::Debug("[{}] flags 0x3C = {:08X}", i, s_Data.m_Controller.m_nFlags0x34);
            Logger::Debug("[{}] flags 0x40 = {:08X}", i, s_Data.m_Controller.m_nFlags0x38);
            Logger::Debug("[{}] flags 0x44 = {:08X}", i, s_Data.m_Controller.m_nFlags0x3C);
            Logger::Debug("[{}] flags 0x48 = {:04X}", i, s_Data.m_Controller.m_nFlags0x40);
            Logger::Debug("[{}] connected = {}", i, s_Data.m_Controller.m_bConnectedToMultiplayer);
            Logger::Debug("[{}] net player = {}", i, fmt::ptr(s_Data.m_Controller.m_pNetPlayer));
            Logger::Debug("[{}] character id = {}", i, s_Data.m_Controller.m_SelectedCharacterId.ToString());
            Logger::Debug("[{}] string 0x68 = {}", i, s_Data.m_Controller.m_sUnk0x60);
            Logger::Debug("[{}] flags 0x78 = {:08X}", i, s_Data.m_Controller.m_nFlags0x70);
            Logger::Debug("[{}] outfit id = {}", i, s_Data.m_Controller.m_OutfitId.ToString());
            Logger::Debug("[{}] player session id (?) = {}", i, s_Data.m_Controller.s_sSessionId);
            Logger::Debug("[{}] flags 0xA0 = {:08X}", i, s_Data.m_Controller.m_nFlags0x98);
            Logger::Debug("[{}] hitman entity = {}", i, fmt::ptr(s_Data.m_Controller.m_HitmanEntity.GetEntity()));
            Logger::Debug("[{}] entity vtables = {}", i, fmt::ptr(s_Data.m_Controller.m_pEntityVtables));
            Logger::Debug("[{}] unk 0xB8 = {}", i, fmt::ptr(s_Data.m_Controller.m_unk0xB0));
            Logger::Debug("[{}] unk 0xC0 = {}", i, fmt::ptr(s_Data.m_Controller.m_unk0xB8));
            Logger::Debug("[{}] unk 0xC8 = {}", i, fmt::ptr(s_Data.m_Controller.m_unk0xC0));

        }

        Logger::Debug("Local player data: {}", fmt::ptr(Globals::PlayerRegistry->m_pLocalPlayer));

        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        Logger::Debug("Local player: {} (base {})", fmt::ptr(s_LocalHitman.m_pInterfaceRef), fmt::ptr(s_LocalHitman.m_ref.GetEntity()));


    }

    if (ImGui::Button("Hitmen"))
    {
        m_ShowServerWindow = !m_ShowServerWindow;
    }
}

void Hitmen::OnDrawUI(bool p_HasFocus)
{
    auto s_ImgGuiIO = ImGui::GetIO();

    if (m_ShowServerWindow)
    {
        if (ImGui::Begin("Host Hitmen Server", &m_ShowServerWindow))
        {
            ImGui::Text("Server Port = 6969");

            if (ImGui::Button("Start Server"))
            {
                StartServer(6969);
                m_ShowServerWindow = false;
            }
        }

        ImGui::End();
    }

    if (m_ShowClientWindow)
    {
        if (ImGui::Begin("Connect to Hitmen Server", &m_ShowClientWindow))
        {
            ImGui::Text("Server Port = 6969");

            static char s_ServerAddr[1024] = {};
            ImGui::InputText("Server Address", s_ServerAddr, IM_ARRAYSIZE(s_ServerAddr));

            if (ImGui::Button("Connect"))
            {
                Connect(s_ServerAddr, 6969);
                m_ShowClientWindow = false;
            }
        }

        ImGui::End();
    }
}

void Hitmen::OnDraw3D(IRenderer* p_Renderer)
{
    /*if (m_OtherHitman)
    {
        if (auto* s_SpatialEntity = m_OtherHitman.QueryInterface<ZSpatialEntity>())
        {
            SMatrix s_Transform;
            Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

            float4 s_Min, s_Max;

            s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

            p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
        }
    }*/
}

DEFINE_PLUGIN_DETOUR(Hitmen, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData)
{
    // p_SceneData.m_sceneName = "assembly:/_pro/scenes/users/notex/test.entity";
    //p_SceneData.m_sceneName = "assembly:/_pro/scenes/missions/golden/mission_gecko/scene_gecko_basic.entity";
    //p_SceneData.m_sceneName = "assembly:/_PRO/Scenes/Missions/TheFacility/_Scene_Mission_Polarbear_Module_002_B.entity";
    //p_SceneData.m_sceneBricks.clear();

    //p_SceneData.m_sceneName = "assembly:/_PRO/Scenes/Missions/Ancestral/scene_bulldog.entity";
    //p_SceneData.m_sceneBricks.clear();
    //p_SceneData.m_sceneBricks.push_back("assembly:/_PRO/scenes/missions/golden/mission_gecko/mission_gecko.brick");

    /*
     * Loading scene: assembly:/_pro/scenes/missions/golden/mission_gecko/scene_gecko_basic.entity
+ With brick: assembly:/_PRO/scenes/missions/golden/mission_gecko/mission_gecko.brick
     */
    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Hitmen, void, OnClearScene, ZEntitySceneContext* th, bool forReload)
{
    m_OtherHitman = {};
    m_FirstHitman = {};
    m_SceneLoaded = false;
    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Hitmen, TEntityRef<ZHitman5>*, GetLocalPlayer, ZPlayerRegistry* th, TEntityRef<ZHitman5>* out)
{
    auto s_Result = p_Hook->CallOriginal(th, out);

    return HookResult(HookAction::Return(), out);
}

DEFINE_ZHM_PLUGIN(Hitmen);
