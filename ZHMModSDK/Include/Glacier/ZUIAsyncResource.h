#pragma once

#include "ZString.h"

class ZUIAsyncResourceBase {
public:
    PAD(0x10);
    ZString m_sUrl; // Decimal runtime resource ID for local resources, URL for HTTP ones.
};

static_assert(offsetof(ZUIAsyncResourceBase, m_sUrl) == 0x10);

class ZMenuResourceIndex;

class ZMenuResourceIndexOwner {
public:
    PAD(0x10);
    ZMenuResourceIndex* m_pIndex;
};

static_assert(offsetof(ZMenuResourceIndexOwner, m_pIndex) == 0x10);
