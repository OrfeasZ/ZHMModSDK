#pragma once

#include "ZEntity.h"

class IValueIntGet;

class ZEvergreenCampaignManager :
        public ZEntityImpl {
public:
    TEntityRef<ZEntityImpl> m_ActiveCampaign;
    PAD(0x180);
    TEntityRef<IValueIntGet> m_rSeed;
};

static_assert(offsetof(ZEvergreenCampaignManager, m_rSeed) == 0x1A8);
