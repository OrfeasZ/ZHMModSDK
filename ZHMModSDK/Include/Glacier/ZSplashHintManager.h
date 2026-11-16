#pragma once

#include "ZEntity.h"

class ZSplashHintController;

// Shows one splash hint controller at a time and queues the others.
class ZSplashHintManager {
public:
    struct SQueuedController {
        SQueuedController* m_pNext;
        SQueuedController* m_pPrev;
        TEntityRef<ZSplashHintController> m_rController;
    };

    PAD(0x18);
    TEntityRef<ZSplashHintController> m_rCurrentController; // 0x18
    size_t m_nQueuedControllers; // 0x28
    SQueuedController m_QueuedControllers; // 0x30
};

static_assert(offsetof(ZSplashHintManager, m_rCurrentController) == 0x18);
static_assert(offsetof(ZSplashHintManager, m_nQueuedControllers) == 0x28);
static_assert(offsetof(ZSplashHintManager, m_QueuedControllers) == 0x30);
static_assert(offsetof(ZSplashHintManager::SQueuedController, m_rController) == 0x10);
