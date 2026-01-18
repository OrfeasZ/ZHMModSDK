#pragma once

#include "Globals.h"
#include "ZPrimitives.h"
#include "ZObjectPool.h"
#include "ZResourceID.h"
#include "TArray.h"
#include "THashMap.h"
#include "TSharedPointer.h"
#include "ZSharedPointerTarget.h"
#include "THashSet.h"

class ZRuntimeResourceID;

class ZResourceIndex {
public:
    ZResourceIndex() : val(-1) {}
    ZResourceIndex(int val) : val(val) {}

    int val;
};

enum EResourceStatus {
    RESOURCE_STATUS_UNKNOWN    = 0,
    RESOURCE_STATUS_LOADING    = 1,
    RESOURCE_STATUS_INSTALLING = 2,
    RESOURCE_STATUS_FAILED     = 3,
    RESOURCE_STATUS_VALID      = 4,
};

template <typename T, size_t N>
class TNonReallocatingArray {
public:
    T& operator[](size_t p_Index) {
        return m_Buffer.m_pData[p_Index];
    }

    const T& operator[](size_t p_Index) const {
        return m_Buffer.m_pData[p_Index];
    }

    T* begin() {
        return m_Buffer.m_pData;
    }

    T* end() {
        return m_Buffer.m_pData + m_nSize;
    }

    const T* begin() const {
        return m_Buffer.m_pData;
    }

    const T* end() const {
        return m_Buffer.m_pData + m_nSize;
    }

    size_t size() const {
        return m_nSize;
    }

    size_t capacity() const {
        return m_nCapacity;
    }

private:
    uint32_t m_nSize;
    uint32_t m_nCapacity;
    ZInfiniteBuffer<T> m_Buffer;
};

union SResourceReferenceFlags {
    struct {
        uint8_t languageCode : 5;
        uint8_t acquired : 1;
        int8_t referenceType : 2;
    };

    uint8_t flags;
};

class ZResourceContainer {
public:
    struct SResourceInfo {
        ZRuntimeResourceID rid;
        void* resourceData;
        unsigned long long dataOffset;
        uint32 dataSize;
        uint32 compressedDataSize;
        EResourceStatus status;
        long refCount;
        ZResourceIndex nextNewestIndex;
        int firstReferenceIndex;
        int numReferences;
        unsigned int resourceType;
        int32_t monitorId;
        short priority;
        int8 packageId;
    };

    /**
     * Packed reference information.
     * - 8 bits for the flags
     * - 1 bit for some flag that inverts the index (multiplies it by -1), not sure why
     * - 23 bits for the index
     */
    struct SResourceReferenceInfo {
        uint32_t flags : 8;
        uint32_t unknown : 1;
        uint32_t index : 23;
    };

public:
    TNonReallocatingArray<SResourceInfo, 4194304> m_resources;
    TArray<SResourceReferenceInfo> m_references;
    THashMap<ZRuntimeResourceID, ZResourceIndex, TDefaultHashMapPolicy<ZRuntimeResourceID>> m_indices;
    TArray<ZString> m_MountedPackages;
    TArray<uint32_t> m_firstResourceIndexPerPackage;
    TArray<uint32_t> m_firstReferenceIndexPerPackage;
    TArray<uint8_t> m_firstPackageIndexPerMountedPartition;
    uint8_t m_firstDynamicPackageIndex;
    uint8_t m_firstBaseLanguagePackageIndex;
    THashSet<ZRuntimeResourceID, TDefaultHashSetPolicy<ZRuntimeResourceID>> m_dynamicResources;
};

static_assert(sizeof(ZResourceContainer::SResourceInfo) == 64);

class ZResourcePtr {
public:
    ZResourcePtr() {
        m_nResourceIndex.val = -1;
    }

    ZResourcePtr(const ZResourcePtr& p_Other) {
        m_nResourceIndex = p_Other.m_nResourceIndex;

        if (m_nResourceIndex.val != -1) {
            auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex.val];

            InterlockedIncrement(&s_ResourceInfo.refCount);
        }
    }

    ZResourcePtr(ZResourceIndex p_ResourceIndex) {
        m_nResourceIndex = p_ResourceIndex;

        if (m_nResourceIndex.val != -1) {
            auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex.val];

            InterlockedIncrement(&s_ResourceInfo.refCount);
        }
    }

    ZHMSDK_API ~ZResourcePtr();

