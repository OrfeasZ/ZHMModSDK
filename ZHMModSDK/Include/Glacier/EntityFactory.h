#pragma once

#include "Reflection.h"
#include "ZResource.h"

class IEntityFactory : public IComponentInterface
{
public:
	virtual void IEntityFactory_unk5() = 0;
	virtual void ConfigureEntity() = 0;
	virtual void IEntityFactory_unk7() = 0;
	virtual void IEntityFactory_unk8() = 0;
	virtual void IEntityFactory_unk9() = 0;
	virtual void IEntityFactory_unk10() = 0;
};

class IEntityBlueprintFactory : public IComponentInterface
{
public:
	virtual void IEntityBlueprintFactory_unk5() = 0;
	virtual void IEntityBlueprintFactory_unk6() = 0;
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
	virtual void IEntityBlueprintFactory_unk25() = 0;
	virtual void IEntityBlueprintFactory_unk26() = 0;
	virtual void IEntityBlueprintFactory_unk27() = 0;
	virtual void IEntityBlueprintFactory_unk28() = 0;
	virtual void IEntityBlueprintFactory_unk29() = 0;
	virtual void IEntityBlueprintFactory_unk30() = 0;
	virtual void IEntityBlueprintFactory_unk31() = 0;
	virtual void IEntityBlueprintFactory_unk32() = 0;
};

class ZEntityBlueprintFactoryBase : public IEntityBlueprintFactory
{
public:
	PAD(0x40);
};

class ZCompositeEntityBlueprintFactoryBase : public ZEntityBlueprintFactoryBase
{
public:
	TArray<int64_t> m_aSubEntityOffsets; // 0x48
};

static_assert(offsetof(ZCompositeEntityBlueprintFactoryBase, m_aSubEntityOffsets) == 0x48);

class ZTemplateEntityBlueprintFactory : public ZCompositeEntityBlueprintFactoryBase
{
	
};

class ZTemplateEntityFactory : public IEntityFactory
{
public:
	PAD(0x30);
	TResourcePtr<ZTemplateEntityBlueprintFactory> m_blueprintResource; // 0x38
};

static_assert(offsetof(ZTemplateEntityFactory, m_blueprintResource) == 0x38);
