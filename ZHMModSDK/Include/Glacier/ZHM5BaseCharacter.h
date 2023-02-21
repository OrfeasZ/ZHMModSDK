#pragma once

#include "ZEntity.h"

class ZRagdollHandler;

class IHM5BaseCharacter :
    public IComponentInterface
{
public:
    virtual ~IHM5BaseCharacter() {}
};

class ICharacterTransformState :
    public IComponentInterface
{
public:
    virtual ~ICharacterTransformState() {}
    virtual void ICharacterTransformState_unk0() = 0;
    virtual void ICharacterTransformState_unk1() = 0;
    virtual void ICharacterTransformState_unk2() = 0;
    virtual void ICharacterTransformState_unk3() = 0;
    virtual void ICharacterTransformState_unk4() = 0;
    virtual void ICharacterTransformState_unk5() = 0;
    virtual void ICharacterTransformState_unk6() = 0;
    virtual void ICharacterTransformState_unk7() = 0;
    virtual void ICharacterTransformState_unk8() = 0;
    virtual void ICharacterTransformState_unk9() = 0;
};

class IBaseCharacter :
    public IComponentInterface
{
public:
    virtual ~IBaseCharacter() {}
    virtual void IBaseCharacter_unk0() = 0;
    virtual void IBaseCharacter_unk1() = 0;
    virtual void IBaseCharacter_unk2() = 0;
    virtual void IBaseCharacter_unk3() = 0;
    virtual void IBaseCharacter_unk4() = 0;
    virtual void IBaseCharacter_unk5() = 0;
    virtual void IBaseCharacter_unk6() = 0;
    virtual void IBaseCharacter_unk7() = 0;
    virtual void IBaseCharacter_unk8() = 0;
    virtual void IBaseCharacter_unk9() = 0;
    virtual void IBaseCharacter_unk10() = 0;
};

class IMorphemeCutSequenceAnimatable
{
public:
    virtual void IMorphemeCutSequenceAnimatable_unk0() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk1() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk2() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk3() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk4() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk5() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk6() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk7() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk8() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk9() = 0;
    virtual void IMorphemeCutSequenceAnimatable_unk10() = 0;
};

class IBoneCollidable
{
public:
    virtual void IBoneCollidable_unk0() = 0;
    virtual void IBoneCollidable_unk1() = 0;
    virtual void IBoneCollidable_unk2() = 0;
};

class IItemOwner :
    public IComponentInterface
{
public:
    virtual ~IItemOwner() {}
    virtual void IItemOwner_unk0() = 0;
    virtual void IItemOwner_unk1() = 0;
};

class ICrowdCoreProvider :
    public IComponentInterface
{
public:
    virtual ~ICrowdCoreProvider() {}
    virtual void ICrowdCoreProvider_unk0() = 0;
    virtual void ICrowdCoreProvider_unk1() = 0;
    virtual void ICrowdCoreProvider_unk2() = 0;
    virtual void ICrowdCoreProvider_unk3() = 0;
};

// Size: 0x2D0
class ZHM5BaseCharacter :
    public ZEntityImpl,
    public IHM5BaseCharacter,
    public ICharacterTransformState,
    public IBaseCharacter,
    public IMorphemeCutSequenceAnimatable,
    public IBoneCollidable,
    public IItemOwner,
    public ICrowdCoreProvider
{
public:
    PAD(0x10); // 0x50
    ZRagdollHandler* m_pRagdollHandler; // 0x60
    ZEntityRef m_EventConsumerCollection; // 0x68
    PAD(0x260);
};
