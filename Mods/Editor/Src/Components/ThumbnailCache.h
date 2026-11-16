#pragma once

#include <future>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "D3DUtils.h"
#include "IRenderer.h"

#include "Glacier/ZResourceID.h"

#include <DirectXTex.h>

class ThumbnailCache {
public:
    ~ThumbnailCache();

    const ImGuiTexture* Get(const ZRuntimeResourceID& p_ResourceId);
    void Save(const ZRuntimeResourceID& p_ResourceId, DirectX::ScratchImage p_Captured);
    void Clear();
    bool IsCached(const ZRuntimeResourceID& p_ResourceId);
    void SaveEmpty(const ZRuntimeResourceID& p_ResourceId);

private:
    struct Thumbnail {
        ScopedD3DRef<ID3D12Resource> Texture;
        ImGuiTexture ImGuiTexture;
        int LastUsedFrame;
    };

    std::unordered_map<ZRuntimeResourceID, bool>& GetCachedThumbnails();

    std::unordered_map<ZRuntimeResourceID, Thumbnail> m_Thumbnails;
    int m_LoadFrame = -1;
    int m_LoadsThisFrame = 0;

    std::mutex m_CachedThumbnailsMutex;
    std::optional<std::unordered_map<ZRuntimeResourceID, bool>> m_CachedThumbnails;

    std::future<void> m_SaveTask;
};
