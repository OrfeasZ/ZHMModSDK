#pragma once

#include "IComponentInterface.h"
#include "ISavable.h"
#include "THashMap.h"
#include "THashSet.h"
#include "ZEntity.h"

class ZGameKeywordManager : public IComponentInterface, public ISavable {
public:
    int32_t m_nKeywordIDCounter; // 0x10
    THashMap<int32_t, ZString, TDefaultHashMapPolicy<int32_t>> m_KeywordIDtoStringMap; // 0x18
    THashMap<ZEntityRef, THashSet<int32_t, TDefaultHashSetPolicy<int32_t>>, TDefaultHashMapPolicy<ZEntityRef>> m_HolderToKeywordMap; // 0x38
    THashMap<int32_t, THashSet<ZEntityRef, TDefaultHashSetPolicy<ZEntityRef>>, TDefaultHashMapPolicy<int32_t>> m_KeywordToHolderMap; // 0x58
    THashMap<int32_t, ZEvent<const ZEntityRef&, bool>, TDefaultHashMapPolicy<int32_t>> m_KeywordDelegateMap; // 0x78
    THashMap<ZEntityRef, ZEvent<int32_t, bool>, TDefaultHashMapPolicy<ZEntityRef>> m_EntityDelegateMap; // 0x98
    THashMap<ZString, ZEntityRef, TDefaultHashMapPolicy<ZString>> m_KeywordChannelMap; // 0xB8
    THashMap<ZEntityRef, TArray<ZString>, TDefaultHashMapPolicy<ZEntityRef>> m_EntityToKeywordChannelsMap; // 0xD8
};