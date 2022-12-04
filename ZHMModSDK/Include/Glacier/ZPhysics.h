#pragma once

#include "Reflection.h"
#include "ZEntity.h"

class ZPhysicsObjectProxy;

struct ZPhysicsObjectRef
{
	ZPhysicsObjectProxy* m_pProxy;
};

class ICollisionShapeListener : public IComponentInterface
{
public:
	virtual void ICollisionShapeListener_unk5() = 0;
};

class IPhysicsAccessor : public IComponentInterface
{
public:
	virtual void IPhysicsAccessor_unk5() = 0;
	virtual void IPhysicsAccessor_unk6() = 0;
	virtual void IPhysicsAccessor_unk7() = 0;
	virtual void IPhysicsAccessor_unk8() = 0;
	virtual void IPhysicsAccessor_unk9() = 0;
	virtual void IPhysicsAccessor_unk10() = 0;
};

class IStaticPhysics : public IPhysicsAccessor
{
};

class ZPhysicsBaseEntity : public ZEntityImpl
{
public:
	virtual ~ZPhysicsBaseEntity() = default;
	virtual void ZPhysicsBaseEntity_unk20() = 0;

public:
	PAD(0x08);
};

static_assert(sizeof(ZPhysicsBaseEntity) == 0x20);

class IDebugPhysicsSpatialAccessor
{
public:
	virtual void IDebugPhysicsSpatialAccessor_unk0() = 0;
};

