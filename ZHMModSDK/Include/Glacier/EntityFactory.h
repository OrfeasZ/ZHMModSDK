#pragma once

#include "Reflection.h"
#include "ZResource.h"

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
