#pragma once

#include "Reflection.h"
#include "ZTypeRegistry.h"
#include "ZMemory.h"
#include "Globals.h"

#include <cassert>

class STypeID;
class ZString;

class ZObjectRef
{
public:
    ZObjectRef()
    {
        auto it = (*Globals::TypeRegistry)->m_types.find("void");

        assert(it != (*Globals::TypeRegistry)->m_types.end());

        if (it == (*Globals::TypeRegistry)->m_types.end())
            return;

        m_pTypeID = it->second;
    }
    
    ZObjectRef(STypeID* p_TypeId, void* p_Data) : m_pTypeID(p_TypeId), m_pData(p_Data) {}
    
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

    void Assign(STypeID* p_Type, void* p_Data)
    {
        m_pTypeID = p_Type;
        m_pData = p_Data;
    }

    bool IsEmpty() const
    {
        return m_pTypeID == nullptr || m_pData == nullptr || Hash::Crc32(m_pTypeID->typeInfo()->m_pTypeName) == ZHMTypeId<void>;
    }

public:
    STypeID* m_pTypeID = nullptr;
    void* m_pData = nullptr;
};

template <class T>
class ZVariant :
    public ZObjectRef
{
public:
    ZVariant(ZObjectRef objectRef) : ZObjectRef(objectRef)
    {

    }

    ZVariant() :
        ZObjectRef(GetTypeId(), nullptr)
    {
        m_pData = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
            m_pTypeID->typeInfo()->m_nTypeSize,
            m_pTypeID->typeInfo()->m_nTypeAlignment
        );

        m_pTypeID->typeInfo()->m_pTypeFunctions->construct(m_pData);
    }

    ZVariant(const T& p_Value) :
        ZObjectRef(GetTypeId(), nullptr)
    {
        m_pData = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
            m_pTypeID->typeInfo()->m_nTypeSize,
            m_pTypeID->typeInfo()->m_nTypeAlignment
        );

        m_pTypeID->typeInfo()->m_pTypeFunctions->copyConstruct(m_pData, &p_Value);
    }

    ZVariant(const ZVariant<T>& p_Other) :
        ZObjectRef(p_Other.m_pTypeID, nullptr)
    {
        m_pData = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
            m_pTypeID->typeInfo()->m_nTypeSize,
            m_pTypeID->typeInfo()->m_nTypeAlignment
        );

        m_pTypeID->typeInfo()->m_pTypeFunctions->copyConstruct(m_pData, p_Other.m_pData);
    }

    ~ZVariant()
    {
        m_pTypeID->typeInfo()->m_pTypeFunctions->destruct(m_pData);
        (*Globals::MemoryManager)->m_pNormalAllocator->Free(m_pData);
    }

    T& Get()
    {
        return *static_cast<T*>(m_pData);
    }

    ZVariant<T>& operator=(const T& p_Value)
    {
        m_pTypeID->typeInfo()->m_pTypeFunctions->destruct(m_pData);
        m_pTypeID->typeInfo()->m_pTypeFunctions->copyConstruct(m_pData, &p_Value);
        return *this;
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

template <class T>
class ZVariantRef :
    public ZObjectRef
{
public:
    ZVariantRef(T* p_Value) :
        ZObjectRef(GetTypeId(), p_Value)
    {
    }

    T* Get()
    {
        return static_cast<T*>(m_pData);
    }

    ZVariantRef<T>& operator=(T* p_Value)
    {
        m_pData = p_Value;
        return *this;
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

class ZDynamicObject :
    public ZObjectRef
{
};

class SDynamicObjectKeyValuePair
{
public:
    ZString sKey; // 0x0
    ZDynamicObject value; // 0x10
};
