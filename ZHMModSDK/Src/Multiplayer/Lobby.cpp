#include "Lobby.h"

#include "Logging.h"
#include "ModSDK.h"
#include "Util/ProcessUtils.h"

template <typename TRet, typename... TArgs>
auto ReplaceVtableFunc(uintptr_t p_VtableAddr, size_t p_VtableIndex, TRet (*p_Func)(TArgs...)) {
    // Get pointer to function in the vtable.
    const uintptr_t s_VtableFuncOffset = p_VtableAddr + (p_VtableIndex * sizeof(void*));
    void** s_VTableFuncPtr = reinterpret_cast<void**>(s_VtableFuncOffset);

    // Save original function and replace with ours.
    auto s_OriginalFunc = reinterpret_cast<TRet(*)(TArgs...)>(*s_VTableFuncPtr);

    DWORD s_OldProtect;
    VirtualProtect(s_VTableFuncPtr, sizeof(void*), PAGE_EXECUTE_READWRITE, &s_OldProtect);
    *s_VTableFuncPtr = p_Func;
    VirtualProtect(s_VTableFuncPtr, sizeof(void*), s_OldProtect, nullptr);

    return s_OriginalFunc;
}

class ZLobbyCreate;

struct ZLobbyIdleThing {
    PAD(80);
    EMultiplayerLobbyRequestType req;
};

struct ZLobbyIdle {
    PAD(32);
    ZLobbyIdleThing* thing;
};

static uint64_t CreateLobby(ZLobbyCreate* th) {
    Logger::Debug("Creating lobby!");
    return 69;
}

static uint64_t ConnectingLobby(ZLobbyCreate* th) {
    Logger::Debug("Connecting lobby!");
    return 69;
}

static uint64_t ConnectedLobby(ZLobbyCreate* th) {
    Logger::Debug("Connected lobby!");
    return 69;
}

static uint64_t VersionCheckLobby(ZLobbyCreate* th) {
    Logger::Debug("Version check lobby!");
    return 69;
}

static uint64_t CreateLocalHostLobby(ZLobbyCreate* th) {
    Logger::Debug("Create local host lobby!");
    return 69;
}

static uint64_t JoinLobby(ZLobbyCreate* th) {
    Logger::Debug("Join lobby!");
    return 69;
}

static uint64_t IdleLobby(ZLobbyCreate* th) {
    Logger::Debug("Idle lobby!");
    return 69;
}

static void* IdleLobbyUpdate(ZLobbyIdle* th, void* a2) {
    typedef void*(__thiscall *IdleLobbyUpdate_t)(ZLobbyIdle* th, void* a2);
    static IdleLobbyUpdate_t Original = reinterpret_cast<IdleLobbyUpdate_t>(0x0000000140FFDB10);

    if (th->thing->req == EMultiplayerLobbyRequestType::LOBBY_REQUEST_CREATE) {
        Logger::Warn("Dong");
        th->thing->req = EMultiplayerLobbyRequestType::LOBBY_REQUEST_CREATE_LOCALHOST;
    }

    return Original(th, a2);
}

void Multiplayer::Lobby::Setup() {
    auto s_LobbyVtableRel = Util::ProcessUtils::SearchPattern(
        ModSDK::GetInstance()->GetModuleBase(),
        ModSDK::GetInstance()->GetSizeOfCode(),
        reinterpret_cast<const uint8_t*>(
            "\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x7C\x24\x38\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x54\x24\x20"),
        "xxx????xxxxxxxx????xxxxx"
    );

    if (s_LobbyVtableRel == 0) {
        Logger::Warn("Could not find Epic lobby manager vtable address.");
        return;
    }

    // Get vtable address from relative addr.
    const uintptr_t s_LobbyManagerVtable = s_LobbyVtableRel + 7 + *reinterpret_cast<int32_t*>(s_LobbyVtableRel + 3);
    Logger::Debug("Found Epic lobby manager vtable at {:X}", s_LobbyManagerVtable);

    ReplaceVtableFunc(s_LobbyManagerVtable, 12, &Lobby::OpenInviteDialog);

    const uintptr_t s_LobbyCreateVtable = 0x0000000141F452B8;
    ReplaceVtableFunc(s_LobbyCreateVtable, 5, CreateLobby);

    const uintptr_t s_LobbyConnectingVtable = 0x0000000141F44678;
    ReplaceVtableFunc(s_LobbyConnectingVtable, 5, ConnectingLobby);

    const uintptr_t s_LobbyConnectedVtable = 0x0000000141F44AC0;
    //ReplaceVtableFunc(s_LobbyConnectedVtable, 5, ConnectedLobby);

    const uintptr_t s_LobbyVersionCheckVtable = 0x0000000141F44D28;
    ReplaceVtableFunc(s_LobbyVersionCheckVtable, 5, VersionCheckLobby);

    const uintptr_t s_LobbyCreateLocalHostVtable = 0x0000000141F44C98;
    //ReplaceVtableFunc(s_LobbyCreateLocalHostVtable, 5, CreateLocalHostLobby);

    const uintptr_t s_LobbyJoinVtable = 0x0000000141F44DD8;
    ReplaceVtableFunc(s_LobbyJoinVtable, 5, JoinLobby);

    const uintptr_t s_LobbyIdleVtable = 0x0000000141F45718;
    //ReplaceVtableFunc(s_LobbyIdleVtable, 5, IdleLobby);
    ReplaceVtableFunc(s_LobbyIdleVtable, 10, IdleLobbyUpdate);

    uint8_t s_LobbyRequest = static_cast<uint8_t>(EMultiplayerLobbyRequestType::LOBBY_REQUEST_CREATE_LOCALHOST);

    SDK()->PatchCode(
        "\xC7\x41\x08\x00\x00\x00\x00\x48\x83\xC4\x00\xC3\x48\x83\xC4",
        "xxx????xxx?xxxx",
        &s_LobbyRequest,
        1,
        3
    );
}

void Multiplayer::Lobby::OpenInviteDialog(ZGameLobbyManager* th) {
    Logger::Debug("Opening invite dialog!");
}
