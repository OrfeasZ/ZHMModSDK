#pragma once

#include "Reflection.h"
#include "TArray.h"

class IContractObjective;
class IContractModule;

struct SAgencyPickupInfo {
    ZRepositoryID m_AgencyPickupId; // 0x0
    TArray<ZRepositoryID> m_aItemIds; // 0x10
    TArray<ZRepositoryID> m_aModifierIds; // 0x28
};

class ZContractsManager : public IComponentInterface {
public:
    struct SContractContext {
        ZString m_sScene; // 0x0
        ZString m_sLocationId; // 0x10
        ZString m_sContractId; // 0x20
        ZRepositoryID m_EnabledEntranceId; // 0x30
        ZRepositoryID m_StartupDisguiseId; // 0x40
        ZString m_OutfitToken; // 0x50
        TArray<IContractObjective*> m_aObjectives; // 0x60
        TArray<ZDynamicObject> m_aGameChangers; // 0x78
        TArray<IContractModule*> m_aModules; // 0x90
        ZDynamicObject m_contractData; // 0xA8
        uint64_t m_Unk2; // 0xB8
        ZRepositoryID m_Unk3; // 0xC0
        uint32_t m_Unk4; // 0xD0
        ZDynamicObject m_Unk5; // 0xD8
        ZDynamicObject m_Unk6; // 0xE8
    };

    PAD(0x5E0);
    SAgencyPickupInfo m_sAgencyPickup; // 0x5E8
    PAD(0x278);
    SContractContext m_contractContext; // 0x8A0
};