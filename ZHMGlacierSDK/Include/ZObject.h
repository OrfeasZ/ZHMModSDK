#pragma once

#include "Reflection.h"

class STypeID;
class ZString;

class ZObjectRef
{
public:
	STypeID* m_pTypeID = nullptr;
	void* m_pData = nullptr;
};

class ZVariant :
	public ZObjectRef
{
public:
	template <class T>
	bool Is() const
	{
		if (m_pTypeID == nullptr ||
			m_pTypeID->m_pType == nullptr ||
			m_pTypeID->m_pType->m_pTypeName == nullptr)
			return false;

		return ZHMTypeId<T> == Crc32::Calculate(m_pTypeID->m_pType->m_pTypeName);
	}
};

class ZVariantRef :
	public ZObjectRef
{
};

class ZDynamicObject :
	public ZVariant
{
};