#include "ThumbnailCache.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "imgui.h"

#include "Logging.h"
#include "IModSDK.h"
#include "IPluginInterface.h"

static const std::filesystem::path c_AssetThumbnailFolder = "editor_thumbnails";
constexpr size_t c_AssetThumbnailSize = 256;
constexpr size_t c_MaxLoadedThumbnails = 512;

static std::filesystem::path GetThumbnailPath(const ZRuntimeResourceID& p_ResourceId) {
    return c_AssetThumbnailFolder / fmt::format("{:016X}.dds", p_ResourceId.GetID());
}

static std::filesystem::path GetEmptyThumbnailPath(const ZRuntimeResourceID& p_ResourceId) {
    return c_AssetThumbnailFolder / fmt::format("{:016X}.none", p_ResourceId.GetID());
}

// A capture of only the background compresses to identical BC1 blocks.
static bool IsSingleColor(const DirectX::Image& p_Image) {
    constexpr size_t c_BlockSize = 8;

    for (size_t i = c_BlockSize; i + c_BlockSize <= p_Image.slicePitch; i += c_BlockSize) {
        if (std::memcmp(p_Image.pixels + i, p_Image.pixels, c_BlockSize) != 0) {
            return false;
        }
    }

    return true;
}

ThumbnailCache::~ThumbnailCache() {
    if (m_SaveTask.valid()) {
        m_SaveTask.wait();
    }
}

// Needs m_CachedThumbnailsMutex.
std::unordered_map<ZRuntimeResourceID, bool>& ThumbnailCache::GetCachedThumbnails() {
    if (!m_CachedThumbnails) {
        m_CachedThumbnails.emplace();

        std::error_code s_Error;

        for (const auto& s_Entry : std::filesystem::directory_iterator(c_AssetThumbnailFolder, s_Error)) {
            const auto s_Extension = s_Entry.path().extension();

            if (s_Extension != ".dds" && s_Extension != ".none") {
                continue;
            }

            const std::string s_Stem = s_Entry.path().stem().string();
            uint64_t s_Id = 0;

            if (std::from_chars(s_Stem.data(), s_Stem.data() + s_Stem.size(), s_Id, 16).ec != std::errc()) {
                continue;
            }

            (*m_CachedThumbnails)[ZRuntimeResourceID(s_Id)] |= s_Extension == ".none";
        }
    }

    return *m_CachedThumbnails;
}

bool ThumbnailCache::IsCached(const ZRuntimeResourceID& p_ResourceId) {
    std::scoped_lock s_Lock(m_CachedThumbnailsMutex);

    return GetCachedThumbnails().contains(p_ResourceId);
}

void ThumbnailCache::SaveEmpty(const ZRuntimeResourceID& p_ResourceId) {
    std::error_code s_Error;
    std::filesystem::create_directories(c_AssetThumbnailFolder, s_Error);

    const auto s_Path = GetEmptyThumbnailPath(p_ResourceId);

    if (!std::ofstream(s_Path)) {
        Logger::Error("[Editor] Failed to save {}.", s_Path.string());
        return;
    }

    std::scoped_lock s_Lock(m_CachedThumbnailsMutex);
    GetCachedThumbnails()[p_ResourceId] = true;
}

const ImGuiTexture* ThumbnailCache::Get(const ZRuntimeResourceID& p_ResourceId) {
    const int s_Frame = ImGui::GetFrameCount();

    if (const auto s_It = m_Thumbnails.find(p_ResourceId); s_It != m_Thumbnails.end()) {
        s_It->second.LastUsedFrame = s_Frame;
        return &s_It->second.ImGuiTexture;
    }

    {
        std::scoped_lock s_Lock(m_CachedThumbnailsMutex);

        const auto& s_CachedThumbnails = GetCachedThumbnails();
        const auto s_It = s_CachedThumbnails.find(p_ResourceId);

        if (s_It == s_CachedThumbnails.end() || s_It->second) {
            return nullptr;
        }
    }

    // Loading from disk is synchronous, so spread it over frames.
    if (m_LoadFrame != s_Frame) {
        m_LoadFrame = s_Frame;
        m_LoadsThisFrame = 0;
    }

    if (m_LoadsThisFrame >= 4) {
        return nullptr;
    }

    // The textures share ImGui's SRV heap with every other mod, so evict the oldest. Never evict
    // one drawn this or last frame, because it may still be in a draw list.
    if (m_Thumbnails.size() >= c_MaxLoadedThumbnails) {
        const auto s_Oldest = std::ranges::min_element(
            m_Thumbnails,
            {},
            [](const auto& s_Entry) { return s_Entry.second.LastUsedFrame; }
        );

        if (s_Oldest->second.LastUsedFrame >= s_Frame - 1) {
            return nullptr;
        }

        SDK()->DestroyImGuiTexture(s_Oldest->second.Texture, s_Oldest->second.ImGuiTexture);
        m_Thumbnails.erase(s_Oldest);
    }

    ++m_LoadsThisFrame;

    Thumbnail s_Thumbnail {};
    s_Thumbnail.LastUsedFrame = s_Frame;

    if (!SDK()->CreateDDSTextureFromFile(GetThumbnailPath(p_ResourceId).string(), s_Thumbnail.Texture, s_Thumbnail.ImGuiTexture)) {
        // Forget files that don't load, so we don't retry them every frame.
        std::scoped_lock s_Lock(m_CachedThumbnailsMutex);
        GetCachedThumbnails().erase(p_ResourceId);

        return nullptr;
    }

    return &m_Thumbnails.emplace(p_ResourceId, std::move(s_Thumbnail)).first->second.ImGuiTexture;
}

