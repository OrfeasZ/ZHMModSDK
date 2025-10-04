#pragma once

class ZGameLobbyManager;

namespace Multiplayer {
    class Lobby {
    public:
        static void Setup();
        static void OpenInviteDialog(ZGameLobbyManager* th);
    };
}