#pragma once

#include "Globals.h"
#include "Functions.h"
#include "ZPrimitives.h"

class ZRuntimeResourceID;

class ZResourceContainer : IComponentInterface
{
public:
	// sizeof = 64
	struct SResourceInfo
	{
		PAD(0x08);
		void* resourceData; // 0x08
		PAD(0x14); // 0x10
		long refCount; // 0x24
		PAD(0x18);
	};

public:
	TArray<SResourceInfo> m_resources;
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
	PAD(0x04);
};

template<typename T>
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
	virtual void unk05() = 0;
	virtual void unk06() = 0;
	virtual void unk07() = 0;
	virtual void GetResourcePtr(ZResourcePtr& result, const ZRuntimeResourceID& ridResource, int nPriority) = 0;
	virtual void unk09() = 0;
	virtual void unk10() = 0;
	virtual void unk11() = 0;
	virtual void unk12() = 0;
	virtual void unk13() = 0;
	virtual void unk14() = 0;
	virtual void unk15() = 0;
	virtual void unk16() = 0;
	virtual void unk17() = 0;
	virtual void unk18() = 0;
	virtual void unk19() = 0;
	virtual void unk20() = 0;
	virtual void unk21() = 0;
	virtual void unk22() = 0;
	virtual void unk23() = 0;
	virtual void unk24() = 0;
	virtual void unk25() = 0;
	virtual void unk26() = 0;
	virtual void unk27() = 0;
	virtual void unk28() = 0;
	virtual void unk29() = 0;
	virtual void unk30() = 0;
	virtual void unk31() = 0;
	virtual void unk32() = 0;
	virtual void unk33() = 0;
	virtual void unk34() = 0;
	virtual void unk35() = 0;
	virtual void unk36() = 0;
	virtual void unk37() = 0;
	virtual void unk38() = 0;
	virtual void unk39() = 0;
	virtual void unk40() = 0;
	virtual void unk41() = 0;
	virtual void unk42() = 0;
	virtual void unk43() = 0;
	virtual void unk44() = 0;
	virtual void unk45() = 0;
	virtual void unk46() = 0;
	virtual void unk47() = 0;
	virtual void unk48() = 0;
	virtual void unk49() = 0;
	virtual void unk50() = 0;
	virtual void unk51() = 0;
	virtual void unk52() = 0;
	virtual void unk53() = 0;
	virtual void unk54() = 0;
	virtual void unk55() = 0;
	virtual void unk56() = 0;
};
