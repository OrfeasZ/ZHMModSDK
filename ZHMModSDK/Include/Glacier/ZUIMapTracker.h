#pragma once

#include "Reflection.h"
#include "ZEntity.h"
#include "ZResource.h"
#include "ZMath.h"
#include "ZSpatialEntity.h"
#include "ISavable.h"

class ZDynamicObjectEntity;
class ZUIDataProvider;

class UIMapLayer {
public:
    enum class EUIMapLayerID : int32_t {
        eUIMLI_UNSPECIFIED = 0,
        eUIMLI_STAIRCASE = 1,
        eUIMLI_AREA_UNDISCOVERED = 2,
        eUIMLI_TEXT = 3,
        eUIMLI_DROPPED_ITEMS_AND_DISGUISES = 4,
        eUIMLI_NPC = 5,
        eUIMLI_NORTH_INDICATOR = 6,
        eUIMLI_SECURITY_CAMERA = 7,
        eUIMLI_AGENCY_PICKUP = 8,
        eUIMLI_OPPORTUNITY = 9,
        eUIMLI_EXIT = 10,
        eUIMLI_OBJECTIVE = 11,
        eUIMLI_TARGET = 12,
        eUIMLI_OPPONENT = 13,
        eUIMLI_HERO = 14,
    };
};

class ZUIMapTrackerAspect : public ZEntityImpl {
public:
    TResourcePtr<IEntityFactory> m_pViewFactory; // 0x18
    SVector2 m_vScale; // 0x20
    bool m_bFixedRotation; // 0x28
    bool m_bVisibilityCheckCurrentMap; // 0x29
    bool m_bVisibilityCheckDetectors; // 0x2A
    bool m_bShowOnEdge; // 0x2B
    bool m_bShowOnMenuMap; // 0x2C
    bool m_bShowOnMainMap; // 0x2D
    bool m_bShowOnMiniMap; // 0x2E
    bool m_bShowOnLegend; // 0x2F
    bool m_bShowWhenZoomedIn; // 0x30
    bool m_bShowWhenZoomedOut; // 0x31
    bool m_bLevelCheck; // 0x32
    TEntityRef<ZDynamicObjectEntity> m_pHoverView; // 0x38
    TEntityRef<ZUIDataProvider> m_pHoverDataProvider; // 0x48
    UIMapLayer::EUIMapLayerID m_eLayer; // 0x58
    TArray<TEntityRef<ZSpatialEntity>> m_aProxySpatials; // 0x60
};

class ZGenericUIMapTrackerAspect : public ZUIMapTrackerAspect {
public:
    PAD(0xA0); // 0x78
    ZString m_sIconId; // 0x118
};

class ZStashPointTrackerAspect : public ZGenericUIMapTrackerAspect {
public:
    TEntityRef<ZStashPointEntity> m_pStashPoint; // 0x128
};

class ZUIMapTrackerManager : public IComponentInterface, public ISavable {
public:
    TArray<TEntityRef<ZUIMapTrackerAspect>> m_aUIMapTrackers; // 0x10
};