class IPhysicsObject
{
public:
	virtual ~IPhysicsObject() = 0;
	virtual void IPhysicsObject_unk1() = 0;
	virtual void IPhysicsObject_unk2() = 0;
	virtual void IPhysicsObject_unk3() = 0;
	virtual void IPhysicsObject_unk4() = 0;
	virtual void IPhysicsObject_unk5() = 0;
	virtual void IPhysicsObject_unk6() = 0;
	virtual void IPhysicsObject_unk7() = 0;
	virtual void IPhysicsObject_unk8() = 0;
	virtual void SetTransform(const SMatrix& transform) = 0;
	virtual void IPhysicsObject_unk10() = 0;
	virtual void IPhysicsObject_unk11() = 0;
	virtual void IPhysicsObject_unk12() = 0;
	virtual void SetPosition(const float4& trans) = 0;
	virtual void IPhysicsObject_unk14() = 0;
	virtual void IPhysicsObject_unk15() = 0;
	virtual void IPhysicsObject_unk16() = 0;
	virtual void IPhysicsObject_unk17() = 0;
	virtual void IPhysicsObject_unk18() = 0;
	virtual void IPhysicsObject_unk19() = 0;
	virtual void IPhysicsObject_unk20() = 0;
	virtual void IPhysicsObject_unk21() = 0;
	virtual void IPhysicsObject_unk22() = 0;
	virtual void IPhysicsObject_unk23() = 0;
	virtual void IPhysicsObject_unk24() = 0;
	virtual void IPhysicsObject_unk25() = 0;
	virtual void IPhysicsObject_unk26() = 0;
	virtual void IPhysicsObject_unk27() = 0;
	virtual void IPhysicsObject_unk28() = 0;
	virtual void IPhysicsObject_unk29() = 0;
	virtual void IPhysicsObject_unk30() = 0;
	virtual void IPhysicsObject_unk31() = 0;
	virtual void IPhysicsObject_unk32() = 0;
	virtual void IPhysicsObject_unk33() = 0;
	virtual void IPhysicsObject_unk34() = 0;
	virtual void IPhysicsObject_unk35() = 0;
	virtual void IPhysicsObject_unk36() = 0;
	virtual void IPhysicsObject_unk37() = 0;
	virtual void IPhysicsObject_unk38() = 0;
	virtual void IPhysicsObject_unk39() = 0;
	virtual void IPhysicsObject_unk40() = 0;
	virtual void IPhysicsObject_unk41() = 0;
	virtual void IPhysicsObject_unk42() = 0;
	virtual void IPhysicsObject_unk43() = 0;
	virtual void IPhysicsObject_unk44() = 0;
	virtual void IPhysicsObject_unk45() = 0;
	virtual void IPhysicsObject_unk46() = 0;
	virtual void IPhysicsObject_unk47() = 0;
	virtual void IPhysicsObject_unk48() = 0;
	virtual void IPhysicsObject_unk49() = 0;
	virtual void IPhysicsObject_unk50() = 0;
	virtual void IPhysicsObject_unk51() = 0;
	virtual void IPhysicsObject_unk52() = 0;
	virtual void IPhysicsObject_unk53() = 0;
	virtual void IPhysicsObject_unk54() = 0;
	virtual void IPhysicsObject_unk55() = 0;
	virtual void IPhysicsObject_unk56() = 0;
	virtual void IPhysicsObject_unk57() = 0;
	virtual void IPhysicsObject_unk58() = 0;
	virtual void IPhysicsObject_unk59() = 0;
	virtual void IPhysicsObject_unk60() = 0;
	virtual void IPhysicsObject_unk61() = 0;
	virtual void IPhysicsObject_unk62() = 0;
	virtual void IPhysicsObject_unk63() = 0;
	virtual void IPhysicsObject_unk64() = 0;
	virtual void IPhysicsObject_unk65() = 0;
	virtual void IPhysicsObject_unk66() = 0;
	virtual void IPhysicsObject_unk67() = 0;
	virtual void IPhysicsObject_unk68() = 0;
	virtual void IPhysicsObject_unk69() = 0;
	virtual void IPhysicsObject_unk70() = 0;
	virtual void IPhysicsObject_unk71() = 0;
	virtual void IPhysicsObject_unk72() = 0;
	virtual void IPhysicsObject_unk73() = 0;
	virtual void IPhysicsObject_unk74() = 0;
	virtual void IPhysicsObject_unk75() = 0;
	virtual void IPhysicsObject_unk76() = 0;
	virtual void IPhysicsObject_unk77() = 0;
	virtual void IPhysicsObject_unk78() = 0;
	virtual void IPhysicsObject_unk79() = 0;
	virtual void IPhysicsObject_unk80() = 0;
	virtual void IPhysicsObject_unk81() = 0;
	virtual void IPhysicsObject_unk82() = 0;
	virtual void IPhysicsObject_unk83() = 0;
	virtual void IPhysicsObject_unk84() = 0;
	virtual void IPhysicsObject_unk85() = 0;
	virtual void IPhysicsObject_unk86() = 0;
	virtual void IPhysicsObject_unk87() = 0;
	virtual void IPhysicsObject_unk88() = 0;
	virtual void IPhysicsObject_unk89() = 0;
	virtual void IPhysicsObject_unk90() = 0;
	virtual void IPhysicsObject_unk91() = 0;
	virtual void IPhysicsObject_unk92() = 0;
	virtual void IPhysicsObject_unk93() = 0;
	virtual void IPhysicsObject_unk94() = 0;
	virtual void IPhysicsObject_unk95() = 0;
	virtual void IPhysicsObject_unk96() = 0;
	virtual void IPhysicsObject_unk97() = 0;
	virtual void IPhysicsObject_unk98() = 0;
	virtual void IPhysicsObject_unk99() = 0;
	virtual void IPhysicsObject_unk100() = 0;
	virtual void IPhysicsObject_unk101() = 0;
	virtual void IPhysicsObject_unk102() = 0;
	virtual void IPhysicsObject_unk103() = 0;
	virtual void IPhysicsObject_unk104() = 0;
	virtual void IPhysicsObject_unk105() = 0;
	virtual void IPhysicsObject_unk106() = 0;
	virtual void IPhysicsObject_unk107() = 0;
	virtual void IPhysicsObject_unk108() = 0;
	virtual void IPhysicsObject_unk109() = 0;
	virtual void IPhysicsObject_unk110() = 0;
	virtual void IPhysicsObject_unk111() = 0;
	virtual void IPhysicsObject_unk112() = 0;
	virtual void IPhysicsObject_unk113() = 0;
	virtual void IPhysicsObject_unk114() = 0;
	virtual void IPhysicsObject_unk115() = 0;
	virtual void IPhysicsObject_unk116() = 0;
	virtual void IPhysicsObject_unk117() = 0;
	virtual void IPhysicsObject_unk118() = 0;
	virtual void IPhysicsObject_unk119() = 0;
	virtual void IPhysicsObject_unk120() = 0;
	virtual void IPhysicsObject_unk121() = 0;
	virtual void IPhysicsObject_unk122() = 0;
	virtual void IPhysicsObject_unk123() = 0;
	virtual void IPhysicsObject_unk124() = 0;
	virtual void IPhysicsObject_unk125() = 0;
};

class IPhysicsUserDataAccessor
{
public:
	virtual void IPhysicsUserDataAccessor_unk0() = 0;
};

class ZPhysicsObject :
	public IPhysicsObject,
	public IPhysicsUserDataAccessor
{
public:
	virtual ~ZPhysicsObject() = default;

public:
	PAD(0x130);
};

static_assert(sizeof(ZPhysicsObject) == 0x140);

class ZStaticPhysicsAspect :
	public ZPhysicsBaseEntity,
	public ICollisionShapeListener,
	public IStaticPhysics,
	public IDebugPhysicsSpatialAccessor
{
public:
	virtual ~ZStaticPhysicsAspect() = default;
	virtual void ZStaticPhysicsAspect_unk21() = 0;

public:
	PAD(0x08);
	IPhysicsObject* m_pPhysicsObject; // 0x40
	PAD(0x20);
};

static_assert(offsetof(ZStaticPhysicsAspect, m_pPhysicsObject) == 0x40);
static_assert(sizeof(ZStaticPhysicsAspect) == 0x68);
