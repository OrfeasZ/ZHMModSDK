#pragma once

#include "ZEntity.h"

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
	virtual void ICharacterTransformState_unk00() = 0;
	virtual void ICharacterTransformState_unk01() = 0;
	virtual void ICharacterTransformState_unk02() = 0;
	virtual void ICharacterTransformState_unk03() = 0;
	virtual void ICharacterTransformState_unk04() = 0;
	virtual void ICharacterTransformState_unk05() = 0;
	virtual void ICharacterTransformState_unk06() = 0;
	virtual void ICharacterTransformState_unk07() = 0;
	virtual void ICharacterTransformState_unk08() = 0;
	virtual void ICharacterTransformState_unk09() = 0;
};

class IBaseCharacter :
	public IComponentInterface
{
public:
	virtual ~IBaseCharacter() {}
	virtual void IBaseCharacter_unk00() = 0;
	virtual void IBaseCharacter_unk01() = 0;
	virtual void IBaseCharacter_unk02() = 0;
	virtual void IBaseCharacter_unk03() = 0;
	virtual void IBaseCharacter_unk04() = 0;
	virtual void IBaseCharacter_unk05() = 0;
	virtual void IBaseCharacter_unk06() = 0;
	virtual void IBaseCharacter_unk07() = 0;
	virtual void IBaseCharacter_unk08() = 0;
	virtual void IBaseCharacter_unk09() = 0;
	virtual void IBaseCharacter_unk10() = 0;
};

class IMorphemeCutSequenceAnimatable
{
public:
	virtual void IMorphemeCutSequenceAnimatable_unk00() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk01() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk02() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk03() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk04() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk05() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk06() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk07() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk08() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk09() = 0;
	virtual void IMorphemeCutSequenceAnimatable_unk10() = 0;
};

class IBoneCollidable
{
public:
	virtual void IBoneCollidable_unk00() = 0;
	virtual void IBoneCollidable_unk01() = 0;
	virtual void IBoneCollidable_unk02() = 0;
};

class IItemOwner :
	public IComponentInterface
{
public:
	virtual ~IItemOwner() {}
	virtual void IItemOwner_unk00() = 0;
	virtual void IItemOwner_unk01() = 0;
};

class ICrowdCoreProvider :
	public IComponentInterface
{
public:
	virtual ~ICrowdCoreProvider() {}
	virtual void ICrowdCoreProvider_unk00() = 0;
	virtual void ICrowdCoreProvider_unk01() = 0;
	virtual void ICrowdCoreProvider_unk02() = 0;
	virtual void ICrowdCoreProvider_unk03() = 0;
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
	PAD(0x18); // 0x50
	ZEntityRef m_EventConsumerCollection; // 0x68
	PAD(0x260);
};
