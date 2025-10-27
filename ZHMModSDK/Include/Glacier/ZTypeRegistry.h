#pragma once

#include "Reflection.h"
#include "THashMap.h"
#include "ZString.h"
#include "Reflection.h"
#include "TArray.h"

struct TypeMapHashingPolicy {
    uint64_t operator()(const ZString& p_Value) {
        return Hash::Fnv1a64_Lower(p_Value.c_str(), p_Value.size());
    }
};

class ZTypeRegistry {
public:
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual ZObjectRef* GetVariantRef(ZObjectRef& result) = 0;
    virtual void* QueryInterface(STypeID* iid) = 0;
    virtual ~ZTypeRegistry() = 0;

    STypeID* GetTypeID(const ZString& p_TypeName) const {
        const auto s_HashMapIterator = m_types.find(p_TypeName);

        if (s_HashMapIterator != m_types.end()) {
            return s_HashMapIterator->second;
        }

        return nullptr;
    }

public:
    PAD(56);
    THashMap<ZString, STypeID*, TypeMapHashingPolicy> m_types;
    TArray<IType*> m_unnamedTypes;
};