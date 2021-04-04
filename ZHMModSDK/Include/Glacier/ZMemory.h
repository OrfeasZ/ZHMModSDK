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

class ZMemoryManager
{
public:
	char pad[0x10];
	IAllocator* m_pNormalAllocator;
};
