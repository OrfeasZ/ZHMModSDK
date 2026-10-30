#pragma once

#include "Reflection.h"
#include "ZTypeRegistry.h"
#include "ZMemory.h"
#include "Globals.h"
#include "ZObjectPool.h"

#include <cassert>

struct STypeID;
class ZString;
struct SDynamicObjectKeyValuePair;

class ZObjectRef {
public:
    ZHMSDK_API static STypeID* GetVoidType();

    ZObjectRef() {
        m_pTypeID = GetVoidType();
    }

    ZObjectRef(const ZObjectRef& p_Other) : m_pTypeID(p_Other.m_pTypeID) {
        if (p_Other.m_pTypeID != GetVoidType()) {
            AllocateMemory();

            m_pTypeID->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(m_pData, p_Other.m_pData);
        }
    }

    ZObjectRef(ZObjectRef&& p_Other) noexcept : m_pTypeID(p_Other.m_pTypeID), m_pData(p_Other.m_pData) {
        p_Other.m_pTypeID = GetVoidType();
        p_Other.m_pData = nullptr;
    }

    ~ZObjectRef() {
        Clear();
    }

    template <class T>
    static ZObjectRef From(const T& p_Variant) {
        ZObjectRef s_Obj;
        s_Obj.Replace(p_Variant);

        return s_Obj;
    }

    ZObjectRef& operator=(const ZObjectRef& p_Other) {
        if (this == &p_Other) {
            return *this;
        }

        Clear();

        m_pTypeID = p_Other.m_pTypeID;
        m_pData = nullptr;

        if (p_Other.m_pTypeID != GetVoidType()) {
            AllocateMemory();

            m_pTypeID->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(m_pData, p_Other.m_pData);
        }

        return *this;
    }

    ZObjectRef& operator=(ZObjectRef&& p_Other) noexcept {
        Clear();

        m_pTypeID = p_Other.m_pTypeID;
        m_pData = p_Other.m_pData;

        p_Other.m_pTypeID = GetVoidType();
        p_Other.m_pData = nullptr;

        return *this;
    }

    template <class T>
    [[nodiscard]] bool Is() const {
        if (m_pTypeID == nullptr ||
            m_pTypeID->GetTypeInfo() == nullptr ||
            m_pTypeID->GetTypeInfo()->pszTypeName == nullptr) {
            return false;
        }

        return ZHMTypeId<T> == Hash::Crc32(m_pTypeID->GetTypeInfo()->pszTypeName);
    }

    template <class T>
    [[nodiscard]] T* As() const {
        if (!Is<T>()) {
            return nullptr;
        }

        return static_cast<T*>(m_pData);
    }

    void Clear() {
        DestroyMemory();

        m_pTypeID = GetVoidType();
    }

    template <class T>
    void Replace(const T& p_Value) {
        DestroyMemory();

        m_pTypeID = (*Globals::TypeRegistry)->GetTypeID(ZHMTypeName<T>);

        AllocateMemory();

        m_pTypeID->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(m_pData, &p_Value);
    }

    void Assign(STypeID* p_Type, void* p_Data) {
        DestroyMemory();

        m_pTypeID = p_Type;

        AllocateMemory();

        m_pTypeID->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(m_pData, p_Data);
    }

    template <class T>
    ZObjectRef& operator=(const T& p_Value) {
        Replace(p_Value);
        return *this;
    }

    void UNSAFE_Assign(STypeID* p_Type, void* p_Data) {
        Clear();

        m_pTypeID = p_Type;
        m_pData = p_Data;
    }

    void UNSAFE_SetType(STypeID* p_Type) {
        m_pTypeID = p_Type;
    }

    bool IsEmpty() const {
        return m_pTypeID == nullptr || m_pData == nullptr || Hash::Crc32(m_pTypeID->GetTypeInfo()->pszTypeName) ==
            ZHMTypeId<void>;
    }

    STypeID* GetTypeID() const {
        return m_pTypeID;
    }

