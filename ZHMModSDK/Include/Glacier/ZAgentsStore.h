#pragma once

#include "TArray.h"
#include "ZEntity.h"

namespace AI::Private {
    // Indexed by agent index.
    class ZAgentsStoreData {
    public:
        struct SAgent {
            PAD(0x140);

            // The CT_CurrentSetpiece*Distractions conditions skip entries without an entity.
            TArray<ZEntityRef> m_aCurrentSetpieceVisualDistractions; // 0x140
            PAD(0x10); // 0x158
            TArray<ZEntityRef> m_aCurrentSetpieceSoundDistractions; // 0x168
            PAD(0x480); // 0x180
        };

        PAD(0x08);
        TArray<SAgent> m_aAgents; // 0x08
    };
}

namespace AIFramework::Private {
    template <typename TData>
    class TAgentsStore {
    public:
        PAD(0xD0);
        TData* m_pData; // 0xD0
    };
}

// When a table changes, the distraction lists of its agents are rebuilt from its rows.
class ZSetpieceDistractionTable {
public:
    struct SRow {
        PAD(0x06);
        uint16_t m_nAgentIndex; // 0x06
        ZEntityRef m_Entity; // 0x08
    };

    PAD(0x10);
    TArray<SRow> m_aRows; // 0x10
};

class ZSetpieceDistractionTables {
public:
    PAD(0x250);
    ZSetpieceDistractionTable* m_pSoundDistractions; // 0x250
    ZSetpieceDistractionTable* m_pVisualDistractions; // 0x258
};

static_assert(offsetof(AI::Private::ZAgentsStoreData::SAgent, m_aCurrentSetpieceVisualDistractions) == 0x140);
static_assert(offsetof(AI::Private::ZAgentsStoreData::SAgent, m_aCurrentSetpieceSoundDistractions) == 0x168);
static_assert(sizeof(AI::Private::ZAgentsStoreData::SAgent) == 0x600);
static_assert(offsetof(AI::Private::ZAgentsStoreData, m_aAgents) == 0x08);
static_assert(offsetof(AIFramework::Private::TAgentsStore<AI::Private::ZAgentsStoreData>, m_pData) == 0xD0);
static_assert(sizeof(ZSetpieceDistractionTable::SRow) == 0x10);
static_assert(offsetof(ZSetpieceDistractionTable, m_aRows) == 0x10);
static_assert(offsetof(ZSetpieceDistractionTables, m_pSoundDistractions) == 0x250);
static_assert(offsetof(ZSetpieceDistractionTables, m_pVisualDistractions) == 0x258);
