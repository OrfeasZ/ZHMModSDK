#pragma once

#include "ZSpatialEntity.h"
#include "SColorRGB.h"
#include "ZSparseBitArray.h"

class IRoomEntity;

class IGateEntity : public IComponentInterface {
};

class ZGateEntity : public ZBoundedEntity, public IGateEntity {
public:
    PAD(0x38);
    bool m_bIsOpen; // 0xF8
    bool m_bSound; // 0xF9
    bool m_bVisibility; // 0xFA
    bool m_bPerception; // 0xFB
    bool m_bReasoning; // 0xFC
    bool m_bAutoClientRooms; // 0xFD
    bool m_bDisableCulling; // 0xFE
    bool m_bUseInFallbacks; // 0xFF
    float32 m_fOpenFraction; // 0x100
    SColorRGB m_ClosedColor; // 0x104
    SVector3 m_vPortalSize; // 0x110
    TEntityRef<IRoomEntity> m_RoomLeft; // 0x120
    SVector2 m_vLeftFadeAngles; // 0x130
    float32 m_fLeftFadeLength; // 0x138
    float32 m_fLeftFadeStart; // 0x13C
    bool m_bFlipLeftNormal; // 0x140
    TEntityRef<IRoomEntity> m_RoomRight; // 0x148
    SVector2 m_vRightFadeAngles; // 0x158
    float32 m_fRightFadeLength; // 0x160
    float32 m_fRightFadeStart; // 0x164
    float32 m_fClipDistance; // 0x168
    TArray<ZEntityRef> m_Clients; // 0x170
    ZResourcePtr m_pHelper; // 0x188
    ZResourcePtr m_pHelperClosed; // 0x190
    ZEntityRef m_PierceOccluder; // 0x198
    SVector3 m_vConnectorOffset; // 0x1A0
    PAD(0x8);
};

class IRoomEntity : public IComponentInterface {
};

class ZRoomEntity : public ZBoundedEntity, public IRoomEntity {
public:
    TArray<ZGateEntity*> m_Gates; // 0xC0
    PAD(0x8); // 0xD8
    uint16 m_nRoomID; // 0xE0
};

struct SRoomInfoHeader {
    PAD(0xE0);
};

struct SGateInfoHeader {
    ZGateEntity* pGateEntity;
    PAD(0xC8);
};

class ZRoomManager {
public:
    TArray<ZRoomEntity*> m_RoomEntities; // 0x0
    PAD(0x6B8);
    TArray<SRoomInfoHeader> m_RoomHeaders; // 0x6D0
    TArray<SGateInfoHeader> m_GateHeaders; // 0x6E8
    PAD(0x318);
    ZSparseBitArray m_NodesVisibleRender; // 0xA18
    ZSparseBitArray m_RoomsVisibleRender; // 0xA38
    ZSparseBitArray m_GatesVisibleRender; // 0xA58
    ZSparseBitArray m_NodesVisibleMain; // 0xA78
    ZSparseBitArray m_RoomsVisibleMain; // 0xA98
    ZSparseBitArray m_GatesVisibleMain; // 0xAB8
    ZSparseBitArray m_NodesVisibleChangedMain; // 0xAD8
};
