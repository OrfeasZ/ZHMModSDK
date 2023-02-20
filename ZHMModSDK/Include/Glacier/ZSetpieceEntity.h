#pragma once

#include "ZEntity.h"
#include "ZSpatialEntity.h"

class ZSetpieceEntity : public ZEntityImpl
{
public:
    virtual ~ZSetpieceEntity() = default;

public:
    ZRepositoryID m_sTypeId; // 0x18
    ZRepositoryID m_sId; // 0x28
    ZEntityRef m_rSetpiece; // 0x38
    TEntityRef<ZSpatialEntity> m_rSpatial; // 0x40
    TEntityRef<ZSetpieceEntity> m_rOriginal; // 0x50
    PAD(0x10);
};

static_assert(offsetof(ZSetpieceEntity, m_sTypeId) == 0x18);
static_assert(offsetof(ZSetpieceEntity, m_rSpatial) == 0x40);
static_assert(offsetof(ZSetpieceEntity, m_rOriginal) == 0x50);
static_assert(sizeof(ZSetpieceEntity) == 0x70);
