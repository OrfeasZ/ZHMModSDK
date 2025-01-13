#pragma once

#include "ZString.h"

#include <functional>

#include "ZHttp.h"
#include "ZObject.h"

class ZAsyncContext;

class ZServerProxyRoute {
public:
    using TFnRouteMatch = std::function<bool(const ZString&, const ZDynamicObject&, ZDynamicObject&)>;

    using TFnRouteExecute = std::function<void(
        const ZDynamicObject&, std::function<void(const ZDynamicObject&)>, std::function<void(int)>, ZAsyncContext*,
        const SHttpRequestBehavior&
    )>;

    ZString m_sUrl;
    TFnRouteMatch m_fnMatch;
    TFnRouteExecute m_fnExecute;
};
