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
    void ZoomCameraToSelection() {
        TArray<TEntityRef<ZSpatialEntity>> s_SelectedEntities;

        Functions::ZFreeCameraControlEditorStyleEntity_GetSelectedEntities->Call(this, s_SelectedEntities);
        Functions::ZFreeCameraControlEditorStyleEntity_MoveCameraToSpatialEntities->Call(this, s_SelectedEntities, true);

        float4 s_AABBMin {};
        float4 s_AABBMax {};

        Functions::ZFreeCameraControlEditorStyleEntity_GetSelectionCenter->Call(this, m_vHookPoint, s_AABBMin, s_AABBMax);
    }

    TEntityRef<ZSpatialEntity> m_cameraEntity; // 0x20
    bool m_bActive; // 0x30
    TEntityRef<ZCameraEntity> m_pControlledCameraEntity; // 0x38
    bool m_bRotationWasActive; // 0x48
    bool m_bObjectHookWasActive; // 0x49
    float4 m_vMousePos; // 0x50
    float4 m_vLastDragPoint; // 0x60
    float4 m_vHookPoint; // 0x70
    bool m_bDraggingIsActive; // 0x80
    float m_fSpeed; // 0x84
    bool m_bZoomToSelectedPivotNextTime; // 0x88
    float m_fZoomToPivotDistance; // 0x8C
    bool m_bPickingWasPermitted; // 0x90
    int32 m_nHookDebugDrawId; // 0x94
    bool m_bZoomToSelectionWasActive; // 0x98
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