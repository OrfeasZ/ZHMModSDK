#pragma once

#include <functional>

#include "ZString.h"
#include "ZObject.h"

struct SSaveLoadStatus;
struct SOnlineSaveResultData;

struct SSaveGameMetaData {
    PAD(0x80);
    bool m_bIsOnline; // 0x80
    bool m_bIsVR; // 0x81
};

static_assert(offsetof(SSaveGameMetaData, m_bIsOnline) == 0x80);
static_assert(offsetof(SSaveGameMetaData, m_bIsVR) == 0x81);

using TFnSaveStatusCallback = std::function<void(
    const SSaveLoadStatus&, const ZString&, const SSaveGameMetaData&&, const SOnlineSaveResultData&&, const ZObjectRef&&
)>;
