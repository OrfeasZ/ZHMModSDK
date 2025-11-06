#pragma once

#include "ZString.h"
#include "TArray.h"
#include "Reflection.h"
#include "ZObject.h"
#include "Globals.h"

class IEntityBlueprintFactory;
class ZEntityBlueprintFactoryBase;
class ZEntityType;
class ZActor;
class STypeID;
class ZEntityRef;

class IEntityFactory : public IComponentInterface {
public:
    virtual void IEntityFactory_unk5() = 0;
    virtual void ConfigureEntity() = 0;
    virtual void IEntityFactory_unk7() = 0;
    virtual ZEntityBlueprintFactoryBase* GetBlueprint() = 0;
    virtual void IEntityFactory_unk9() = 0;
    virtual void IEntityFactory_unk10() = 0;
};

class IEntityBlueprintFactory : public IComponentInterface {
public:
    virtual void GetMemoryRequirements(uint32_t&, uint32_t&, int64_t&) = 0;
    virtual ZEntityType* GetFactoryEntityType() = 0;
    virtual void IEntityBlueprintFactory_unk7() = 0;
    virtual void IEntityBlueprintFactory_unk8() = 0;
    virtual void IEntityBlueprintFactory_unk9() = 0;
    virtual void IEntityBlueprintFactory_unk10() = 0;
    virtual void IEntityBlueprintFactory_unk11() = 0;
    virtual void IEntityBlueprintFactory_unk12() = 0;
    virtual void IEntityBlueprintFactory_unk13() = 0;
    virtual void IEntityBlueprintFactory_unk14() = 0;
    virtual void IEntityBlueprintFactory_unk15() = 0;
    virtual void IEntityBlueprintFactory_unk16() = 0;
    virtual void IEntityBlueprintFactory_unk17() = 0;
    virtual void IEntityBlueprintFactory_unk18() = 0;
    virtual void IEntityBlueprintFactory_unk19() = 0;
    virtual bool IEntityBlueprintFactory_unk20() = 0;
    virtual int64_t GetSubEntitiesCount() = 0;
    virtual void IEntityBlueprintFactory_unk22() = 0;
    virtual void IEntityBlueprintFactory_unk23() = 0;
    virtual void IEntityBlueprintFactory_unk24() = 0;
    virtual ZEntityBlueprintFactoryBase* GetSubEntityBlueprint(int index) = 0;
    virtual uint64_t GetSubEntityId(int index) = 0;
    virtual int GetSubEntityIndex(unsigned long long nEntityID) const = 0;
    virtual ZEntityType** GetSubEntity(ZEntityType**, int index) = 0;
    virtual void IEntityBlueprintFactory_unk29() = 0;
    virtual void IEntityBlueprintFactory_unk30() = 0;
    virtual void IEntityBlueprintFactory_unk31() = 0;
    virtual void IEntityBlueprintFactory_unk32() = 0;

