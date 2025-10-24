#pragma once

#include "ZPrimitives.h"
#include "ZEntity.h"
#include "THashMap.h"

class ZEntitySceneContext;

class ZEntityManager {
public:
    virtual ~ZEntityManager() {}

public:
    ZEntitySceneContext* m_pContext; // 0x8
    PAD(0x18); // 0x10
    THashMap<uint64_t, ZEntityRef> m_DynamicEntities; // 0x28
    PAD(0xC0); // 0x48
    THashMap<uint64_t, uint64_t> m_DynamicEntityIdToCount; // 0x108
};