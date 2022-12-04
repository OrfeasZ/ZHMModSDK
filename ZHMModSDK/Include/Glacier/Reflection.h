#pragma once

#include "ZPrimitives.h"
#include "CompileReflection.h"
#include "TArray.h"

class STypeID;
class ZString;
class IType;
class ZObjectRef;

class IComponentInterface
{
public:
	virtual ~IComponentInterface() = 0;
	virtual ZObjectRef* getAsObjectRef(ZObjectRef* result) = 0;
	virtual int addRef() = 0;
	virtual int release() = 0;
	virtual void* getSubclassStart(STypeID* type) = 0;
};

enum ETypeInfoFlags : uint16_t
{
	TIF_Entity = 0x01,
	TIF_Resource = 0x02,
	TIF_Class = 0x04,
	TIF_Enum = 0x08,
	TIF_Container = 0x10,
	TIF_Array = 0x20,
	TIF_FixedArray = 0x40,
	TIF_Map = 0x200,
	TIF_Primitive = 0x400
};

class STypeFunctions
{
public:
	void (*construct)(void*);
	void (*copyConstruct)(void*, const void*);
	void (*destruct)(void*);
	void (*assign)(void*, void*);
	bool (*equal)(void*, void*);
	bool (*smaller)(void*, void*);
	void (*minus)(void*, void*, void*);
	void (*plus)(void*, void*, void*);
	void (*mult)(void*, void*, void*);
	void (*div)(void*, void*, void*);
};

class IType
{
public:
	inline bool isEntity() const
	{
		return m_nTypeInfoFlags & TIF_Entity;
	}

	inline bool isResource() const
	{
		return m_nTypeInfoFlags & TIF_Resource;
	}

	inline bool isClass() const
	{
		return m_nTypeInfoFlags & TIF_Class;
	}

	inline bool isEnum() const
	{
		return m_nTypeInfoFlags & TIF_Enum;
	}

	inline bool isContainer() const
	{
		return m_nTypeInfoFlags & TIF_Container;
	}

	inline bool isArray() const
	{
		return m_nTypeInfoFlags & TIF_Array;
	}

	inline bool isFixedArray() const
	{
		return m_nTypeInfoFlags & TIF_FixedArray;
	}

	inline bool isMap() const
	{
		return m_nTypeInfoFlags & TIF_Map;
	}

	inline bool isPrimitive() const
	{
		return m_nTypeInfoFlags & TIF_Primitive;
	}

public:
	STypeFunctions* m_pTypeFunctions;
	uint16_t m_nTypeSize;
	uint16_t m_nTypeAlignment;
	uint16_t m_nTypeInfoFlags;
	char* m_pTypeName;
	STypeID* m_pTypeID;
	bool (*fromString)(void*, IType*, const ZString&);
	uint32_t(*toString)(void*, IType*, char*, uint32_t, const ZString&);
};

class SInputPinEntry
{
public:
	const char* m_pName;
	unsigned int m_nPinID;
	PAD(32);
};

enum EPropertyInfoFlags
{
	E_RUNTIME_EDITABLE = 1,
	E_CONST_AFTER_START = 2,
	E_STREAMABLE = 4,
	E_MEDIA_STREAMABLE = 8,
	E_HAS_GETTER_SETTER = 16,
	E_WEAK_REFERENCE = 32
};

class ZClassProperty
{
public:
	const char* m_pName;
	uint32_t m_nPropertyID;
	STypeID* m_pType;
	uint64_t m_nOffset;
	EPropertyInfoFlags m_nFlags;
	void (*set)(void*, void*, uint64_t, bool);
	void (*get)(void*, void*, uint64_t);
};

class ZClassConstructorInfo
{
public:
	uint64_t m_nArgumentCount;
	void (*m_fUnk0x8)();
	STypeID* m_pReturnType;
	STypeID* m_pArgType;
};

class ZClassConstructor
{
public:
	void (*construct)(void*);
	PAD(8);
	ZClassConstructorInfo* m_pInfo;
};

class ZClassComponent
{
public:
	STypeID* m_pType;
	uint64_t m_nOffset;
};

class IClassType :
	public IType
{
public:
	uint16_t m_nPropertyCount;
	uint16_t m_nConstructorCount;
	uint16_t m_nBaseClassCount;
	uint16_t m_nInterfaceCount;
	uint16_t m_nInputCount;
	ZClassProperty* m_pProperties;
	ZClassConstructor* m_pConstructors;
	ZClassComponent* m_pBaseClasses;
	ZClassComponent* m_pInterfaces;
	SInputPinEntry* m_pInputs;
};

class ZEnumEntry
{
public:
	char* m_pName;
	int32_t m_nValue;
};

class IEnumType :
	public IType
{
public:
	TArray<ZEnumEntry> m_entries;
};

class SArrayFunctions
{
public:
	void* (*begin)(void*);
	void* (*end)(void*);
	void* (*next)(void*, void*);
	size_t(*size)(void*);
	// TODO: There's more shit here. Map it out.
};

class STypeID
{
public:
	inline IType* typeInfo() const
	{
		if (m_nFlags == 1 || (!m_pType && m_pSource))
			return m_pSource->m_pType;

		return m_pType;
	}

public:
	uint16_t m_nFlags;
	uint16_t m_nTypeNum;
	IType* m_pType;
	STypeID* m_pSource;
};

class IArrayType :
	public IType
{
public:
	inline size_t fixedArraySize() const
	{
		return m_nTypeSize / m_pArrayElementType->typeInfo()->m_nTypeSize;
	}

public:
	STypeID* m_pArrayElementType;
	SArrayFunctions* m_pArrayFunctions;
	void (*resize)(void*, size_t);
};
