#pragma once

#include "ZString.h"
#include "TArray.h"
#include "Reflection.h"

class ZEntityType;
class ZActor;
class STypeID;
class ZEntityRef;

class IEntity : 
	public IComponentInterface
{
public:
	virtual ~IEntity() {}
};

class ZPinFunctions
{
public:
	void (*trigger)();
	void (*unk00)();
	void (*unk01)();
	void (*unk02)();
	int32_t m_nPin;
};

class ZPinData
{
public:
	int32_t m_unk00;
	IType* m_pType; 
};

class ZPin
{
public:
	int64_t m_nObjectOffset;
	ZPinFunctions* m_pPinFunctions;
	ZPinData* m_pPinData;
	void* m_unk00;
	int32_t m_nPin;
};

class ZEntityPropertyType
{
public:
	ZClassProperty* getPropertyInfo() const
	{
		return reinterpret_cast<ZClassProperty*>(reinterpret_cast<uintptr_t>(this) - 16);
	}
};

class ZEntityProperty
{
public:
	ZEntityPropertyType* m_pType;
	int64_t m_nOffset;
	uint32_t m_nPropertyId;
	uint32_t m_nUnk01;
	uint64_t m_nUnk02;
	uint32_t m_nUnk03;
};

class ZEntityInterface
{
public:
	STypeID* m_pTypeId;
	int64_t m_nOffset;
};

class ZEntityType
{
public:
	uint64_t m_nUnk01;
	TArray<ZEntityProperty>* m_pProperties01;
	TArray<ZEntityProperty>* m_pProperties02;
	PAD(0x08);
	TArray<ZEntityInterface>* m_pInterfaces; // 32
	PAD(0x10);
	TArray<ZPin>* m_pInputs;
	TArray<ZPin>* m_pOutputs;
	int64_t m_nOffsetToEntity;
	int64_t m_nOffsetToBase;
	uint64_t m_nEntityId;
};

// Size = 0x18
class ZEntityImpl :
	public IEntity
{
public:
	virtual ~ZEntityImpl() {}
	virtual ZEntityRef* GetID(ZEntityRef* result) = 0;
	virtual void Activate(int) = 0;
	virtual void Deactivate(int) = 0;
	
public:
	ZEntityType* m_pType;
	uint32_t m_unk00;
	uint32_t m_unk01;
};

class ZEntityRef
{
public:
	ZEntityType** m_pEntity;

public:
	ZEntityImpl* GetBaseEntity()
	{
		if (!*m_pEntity)
			return nullptr;

		/*if (!(*m_pEntity)->m_pInterfaces)
			return nullptr;

		for (auto& s_Interface : *(*m_pEntity)->m_pInterfaces)
		{
			if (!s_Interface.m_pTypeId || !s_Interface.m_pTypeId->typeInfo())
				continue;

			// TODO: Cache the type instead.
			if (s_Interface.m_pTypeId->typeInfo()->m_pTypeName == std::string("void"))
			{
				auto s_RealPtr = reinterpret_cast<uintptr_t>(m_pEntity) + s_Interface.m_nOffset;
				return reinterpret_cast<ZEntityImpl*>(s_RealPtr);
			}
		}*/

		auto s_RealPtr = reinterpret_cast<uintptr_t>(m_pEntity) - sizeof(uintptr_t);
		return reinterpret_cast<ZEntityImpl*>(s_RealPtr);
	}
};

template <typename T>
class TEntityRef
{
public:
	ZEntityRef m_ref;
	T* m_pInterfaceRef;
};