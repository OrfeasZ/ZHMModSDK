#pragma once

#include "Reflection.h"
#include "ZTypeRegistry.h"
#include "ZMemory.h"
#include "Globals.h"

class STypeID;
class ZString;

class ZObjectRef
{
protected:
	ZObjectRef(STypeID* p_TypeId, void* p_Data) : m_pTypeID(p_TypeId), m_pData(p_Data) {}

public:
	template <class T>
	bool Is() const
	{
		if (m_pTypeID == nullptr ||
			m_pTypeID->typeInfo() == nullptr ||
			m_pTypeID->typeInfo()->m_pTypeName == nullptr)
			return false;

		return ZHMTypeId<T> == Hash::Crc32(m_pTypeID->typeInfo()->m_pTypeName);
	}

	template <class T>
	T* As() const
	{
		if (!Is<T>())
			return nullptr;

		return static_cast<T*>(m_pData);
	}
	
protected:
	STypeID* m_pTypeID = nullptr;
	void* m_pData = nullptr;

};

template <class T>
class ZVariant :
	public ZObjectRef
{
public:
	ZVariant(const T& p_Value) :
		ZObjectRef(GetTypeId(), nullptr)
	{
		m_pData = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
			m_pTypeID->typeInfo()->m_nTypeSize, 
			m_pTypeID->typeInfo()->m_nTypeAlignment
		);

		m_pTypeID->typeInfo()->m_pTypeFunctions->copyConstruct(m_pData, &p_Value);
	}

	~ZVariant()
	{
		m_pTypeID->typeInfo()->m_pTypeFunctions->destruct(m_pData);
		(*Globals::MemoryManager)->m_pNormalAllocator->Free(m_pData);
	}
	
	T& Get()
	{
		return *As<T>();
	}

	void operator=(const T& p_Value)
	{
		m_pTypeID->typeInfo()->m_pTypeFunctions->destruct(m_pData);
		m_pTypeID->typeInfo()->m_pTypeFunctions->copyConstruct(m_pData, &p_Value);
	}

private:
	STypeID* GetTypeId()
	{
		auto it = (*Globals::TypeRegistry)->m_types.find(ZHMTypeName<T>);

		assert(it != (*Globals::TypeRegistry)->m_types.end());
		
		if (it == (*Globals::TypeRegistry)->m_types.end())
			return nullptr;

		return it->second;
	}
};
