#pragma once

#include "ZEntity.h"

class ZRepositoryKeyEntity : public ZEntityImpl
{
public:
    ZRepositoryID m_RepositoryId; // 0x18
};

class ZItemRepositoryKeyEntity : public ZRepositoryKeyEntity
{

};
