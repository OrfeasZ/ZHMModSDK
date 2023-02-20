#pragma once

#include "ZEntity.h"
#include "ZPrimitives.h"
#include "ZMath.h"
#include "ZResource.h"

class ZSpatialEntity :
    public ZEntityImpl
{
public:
    enum ERoomBehaviour
    {
        ROOM_STATIC = 0,
        ROOM_DYNAMIC = 1,
        ROOM_STATIC_OUTSIDE_CLIENT = 2
    };

    virtual void ZSpatialEntity_unk20() = 0;
    virtual void ZSpatialEntity_unk21() = 0;
    virtual void ZSpatialEntity_unk22() = 0;
    virtual void ZSpatialEntity_unk23() = 0;
    virtual void ZSpatialEntity_unk24() = 0;
    virtual void ZSpatialEntity_unk25() = 0;
    virtual void ZSpatialEntity_unk26() = 0;
    virtual void ZSpatialEntity_unk27() = 0;
    virtual void ZSpatialEntity_unk28() = 0;
    virtual void ZSpatialEntity_unk29() = 0;
    virtual void ZSpatialEntity_unk30() = 0;
    virtual void SetWorldMatrix(const SMatrix& matrix) = 0;
    virtual void CalculateBounds(float4& min, float4& max, uint32_t a3, uint32_t a4) = 0;
    virtual void ZSpatialEntity_unk33() = 0;
    virtual void ZSpatialEntity_unk34() = 0;
    virtual void ZSpatialEntity_unk35() = 0;
    virtual void ZSpatialEntity_unk36() = 0;
    virtual void ZSpatialEntity_unk37() = 0;
    virtual void ZSpatialEntity_unk38() = 0;
    virtual void ZSpatialEntity_unk39() = 0;
    virtual void ZSpatialEntity_unk40() = 0;
    virtual void ZSpatialEntity_unk41() = 0;

public:
    SMatrix GetWorldMatrix()
    {
        // This is probably something like "is this transform dirty and needs to be updated?".
        if ((m_nUnknownFlags & 0x80000) != 0)
            Functions::ZSpatialEntity_UnknownTransformUpdate->Call(this);

        return m_mTransform;
    }

public:
    PAD(0x08);
    SMatrix43 m_mTransform; // 0x20
    PAD(0x1C); // 0x50
    uint32_t m_nUnknownFlags; // 0x6C
    TEntityRef<ZSpatialEntity> m_eidParent; // 0x70
    PAD(0x20);
};

static_assert(offsetof(ZSpatialEntity, m_mTransform) == 0x20);
static_assert(offsetof(ZSpatialEntity, m_nUnknownFlags) == 0x6C);
static_assert(sizeof(ZSpatialEntity) == 0xA0);

class ZBoundedEntity :
    public ZSpatialEntity
{
public:
    PAD(0x18);
};

static_assert(sizeof(ZBoundedEntity) == 0xB8);


class ZPFObstacleEntity :
    public ZBoundedEntity
{
public:
    PAD(0x08); // 0xB8
    SVector3 m_vGlobalSize; // 0xC0
    float32 m_fPenaltyMultiplier; // 0xCC
    uint32 m_nObstacleBlockageFlags; // 0xD0
    bool m_bEnabled; // 0xD4
    ZResourcePtr m_pHelper; // 0xD8
};
