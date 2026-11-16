#pragma once

#include "Reflection.h"
#include "ZItem.h"
#include "ISavable.h"
#include "ZBitArray.h"

class ZAABBTreeNode;
class ZFirearmConfigDescriptor;
class ZOutfitConfigDescriptor;

struct SRegisteredFactory {
    uint32_t m_nHashCode; // 0x0
    uint64_t m_pFactory; // 0x8
};

class ZConfigDatabase {
public:
    THashMap<uint32_t, ZItemConfigDescriptor*, TDefaultHashMapPolicy<uint32_t>> m_aItemConfigDescriptors; // 0x0
    THashMap<uint32_t, ZFirearmConfigDescriptor*, TDefaultHashMapPolicy<uint32_t>> m_aFirearmConfigDescriptors; // 0x20
    THashMap<uint32_t, ZOutfitConfigDescriptor*, TDefaultHashMapPolicy<uint32_t>> m_aOutfitConfigDescriptors; // 0x40
};

class ZWorldInventory : public IComponentInterface, public ISavable {
public:
    class ZRegisteredEntityData {
    public:
        ZEntityRef m_rEntity; // 0x0
        ZBitArray m_flags; // 0x8
        void* m_pTreeNode; // 0x28
        float4 m_vNewMin; // 0x30
        float4 m_vNewMax; // 0x40
        bool m_bBoundsChanged; // 0x50
        TEntityRef<ZSpatialEntity> m_rSpatial; // 0x58
        float32 m_fHalfSize; // 0x68
    };

    PAD(0x40); // 0x10
    bool m_bResourceCleanupEnabled; // 0x50
    TArray<SRegisteredFactory> m_aFactories; // 0x58
    TArray<TEntityRef<IItemBase>> m_aItems; // 0x70
    TArray<uint32_t> m_aLoadingItemsTickets; // 0x88
    ZConfigDatabase m_ConfigDatabase; // 0xA0
    TArray<TEntityRef<IItem>> m_aNonOwnedItems; // 0x100
    ZAABBTreeNode* m_pSpatialRegistry; // 0x118
    TNonReallocatingArray<ZRegisteredEntityData, 2000> m_aRegisteredEntities; // 0x120
    bool m_bHasRequests; // 0x140
};

static_assert(offsetof(ZWorldInventory, m_bResourceCleanupEnabled) == 0x50);
static_assert(offsetof(ZWorldInventory, m_aFactories) == 0x58);
static_assert(offsetof(ZWorldInventory, m_aItems) == 0x70);
static_assert(offsetof(ZWorldInventory, m_aNonOwnedItems) == 0x100);