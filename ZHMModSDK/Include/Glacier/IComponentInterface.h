#pragma once

class ZObjectRef;
struct STypeID;

class IComponentInterface {
public:
    virtual ~IComponentInterface() = 0;
    virtual ZObjectRef* GetVariantRef(ZObjectRef& result) = 0;
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual void* QueryInterface(STypeID* iid) = 0;
};