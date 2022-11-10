#pragma once

#include "Reflection.h"
#include "ZMath.h"
#include "ZEntity.h"

struct ZRayQueryInput
{
	float4 m_vFrom;
	float4 m_vTo;
	PAD(0x40); // 0x20
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
	virtual void ZCollisionManager_unknown5() = 0;
	virtual void ZCollisionManager_unknown6() = 0;
	virtual void ZCollisionManager_unknown7() = 0;
	virtual void ZCollisionManager_unknown8() = 0;
	virtual void ZCollisionManager_unknown9() = 0;
	virtual void ZCollisionManager_unknown10() = 0;
	virtual void ZCollisionManager_unknown11() = 0;
	virtual void ZCollisionManager_unknown12() = 0;
	virtual void ZCollisionManager_unknown13() = 0;
	virtual void ZCollisionManager_unknown14() = 0;
	virtual void ZCollisionManager_unknown15() = 0;
	virtual void ZCollisionManager_unknown16() = 0;
	virtual void ZCollisionManager_unknown17() = 0;
	virtual void ZCollisionManager_unknown18() = 0;
	virtual void ZCollisionManager_unknown19() = 0;
	virtual bool RayCastClosestHit(const ZRayQueryInput& input, ZRayQueryOutput* output) = 0;
	virtual void ZCollisionManager_unknown21() = 0;
	virtual void ZCollisionManager_unknown22() = 0;
	virtual void ZCollisionManager_unknown23() = 0;
	virtual void ZCollisionManager_unknown24() = 0;
	virtual void ZCollisionManager_unknown25() = 0;
	virtual void ZCollisionManager_unknown26() = 0;
	virtual void ZCollisionManager_unknown27() = 0;
	virtual void ZCollisionManager_unknown28() = 0;
	virtual void ZCollisionManager_unknown29() = 0;
	virtual void ZCollisionManager_unknown30() = 0;
	virtual void ZCollisionManager_unknown31() = 0;
	virtual void ZCollisionManager_unknown32() = 0;
	virtual void ZCollisionManager_unknown33() = 0;
	virtual void ZCollisionManager_unknown34() = 0;
	virtual void ZCollisionManager_unknown35() = 0;
	virtual void ZCollisionManager_unknown36() = 0;
	virtual void ZCollisionManager_unknown37() = 0;
	virtual void ZCollisionManager_unknown38() = 0;
	virtual void ZCollisionManager_unknown39() = 0;
	virtual void ZCollisionManager_unknown40() = 0;
	virtual void ZCollisionManager_unknown41() = 0;
	virtual void ZCollisionManager_unknown42() = 0;
	virtual void ZCollisionManager_unknown43() = 0;
	virtual void ZCollisionManager_unknown44() = 0;
	virtual void ZCollisionManager_unknown45() = 0;
	virtual void ZCollisionManager_unknown46() = 0;
	virtual void ZCollisionManager_unknown47() = 0;
	virtual void ZCollisionManager_unknown48() = 0;
	virtual void ZCollisionManager_unknown49() = 0;
	virtual void ZCollisionManager_unknown50() = 0;
	virtual void ZCollisionManager_unknown51() = 0;
	virtual void ZCollisionManager_unknown52() = 0;
	virtual void ZCollisionManager_unknown53() = 0;
	virtual void ZCollisionManager_unknown54() = 0;
	virtual void ZCollisionManager_unknown55() = 0;
	virtual void ZCollisionManager_unknown56() = 0;
	virtual void ZCollisionManager_unknown57() = 0;
	virtual void ZCollisionManager_unknown58() = 0;
	virtual void ZCollisionManager_unknown59() = 0;
	virtual void ZCollisionManager_unknown60() = 0;
	virtual void ZCollisionManager_unknown61() = 0;
	virtual void ZCollisionManager_unknown62() = 0;
	virtual void ZCollisionManager_unknown63() = 0;
	virtual void ZCollisionManager_unknown64() = 0;
	virtual void ZCollisionManager_unknown65() = 0;
	virtual void ZCollisionManager_unknown66() = 0;
	virtual void ZCollisionManager_unknown67() = 0;
	virtual void ZCollisionManager_unknown68() = 0;
	virtual void ZCollisionManager_unknown69() = 0;
	virtual void ZCollisionManager_unknown70() = 0;
	virtual void ZCollisionManager_unknown71() = 0;
	virtual void ZCollisionManager_unknown72() = 0;
	virtual void ZCollisionManager_unknown73() = 0;
	virtual void ZCollisionManager_unknown74() = 0;
	virtual void ZCollisionManager_unknown75() = 0;
	virtual void ZCollisionManager_unknown76() = 0;
	virtual void ZCollisionManager_unknown77() = 0;
	virtual void ZCollisionManager_unknown78() = 0;
	virtual void ZCollisionManager_unknown79() = 0;
	virtual void ZCollisionManager_unknown80() = 0;
	virtual void ZCollisionManager_unknown81() = 0;
	virtual void ZCollisionManager_unknown82() = 0;
	virtual void ZCollisionManager_unknown83() = 0;
	virtual void ZCollisionManager_unknown84() = 0;
	virtual void ZCollisionManager_unknown85() = 0;
	virtual void ZCollisionManager_unknown86() = 0;
	virtual void ZCollisionManager_unknown87() = 0;
	virtual void ZCollisionManager_unknown88() = 0;
	virtual void ZCollisionManager_unknown89() = 0;
	virtual void ZCollisionManager_unknown90() = 0;
	virtual void ZCollisionManager_unknown91() = 0;
	virtual void ZCollisionManager_unknown92() = 0;
	virtual void ZCollisionManager_unknown93() = 0;
	virtual void ZCollisionManager_unknown94() = 0;
	virtual void ZCollisionManager_unknown95() = 0;
	virtual void ZCollisionManager_unknown96() = 0;
	virtual void ZCollisionManager_unknown97() = 0;
	virtual void ZCollisionManager_unknown98() = 0;
	virtual void ZCollisionManager_unknown99() = 0;
	virtual void ZCollisionManager_unknown100() = 0;
	virtual void ZCollisionManager_unknown101() = 0;
	virtual void ZCollisionManager_unknown102() = 0;
	virtual void ZCollisionManager_unknown103() = 0;
	virtual void ZCollisionManager_unknown104() = 0;
	virtual void ZCollisionManager_unknown105() = 0;
	virtual void ZCollisionManager_unknown106() = 0;
	virtual void ZCollisionManager_unknown107() = 0;
	virtual void ZCollisionManager_unknown108() = 0;
	virtual void ZCollisionManager_unknown109() = 0;
	virtual void ZCollisionManager_unknown110() = 0;
	virtual void ZCollisionManager_unknown111() = 0;
	virtual void ZCollisionManager_unknown112() = 0;
	virtual void ZCollisionManager_unknown113() = 0;
	virtual void ZCollisionManager_unknown114() = 0;
	virtual void ZCollisionManager_unknown115() = 0;
	virtual void ZCollisionManager_unknown116() = 0;
	virtual void ZCollisionManager_unknown117() = 0;
	virtual void ZCollisionManager_unknown118() = 0;
	virtual void ZCollisionManager_unknown119() = 0;
	virtual void ZCollisionManager_unknown120() = 0;
	virtual void ZCollisionManager_unknown121() = 0;
	virtual void ZCollisionManager_unknown122() = 0;
	virtual void ZCollisionManager_unknown123() = 0;
};
