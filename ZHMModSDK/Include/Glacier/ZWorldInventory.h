#pragma once

#include "Reflection.h"
#include "ZItem.h"
#include "ISavable.h"

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

    bool m_bResourceCleanupEnabled; // 0x10
    TArray<SRegisteredFactory> m_aFactories; // 0x18
    TArray<TEntityRef<IItemBase>> m_aItems; // 0x30
    TArray<uint32_t> m_aLoadingItemsTickets; // 0x48
    ZConfigDatabase m_ConfigDatabase; // 0x60
    TArray<TEntityRef<IItem>> m_aNonOwnedItems; // 0xC0
    ZAABBTreeNode* m_pSpatialRegistry; // 0xD8
    TNonReallocatingArray<ZRegisteredEntityData, 1500> m_aRegisteredEntities; // 0xE0
    bool m_bHasRequests; // 0x100
};