    bool IsTemplateEntityBlueprintFactory() const {
        return Globals::ZTemplateEntityBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsAspectEntityBlueprintFactory() const {
        return Globals::ZAspectEntityBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsCppEntityBlueprintFactory() const {
        return Globals::ZCppEntityBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsExtendedCppEntityBlueprintFactory() const {
        return Globals::ZExtendedCppEntityBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsUIControlBlueprintFactory() const {
        return Globals::ZUIControlBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsRenderMaterialEntityBlueprintFactory() const {
        return Globals::ZRenderMaterialEntityBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsBehaviorTreeEntityBlueprintFactory() const {
        return Globals::ZBehaviorTreeEntityBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsAudioSwitchBlueprintFactory() const {
        return Globals::ZAudioSwitchBlueprintFactory_vtbl == *(void**)this;
    }

    bool IsAudioStateBlueprintFactory() const {
        return Globals::ZAudioStateBlueprintFactory_vtbl == *(void**)this;
    }
};

class IEntity :
        public IComponentInterface {
public:
    virtual ~IEntity() {}
};

class ZPinFunctions {
public:
    void (*trigger)();
    void (*unk1)();
    void (*unk2)();
    void (*unk3)();
    int32_t m_nPin;
};

class ZPinData {
public:
    int32_t m_unk0x0;
    IType* m_pType;
};

class ZPin {
public:
    int64_t m_nObjectOffset;
    ZPinFunctions* m_pPinFunctions;
    ZPinData* m_pPinData;
    void* m_unk0x18;
    int32_t m_nPin;
};

class ZEntityPropertyType {
public:
    ZClassProperty* getPropertyInfo() const {
        return reinterpret_cast<ZClassProperty*>(reinterpret_cast<uintptr_t>(this) - 16);
    }
};

class ZEntityProperty {
public:
    ZEntityPropertyType* m_pType;
    int64_t m_nOffset;
    uint32_t m_nPropertyId;
    uint32_t m_nUnk01;
    uint64_t m_nUnk02;
    uint32_t m_nUnk03;
};

class ZEntityInterface {
public:
    STypeID* m_pTypeId;
    int64_t m_nOffset;
};

class ZEntityType {
public:
    ZEntityProperty* FindProperty(uint32_t p_PropertyId) const {
        if (!m_pProperties01) {
            return nullptr;
        }

        auto s_Property = std::find_if(
            m_pProperties01->begin(),
            m_pProperties01->end(),
            [p_PropertyId](const ZEntityProperty& p_Property) {
                return p_Property.m_nPropertyId == p_PropertyId;
            }
        );

        if (s_Property != m_pProperties01->end()) {
            return s_Property;
        }

        return nullptr;
    }

    uint32_t m_nUnkFlags;
    TArray<ZEntityProperty>* m_pProperties01;
    TArray<ZEntityProperty>* m_pProperties02;
    PAD(0x08);
    TArray<ZEntityInterface>* m_pInterfaces; // 32
    PAD(0x10);
    TArray<ZPin>* m_pInputs;
    TArray<ZPin>* m_pOutputs;
    int64_t m_nLogicalParentEntityOffset;
    int64_t m_nOwningEntityOffset;
    uint64_t m_nEntityId;
};

// Size = 0x18
class ZEntityImpl :
        public IEntity {
public:
    enum class EEntityFlags {
        ENTITYFLAG_INITIALIZED       = 1,
        ENTITYFLAG_POSTINITIALIZED   = 2,
        ENTITYFLAG_ACTIVATED         = 4,
        ENTITYFLAG_PREDELETED        = 8,
        ENTITYFLAG_EDITMODE          = 16,
        ENTITYFLAG_READONLY_FLAG_SET = 32,
        ENTITYFLAG_READONLY          = 64
    };

    virtual ~ZEntityImpl() {}
    virtual ZEntityRef* GetID(ZEntityRef& result) = 0;
    virtual void Activate(int) = 0;
    virtual void Deactivate(int) = 0;
    virtual void ZEntityImpl_unk8() = 0;
    virtual void ZEntityImpl_unk9() = 0;
    virtual void ZEntityImpl_unk10() = 0;
    virtual void ZEntityImpl_unk11() = 0;
    virtual void ZEntityImpl_unk12() = 0;
    virtual void ZEntityImpl_unk13() = 0;
    virtual void ZEntityImpl_unk14() = 0;
    virtual void Start() = 0;
    virtual void ZEntityImpl_unk16() = 0;
    virtual void ZEntityImpl_unk17() = 0;
    virtual void ZEntityImpl_unk18() = 0;
    virtual void ZEntityImpl_unk19() = 0;

    inline ZEntityType* GetType() const {
        if ((reinterpret_cast<ptrdiff_t>(m_pType) & 1) == 0)
            return m_pType;

        return *reinterpret_cast<ZEntityType**>(
            reinterpret_cast<intptr_t>(&m_pType) + (reinterpret_cast<ptrdiff_t>(m_pType) >> 1)
        );
    }

public:
    ZEntityType* m_pType;
    uint32_t m_nEntityPtrIndex;
    uint32_t m_nEntityFlags;
};

class ZEntityRef {
public:
    ZEntityType** m_pEntity = nullptr;

public:
    ZEntityRef() {}

    ZEntityRef(ZEntityType** p_EntityRef) :
        m_pEntity(p_EntityRef) {}

    bool operator==(const ZEntityRef& p_Other) const {
        return GetEntity() == p_Other.GetEntity();
    }

    operator bool() const {
        return GetEntity() != nullptr;
    }

    ZEntityImpl* GetEntity() const {
        if (!m_pEntity)
            return nullptr;

        auto s_RealPtr = reinterpret_cast<uintptr_t>(m_pEntity) - sizeof(uintptr_t);
        return reinterpret_cast<ZEntityImpl*>(s_RealPtr);
    }

    ZEntityImpl* operator->() const {
        return GetEntity();
    }

    ZEntityRef GetLogicalParent() const {
        const auto s_Entity = GetEntity();

        if (!s_Entity || !s_Entity->GetType() || s_Entity->GetType()->m_nLogicalParentEntityOffset == 0)
            return {};

        return {
            reinterpret_cast<ZEntityType**>(reinterpret_cast<uintptr_t>(m_pEntity) + s_Entity->GetType()->
                m_nLogicalParentEntityOffset)
        };
    }

    ZHMSDK_API void SetLogicalParent(ZEntityRef entityRef);

    bool IsAnyParent(const ZEntityRef& p_Other) const {
        if (!p_Other)
            return false;

        if (!GetEntity())
            return false;

        if (GetLogicalParent() == p_Other || GetOwningEntity() == p_Other)
            return true;

        return GetLogicalParent().IsAnyParent(p_Other);
    }

    ZEntityRef GetOwningEntity() const {
        const auto s_Entity = GetEntity();

        if (!s_Entity || !s_Entity->GetType() || s_Entity->GetType()->m_nOwningEntityOffset == 0)
            return {};

        return {
            reinterpret_cast<ZEntityType**>(reinterpret_cast<uintptr_t>(m_pEntity) + s_Entity->GetType()->
                m_nOwningEntityOffset)
        };
    }

    ZEntityRef GetClosestParentWithBlueprintFactory() const {
        if (!GetLogicalParent())
            return {};

        if (GetLogicalParent().GetBlueprintFactory())
            return GetLogicalParent();

        return GetLogicalParent().GetClosestParentWithBlueprintFactory();
    }

    ZEntityBlueprintFactoryBase* GetBlueprintFactory() const {
        const auto* s_Entity = GetEntity();

        if (!s_Entity)
            return nullptr;

        const auto* s_Type = s_Entity->GetType();

        if (!s_Type)
            return nullptr;

        if ((s_Type->m_nUnkFlags & 0x200) == 0) // IsRootFactoryEntity or something
            return nullptr;

        auto s_RootEntity = QueryInterface<void>();

        if (!s_RootEntity)
            return nullptr;

        // Pointer to IEntityBlueprintFactory stored right before the start of this entity.
        return *reinterpret_cast<ZEntityBlueprintFactoryBase**>(reinterpret_cast<uintptr_t>(s_RootEntity) - sizeof(
            uintptr_t));
    }

    template <class T>
    T* QueryInterface() const {
        const auto s_Entity = GetEntity();

        if (!s_Entity || !*Globals::TypeRegistry || !s_Entity->GetType())
            return nullptr;

        const auto s_TypeID = (*Globals::TypeRegistry)->GetTypeID(ZHMTypeName<T>);

        if (!s_TypeID)
            return nullptr;

        for (const auto& s_Interface : *s_Entity->GetType()->m_pInterfaces) {
            if (s_Interface.m_pTypeId == s_TypeID) {
                return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_pEntity) + s_Interface.m_nOffset);
            }
        }

        return nullptr;
    }

    template <class T>
    T* QueryInterface(STypeID* p_TypeID) const {
        const auto s_Entity = GetEntity();

        if (!s_Entity || !*Globals::TypeRegistry || !s_Entity->GetType())
            return nullptr;

        for (const auto& s_Interface : *s_Entity->GetType()->m_pInterfaces) {
            if (s_Interface.m_pTypeId == p_TypeID) {
                return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_pEntity) + s_Interface.m_nOffset);
            }
        }

        return nullptr;
    }

    template <class T>
    bool HasInterface() const {
        const auto s_Entity = GetEntity();

        if (!s_Entity || !*Globals::TypeRegistry)
            return false;

        const auto s_TypeID = (*Globals::TypeRegistry)->GetTypeID(ZHMTypeName<T>);

        if (!s_TypeID)
            return false;

        for (const auto& s_Interface : *s_Entity->GetType()->m_pInterfaces) {
            if (s_Interface.m_pTypeId == s_TypeID) {
                return true;
            }
        }

        return false;
    }

    bool HasInterface(const ZString& p_TypeName) const {
        const auto s_Entity = GetEntity();

        if (!s_Entity || !*Globals::TypeRegistry || !s_Entity->GetType())
            return false;

        const auto s_TypeID = (*Globals::TypeRegistry)->GetTypeID(p_TypeName);

        if (!s_TypeID)
            return false;

        for (const auto& s_Interface : *s_Entity->GetType()->m_pInterfaces) {
            if (s_Interface.m_pTypeId == s_TypeID) {
                return true;
            }
        }

        return false;
    }

    template <typename T>
    ZVariant<T> GetProperty(const uint32_t nPropertyID) const {
        ZVariant<T> s_PropertyVal;

        const auto s_Entity = GetEntity();

        if (!s_Entity || !*Globals::MemoryManager)
            return s_PropertyVal;

        const auto s_Type = s_Entity->GetType();

        if (!s_Type || !s_Type->m_pProperties01)
            return s_PropertyVal;

        for (uint32_t i = 0; i < s_Type->m_pProperties01->size(); ++i) {
            const ZEntityProperty* s_Property = &s_Type->m_pProperties01->operator[](i);

            if (s_Property->m_nPropertyId != nPropertyID)
                continue;

            const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();
            const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(m_pEntity) + s_Property->m_nOffset;

            const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
            const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;

            auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

            if (s_PropertyInfo->m_nFlags & EPropertyInfoFlags::E_HAS_GETTER_SETTER) {
                s_PropertyInfo->get(reinterpret_cast<void*>(s_PropertyAddress), s_Data, s_PropertyInfo->m_nOffset);
            }
            else {
                s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
                    s_Data, reinterpret_cast<void*>(s_PropertyAddress)
                );
            }

            s_PropertyVal.UNSAFE_Assign(s_PropertyInfo->m_pType, s_Data);

            break;
        }

        return s_PropertyVal;
    }

    template <typename T>
    ZVariant<T> GetProperty(const ZString& p_PropertyName) const {
        return GetProperty<T>(Hash::Crc32(p_PropertyName.c_str(), p_PropertyName.size()));
    }

    ZHMSDK_API bool SetProperty(uint32_t p_PropertyId, const ZObjectRef& p_Value, bool p_InvokeChangeHandlers = true);

    bool SetProperty(const ZString& p_PropertyName, const ZObjectRef& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty(Hash::Crc32(p_PropertyName.c_str(), p_PropertyName.size()), p_Value, p_InvokeChangeHandlers);
    }

    template <class T>
    bool SetProperty(uint32_t p_PropertyId, const T& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty(p_PropertyId, ZVariant<T>(p_Value), p_InvokeChangeHandlers);
    }

    template <class T>
    bool SetProperty(const ZString& p_PropertyName, const T& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty<T>(
            Hash::Crc32(p_PropertyName.c_str(), p_PropertyName.size()), p_Value, p_InvokeChangeHandlers
        );
    }

    template <class T>
    bool SetProperty(uint32_t p_PropertyId, const ZVariant<T>& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty(p_PropertyId, static_cast<const ZObjectRef&>(p_Value), p_InvokeChangeHandlers);
    }

    template <class T>
    bool SetProperty(const ZString& p_PropertyName, const ZVariant<T>& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty<T>(
            Hash::Crc32(p_PropertyName.c_str(), p_PropertyName.size()),
            static_cast<const ZObjectRef&>(p_Value),
            p_InvokeChangeHandlers
        );
    }

    template <class T>
    bool SetProperty(uint32_t p_PropertyId, const ZVariantRef<T>& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty(p_PropertyId, static_cast<const ZObjectRef&>(p_Value), p_InvokeChangeHandlers);
    }

    template <class T>
    bool SetProperty(const ZString& p_PropertyName, const ZVariantRef<T>& p_Value, bool p_InvokeChangeHandlers = true) {
        return SetProperty<T>(
            Hash::Crc32(p_PropertyName.c_str(), p_PropertyName.size()),
            static_cast<const ZObjectRef&>(p_Value),
            p_InvokeChangeHandlers
        );
    }

    void SignalInputPin(const ZString& p_PinName, const ZObjectRef& p_Data = ZObjectRef()) const {
        SignalInputPin(Hash::Crc32(p_PinName.c_str(), p_PinName.size()), p_Data);
    }

    ZHMSDK_API void SignalInputPin(uint32_t p_PinId, const ZObjectRef& p_Data = ZObjectRef()) const;

    void SignalOutputPin(const ZString& p_PinName, const ZObjectRef& p_Data = ZObjectRef()) const {
        SignalOutputPin(Hash::Crc32(p_PinName.c_str(), p_PinName.size()), p_Data);
    }

    ZHMSDK_API void SignalOutputPin(uint32_t p_PinId, const ZObjectRef& p_Data = ZObjectRef()) const;

    struct hasher {
        size_t operator()(const ZEntityRef& p_Ref) const noexcept {
            return reinterpret_cast<uintptr_t>(p_Ref.GetEntity());
        }
    };
};

template <>
struct std::hash<ZEntityRef> {
    size_t operator()(const ZEntityRef& p_Ref) const noexcept {
        return reinterpret_cast<uintptr_t>(p_Ref.GetEntity());
    }
};

template <typename T>
class TEntityRef {
public:
    TEntityRef() = default;

    explicit TEntityRef(ZEntityRef p_Ref) :
        m_ref(p_Ref), m_pInterfaceRef(p_Ref.QueryInterface<T>()) {}

    ZEntityRef m_ref;
    T* m_pInterfaceRef = nullptr;

    operator bool() const {
        return m_ref && m_pInterfaceRef != nullptr;
    }
};

class ZRepositoryItemEntity :
        public ZEntityImpl {
public:
    ZRepositoryID m_sId;
};