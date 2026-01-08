#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <optional>
#include <directx/d3d12.h>
#include <dxgi.h>
#include <unordered_map>
#include <vector>

#include "Hook.h"

namespace Rendering {
    class D3D12Hooks {
    public:
        ~D3D12Hooks();

    public:
        void Startup();

    private:
        DECLARE_DETOUR_WITH_CONTEXT(
            D3D12Hooks, HRESULT, D3D12CreateDevice, IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel,
            REFIID riid, void** ppDevice
        );
        DECLARE_DETOUR_WITH_CONTEXT(D3D12Hooks, HRESULT, CreateDXGIFactory1, REFIID riid, void** ppFactory);
    };
}