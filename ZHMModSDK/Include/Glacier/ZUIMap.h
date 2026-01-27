#pragma once

#include "ZObject.h"
#include "ZEntity.h"
#include "ZItem.h"

class IUIMapProvider;

class ZUIMapDebug {
public:
    virtual ~ZUIMapDebug() = 0;
};

class ZWorldMapMetaData {
public:
    ZDynamicObject m_data;
};

class ZUIMapConfig : public ZEntityImpl, public ZUIDataProvider {
public:
    TEntityRef<IUIMapProvider> m_pCurrentMapProvider;
    PAD(0x20);
    bool m_bSetAsActive;
    ZResourcePtr m_pMetaDataResource;
    ZWorldMapMetaData m_metaData;
    TArray<TEntityRef<IUIMapProvider>> m_aProviders;
    TEntityRef<IUIMapProvider> m_pSatelliteMap;
    TEntityRef<ZBoxVolumeEntity> m_pNavigationalBoundsOverride;
    float32 m_fZoomedInLevel;
    float32 m_fZoomedOutLevel;
    bool m_bUseFixedMap;
};

class ZUIMapManager : IComponentInterface, ZUIMapDebug {
public:
    TArray<TEntityRef<ZUIMapConfig>> m_aMapConfigs;
    TEntityRef<ZUIMapConfig> m_pFocusMapConfig;
};