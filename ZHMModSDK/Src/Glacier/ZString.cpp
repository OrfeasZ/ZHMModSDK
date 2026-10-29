#include <Glacier/ZString.h>

#include "Globals.h"
#include "Functions.h"
#include "ModSDK.h"

ZString::~ZString() {
    Free();
}

void ZString::Allocate(const char* str, uint32_t size) {
    ModSDK::GetInstance()->AllocateZString(this, str, size);
}

void ZString::Free() {
    if (!IsAllocated()) {
        return;
    }

    Functions::ZString_ZImpl_Free->Call(GetImpl());
}