    void* GetData() const {
        return m_pData;
    }

    void* AllocateMemory() {
        uint32_t s_Size;

        if (m_pTypeID->m_nFlags == 2) {
            s_Size = 8;
        }
        else {
            s_Size = m_pTypeID->GetTypeInfo()->m_nTypeSize;
        }

        if (s_Size <= 8) {
            if (Globals::Variant8BytePool && Globals::Variant8BytePool->m_pData) {
                m_pData = Globals::Variant8BytePool->Alloc();
            }
        }
        else if (s_Size <= 0x20) {
            if (Globals::Variant32BytePool && Globals::Variant32BytePool->m_pData) {
                m_pData = Globals::Variant32BytePool->Alloc();
            }
        }

        if (!m_pData) {
            const uint32_t s_Alignment = m_pTypeID->m_nFlags == 2
                ? 8
                : m_pTypeID->GetTypeInfo()->m_nTypeAlignment;

            m_pData = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
                s_Size,
                s_Alignment
            );
        }

        return m_pData;
    }

    void DestroyMemory() {
        if (!m_pData) {
            return;
        }

        // char[INT32_MAX] has type number 21.
        if (m_pTypeID->m_nTypeNum != 21) {
            if (m_pTypeID->m_nFlags != 2) {
                m_pTypeID->GetTypeInfo()->m_pTypeFunctions->destruct(m_pData);
            }

            if (Globals::Variant8BytePool && Globals::Variant8BytePool->Contains(m_pData)) {
                Globals::Variant8BytePool->Free(m_pData);
            }
            else if (Globals::Variant32BytePool && Globals::Variant32BytePool->Contains(m_pData)) {
                Globals::Variant32BytePool->Free(m_pData);
            }
            else {
                (*Globals::MemoryManager)->m_pNormalAllocator->Free(m_pData);
            }
        }

        m_pData = nullptr;
    }

protected:
    STypeID* m_pTypeID = nullptr;
    void* m_pData = nullptr;
};

template <class T>
class ZVariant :
    public ZObjectRef {
public:
    ZVariant() {
        Replace(T());
    }

    ZVariant(const T& p_Value) {
        Replace(p_Value);
    }

    ZVariant(const ZVariant<T>& p_Other) {
        Replace(p_Other);
    }

    ZVariant(ZVariant<T>&& p_Other) noexcept {
        m_pTypeID = p_Other.m_pTypeID;
        m_pData = p_Other.m_pData;

        p_Other.m_pTypeID = GetVoidType();
        p_Other.m_pData = nullptr;
    }

    T& Get() {
        return *As<T>();
    }

    ZVariant<T>& operator=(const T& p_Value) {
        Replace(p_Value);
        return *this;
    }

    ZVariant<T>& operator=(ZVariant<T>&& p_Other) noexcept {
        Clear();

        m_pTypeID = p_Other.m_pTypeID;
        m_pData = p_Other.m_pData;

        p_Other.m_pTypeID = GetVoidType();
        p_Other.m_pData = nullptr;

        return *this;
    }
};

template <class T>
class ZVariantRef :
    public ZObjectRef {
public:
    ZVariantRef(T* p_Value) {
        m_pTypeID = (*Globals::TypeRegistry)->GetTypeID(ZHMTypeName<T>);
        m_pData = p_Value;
    }

    T* Get() {
        return static_cast<T*>(m_pData);
    }

    ZVariantRef<T>& operator=(T* p_Value) {
        Clear();

        m_pData = p_Value;
        return *this;
    }
};

