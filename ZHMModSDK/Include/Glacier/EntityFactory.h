#pragma once

#include "Reflection.h"
#include "ZResource.h"

class IEntityBlueprintFactory : public IComponentInterface
{

};

class ZEntityBlueprintFactoryBase : public IEntityBlueprintFactory
{

};

class ZCompositeEntityBlueprintFactoryBase : public ZEntityBlueprintFactoryBase
{
public:
	PAD(0x40);
	TArray<long long> m_aSubEntityOffsets;
};

class IEntityFactory : public IComponentInterface
{

};

class ZTemplateEntityBlueprintFactory : public IEntityFactory
{

};

class ZTemplateEntityFactory : public IEntityFactory
{
public:
	PAD(0x30);
	TResourcePtr<ZTemplateEntityBlueprintFactory> m_blueprintResource;
};
