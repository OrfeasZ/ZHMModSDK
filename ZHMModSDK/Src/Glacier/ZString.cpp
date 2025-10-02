#include <Glacier/ZString.h>

#include "Globals.h"
#include "Functions.h"
#include "ModSDK.h"

ZString::~ZString() {
    if (IsAllocated()) {
        Functions::ZString_ZImpl_Free->Call(
            reinterpret_cast<ZImpl*>(reinterpret_cast<uintptr_t>(m_pChars) - sizeof(ZImpl))
        );
    }
}

void ZString::Allocate(const char* str, uint32_t size) {
    ModSDK::GetInstance()->AllocateZString(this, str, size);
}