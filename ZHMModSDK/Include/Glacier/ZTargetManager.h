#pragma once

#include "Reflection.h"
#include "TArray.h"
#include "ZActor.h"

class ZTargetManager : public IComponentInterface {
public:
    struct STargetInfo {
        uint32 m_tokenId;
        TEntityRef<ZActor> m_rActor;
        bool m_bInFustrum;
        float32 m_fDistance;
        bool m_bVisible;
        TArray<IAsyncRayHandle*> m_aRayHandles;
        ZRepositoryID m_RepositoryId;
        bool m_bIsHidden;
    };

    TArray<STargetInfo> m_aTargets;
};