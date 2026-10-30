#pragma once

#include "ZEntity.h"
#include "Enums.h"
#include "TSharedValue.h"
#include "ZCurve.h"
#include "ZGameTime.h"
#include "ZMath.h"
#include "ZResource.h"
#include "IBoolConditionListener.h"

class ZSequenceEntity;
struct STrajectorySpeed;
class IEntityGroup;
class IBoolCondition;
class ZSplineEntity;

class ISequenceItem : public IComponentInterface {
public:
    virtual ~ISequenceItem() = 0;
};

class ISequenceValueSource : public IComponentInterface {
public:
    virtual ~ISequenceValueSource() = 0;
};

class ITrajectorySequenceItem : public IComponentInterface {
public:
    virtual ~ITrajectorySequenceItem() = 0;
};

class ISequenceTrack : public IComponentInterface {
public:
    virtual ~ISequenceTrack() = 0;
};

class ITrajectoryTrack : public IComponentInterface {
public:
    virtual ~ITrajectoryTrack() = 0;
    virtual bool GetStartWorldTransform(SMatrix& matrix) = 0;
    virtual ECoordinateSpace GetCoordinateSpaceOfFirstItem() = 0;
};

class ZSequenceItemBase : public ZEntityImpl, public ISequenceItem {
public:
    ZEntityRef m_pTrack;
    ZGameTime m_startTime;
    ZGameTime m_duration;
    bool m_triggerIfSpanning;
    ZEntityRef m_pNextItem;
    ZEntityRef m_pPreviousItem;
};

class ZEulerAngleTrajectorySource :
    public ZSequenceItemBase,
    public ISequenceValueSource,
    public ITrajectorySequenceItem {
public:
    ECoordinateSpace m_eCoordinateSpace;
    TSharedValue<ZCurve> m_CurveX;
    TSharedValue<ZCurve> m_CurveY;
    TSharedValue<ZCurve> m_CurveZ;
    TSharedValue<ZCurve> m_CurveRotX;
    TSharedValue<ZCurve> m_CurveRotY;
    TSharedValue<ZCurve> m_CurveRotZ;
};

class ZSequenceTrackBase : public ZEntityImpl, public ISequenceTrack {
public:
    class UpdateHelperData;

    virtual void ZSequenceTrackBase_Unk20() = 0;
    virtual void ZSequenceTrackBase_Unk21() = 0;
    virtual void ZSequenceTrackBase_Unk22() = 0;
    virtual void ZSequenceTrackBase_Unk23() = 0;
    virtual void ZSequenceTrackBase_Unk24() = 0;

    TArray<TEntityRef<ISequenceItem>> m_aItems;
    TEntityRef<ZSequenceEntity> m_pSequence;
    UpdateHelperData* m_helperData;
};

class ZEntityTrackBase : public ZSequenceTrackBase {
public:
    virtual void ZEntityTrackBase_Unk25() = 0;

    TEntityRef<IEntityGroup> m_entityGroup;
};

class ZTrajectoryTrackBase : public ZEntityTrackBase, public ITrajectoryTrack {
public:
    virtual void ZTrajectoryTrackBase_Unk26() = 0;
    virtual SMatrix* GetTargetObjectToParentMatrix(SMatrix& result) = 0;
    virtual void SetTargetObjectToParentMatrix(const SMatrix& mObjectToParent) = 0;
    virtual SMatrix* GetTargetObjectToWorldMatrix(SMatrix& result) = 0;
    virtual void SetTargetObjectToWorldMatrix(const SMatrix& mObjectToWorld, TEntityRef<ISequenceItem> pItem, ZGameTime newItemTime) = 0;

    bool m_bRestoreValueAfterEnd;
    float32 m_fBlendInTime;
    SMatrix m_OriginalValue;
    SMatrix m_targetStartTransform;
    bool m_bSequenceOriginWarningSent;
    STrajectorySpeed* m_pSpeedInfo;
};

class ZTrajectoryTrack : public ZTrajectoryTrackBase {
};

class ZAnimationResource;

class ZMorphemeAnimSourceBase : public ZSequenceItemBase, public ISequenceValueSource {
public:
    TResourcePtr<ZAnimationResource> m_pAnimationResource;
    ZGameTime m_startAnimationTime;
    ZGameTime m_SkipFromEnd;
    bool m_bCrop;
    bool m_bLoop;
    bool m_triggerIfSpanningOverride;
};

