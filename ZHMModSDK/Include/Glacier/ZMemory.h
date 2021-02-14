#pragma once

#include "Reflection.h"
#include "ZPrimitives.h"

class IAllocator : public IComponentInterface
{
public:
	virtual ~IAllocator() {}
	virtual size_t DefaultAlignment() = 0;
	virtual bool SupportsAlignment() = 0;
	virtual void* Allocate(size_t p_Size) = 0;
	virtual void* AllocateAligned(size_t p_Size, size_t p_Alignment) = 0;
	virtual void Free(void* p_Memory) = 0;
};

class ZMemoryManager
{
public:
	PAD(0x10);
	IAllocator* m_pNormalAllocator;
};
