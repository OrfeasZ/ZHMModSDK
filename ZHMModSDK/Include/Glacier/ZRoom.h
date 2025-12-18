#pragma once

#include "ZSpatialEntity.h"

class IRoomEntity : public IComponentInterface {
};

class ZRoomEntity : public ZBoundedEntity, public IRoomEntity {
public:
    PAD(0xE0);
    uint16 m_nRoomID;
};

class ZRoomManager {
public:
    TArray<ZRoomEntity*> m_RoomEntities;
};