public:
    ZResourceContainer::SResourceInfo& GetResourceInfo() const {
        auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex.val];

        return s_ResourceInfo;
    }

    void* GetResourceData() const {
        if (m_nResourceIndex.val < 0)
            return nullptr;

        auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex.val];

        return s_ResourceInfo.resourceData;
    }

    operator bool() const {
        return GetResourceData() != nullptr;
    }

public:
    ZResourceIndex m_nResourceIndex;
    uint32_t m_Padding = 0;
};

static_assert(sizeof(ZResourcePtr) == 8);

template <typename T>
class TResourcePtr : public ZResourcePtr {
public:
    TResourcePtr() = default;

    explicit TResourcePtr(const ZResourceIndex p_Index) {
        m_nResourceIndex = p_Index;
    }

public:
    T* GetResource() const {
        return static_cast<T*>(GetResourceData());
    }

    operator T*() const {
        return GetResource();
    }
};

class ZResourceManager : public IComponentInterface {
public:
    virtual ~ZResourceManager() {}
    virtual void ZResourceManager_unk5() = 0;
    virtual void ZResourceManager_unk6() = 0;
    virtual void ZResourceManager_unk7() = 0;
    virtual ZResourcePtr* GetResourcePtr(ZResourcePtr& result, const ZRuntimeResourceID& ridResource, int nPriority) = 0;
    virtual ZResourcePtr* LoadResource(ZResourcePtr& result, const ZRuntimeResourceID& ridResource) = 0;
    virtual void ZResourceManager_unk10() = 0;
    virtual void ZResourceManager_unk11() = 0;
    virtual void ZResourceManager_unk12() = 0;
    virtual void ZResourceManager_unk13() = 0;
    virtual void ZResourceManager_unk14() = 0;
    virtual void ZResourceManager_unk15() = 0;
    virtual void ZResourceManager_unk16() = 0;
    virtual void ZResourceManager_unk17() = 0;
    virtual void ZResourceManager_unk18() = 0;
    virtual void ZResourceManager_unk19() = 0;
    virtual void ZResourceManager_unk20() = 0;
    virtual void ZResourceManager_unk21() = 0;
    virtual void ZResourceManager_unk22() = 0;
    virtual void ZResourceManager_unk23() = 0;
    virtual void Update(bool bSendStatusChangedNotifications) = 0;
    virtual void ZResourceManager_unk25() = 0;
    virtual void ZResourceManager_unk26() = 0;
    virtual void ZResourceManager_unk27() = 0;
    virtual void ZResourceManager_unk28() = 0;
    virtual void ZResourceManager_unk29() = 0;
    virtual void ZResourceManager_unk30() = 0;
    virtual ZMutex& GetMutex() const = 0;
    virtual void ZResourceManager_unk32() = 0;
    virtual void ZResourceManager_unk33() = 0;
    virtual void ZResourceManager_unk34() = 0;
    virtual void ZResourceManager_unk35() = 0;
    virtual void ZResourceManager_unk36() = 0;
    virtual void ZResourceManager_unk37() = 0;
    virtual void ZResourceManager_unk38() = 0;
    virtual void ZResourceManager_unk39() = 0;
    virtual void ZResourceManager_unk40() = 0;
    virtual void ZResourceManager_unk41() = 0;
    virtual void ZResourceManager_unk42() = 0;
    virtual void ZResourceManager_unk43() = 0;
    virtual void ZResourceManager_unk44() = 0;
    virtual void ZResourceManager_unk45() = 0;
    virtual void ZResourceManager_unk46() = 0;
    virtual bool DoneLoading() = 0;
    virtual void ZResourceManager_unk48() = 0;
    virtual void ZResourceManager_unk49() = 0;
    virtual void ZResourceManager_unk50() = 0;
    virtual void ZResourceManager_unk51() = 0;
    virtual void ZResourceManager_unk52() = 0;
    virtual void ZResourceManager_unk53() = 0;
    virtual void ZResourceManager_unk54() = 0;
    virtual void ZResourceManager_unk55() = 0;
    virtual void ZResourceManager_unk56() = 0;

public:
    PAD(420);
    volatile LONG m_nNumProcessing; // 428 (0x1AC)
    PAD(0x128);
    THashSet<ZResourceIndex, TDefaultHashSetPolicy<ZResourceIndex>> m_pendingUninstalls; // 0x2D8
};

