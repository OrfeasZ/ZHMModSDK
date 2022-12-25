#pragma once

#include "Globals.h"
#include "Functions.h"
#include "ZPrimitives.h"

class ZRuntimeResourceID;

class ZResourceIndex
{
public:
	ZResourceIndex(int val) : val(val)
	{

	}

	int val;
};

enum EResourceStatus
{
	RESOURCE_STATUS_UNKNOWN = 0,
	RESOURCE_STATUS_LOADING = 1,
	RESOURCE_STATUS_INSTALLING = 2,
	RESOURCE_STATUS_FAILED = 3,
	RESOURCE_STATUS_VALID = 4,
};

class ZResourceContainer
{
public:
	struct SResourceInfo
	{
		ZRuntimeResourceID rid;
		void* resourceData;
		unsigned long long dataOffset;
		unsigned int finalDataSize;
		unsigned int dataSize;
		EResourceStatus status;
		long refCount;
		ZResourceIndex nextNewestIndex;
		unsigned int firstReferenceIndex;
		unsigned int numReferences;
		unsigned int resourceType;
		short priority;
		int monitorId;
	};

public:
	unsigned int m_resourcesSize;
	unsigned int m_Unknown;
	TArray<SResourceInfo> m_resources;
	TArray<unsigned int> m_references;
	THashMap<ZRuntimeResourceID, ZResourceIndex, TDefaultHashMapPolicy<ZRuntimeResourceID>> m_indices;
	TArray<ZString> m_MountedPackages;
};

class ZResourcePtr
{
public:
	~ZResourcePtr()
	{
		if (m_nResourceIndex < 0)
			return;

		auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex];

		if (InterlockedDecrement(&s_ResourceInfo.refCount) == 0 && s_ResourceInfo.resourceData)
			Functions::ZResourceManager_UninstallResource->Call(Globals::ResourceManager, m_nResourceIndex);
	}

public:
	ZResourceContainer::SResourceInfo& GetResourceInfo() const
	{
		auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex];

		return s_ResourceInfo;
	}

	void* GetResourceData() const
	{
		if (m_nResourceIndex < 0)
			return nullptr;

		auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex];
		
		return s_ResourceInfo.resourceData;
	}

	operator bool() const
	{
		return GetResourceData() != nullptr;
	}
	
public:
	int32_t m_nResourceIndex = -1;
	uint32_t m_Padding;
};

template <typename T>
class TResourcePtr : public ZResourcePtr
{
public:
	TResourcePtr()
	{
		static_assert(std::is_base_of_v<IComponentInterface, T>, "TResourcePtr type must implement IComponentInterface.");
	}
	
public:
	T* GetResource() const
	{
		return static_cast<T*>(GetResourceData());
	}

	operator T*() const
	{
		return GetResource();
	}
};

class ZResourceManager : public IComponentInterface
{
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
