#pragma once

#include "Common.h"
#include "Hooks.h"

class ZUpdateEventContainer;

class ZGameLoopManager
{
public:
    virtual ~ZGameLoopManager() {}

public:
    PAD(0x50);
    ZUpdateEventContainer* m_pUpdateEventContainer; // 0x58

public:
    void RegisterFrameUpdate(const ZDelegate<void(const SGameUpdateEvent&)>& p_Delegate, int p_Priority, EUpdateMode p_UpdateMode)
    {
        Hooks::ZUpdateEventContainer_AddDelegate->Call(m_pUpdateEventContainer, p_Delegate, p_Priority, p_UpdateMode);
    }

    void UnregisterFrameUpdate(const ZDelegate<void(const SGameUpdateEvent&)>& p_Delegate, int p_Priority, EUpdateMode p_UpdateMode)
    {
        Hooks::ZUpdateEventContainer_RemoveDelegate->Call(m_pUpdateEventContainer, p_Delegate, p_Priority, p_UpdateMode);
    }
};
