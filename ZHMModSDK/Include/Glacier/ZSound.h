#pragma once

#include "ZSpatialEntity.h"
#include "ZCurve.h"
#include "TSharedValue.h"

class ISoundAmbienceElement;
class ISoundGateController;

class ZSoundGateEntity : public ZBoundedEntity {
public:
    bool m_bEnabled; // 0xB8
    bool m_bIsOpen; // 0xB9
    float32 m_fThicknessSource; // 0xBC
    float32 m_fThicknessDestination; // 0xC0
    float32 m_fLateralMargin; // 0xC4
    float32 m_fLateralMarginLeft; // 0xC8
    float32 m_fLateralMarginRight; // 0xCC
    float32 m_fVerticalMargin; // 0xD0
    float32 m_fVerticalMarginTop; // 0xD4
    float32 m_fVerticalMarginBottom; // 0xD8
    float32 m_fOcclusionRadius; // 0xDC
    float32 m_fOcclusionMinRadius; // 0xE0
    float32 m_fOcclusionMaxDistance; // 0xE4
    float32 m_fOcclusionMinDistance; // 0xE8
    uint32 m_nOcclusionMaxAngleDeg; // 0xEC
    float32 m_fDestinationDiffractionScaling; // 0xF0
    float32 m_fOpenOcclusionBlendDistance; // 0xF4
    float32 m_fProjectionBlendDistance; // 0xF8
    ESoundGateType m_GateType; // 0xFC
    ESoundGateFlags m_GateFlags; // 0x100
    SVector2 m_vLocalSize; // 0x104
    float32 m_fOcclusionFactorOpen; // 0x10C
    float32 m_fOcclusionFactorClosed; // 0x110
    float32 m_fOpenCloseChangeTime; // 0x114
    bool m_bIgnoreForProjection; // 0x118
    TEntityRef<ISoundAmbienceElement> m_AmbienceSide0; // 0x120
    TEntityRef<ISoundAmbienceElement> m_AmbienceSide1; // 0x130
    TSharedValue<ZCurve> m_FadeOutCurve; // 0x140
    TSharedValue<ZCurve> m_FadeInCurve; // 0x148
    TSharedValue<ZCurve> m_OcclusionTransitionCurve; // 0x150
    TEntityRef<ISoundGateController> m_rController; // 0x158
    float32 m_fReverbBlendDistance; // 0x168
    PAD(0xD4); // 0x16C
};