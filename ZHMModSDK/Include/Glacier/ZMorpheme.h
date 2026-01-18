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
    ZAnimationBoneData* m_pBoneData;
    ZDelegate<void()> m_NotifyBonesChanged;
    ZDelegate<void()> m_ResendInfo;
};

class IMorphemeEntity {
public:
    virtual ~IMorphemeEntity() = 0;
};

class ZMorphemeBaseEntity : public ZEntityImpl {
public:
    TResourcePtr<ZNetworkResource> m_pRuntimeNetworkResource;
    ZEntityRef m_pVariationResourceEntity;
    int32 m_networkInstanceID;
    TEntityRef<IVariationResourceEntity> m_pVariationResourceInterface;
    PAD(0x40);
    bool m_bInitialized : 1;
    bool m_bInstantiateNetwork : 1;
};

class ZMorphemeEntity : public ZMorphemeBaseEntity, public IMorphemeEntity, public IBoneAnimator {
public:
    bool m_bInstantiateNetworkPropertyVal;
    float4 m_postProcessorGroundWorldOffset;
    float4 m_postProcessorGroundWorldPosition;
    PAD(0x20);
    SBoneAnimatorInfo m_BoneAnimatorInfo;
    bool m_bIsMeshToRigMapInitialized;
    uint64 m_nRealignUserData;
    TArray<int> m_aMeshIDToRigID;
    ZMorphemeNetworkInstance* m_pMorphemeNetworkInstance;
};