#pragma once

#include "ZEntity.h"
#include "ZItem.h"

class ZClothBundleEntity;

class ZStashPointEntity :
    public ZSpatialEntity,
    public ISavableEntity,
    public ZUIDataProvider {
public:
    TEntityRef<ZClothBundleEntity> m_rMockClothbundle; // 0xE8
    TEntityRef<ZSpatialEntity> m_rBundlePosition; // 0xF8
    TEntityRef<ZItemRepositoryKeyEntity> m_rContainerItemKey; // 0x108
    ZRepositoryID m_sId; // 0x118
    bool m_bAllowItemPickup; // 0x128
    bool m_bMainItemVisibleOnSpawn; // 0x129
    bool m_bHide; // 0x12A
    bool m_bUnlockedByDefault; // 0x12B
    bool m_bIsPreparing; // 0x12C
    uint32 m_iMainItemTicket; // 0x130
    PAD(0x1C);
    TEntityRef<IItem> m_rMainItem; // 0x150
    TArray<unsigned int> m_aRequestedItemTickets; // 0x160
    TArray<TEntityRef<IItem>> m_aOwnedItems; // 0x178
    PAD(0x4);
    EStashpointContainedEntityType m_eContainedEntityType; // 0x194
    ZRepositoryID m_mainEntityRepositoryId; // 0x198
};