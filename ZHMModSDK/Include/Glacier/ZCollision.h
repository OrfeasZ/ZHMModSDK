#pragma once

#include "Reflection.h"
#include "ZMath.h"
#include "ZEntity.h"
#include "ZPhysics.h"

struct ZRayQueryInput
{
    float4 m_vFrom;
    float4 m_vTo;
    uint32 m_nRayFilter;
    ECollidablesType m_eType;
    ERayDetailLevel m_eRayDetailLevel;
    ZDelegate<bool(ZEntityRef, TEntityRef<ZSpatialEntity>)> m_FilterCallback;
    bool m_bIgnoreTransparentMaterials;
    bool m_bIgnoreDecalMaterials;
};

static_assert(sizeof(ZRayQueryInput) == 0x60);

struct ZRayQueryOutput
{
    float4 m_vPosition;
    float4 m_vNormal; // 0x10
    PAD(0x20); // 0x20
    ZEntityRef m_BlockingEntity; // 0x40
    PAD(0x58); // 0x48
};

static_assert(sizeof(ZRayQueryOutput) == 0xA0);
static_assert(offsetof(ZRayQueryOutput, m_BlockingEntity) == 0x40);

class ZCollisionManager : public IComponentInterface
{
public:
    virtual void ZCollisionManager_unk5() = 0;
    virtual void ZCollisionManager_unk6() = 0;
    virtual void ZCollisionManager_unk7() = 0;
    virtual void ZCollisionManager_unk8() = 0;
    virtual void ZCollisionManager_unk9() = 0;
    virtual void ZCollisionManager_unk10() = 0;
    virtual void ZCollisionManager_unk11() = 0;
    virtual void ZCollisionManager_unk12() = 0;
    virtual void ZCollisionManager_unk13() = 0;
    virtual void ZCollisionManager_unk14() = 0;
    virtual void ZCollisionManager_unk15() = 0;
    virtual void ZCollisionManager_unk16() = 0;
    virtual void ZCollisionManager_unk17() = 0;
    virtual void ZCollisionManager_unk18() = 0;
    virtual void ZCollisionManager_unk19() = 0;
    virtual bool RayCastClosestHit(const ZRayQueryInput& input, ZRayQueryOutput* output) = 0;
    virtual void ZCollisionManager_unk21() = 0;
    virtual void ZCollisionManager_unk22() = 0;
    virtual void ZCollisionManager_unk23() = 0;
    virtual void ZCollisionManager_unk24() = 0;
    virtual void ZCollisionManager_unk25() = 0;
    virtual void ZCollisionManager_unk26() = 0;
    virtual void ZCollisionManager_unk27() = 0;
    virtual void ZCollisionManager_unk28() = 0;
    virtual void ZCollisionManager_unk29() = 0;
    virtual void ZCollisionManager_unk30() = 0;
    virtual void ZCollisionManager_unk31() = 0;
    virtual void ZCollisionManager_unk32() = 0;
    virtual void ZCollisionManager_unk33() = 0;
    virtual void ZCollisionManager_unk34() = 0;
    virtual void ZCollisionManager_unk35() = 0;
    virtual void ZCollisionManager_unk36() = 0;
    virtual void ZCollisionManager_unk37() = 0;
    virtual void ZCollisionManager_unk38() = 0;
    virtual void ZCollisionManager_unk39() = 0;
    virtual void ZCollisionManager_unk40() = 0;
    virtual void ZCollisionManager_unk41() = 0;
    virtual void ZCollisionManager_unk42() = 0;
    virtual void ZCollisionManager_unk43() = 0;
    virtual void ZCollisionManager_unk44() = 0;
    virtual void ZCollisionManager_unk45() = 0;
    virtual void ZCollisionManager_unk46() = 0;
    virtual void ZCollisionManager_unk47() = 0;
    virtual void ZCollisionManager_unk48() = 0;
    virtual void ZCollisionManager_unk49() = 0;
    virtual void ZCollisionManager_unk50() = 0;
    virtual void ZCollisionManager_unk51() = 0;
    virtual void ZCollisionManager_unk52() = 0;
    virtual void ZCollisionManager_unk53() = 0;
    virtual void ZCollisionManager_unk54() = 0;
    virtual void ZCollisionManager_unk55() = 0;
    virtual void ZCollisionManager_unk56() = 0;
    virtual void ZCollisionManager_unk57() = 0;
    virtual void ZCollisionManager_unk58() = 0;
    virtual void ZCollisionManager_unk59() = 0;
    virtual void ZCollisionManager_unk60() = 0;
    virtual void ZCollisionManager_unk61() = 0;
    virtual void ZCollisionManager_unk62() = 0;
    virtual void ZCollisionManager_unk63() = 0;
    virtual void ZCollisionManager_unk64() = 0;
    virtual void ZCollisionManager_unk65() = 0;
    virtual void ZCollisionManager_unk66() = 0;
    virtual void ZCollisionManager_unk67() = 0;
    virtual void ZCollisionManager_unk68() = 0;
    virtual void ZCollisionManager_unk69() = 0;
    virtual void ZCollisionManager_unk70() = 0;
    virtual void ZCollisionManager_unk71() = 0;
    virtual void ZCollisionManager_unk72() = 0;
    virtual void ZCollisionManager_unk73() = 0;
    virtual void ZCollisionManager_unk74() = 0;
    virtual void ZCollisionManager_unk75() = 0;
    virtual void ZCollisionManager_unk76() = 0;
    virtual void ZCollisionManager_unk77() = 0;
    virtual void ZCollisionManager_unk78() = 0;
    virtual void ZCollisionManager_unk79() = 0;
    virtual void ZCollisionManager_unk80() = 0;
    virtual void ZCollisionManager_unk81() = 0;
    virtual void ZCollisionManager_unk82() = 0;
    virtual void ZCollisionManager_unk83() = 0;
    virtual void ZCollisionManager_unk84() = 0;
    virtual void ZCollisionManager_unk85() = 0;
    virtual void ZCollisionManager_unk86() = 0;
    virtual void ZCollisionManager_unk87() = 0;
    virtual void ZCollisionManager_unk88() = 0;
    virtual void ZCollisionManager_unk89() = 0;
    virtual void ZCollisionManager_unk90() = 0;
    virtual void ZCollisionManager_unk91() = 0;
    virtual void ZCollisionManager_unk92() = 0;
    virtual void ZCollisionManager_unk93() = 0;
    virtual void ZCollisionManager_unk94() = 0;
    virtual void ZCollisionManager_unk95() = 0;
    virtual void ZCollisionManager_unk96() = 0;
    virtual void ZCollisionManager_unk97() = 0;
    virtual void ZCollisionManager_unk98() = 0;
    virtual void ZCollisionManager_unk99() = 0;
    virtual void ZCollisionManager_unk100() = 0;
    virtual void ZCollisionManager_unk101() = 0;
    virtual void ZCollisionManager_unk102() = 0;
    virtual void ZCollisionManager_unk103() = 0;
    virtual void ZCollisionManager_unk104() = 0;
    virtual void ZCollisionManager_unk105() = 0;
    virtual void ZCollisionManager_unk106() = 0;
    virtual void ZCollisionManager_unk107() = 0;
    virtual void ZCollisionManager_unk108() = 0;
    virtual void ZCollisionManager_unk109() = 0;
    virtual void ZCollisionManager_unk110() = 0;
    virtual void ZCollisionManager_unk111() = 0;
    virtual void ZCollisionManager_unk112() = 0;
    virtual void ZCollisionManager_unk113() = 0;
    virtual void ZCollisionManager_unk114() = 0;
    virtual void ZCollisionManager_unk115() = 0;
    virtual void ZCollisionManager_unk116() = 0;
    virtual void ZCollisionManager_unk117() = 0;
    virtual void ZCollisionManager_unk118() = 0;
    virtual void ZCollisionManager_unk119() = 0;
    virtual void ZCollisionManager_unk120() = 0;
    virtual void ZCollisionManager_unk121() = 0;
    virtual void ZCollisionManager_unk122() = 0;
    virtual void ZCollisionManager_unk123() = 0;
};
