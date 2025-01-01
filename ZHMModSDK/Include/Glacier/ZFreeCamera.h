#pragma once

#include "ZCameraEntity.h"
#include "ZEntity.h"

class IFreeCameraControl {
public:
    virtual void IFreeCameraControl_unk0() = 0;
    virtual void SetActive(bool active) = 0;
    virtual void IFreeCameraControl_unk2() = 0;
    virtual void SetCameraEntity(const TEntityRef<ZCameraEntity>& entity) = 0;
    virtual void IFreeCameraControl_unk4() = 0;
};

class ZFreeCameraControlEntity :
        public ZEntityImpl,
        public IFreeCameraControl {
public:
    PAD(0x11); // 0x20
    bool m_bFreezeCamera; // 0x31
};

class ZFreeCameraControlEditorStyleEntity :
        public ZEntityImpl,
        public IFreeCameraControl {
public:
    TEntityRef<ZSpatialEntity> m_cameraEntity; // 0x20
    bool m_bActive; // 0x30
};

class ZSelectionForFreeCameraEditorStyleEntity :
        public ZEntityImpl // Offset 0x0
{
public:
    virtual void Init() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk21() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk22() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk23() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk24() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk25() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk26() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk27() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk28() = 0;
    virtual void ZSelectionForFreeCameraEditorStyleEntity_unk29() = 0;

    TArray<ZEntityRef> m_selection; // 0x18
};
