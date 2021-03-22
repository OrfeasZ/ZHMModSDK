#pragma once

#include "ZResourceID.h"
#include "Reflection.h"
#include "TArray.h"

class ZEntityScope;

class IEntitySceneContext :
	public IComponentInterface
{
};

class ZSceneData
{
public:
	ZString m_sceneName; // 0x00
	TArray<ZString> m_sceneBricks; // 0x10
	bool m_bStartScene; // 0x28
	bool m_unk01; // 0x29
	ZString m_type; // 0x30
	ZString m_codeNameHint; // 0x40
};

class ZEntitySceneContext :
	public IEntitySceneContext
{
public:
	virtual ~ZEntitySceneContext() = 0;
	virtual void unk00() = 0;
	virtual void unk01() = 0;
	virtual void unk02() = 0;
	virtual void unk03() = 0;
	virtual void unk04() = 0;
	virtual void unk05() = 0;
	virtual void unk06() = 0;
	virtual void unk07() = 0;
	virtual void unk08() = 0;
	virtual void unk09() = 0;
	virtual void unk10() = 0;
	virtual void unk11() = 0;
	virtual void unk12() = 0;
	virtual void unk13() = 0;
	virtual void unk14() = 0;
	virtual void unk15() = 0;
	virtual void LoadScene(const ZSceneData& data) = 0;
	virtual void unk16() = 0;
	virtual void unk17() = 0;
	virtual void unk18() = 0;
	virtual void unk19() = 0;
	virtual void unk20() = 0;
	virtual void unk21() = 0;
	virtual void unk22() = 0;
	virtual void unk23() = 0;
	virtual void unk24() = 0;
	virtual void unk25() = 0;
	virtual void unk26() = 0;

public:
	PAD(0x08);
	ZSceneData m_sceneData;
	PAD(136); // 96
	ZEntityScope* m_pEntityScope; // 232
};
