#pragma once

#include "ZPrimitives.h"
#include "TArray.h"

class ZKnowledge
{

};

struct SBehaviorBase
{
    uint32_t m_Type;
};

struct SBehaviorData
{
    PAD(0xA0);
    SBehaviorBase* m_pCurrentBehavior; // 0xA0
    PAD(0x50); // 0xA8
};

static_assert(sizeof(SBehaviorData) == 248);

class ZBehaviorService
{
public:
    virtual ~ZBehaviorService() = 0;

public:
    SBehaviorData m_aKnowledgeData[500];
};
