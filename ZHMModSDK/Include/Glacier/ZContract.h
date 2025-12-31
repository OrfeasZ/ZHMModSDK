#pragma once

#include "Reflection.h"
#include "TArray.h"

struct SAgencyPickupInfo {
    ZRepositoryID m_AgencyPickupId; // 0x0
    TArray<ZRepositoryID> m_aItemIds; // 0x10
    TArray<ZRepositoryID> m_aModifierIds; // 0x28
};

class ZContractsManager : public IComponentInterface {
public:
    PAD(0x5E0);
    SAgencyPickupInfo m_sAgencyPickup; // 0x5E8
};