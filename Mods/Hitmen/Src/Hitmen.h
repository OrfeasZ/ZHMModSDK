#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"
#include "steam/steamnetworkingtypes.h"

class BinaryStreamReader;
class ISteamNetworkingSockets;
class ZHitman5;

class Hitmen : public IPluginInterface
{
public:
    Hitmen();
    ~Hitmen() override;

    void OnEngineInitialized() override;
    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnServerStatus(SteamNetConnectionStatusChangedCallback_t* p_Info);
    void OnClientStatus(SteamNetConnectionStatusChangedCallback_t* p_Info);
    void OnDraw3D(IRenderer* p_Renderer) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void StartServer(uint16_t p_Port);
    void Connect(const std::string& p_Address, uint16_t p_Port);
    void UpdateServer();
    void UpdateClient();

    void SendInputsAndPosition(HSteamNetConnection p_Connection);
    void SendNpcPositions(HSteamNetConnection p_Connection);

    void OnInputsAndPosition(BinaryStreamReader& p_Reader);
    void OnNpcPositions(BinaryStreamReader& p_Reader);

private:
    DECLARE_PLUGIN_DETOUR(Hitmen, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(Hitmen, void, OnClearScene, ZEntitySceneContext* th, bool forReload);
    DECLARE_PLUGIN_DETOUR(Hitmen, TEntityRef<ZHitman5>*, GetLocalPlayer, ZPlayerRegistry* th, TEntityRef<ZHitman5>* out);

private:
    bool m_Initialized = false;
    ZEntityRef m_OtherHitman;
    ZEntityRef m_OurHitman;
    TEntityRef<ZHitman5> m_FirstHitman;
    ISteamNetworkingSockets* m_Sockets = nullptr;
    HSteamListenSocket m_ServerSocket;
    HSteamNetConnection m_ClientConnection;
    HSteamNetPollGroup m_PollGroup;
    bool m_SceneLoaded = false;
    bool m_IsServer = false;
    bool m_IsClient = false;

    bool m_ShowServerWindow = false;
    bool m_ShowClientWindow = false;

    bool m_Connected = false;
    float m_UpdateTimer = 0.f;
    float m_NpcUpdateTimer = 0.f;
};

DECLARE_ZHM_PLUGIN(Hitmen)
