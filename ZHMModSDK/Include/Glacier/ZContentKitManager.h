#pragma once

#include "Reflection.h"
#include "TMap.h"
#include "ZEntity.h"
#include "ZOutfit.h"

class ZContentKitManager : public IComponentInterface
{
public:
    TMap<ZRepositoryID const, TEntityRef<ZGlobalOutfitKit>> m_repositoryGlobalOutfitKits;
};
