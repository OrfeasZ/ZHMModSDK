#pragma once

#include "ZPrimitives.h"
#include "CompileReflection.h"
#include "TArray.h"

struct STypeID;
struct IType;
class ZString;
class ZObjectRef;

enum ETypeInfoFlags : uint16_t {
    TIF_Entity     = 0x01,
    TIF_Resource   = 0x02,
    TIF_Class      = 0x04,
    TIF_Enum       = 0x08,
    TIF_Container  = 0x10,
    TIF_Array      = 0x20,
    TIF_FixedArray = 0x40,
    TIF_Map        = 0x200,
    TIF_Primitive  = 0x400
};

struct STypeFunctions {
    void(*placementConstruct)(void*);
    void(*placementCopyConstruct)(void*, const void*);
    void(*destruct)(void*);
    void(*assign)(void*, const void*);
    bool(*equal)(const void*, const void*);
    bool(*smaller)(const void*, const void*);
    void(*minus)(const void*, const void*, void*);
    void(*plus)(const void*, const void*, void*);
    void(*mult)(const void*, const void*, void*);
    void(*div)(const void*, const void*, void*);
};

struct IType {
    bool IsEntity() const {
        return m_nTypeInfoFlags & TIF_Entity;
    }

    bool IsResource() const {
        return m_nTypeInfoFlags & TIF_Resource;
    }

    bool IsClass() const {
        return m_nTypeInfoFlags & TIF_Class;
    }

    bool IsEnum() const {
        return m_nTypeInfoFlags & TIF_Enum;
    }

    bool IsContainer() const {
        return m_nTypeInfoFlags & TIF_Container;
    }

    bool IsArray() const {
        return m_nTypeInfoFlags & TIF_Array;
    }

    bool IsFixedArray() const {
        return m_nTypeInfoFlags & TIF_FixedArray;
    }

    bool IsMap() const {
        return m_nTypeInfoFlags & TIF_Map;
    }

    bool IsPrimitive() const {
        return m_nTypeInfoFlags & TIF_Primitive;
    }

    const STypeFunctions* m_pTypeFunctions;
    uint16_t m_nTypeSize;
    uint8_t m_nTypeAlignment;
    uint16_t m_nTypeInfoFlags;
    const char* pszTypeName;
    STypeID* typeID;
    bool (*fromString)(void*, IType*, const ZString&);
    uint32_t(*toString)(void*, IType*, char*, uint32_t, const ZString&);
};

enum EPropertyInfoFlags {
    E_RUNTIME_EDITABLE  = 1,
    E_CONST_AFTER_START = 2,
    E_STREAMABLE        = 4,
    E_MEDIA_STREAMABLE  = 8,
    E_HAS_GETTER_SETTER = 16,
    E_WEAK_REFERENCE    = 32
};

struct SPropertyInfo {
    STypeID* m_Type;
    int64_t m_nExtraData;
    uint32_t m_Flags;
    void (*m_PropertySetCallBack)(void*, void*, uint64_t, bool);
    void (*m_PropetyGetter)(void*, void*, uint64_t);
};

struct SNamedPropertyInfo {
    const char* m_pszPropertyName;
    uint32 m_nPropertyID;
    SPropertyInfo m_propertyInfo;
};

struct SDelegateBaseInvoker {
    uint32 argCount;
    ZObjectRef(*pfInvoke)(void*, const TArrayRef<ZObjectRef>*);
    STypeID* retType;
    STypeID* a0Type;
};

class ZGenericMemberFunctionTarget;

class TGenericMemberFunctionPtr {
public:
    void (*__pfn)(ZGenericMemberFunctionTarget*);
    int64 __delta;
};

class ZConstructorInfo {
public:
    TGenericMemberFunctionPtr m_mfp;
    const SDelegateBaseInvoker* m_pInvokeData;
};

struct SBaseClassInfo {
    STypeID* m_Type;
    uint64_t m_nOffset;
};

class ZPinFunctor {
public:
    void (*pfInvoke)(TGenericMemberFunctionPtr, ZGenericMemberFunctionTarget*, const ZObjectRef*, uint32);
    TGenericMemberFunctionPtr func;
};

struct SPinInfo {
    ZPinFunctor m_functor;
    uint32_t m_nExtraData;
};

struct SInputPinEntry {
    uint32_t m_nPinID;
    SPinInfo m_InputPinInfo;
};

struct SComponentMapEntry {
    STypeID* type;
    uint64_t nOffset;
};

struct IClassType {
    IType type;
    uint16 m_nProperties;
    uint16 m_nConstructors;
    uint16 m_nBaseClasses;
    uint16 m_nInterfaces;
    uint16 m_nInputPins;
    const SNamedPropertyInfo* m_pProperties;
    const ZConstructorInfo* m_pConstructors;
    const SBaseClassInfo* m_pBaseClasses;
    const SComponentMapEntry* m_pInterfaces;
    const SInputPinEntry* m_pInputPins;
};

class IEnumType {
public:
    struct SEnumItem {
        const char* szName;
        int32_t nValue;
    };

    IType type;
    TArrayRef<const SEnumItem> items;
};

struct SContainerTypeVTable {
    void* (*begin)(void*);
    void* (*end)(void*);
    void* (*next)(void*, void*);
    uint32_t(*getSize)(void*);
    void (*setDeserializedElement)(void*, ZObjectRef, int32_t);
    void (*clear)(void*);
    void (*reserve)(void*, uint32_t);
};

struct IContainerType {
    IType type;
    STypeID* elementType;
    SContainerTypeVTable* pVTab;
};

struct IArrayType {
    IContainerType containerType;
    void (*setSize)(void*, uint32_t);
};

struct STypeID {
    IType* GetTypeInfo() const {
        if (m_nFlags == 1 || (!m_pType && m_pSource)) {
            return m_pSource->m_pType;
        }

        return m_pType;
    }

    uint16_t m_nFlags;
    uint16_t m_nTypeNum;
    IType* m_pType;
    STypeID* m_pSource;
};