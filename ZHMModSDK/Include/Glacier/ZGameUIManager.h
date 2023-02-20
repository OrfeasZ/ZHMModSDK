#pragma once

#include "ZEntity.h"

class ZGameUIManagerEntity;

class IUITextureRequestResolver
{
public:
    virtual void IUITextureRequestResolver_unk0() = 0;
};

class ZGameUIManager :
    public IComponentInterface,
    public IUITextureRequestResolver
{
public:
    virtual ~ZGameUIManager();

public:
    PAD(0x68);
    TEntityRef<ZGameUIManagerEntity> m_pGameUIManagerEntity; // 0x78
};
