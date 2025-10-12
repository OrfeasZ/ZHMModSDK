#pragma once

#include "Globals.h"
#include "ZPrimitives.h"
#include "ZObjectPool.h"
#include "ZResourceID.h"
#include "TArray.h"
#include "THashMap.h"
#include "TSharedPointer.h"
#include "ZSharedPointerTarget.h"

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

struct SResourceReferenceFlags {
    uint8_t languageCode : 5;
    uint8_t acquired : 1;
    int8_t referenceType : 2;
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

    struct SResourceReferenceInfo {
        ZResourceIndex m_Index;
        SResourceReferenceFlags m_Flags;
    };

public:
    TNonReallocatingArray<SResourceInfo, 4194304> m_resources;
    TArray<SResourceReferenceInfo> m_references;
    THashMap<ZRuntimeResourceID, ZResourceIndex, TDefaultHashMapPolicy<ZRuntimeResourceID>> m_indices;
    TArray<ZString> m_MountedPackages;
};

static_assert(sizeof(ZResourceContainer::SResourceInfo) == 64);

class ZResourcePtr {
public:
    ZHMSDK_API ~ZResourcePtr();

public:
    ZResourceContainer::SResourceInfo& GetResourceInfo() const {
        auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex];

        return s_ResourceInfo;
    }

    void* GetResourceData() const {
        if (m_nResourceIndex < 0)
            return nullptr;

        auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex];

        return s_ResourceInfo.resourceData;
    }

    operator bool() const {
        return GetResourceData() != nullptr;
    }

public:
    int32_t m_nResourceIndex = -1;
    uint32_t m_Padding;
};

template <typename T>
class TResourcePtr : public ZResourcePtr {
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
    virtual void GetResourcePtr(ZResourcePtr& result, const ZRuntimeResourceID& ridResource, int nPriority) = 0;
    virtual void ZResourceManager_unk9() = 0;
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
    virtual void ZResourceManager_unk24() = 0;
    virtual void ZResourceManager_unk25() = 0;
    virtual void ZResourceManager_unk26() = 0;
    virtual void ZResourceManager_unk27() = 0;
    virtual void ZResourceManager_unk28() = 0;
    virtual void ZResourceManager_unk29() = 0;
    virtual void ZResourceManager_unk30() = 0;
    virtual void ZResourceManager_unk31() = 0;
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
    virtual void ZResourceManager_unk47() = 0;
    virtual void ZResourceManager_unk48() = 0;
    virtual void ZResourceManager_unk49() = 0;
    virtual void ZResourceManager_unk50() = 0;
    virtual void ZResourceManager_unk51() = 0;
    virtual void ZResourceManager_unk52() = 0;
    virtual void ZResourceManager_unk53() = 0;
    virtual void ZResourceManager_unk54() = 0;
    virtual void ZResourceManager_unk55() = 0;
    virtual void ZResourceManager_unk56() = 0;
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