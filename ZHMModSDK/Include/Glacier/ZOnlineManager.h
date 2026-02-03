#pragma once

#include "IComponentInterface.h"
#include "TCheatProtect.h"

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
    PAD(0x3F0); // 0x18
    TCheatProtect<ZString> m_sGlobalServiceAuthenticationToken; // 0x408
    TCheatProtect<ZString> m_sGlobalServiceRefreshToken; // 0x418
    bool m_bIsRenewingResourceSignature; // 0x428
    ZString m_sResourcesBlobUrl; // 0x430
    TCheatProtect<ZString> m_sResourcesSignature; // 0x440
};