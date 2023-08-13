#pragma once

#include "Reflection.h"

typedef uint64_t CSteamID;

class ZGameLobbyManager : public IComponentInterface {
public:
    virtual void ZGameLobbyManager_unk5() = 0;
    virtual void ZGameLobbyManager_unk6() = 0;
    virtual void ZGameLobbyManager_unk7() = 0;
    virtual void OnDisconnected() = 0;
    virtual void ZGameLobbyManager_unk9() = 0;
    virtual void ZGameLobbyManager_unk10() = 0;
    virtual void ZGameLobbyManager_unk11() = 0;
    virtual void OpenInviteDialog() = 0;
    virtual void SetReady() = 0;
    virtual void ZGameLobbyManager_unk14() = 0;
    virtual void ZGameLobbyManager_unk15() = 0;
    virtual void SetLobbyData(const ZString& key, const ZString& value) = 0;
    virtual void ZGameLobbyManager_unk17() = 0;
    virtual void ZGameLobbyManager_unk18() = 0;
    virtual void ZGameLobbyManager_unk19() = 0;
    virtual void ZGameLobbyManager_unk20() = 0;
    virtual void ZGameLobbyManager_unk21() = 0;
    virtual void ZGameLobbyManager_unk22() = 0;
    virtual void ZGameLobbyManager_unk23() = 0;
    virtual void UpdateLobbyData() = 0;
    virtual void ZGameLobbyManager_unk25() = 0;
};

/*
Steam lobby manager:

CRITICAL_SECTION m_CS; // 0xD8 (216)
ZString m_LobbyName; // 0xB8 (184)
CSteamID m_LobbyId; // 0x108 (264)
*/

class ZGameLobbyManagerEpic : public ZGameLobbyManager {
    
};

class ZLobbyContext {
	
};

namespace hsm {
	struct Transition {
		void* NewBaseCtor;
		void* Unk0x8;
		uint32_t Unk0x10;
	};

	class HSMStateBase {
	public:
		virtual ~HSMStateBase() {}
		virtual void HSMStateBase_unk1() = 0;
		virtual void HSMStateBase_unk2() = 0;
		virtual void HSMStateBase_unk3() = 0;
		virtual void HSMStateBase_unk4() = 0;
		virtual void OnEnter() = 0;
		virtual void HSMStateBase_unk6() = 0;
		virtual void HSMStateBase_unk7() = 0;
		virtual void HSMStateBase_unk8() = 0;
		virtual void GetTransition(Transition& next) = 0; // Called directly after OnEnter.
		virtual void Update(Transition& next) = 0;
		virtual void HSMStateBase_unk11() = 0;
		virtual void HSMStateBase_unk12() = 0;
		virtual void HSMStateBase_unk13() = 0;
		virtual void HSMStateBase_unk14() = 0;
		virtual void HSMStateBase_unk15() = 0;
		virtual void HSMStateBase_unk16() = 0;
	};
}