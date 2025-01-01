#pragma once

#include "TArray.h"
#include "Reflection.h"

class IEnumType :
        public IType {
public:
    TArray<ZEnumEntry> m_entries;
};
