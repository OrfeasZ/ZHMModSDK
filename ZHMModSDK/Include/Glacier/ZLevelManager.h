#pragma once

#include "ZPrimitives.h"

class ZLevelSavableHandler;

class ZLevelManager {
public:
    enum class EGameState {
        EGS_Disabled = 0,
        EGS_PreloadAssets = 1,
        EGS_WaitingForLoadVideo = 2,
        EGS_Precaching = 3,
        EGS_Preparing = 4,
        EGS_WaitingForPrecache = 5,
        EGS_LoadSaveGame = 6,
        EGS_Activating = 7,
        EGS_ActivatedStart = 8,
        EGS_Activated = 9,
        EGS_Playing = 10,
        EGS_Deactivating = 11
    };

    virtual ~ZLevelManager() {}

public:
    PAD(0x160);
    ZLevelSavableHandler* m_pSavableHandler;
};