#pragma once

#include "Globals.h"
#include "Glacier/ZServerProxyRoute.h"

inline ZServerProxyRoute* FindServerProxyRoute(const ZString& p_RouteName) {
    if (!Globals::ZProfileServerPageProxyBase_m_aRouteMap) {
        return nullptr;
    }

    for (auto& s_Pair : *Globals::ZProfileServerPageProxyBase_m_aRouteMap) {
        if (s_Pair.first == p_RouteName) {
            return s_Pair.second;
        }
    }

    return nullptr;
}

// Restoring drops later overrides of the same route, so destroy them in reverse order.
class ServerProxyRouteOverride {
public:
    using TFnExecute = std::function<void(
        const ZServerProxyRoute::TFnRouteExecute& p_Original, const ZDynamicObject& p_Request,
        std::function<void(const ZDynamicObject&)> p_OkCb, std::function<void(int)> p_ErrorCb,
        ZAsyncContext* p_AsyncContext, const SHttpRequestBehavior& p_Behavior
    )>;

    ServerProxyRouteOverride(const ZString& p_RouteName, TFnExecute p_Execute) {
        m_Route = FindServerProxyRoute(p_RouteName);

        if (!m_Route) {
            return;
        }

        m_Original = m_Route->m_fnExecute;

        m_Route->m_fnExecute = [s_Original = m_Original, s_Execute = std::move(p_Execute)](
            const ZDynamicObject& p_Request,
            std::function<void(const ZDynamicObject&)> p_OkCb,
            std::function<void(int)> p_ErrorCb,
            ZAsyncContext* p_AsyncContext,
            const SHttpRequestBehavior& p_Behavior
        ) {
            s_Execute(s_Original, p_Request, std::move(p_OkCb), std::move(p_ErrorCb), p_AsyncContext, p_Behavior);
        };
    }

    ~ServerProxyRouteOverride() {
        if (m_Route) {
            m_Route->m_fnExecute = m_Original;
        }
    }

    ServerProxyRouteOverride(const ServerProxyRouteOverride&) = delete;
    ServerProxyRouteOverride& operator=(const ServerProxyRouteOverride&) = delete;

private:
    ZServerProxyRoute* m_Route = nullptr;
    ZServerProxyRoute::TFnRouteExecute m_Original;
};