class ZResourceDataBuffer : public ZSharedPointerTarget {
public:
    void* m_pData = nullptr;
    uint32 m_nSize = 0;
    uint32 m_nCapacity = 0;
    bool m_bOwnsDataPtr = false;
};

typedef TSharedPointer<ZResourceDataBuffer> ZResourceDataPtr;

class ZResourceReader : public ZSharedPointerTarget {
public:
    ZResourceIndex m_ResourceIndex;
    ZResourceDataPtr m_pResourceData;
    uint32 m_nResourceDataSize = 0;
    TArray<ZResourceIndex> m_ReferenceIndices;
    TArray<SResourceReferenceFlags> m_ReferenceFlags;
};

static_assert(sizeof(ZResourceReader) == 88);

typedef TSharedPointer<ZResourceReader> ZResourceReaderPtr;

class ZResourcePending {
public:
    ZResourcePtr m_pResource;
    ZResourceReaderPtr m_pResourceReader;
};

class IPackageManager : public IComponentInterface {
public:
    enum EPartitionType {
        Standard,
        Addon,
    };

    struct SPartitionInfo {
        uint32_t m_nIndex; // 0
        ZString m_sPartitionID; // 8
        EPartitionType m_eType; // 24
        uint32_t m_patchLevel; // 28
        uint64_t a32; // 32
        uint64_t a40; // 40
        ZString m_sMountPath; // 48
        uint64_t a64; // 64
        bool a72; // 72
        SPartitionInfo* m_pParent; // 80
        TArray<SPartitionInfo*> m_aAddons; // 88
    };

    virtual ~IPackageManager() {}

public:
    PAD(24);
};

static_assert(sizeof(IPackageManager::SPartitionInfo) == 112);

class ZPackageManagerBase : public IPackageManager {
public:
    virtual ~ZPackageManagerBase() {}
    virtual void ZPackageManagerBase_unk5() = 0;
    virtual void ZPackageManagerBase_unk6() = 0;
    virtual void MountPartitionsForRoots(const TArray<ZResourceID>& roots) = 0;
    virtual void ZPackageManagerBase_unk8() = 0;
    virtual void ZPackageManagerBase_unk9() = 0;
    virtual void ZPackageManagerBase_unk10() = 0;
    virtual void ZPackageManagerBase_unk11() = 0;
    virtual void UnmountPartitions(uint8_t p_FirstPackageIndex) = 0;
    virtual void ZPackageManagerBase_unk13() = 0;
    virtual void ZPackageManagerBase_unk14() = 0;
    virtual void ZPackageManagerBase_unk15() = 0;
    virtual void ZPackageManagerBase_unk16() = 0;
    virtual void ZPackageManagerBase_unk17() = 0;
    virtual void ZPackageManagerBase_unk18() = 0;
    virtual void ZPackageManagerBase_unk19() = 0;
    virtual void ZPackageManagerBase_unk20() = 0;
    virtual void ZPackageManagerBase_unk21() = 0;
    virtual void ZPackageManagerBase_unk22() = 0;
    virtual void ZPackageManagerBase_unk23() = 0;
    virtual void ZPackageManagerBase_unk24() = 0;
    virtual void ZPackageManagerBase_unk25() = 0;
    virtual void ZPackageManagerBase_unk26() = 0;
    virtual void ZPackageManagerBase_unk27() = 0;
    virtual void ZPackageManagerBase_unk28() = 0;
    virtual void ZPackageManagerBase_unk29() = 0;
    virtual void ZPackageManagerBase_unk30() = 0;
    virtual void MountResourcePackagesInPartition(SPartitionInfo* info, SPartitionInfo* languagePartition) = 0;
    virtual void ZPackageManagerBase_unk32() = 0;
    virtual void ZPackageManagerBase_unk33() = 0;

public:
    PAD(0x48);
    TArray<SPartitionInfo*> m_aPartitionInfos; // 0x68
    PAD(0x490);
    THashMap<ZResourceID, SPartitionInfo*> m_sceneToPartitionMap; // 0x510
};

static_assert(offsetof(ZPackageManagerBase, m_aPartitionInfos) == 0x68);
static_assert(offsetof(ZPackageManagerBase, m_sceneToPartitionMap) == 0x510);