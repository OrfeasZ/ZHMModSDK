#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "imgui.h"

#include "Glacier/ZResourceID.h"

#include "AssetPreview.h"
#include "TextureReadback.h"
#include "ThumbnailCache.h"

struct AssetItem {
    std::string Name;
    std::string SearchName;
    std::string Path;
    std::optional<ZRuntimeResourceID> ResId;
    std::vector<std::shared_ptr<AssetItem>> Children;
    size_t FolderCount = 0;
};

// Payload holds a const AssetItem*.
inline constexpr char c_AssetItemPayloadType[] = "ASSET_ITEM";

std::string_view GetAssetItemLabel(const AssetItem& p_Item);

class AssetBrowser {
public:
    explicit AssetBrowser(EntityTreeDeleter& p_TreeDeleter);

    void Draw(const char* p_WindowName);
    void Update(float p_DeltaSeconds);
    void OnClearScene();

    void LoadFactory(const ZRuntimeResourceID& p_ResourceId, TResourcePtr<ZTemplateEntityFactory>& p_Factory);
    static EResourceStatus GetFactoryStatus(const TResourcePtr<ZTemplateEntityFactory>& p_Factory);

    void CaptureThumbnail();
    void ClearHoverPreview();

private:
    enum class CaptureState {
        Idle,
        Loading,
        Settling,
        Capturing,

        Failed
    };

    using FolderPath = std::vector<const AssetItem*>;

    struct TileGroup {
        std::string Path;
        std::vector<const AssetItem*> Items;
        bool IsCollapsed = false;
    };

    struct TileRow {
        size_t Group;
        size_t FirstItem;
        bool IsHeader;
        float Top;
    };

    void UpdateDownload();
    void UpdateTileGroups();
    void CollectSearchResults(const AssetItem& p_Folder, const std::string& p_FolderPath);
    void UpdateTileRows(int p_Columns, float p_HeaderHeight, float p_TileRowHeight);

    void DrawToolbar();
    void DrawFolderTree(float p_Height);
    void DrawFolderRows(const AssetItem& p_Folder, FolderPath& p_RowPath, float p_Width);
    void DrawContent(float p_Height);
    void DrawTile(const TileGroup& p_Group, const AssetItem& p_Item, float p_TileWidth, float p_TileHeight);
    void DrawFooter(float p_Height);
    void DrawHoverPopup();

    const AssetItem& GetCurrentFolder() const;
    bool IsSearching() const;
    bool IsItemShown(const AssetItem& p_Item) const;

    const ImGuiTexture* GetThumbnail(const ZRuntimeResourceID& p_ResourceId, const std::string& p_Path);
    const ImGuiTexture* GetHoverPreviewTexture(const ZRuntimeResourceID& p_ResourceId);
    void ClearThumbnailCache();
    void QueueThumbnails(const std::vector<std::pair<ZRuntimeResourceID, std::string>>& p_Items);
    void SetHoverPreview(const ZRuntimeResourceID& p_ResourceId, const std::string& p_Path);
    size_t GetQueuedThumbnailCount();

    void PrefetchQueued();
    bool MountChunk(const ZRuntimeResourceID& p_ResourceId);
    void UpdateLoading();
    void UpdateSettling(bool p_Reframed);
    void StartCapture(const ZRuntimeResourceID& p_ResourceId, bool p_IsHover);
    void AbortCapture();
    void DestroyPreview();
    std::string GetItemPath(const ZRuntimeResourceID& p_ResourceId);

    std::shared_ptr<AssetItem> m_Root;
    std::future<std::shared_ptr<AssetItem>> m_Download;

    FolderPath m_CurrentPath;
    std::vector<FolderPath> m_History;
    std::string m_SearchFilter;
    bool m_IsSearchEverywhere = true;
    bool m_ShowTemplates = true;
    bool m_ShowPrims = true;
    int m_TileSize = 60;

    std::vector<TileGroup> m_TileGroups;
    size_t m_TileCount = 0;
    std::string m_TileGroupsQuery;
    std::string m_TileGroupsLowerQuery;
    bool m_TileGroupsEverywhere = true;
    bool m_TileGroupsShowTemplates = true;
    bool m_TileGroupsShowPrims = true;
    const AssetItem* m_TileGroupsFolder = nullptr;

    std::vector<TileRow> m_TileRows;
    float m_TileRowsHeight = 0.f;
    int m_TileRowsColumns = 0;
    float m_TileRowsTileRowHeight = 0.f;
    bool m_AreTileRowsStale = true;

    // Navigation is applied after drawing, because the content view iterates the current folder.
    std::optional<FolderPath> m_NextPath;
    bool m_IsGoingBack = false;
    const AssetItem* m_HoveredItem = nullptr;
    const TileGroup* m_HoveredGroup = nullptr;
    ImVec2 m_HoveredTileMin;

    AssetPreview m_AssetPreview;
    ThumbnailCache m_ThumbnailCache;

    std::mutex m_QueueMutex;
    std::deque<ZRuntimeResourceID> m_Queue;
    std::optional<ZRuntimeResourceID> m_HoverRequest;

    std::unordered_map<ZRuntimeResourceID, std::string> m_ItemPaths;

    std::optional<TResourcePtr<ZTemplateEntityFactory>> m_PreviewFactory;

    // TResourcePtr's assignment doesn't count references, so these must not move.
    std::unordered_map<ZRuntimeResourceID, TResourcePtr<ZTemplateEntityFactory>> m_Prefetched;

    float m_Yaw = 0.f;
    CaptureState m_CaptureState = CaptureState::Idle;
    // The render thread reads this while m_CaptureRequested is set.
    ZRuntimeResourceID m_CaptureResourceId {};
    int m_CaptureFrames = 0;
    int m_CaptureStableFrames = 0;
    bool m_WasPreviewInUse = false;
    bool m_CaptureIsHover = false;
    std::atomic<bool> m_CaptureRequested = false;

    std::atomic<uint64_t> m_HoverShownResourceId = 0;

    std::unordered_set<ZRuntimeResourceID> m_ThumbnailsRequested;

    TextureReadback m_Readback;
    ZRuntimeResourceID m_ReadbackResourceId {};
};
