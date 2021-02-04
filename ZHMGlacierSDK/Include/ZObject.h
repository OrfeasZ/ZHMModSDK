#pragma once

#include "ZPrimitives.h"

class STypeID;
class ZString;

class ZObjectRef
{
public:
	STypeID* m_pTypeID;
	void* m_pData;
};

class ZVariant :
	public ZObjectRef
{
};

class ZVariantRef :
	public ZObjectRef
{
};

class ZDynamicObject :
	public ZVariant
{
public:
	typedef void (__fastcall* ToString_t)(ZDynamicObject* th, ZString* a2);
	static ToString_t ToString;
};