class ZDynamicObject :
    public ZObjectRef {
public:
    [[nodiscard]] bool IsObject() const { return Is<TArray<SDynamicObjectKeyValuePair>>(); }
    [[nodiscard]] bool IsArray() const { return Is<TArray<ZDynamicObject>>(); }

    /// Set a value for the given key (replaces the value if it already exists).
    /// \param p_Key The key to set the value for.
    /// \param p_Value The value to set.
    /// \return True if the value was set, false if it wasn't (e.g. because the ZDynamicObject was not an object).
    bool Set(const ZString& p_Key, const ZDynamicObject& p_Value);

    /// Set a value for the given index (replaces the value if it already exists).
    /// \param p_Index The index to set the value for.
    /// \param p_Value The value to set.
    /// \return True if the value was set, false if it wasn't (e.g. because the ZDynamicObject was not an array or the index was out of bounds).
    bool Set(size_t p_Index, const ZDynamicObject& p_Value);

    /// Get a value for the given key.
    /// \param p_Key The key to get the value for.
    /// \param p_Value The returned value.
    /// \return True if the value was found, false if it wasn't.
    bool Get(const ZString& p_Key, ZDynamicObject& p_Value) const;

    /// Get a value for the given index.
    /// \param p_Index The index to get the value for.
    /// \param p_Value The returned value.
    /// \return True if the value was found, false if it wasn't.
    bool Get(size_t p_Index, ZDynamicObject& p_Value) const;

    /// Push a value to the end of the array.
    /// \param p_Value The value to push.
    /// \return True if the value was pushed, false if it wasn't (e.g. because the ZDynamicObject was not an array).
    bool PushBack(const ZDynamicObject& p_Value);

    /// Get the number of entries in this object (number of key-value pairs or number of array elements).
    /// \return The number of entries.
    [[nodiscard]] size_t Size() const;

    /// Get the keys of this object (if this ZDynamicObject is an object).
    /// \return The keys.
    [[nodiscard]] TArray<ZString> Keys() const;

    [[nodiscard]] ZDynamicObject& operator[](const ZString& p_Key);

    [[nodiscard]] const ZDynamicObject& operator[](const ZString& p_Key) const;

    [[nodiscard]] ZDynamicObject& operator[](size_t p_Index);

    [[nodiscard]] const ZDynamicObject& operator[](size_t p_Index) const;

    [[nodiscard]] ZDynamicObject* begin();

    [[nodiscard]] ZDynamicObject* end();

    void Insert(size_t p_Index, const ZDynamicObject& p_Value);

    /// Get a value for the given key and type.
    /// \param p_Key The key to get the value for.
    /// \param p_Value The returned value.
    /// \tparam T The type of the value to get.
    /// \return True if the value was found, false if it wasn't or was not of the given type.
    template <class T>
    bool Get(const ZString& p_Key, T& p_Value) const {
        ZDynamicObject s_Value;

        if (!Get(p_Key, s_Value)) {
            return false;
        }

        if (auto s_RealValue = s_Value.As<T>()) {
            p_Value = *s_RealValue;
            return true;
        }

        return false;
    }

    /// Set a value of the given type for the given key (replaces the value if it already exists).
    /// \param p_Key The key to set the value for.
    /// \param p_Value The value to set.
    /// \tparam T The type of the value to set.
    /// \return True if the value was set, false if it wasn't (e.g. because the ZDynamicObject was not an object).
    template <class T>
    bool Set(const ZString& p_Key, const T& p_Value) {
        ZVariant s_Value(p_Value);
        ZDynamicObject s_DynamicValue(std::move(s_Value));
        return Set(p_Key, s_DynamicValue);
    }

    template <class T>
    ZDynamicObject& operator=(const T& p_Value) {
        Replace(p_Value);
        return *this;
    }

    /// Create a new dynamic object that represents an associative key-value object.
    static ZDynamicObject Object() {
        ZDynamicObject s_Obj;
        s_Obj = TArray<SDynamicObjectKeyValuePair>();
        return s_Obj;
    }

    /// Create a new dynamic object that represents an array.
    static ZDynamicObject Array() {
        ZDynamicObject s_Obj;
        s_Obj = TArray<ZDynamicObject>();
        return s_Obj;
    }
};

struct SDynamicObjectKeyValuePair {
    ZString sKey; // 0x0
    ZDynamicObject value; // 0x10
};