#pragma once

#include "ZEntity.h"

class ZBoxVolumeEntity;

class ZAIAreaEntityBase :
        public ZEntityImpl {
public:
    bool m_bDbgAreaIsRegistered; // 0x1C
    uint32 m_nDbgConcept; // 0x20
    uint32 m_nDbgParentConcept; // 0x24
};

class ZAIAreaEntity :
        public ZAIAreaEntityBase {
public:
    TEntityRef<ZAIAreaEntityBase> m_rParentArea; // 0x28
    TArray<TEntityRef<ZBoxVolumeEntity>> m_aAreaVolumes; // 0x38
};