void ThumbnailCache::Save(const ZRuntimeResourceID& p_ResourceId, DirectX::ScratchImage p_Captured) {
    if (m_SaveTask.valid()) {
        m_SaveTask.wait();
    }

    m_SaveTask = std::async(std::launch::async, [this, p_ResourceId, s_Captured = std::move(p_Captured)] {
        const DXGI_FORMAT s_SourceFormat = s_Captured.GetMetadata().format;

        const auto s_LogFailure = [&](const char* p_Step, HRESULT p_Result) {
            Logger::Error(
                "[Editor] Failed to {} the thumbnail of asset {} (HRESULT 0x{:08X}, format {}).",
                p_Step,
                p_ResourceId,
                static_cast<uint32_t>(p_Result),
                static_cast<uint32_t>(s_SourceFormat)
            );
        };

        // Resizing through WIC fails under Wine.
        DirectX::ScratchImage s_Resized;
        HRESULT s_Result = DirectX::Resize(
            *s_Captured.GetImage(0, 0, 0),
            c_AssetThumbnailSize,
            c_AssetThumbnailSize,
            DirectX::TEX_FILTER_FORCE_NON_WIC,
            s_Resized
        );

        if (FAILED(s_Result)) {
            s_LogFailure("resize", s_Result);
            return;
        }

        // The RT's alpha doesn't hold coverage, so a threshold of 0 makes every pixel opaque.
        DirectX::ScratchImage s_Compressed;
        s_Result = DirectX::Compress(
            *s_Resized.GetImage(0, 0, 0),
            DirectX::IsSRGB(s_SourceFormat) ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM,
            DirectX::TEX_COMPRESS_DEFAULT,
            0.f,
            s_Compressed
        );

        if (FAILED(s_Result)) {
            s_LogFailure("compress", s_Result);
            return;
        }

        if (IsSingleColor(*s_Compressed.GetImage(0, 0, 0))) {
            SaveEmpty(p_ResourceId);
            return;
        }

        const auto s_Path = GetThumbnailPath(p_ResourceId);

        std::error_code s_Error;
        std::filesystem::create_directories(c_AssetThumbnailFolder, s_Error);

        s_Result = DirectX::SaveToDDSFile(
            *s_Compressed.GetImage(0, 0, 0), DirectX::DDS_FLAGS_NONE, s_Path.wstring().c_str()
        );

        if (FAILED(s_Result)) {
            s_LogFailure("save", s_Result);
            return;
        }

        std::scoped_lock s_Lock(m_CachedThumbnailsMutex);
        GetCachedThumbnails().try_emplace(p_ResourceId, false);
    });
}

void ThumbnailCache::Clear() {
    if (m_SaveTask.valid()) {
        m_SaveTask.wait();
    }

    {
        std::scoped_lock s_Lock(m_CachedThumbnailsMutex);
        m_CachedThumbnails.emplace();
    }

    for (auto& [s_ResourceId, s_Thumbnail] : m_Thumbnails) {
        SDK()->DestroyImGuiTexture(s_Thumbnail.Texture, s_Thumbnail.ImGuiTexture);
    }

    m_Thumbnails.clear();

    std::error_code s_Error;
    std::filesystem::remove_all(c_AssetThumbnailFolder, s_Error);

    if (s_Error) {
        Logger::Error("[Editor] Failed to delete the asset thumbnail cache: {}", s_Error.message());
    }
}
