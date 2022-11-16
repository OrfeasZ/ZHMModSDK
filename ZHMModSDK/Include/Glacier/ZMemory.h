#pragma once

#include "Reflection.h"

class IAllocator : public IComponentInterface
{
public:
	virtual ~IAllocator() {}
	virtual size_t DefaultAlignment() = 0;
	virtual bool SupportsAlignment() = 0;
	virtual void* Allocate(size_t p_Size) = 0;
	virtual void* AllocateAligned(size_t p_Size, size_t p_Alignment) = 0;
	virtual void Free(void* p_Memory) = 0;
	virtual int64_t GetAllocationSize(void* p_Memory) = 0;
};

class IPageAllocator
{
public:
	virtual ~IPageAllocator() = default;
	virtual void IPageAllocator_unk01() = 0;
	virtual void IPageAllocator_unk02() = 0;
	virtual void IPageAllocator_unk03() = 0;
	virtual void IPageAllocator_unk04() = 0;
	virtual void IPageAllocator_unk05() = 0;
	virtual void IPageAllocator_unk06() = 0;
	virtual void IPageAllocator_unk07() = 0;
	virtual IAllocator* GetAllocator(void* object) = 0;
};

class ZMemoryManager
{
public:
	IPageAllocator* m_pPageAllocator;
	PAD(0x08);
	IAllocator* m_pNormalAllocator;
};
