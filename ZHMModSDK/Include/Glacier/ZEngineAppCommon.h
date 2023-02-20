#pragma once

#include "ZEntity.h"
#include "ZPrimitives.h"

class ZCameraEntity;
class ZFreeCameraControlEntity;
class ZFreeCameraControlEditorStyleEntity;

// Size = 0xA0 (unverified)
class ZEngineAppCommon
{
public:
    PAD(0x38);
    TEntityRef<ZFreeCameraControlEntity> m_pFreeCameraControl01; // 0x38
    TEntityRef<ZFreeCameraControlEditorStyleEntity> m_pFreeCameraControlEditorStyle01; // 0x48
    TEntityRef<ZCameraEntity> m_pFreeCamera01; // 0x58
    PAD(0x38); // 0x68
};