class ZMorphemeTrajectorySource : public ZMorphemeAnimSourceBase, public ITrajectorySequenceItem {
public:
    ECoordinateSpace m_eCoordinateSpace;
    float32 m_fBlendOut;
};

class ZMorphemeTrajectoryTrack : public ZTrajectoryTrackBase {
    bool m_bIsCollidedTransformStored;
    SMatrix m_mSequenceEndTransform;
    SMatrix m_mDeltaTransform;
    bool m_bRunAIInGaps;
};

namespace NMP {
    class DataBuffer;
    class Vector3;
    class Quat;
}

namespace MR {
    class AnimScalingFacial;
    class TrajectoryControlBase;
    class ILowLevelAnimSource;
    class RigToAnimMap;
    class EventSequenceHeader;
    class Rig;

    class IAnimSource {
    public:
        virtual ~IAnimSource() = 0;
        virtual void computeAtTime(float, MR::AnimScalingFacial*, NMP::DataBuffer*) = 0;
        virtual void computeAtTimeSingleTransform(float, unsigned int, NMP::Vector3*, NMP::Quat*) = 0;
        virtual float getDuration() = 0;
        virtual float getSampleFrequency() = 0;
        virtual const MR::TrajectoryControlBase* getTrajectoryChannelData() = 0;
        virtual void locate() = 0;
        virtual void dislocate() = 0;
        virtual bool CanRetarget() = 0;
    };
}

class ZFoVAnimation;
class ZAnimatedBoneScales;

class ZAnimationResource {
public:
    ZRuntimeResourceID m_ridResource;
    SMatrix43 m_sequenceOrigin;
    ZFoVAnimation* m_pFoVAnimation;
    ZAnimatedBoneScales* m_pAnimatedBoneScales;
    MR::IAnimSource* m_pAnimationSource;
    MR::RigToAnimMap* m_pRigToAnimMap;
    MR::EventSequenceHeader* m_pEventSequenceHeaderAMD;
    MR::Rig* m_pRig;
};

class ZSequenceEntity :
    public ZEntityImpl,
    public IBoolConditionListener {
public:
    PAD(0x60);
    TArray<ZEntityRef> m_aTracksAndGroups; // 0x80
    ZGameTime m_sequenceTime; // 0x98
    ZGameTime m_duration; // 0xA0
    ZGameTime m_startTime; // 0xA8
    bool m_bUpdateOnGameTime; // 0xB0
    PAD(0xF);
    bool m_bSaveState; // 0xC0
    TEntityRef<ZSpatialEntity> m_sequenceOrigin; // 0xC8
    TEntityRef<IBoolCondition> m_rUpdateCondition; // 0xD8
    int32 m_nLoopCount; // 0x140
    bool m_bPausedOnStart; // 0x152
    bool m_bLetterbox; // 0x170
    ZGameTime m_letterboxFadeIn; // 0x178
    ZGameTime m_letterboxFadeOut; // 0x180
    ZSequenceEntity_ELetterBoxAspect m_eLetterBoxAspect; // 0x188
    bool m_bFadeOutBeforeEnd; // 0x18C
    bool m_bCanBeInterrupted; // 0x18D
    bool m_bResetOnStop; // 0x18E
    bool m_bRestoreAborted; // 0x18F
    bool m_bRestoreFromStop; // 0x190
    EVrSequenceCameraMode m_eVrCameraMode; // 0x194
    bool m_bIsVrCameraTranslationAllowed; // 0x198
};

class ISequenceTrackGroup : public IComponentInterface {
};

class IEntityGroup : public IComponentInterface {
};

class ZTrackGroupBase : public ZEntityImpl, public ISequenceTrackGroup {
public:
    TArray<ZEntityRef> m_aTracksAndGroups;
    TEntityRef<ZSequenceEntity> m_pSequence;
};

class ZEntityGroup : public ZTrackGroupBase, public IEntityGroup {
public:
    ZEntityRef m_targetEntity;
};

class ZBezierSplineTrajectorySource :
    public ZSequenceItemBase,
    public ISequenceValueSource,
    public ITrajectorySequenceItem {
public:
    ECoordinateSpace m_eCoordinateSpace; // 0x60
    TEntityRef<ZSplineEntity> m_rSpline; // 0x68
};