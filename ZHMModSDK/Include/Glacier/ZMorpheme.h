#pragma once

#include "ZEntity.h"
#include "ZResource.h"
#include "ZDelegate.h"
#include "ZMath.h"
#include "ZPhysics.h"

class ZNetworkResource;
class IVariationResourceEntity;
class ZMorphemeNetworkInstance;
class ZAnimationBoneData;

struct SBoneAnimatorInfo {
    ZAnimationBoneData* m_pBoneData; // 0x0
    ZDelegate<void()> m_NotifyBonesChanged; // 0x8
    ZDelegate<void()> m_ResendInfo; // 0x28
};

class IMorphemeEntity {
public:
    virtual ~IMorphemeEntity() = 0;
};

class ZMorphemeBaseEntity : public ZEntityImpl {
public:
    TResourcePtr<ZNetworkResource> m_pRuntimeNetworkResource; // 0x18
    ZEntityRef m_pVariationResourceEntity; // 0x20
    int32 m_networkInstanceID; // 0x28
    TEntityRef<IVariationResourceEntity> m_pVariationResourceInterface; // 0x30
    PAD(0x40); // 0x40
    bool m_bInitialized : 1; // 0x80
    bool m_bInstantiateNetwork : 1;
};

class ZMorphemeEntity : public ZMorphemeBaseEntity, public IMorphemeEntity, public IBoneAnimator {
public:
    bool m_bInstantiateNetworkPropertyVal; // 0x98
    float4 m_postProcessorGroundWorldOffset; // 0xA0
    float4 m_postProcessorGroundWorldPosition; // 0xB0
    PAD(0x20); // 0xC0
    SBoneAnimatorInfo m_BoneAnimatorInfo; // 0xE0
    bool m_bIsMeshToRigMapInitialized; // 0x128
    uint64 m_nRealignUserData; // 0x130
    TArray<int> m_aMeshIDToRigID; // 0x138
    ZMorphemeNetworkInstance* m_pMorphemeNetworkInstance; // 0x150
};