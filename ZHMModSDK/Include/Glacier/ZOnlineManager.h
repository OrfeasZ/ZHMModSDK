#pragma once

#include "IComponentInterface.h"
#include "TCheatProtect.h"
#include "Enums.h"

class IOnlineConfigurationListener {
public:
    virtual ~IOnlineConfigurationListener() = 0;
};

class ZOnlineStateMachine {
public:
    virtual ~ZOnlineStateMachine() = 0;
};

class ZOnlineManager :
    public IComponentInterface,
    public IOnlineConfigurationListener,
    public ZOnlineStateMachine {
public:
    bool IsConnected() const {
        return m_eState == ZOnlineManager_EState::eCONNECTED ||
               m_eState == ZOnlineManager_EState::eRETRY_SAVE_EVENTS ||
               m_eState == ZOnlineManager_EState::eREAUTHENTICATE;
    }

    ZOnlineManager_EState m_eState; // 0x18
    PAD(0x3EC); // 0x1C
    TCheatProtect<ZString> m_sGlobalServiceAuthenticationToken; // 0x408
    TCheatProtect<ZString> m_sGlobalServiceRefreshToken; // 0x418
    bool m_bIsRenewingResourceSignature; // 0x428
    ZString m_sResourcesBlobUrl; // 0x430
    TCheatProtect<ZString> m_sResourcesSignature; // 0x440
};