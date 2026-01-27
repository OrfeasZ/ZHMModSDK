#pragma once

#include "ZEntity.h"
#include "ZPrimitives.h"
#include "ZMath.h"
#include "ZResource.h"
#include "Functions.h"

class ZSpatialEntity :
        public ZEntityImpl {
public:
    enum ERoomBehaviour {
        ROOM_STATIC                = 0,
        ROOM_DYNAMIC               = 1,
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
    virtual void SetObjectToWorldMatrixFromEditor(SMatrix mObjectToWorld) = 0;
    virtual void CalculateBounds(float4& vMin_, float4& vMax_, uint32_t nIncludeFlags, uint32_t nExcludeFlags) = 0;
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
    SMatrix GetObjectToWorldMatrix() const {
        if (m_bWorldTransformDirty) {
            Functions::ZSpatialEntity_UpdateCachedWorldMat->Call(this);
        }

        return m_mTransform;
    }

    float4 GetWorldDirection(const float4& p_LocalDirection) const {
        return GetObjectToWorldMatrix().WVectorRotate(p_LocalDirection);
    }

public:
    ZEvent<const ZEntityRef&, const SMatrix43&>* m_pTransformChangeCallBackEvent; // 0x18
    SMatrix43 m_mTransform; // 0x20
    SVector4 m_vObjectToParentRotation; // 0x50
    SVector3 m_vObjectToParentTranslation; // 0x60
    uint32_t m_bVisible : 1; // 0x6C
    uint32_t m_bIsPrivate : 1;
    uint32_t m_bVisibleInBoxReflection : 1;
    uint32_t m_Unk0 : 1;
    uint32_t m_Unk1 : 1;
    uint32_t m_Unk2 : 1;
    uint32_t m_Unk3 : 1;
    uint32_t m_bEditorVisible : 1;
    uint32_t m_bParentHidden : 1;
    uint32_t m_bSceneChild : 1;
    uint32_t m_bFPSDrawMode : 1;
    uint32_t m_bNotifyChange : 1;
    uint32_t m_bRoomDynamicAlways : 1;
    uint32_t m_bParentDynamic : 1;
    uint32_t m_bIsDynamic : 1;
    uint32_t m_bIncludeInParentsBounds : 1;
    uint32_t m_bUpdateBoundsPending : 1;
    uint32_t m_bParentHiddenInBoxReflection : 1;
    uint32_t m_bForceVisible : 1;
    uint32_t m_bWorldTransformDirty : 1;
    uint32_t m_eRoomBehaviour : 2;
    uint32_t m_bHighPriorityTextures : 1;
    TEntityRef<ZSpatialEntity> m_eidParent; // 0x70
    ZSpatialEntity* m_pTransformParent; // 0x80
    ZSpatialEntity* m_pTransformChildren; // 0x88
    ZSpatialEntity* m_pTransformNext; // 0x90
    ZSpatialEntity* m_pTransformPrev; // 0x98
};

static_assert(offsetof(ZSpatialEntity, m_mTransform) == 0x20);
static_assert(sizeof(ZSpatialEntity) == 0xA0);

class ZBoundedEntity :
        public ZSpatialEntity {
public:
    SVector3 m_vCenter; // 0xA0
    SVector3 m_vHalfSize; // 0xAC
};

static_assert(sizeof(ZBoundedEntity) == 0xB8);