#include "AssetBrowser.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "IPluginInterface.h"
#include "SpawnSafety.h"
#include "Logging.h"
#include "IconsMaterialDesign.h"
#include "Util/StringUtils.h"
#include "Widgets.h"

#include <Globals.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZScene.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZResource.h>

AssetBrowser::AssetBrowser(EntityTreeDeleter& p_TreeDeleter) : m_AssetPreview(p_TreeDeleter) {}

void AssetBrowser::OnClearScene() {
    AbortCapture();
    m_Prefetched.clear();
    m_AssetPreview.Destroy();
}

void AssetBrowser::SetHoverPreview(const ZRuntimeResourceID& p_ResourceId, const std::string& p_Path) {
    std::scoped_lock s_Lock(m_QueueMutex);
    m_HoverRequest = p_ResourceId;
    m_ItemPaths.try_emplace(p_ResourceId, p_Path);
}

void AssetBrowser::ClearHoverPreview() {
    std::scoped_lock s_Lock(m_QueueMutex);
    m_HoverRequest.reset();
}

std::string AssetBrowser::GetItemPath(const ZRuntimeResourceID& p_ResourceId) {
    std::scoped_lock s_Lock(m_QueueMutex);
    const auto s_It = m_ItemPaths.find(p_ResourceId);
    return s_It != m_ItemPaths.end() ? s_It->second : "<unknown path>";
}

size_t AssetBrowser::GetQueuedThumbnailCount() {
    std::scoped_lock s_Lock(m_QueueMutex);
    return m_Queue.size();
}

const ImGuiTexture* AssetBrowser::GetHoverPreviewTexture(const ZRuntimeResourceID& p_ResourceId) {
    if (m_HoverShownResourceId != p_ResourceId.GetID()) {
        return nullptr;
    }

    return m_AssetPreview.GetRTTexture();
}

const ImGuiTexture* AssetBrowser::GetThumbnail(const ZRuntimeResourceID& p_ResourceId, const std::string& p_Path) {
    if (const auto s_Thumbnail = m_ThumbnailCache.Get(p_ResourceId)) {
        return s_Thumbnail;
    }

    if (m_ThumbnailsRequested.insert(p_ResourceId).second && !m_ThumbnailCache.IsCached(p_ResourceId)) {
        std::scoped_lock s_Lock(m_QueueMutex);
        m_Queue.push_back(p_ResourceId);
        m_ItemPaths.try_emplace(p_ResourceId, p_Path);
    }

    return nullptr;
}

void AssetBrowser::QueueThumbnails(const std::vector<std::pair<ZRuntimeResourceID, std::string>>& p_Items) {
    std::scoped_lock s_Lock(m_QueueMutex);

    for (const auto& [s_ResourceId, s_Path] : p_Items) {
        if (!m_ThumbnailCache.IsCached(s_ResourceId)) {
            m_Queue.push_back(s_ResourceId);
            m_ItemPaths.try_emplace(s_ResourceId, s_Path);
        }
    }
}

void AssetBrowser::ClearThumbnailCache() {
    {
        std::scoped_lock s_Lock(m_QueueMutex);
        m_Queue.clear();
    }

    m_ThumbnailCache.Clear();
    m_ThumbnailsRequested.clear();
}

void AssetBrowser::Update(float p_DeltaSeconds) {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if ((*Globals::ApplicationEngineWin32)->m_bSceneLoaded && s_Scene) {
        m_AssetPreview.Spawn();
    }

    if (!m_AssetPreview.IsReady()) {
        return;
    }

    std::optional<ZRuntimeResourceID> s_HoverRequest;

    {
        std::scoped_lock s_Lock(m_QueueMutex);
        s_HoverRequest = m_HoverRequest;
    }

    PrefetchQueued();

    const bool s_MustWait = m_AssetPreview.IsPreviewInUse();

    // A spawner's item is still pending registration with ZDistanceConditionManager in the
    // frame it's created, so we delete the preview one frame after the wait ends.
    const bool s_CanDestroyPreview = !s_MustWait && !m_WasPreviewInUse;
    m_WasPreviewInUse = s_MustWait;

    const bool s_HoverChanged = m_CaptureIsHover
        ? s_HoverRequest != m_CaptureResourceId
        : s_HoverRequest.has_value();

    if (s_HoverChanged && m_CaptureState != CaptureState::Capturing && s_CanDestroyPreview) {
        AbortCapture();

        if (s_HoverRequest) {
            StartCapture(*s_HoverRequest, true);
        }
    }

    if (m_CaptureState == CaptureState::Idle && s_CanDestroyPreview) {
        std::optional<ZRuntimeResourceID> s_Next;

        {
            std::scoped_lock s_Lock(m_QueueMutex);

            // Skip items captured since they were queued.
            while (!s_Next && !m_Queue.empty()) {
                if (!m_ThumbnailCache.IsCached(m_Queue.front())) {
                    s_Next = m_Queue.front();
                }

                m_Queue.pop_front();
            }
        }

        if (!s_Next) {
            DestroyPreview();
            m_AssetPreview.StopForceRenderingRT();
            return;
        }

        StartCapture(*s_Next, false);
    }

    if (!m_AssetPreview.ForceRenderRT()) {
        return;
    }

    if (m_CaptureIsHover) {
        m_Yaw = std::fmod(m_Yaw + p_DeltaSeconds * 0.8f, 2.f * std::numbers::pi_v<float>);
    }

    const bool s_Reframed = m_AssetPreview.FramePreview(m_Yaw);

    switch (m_CaptureState) {
        case CaptureState::Idle:
        case CaptureState::Failed:
            break;

        case CaptureState::Loading:
            UpdateLoading();
            break;

        case CaptureState::Settling:
            UpdateSettling(s_Reframed);
            break;

        case CaptureState::Capturing: {
            if (!m_CaptureRequested) {
                m_CaptureState = CaptureState::Idle;
            }

            break;
        }
    }
}

void AssetBrowser::UpdateLoading() {
    ++m_CaptureFrames;

    const auto& s_Factory = *m_PreviewFactory;

    const auto s_Status = GetFactoryStatus(s_Factory);

    if (s_Status == RESOURCE_STATUS_VALID) {
        const std::string s_ItemName = fmt::format(
            "{} ({})", GetItemPath(m_CaptureResourceId), m_CaptureResourceId
        );

        if (m_AssetPreview.SpawnPreview(s_Factory.GetResource(), s_ItemName)) {
            m_CaptureState = CaptureState::Settling;
            m_CaptureFrames = 0;
            m_CaptureStableFrames = 0;
            return;
        }
    }
    else if (s_Status != RESOURCE_STATUS_FAILED && m_CaptureFrames < 600) {
        return;
    }

    Logger::Warn(
        "[Editor] Could not load asset {} ({}): resource status {}, {} frame(s).",
        GetItemPath(m_CaptureResourceId),
        m_CaptureResourceId,
        static_cast<int>(s_Status),
        m_CaptureFrames
    );

    DestroyPreview();
    m_CaptureState = m_CaptureIsHover ? CaptureState::Failed : CaptureState::Idle;
}

void AssetBrowser::UpdateSettling(const bool p_Reframed) {
    ++m_CaptureFrames;
    m_CaptureStableFrames = p_Reframed ? 0 : m_CaptureStableFrames + 1;

    // Wait for bounds that arrive late. The first captures after the RT starts rendering
    // come out blank.
    const bool s_IsSettled =
        ((m_CaptureFrames >= 3 && m_CaptureStableFrames >= 2) || m_CaptureFrames >= 120) &&
        m_AssetPreview.GetForcedRenderFrameCount() >= 10;

    if (!s_IsSettled || m_HoverShownResourceId == m_CaptureResourceId.GetID()) {
        return;
    }

    if (m_CaptureIsHover) {
        m_HoverShownResourceId = m_CaptureResourceId.GetID();
        return;
    }

    // Even empty items should wait to settle to prevent issues when deleting.
    if (!m_AssetPreview.HasRenderables()) {
        m_ThumbnailCache.SaveEmpty(m_CaptureResourceId);
        m_CaptureState = CaptureState::Idle;
        return;
    }

    m_CaptureRequested = true;
    m_CaptureState = CaptureState::Capturing;
}

void AssetBrowser::PrefetchQueued() {
    constexpr size_t c_PrefetchCount = 4;

    std::unordered_set<ZRuntimeResourceID> s_Next;

    {
        std::scoped_lock s_Lock(m_QueueMutex);

        for (const auto& s_ResourceId : m_Queue) {
            if (s_Next.size() >= c_PrefetchCount) {
                break;
            }

            if (!m_ThumbnailCache.IsCached(s_ResourceId)) {
                s_Next.insert(s_ResourceId);
            }
        }
    }

    std::erase_if(m_Prefetched, [&](const auto& s_Entry) { return !s_Next.contains(s_Entry.first); });

    for (const auto& s_ResourceId : s_Next) {
        const auto [s_It, s_IsNew] = m_Prefetched.try_emplace(s_ResourceId);

        if (s_IsNew) {
            LoadFactory(s_ResourceId, s_It->second);
        }
    }
}

void AssetBrowser::StartCapture(const ZRuntimeResourceID& p_ResourceId, bool p_IsHover) {
    DestroyPreview();

    m_CaptureIsHover = p_IsHover;
    m_CaptureResourceId = p_ResourceId;

    // setpiece_mumbai_hips_sniper_scope switches the view to the default postfilter params,
    // which blur it until the game restarts, so skip it unless you want vaseline on your screen.
    if (p_ResourceId == ZRuntimeResourceID(0x00DD4DCFB08B1961)) {
        if (!p_IsHover) {
            m_ThumbnailCache.SaveEmpty(p_ResourceId);
        }

        m_CaptureState = p_IsHover ? CaptureState::Failed : CaptureState::Idle;
        return;
    }

    m_CaptureState = CaptureState::Loading;
    m_CaptureFrames = 0;

    if (const auto s_Prefetched = m_Prefetched.find(p_ResourceId); s_Prefetched != m_Prefetched.end()) {
        m_PreviewFactory.emplace(s_Prefetched->second);
        m_Prefetched.erase(s_Prefetched);
    }
    else {
        LoadFactory(p_ResourceId, m_PreviewFactory.emplace());
    }
}

void AssetBrowser::LoadFactory(const ZRuntimeResourceID& p_ResourceId, TResourcePtr<ZTemplateEntityFactory>& p_Factory) {
    if (MountChunk(p_ResourceId)) {
        Globals::ResourceManager->GetResourcePtr(p_Factory, p_ResourceId, 0);
    }
}

EResourceStatus AssetBrowser::GetFactoryStatus(const TResourcePtr<ZTemplateEntityFactory>& p_Factory) {
    return p_Factory.m_nResourceIndex.val < 0 ? RESOURCE_STATUS_FAILED : p_Factory.GetResourceInfo().status;
}

bool AssetBrowser::MountChunk(const ZRuntimeResourceID& p_ResourceId) {
    auto& s_Indices = (*Globals::ResourceContainer)->m_indices;

    if (s_Indices.find(p_ResourceId) != s_Indices.end()) {
        return true;
    }

    const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(p_ResourceId);

    if (s_ChunkIndices.size() == 0) {
        Logger::Warn("[Editor] Asset {} is in no chunk.", p_ResourceId);
        return false;
    }

    const uint32_t s_ChunkIndex = s_ChunkIndices[0];

    IPackageManager::SPartitionInfo* s_Partition = nullptr;

    for (auto* s_Info : (*Globals::PackageManager)->m_aPartitionInfos) {
        if (s_Info->m_nIndex == s_ChunkIndex) {
            s_Partition = s_Info;
            break;
        }
    }

    size_t s_NewPackageCount = 0;

    for (auto* s_Info = s_Partition; s_Info; s_Info = s_Info->m_pParent) {
        if (!SDK()->IsChunkMounted(s_Info->m_nIndex)) {
            s_NewPackageCount += SDK()->GetResourcePackageCount(s_Info->m_nIndex);
        }
    }

    if (s_NewPackageCount == 0) {
        return true;
    }

    const size_t s_MountedPackageCount = (*Globals::ResourceContainer)->m_MountedPackages.size();

    const size_t s_MaxPackageCount = SDK()->GetMaxResourcePackageCount();

    if (s_MountedPackageCount + s_NewPackageCount > s_MaxPackageCount) {
        Logger::Warn(
            "[Editor] Not mounting chunk {} for asset {}: it needs {} more resource package(s), {} are mounted "
            "and the engine supports {}.",
            s_ChunkIndex,
            p_ResourceId,
            s_NewPackageCount,
            s_MountedPackageCount,
            s_MaxPackageCount
        );
        return false;
    }

    SDK()->MountChunk(s_ChunkIndex);

    return true;
}

void AssetBrowser::DestroyPreview() {
    m_AssetPreview.DestroyPreview();
    m_PreviewFactory.reset();
    m_Yaw = 0.f;
}

void AssetBrowser::AbortCapture() {
    if (m_CaptureState != CaptureState::Idle) {
        if (!m_CaptureIsHover) {
            std::scoped_lock s_Lock(m_QueueMutex);
            m_Queue.push_front(m_CaptureResourceId);
        }
    }

    DestroyPreview();

    m_CaptureState = CaptureState::Idle;
    m_CaptureIsHover = false;
    m_CaptureRequested = false;
    m_HoverShownResourceId = 0;
}

// The game thread can move on once the copy is submitted, because the game renders the
// next item on the same queue, after the copy.
void AssetBrowser::CaptureThumbnail() {
    if (auto s_Captured = m_Readback.Finish()) {
        m_ThumbnailCache.Save(m_ReadbackResourceId, std::move(*s_Captured));
    }

    if (!m_CaptureRequested || m_Readback.IsBusy()) {
        return;
    }

    const ZRuntimeResourceID s_ResourceId = m_CaptureResourceId;
    const auto s_RT = m_AssetPreview.GetRTResource();

    constexpr D3D12_RESOURCE_STATES s_State =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    if (s_RT && m_Readback.Start(s_RT, s_State)) {
        m_ReadbackResourceId = s_ResourceId;
    }
    else if (s_RT) {
        Logger::Error("[Editor] Failed to capture the thumbnail of asset {}.", s_ResourceId);
    }

    m_CaptureRequested = false;
}

static bool IsAssetFolder(const AssetItem& p_Item) {
    return !p_Item.ResId;
}

static void DrawAssetItemContextMenu(const AssetItem& p_Item) {
    if (IsAssetFolder(p_Item) || !ImGui::BeginPopupContextItem()) {
        return;
    }

    if (ImGui::MenuItem("Copy path")) {
        ImGui::SetClipboardText(p_Item.Path.c_str());
    }

    if (ImGui::MenuItem("Copy resource ID")) {
        ImGui::SetClipboardText(fmt::format("{:016X}", p_Item.ResId->GetID()).c_str());
    }

    ImGui::EndPopup();
}

std::string_view GetAssetItemLabel(const AssetItem& p_Item) {
    std::string_view s_Label = p_Item.Name;

    if (s_Label.ends_with(".entitytemplate")) {
        s_Label.remove_suffix(std::string_view(".entitytemplate").size());
    }
    else if (s_Label.ends_with(".prim")) {
        s_Label.remove_suffix(std::string_view(".prim").size());
    }

    return s_Label;
}

static void SortAssetChildren(AssetItem& p_Item) {
    std::ranges::sort(
        p_Item.Children,
        [](const std::shared_ptr<AssetItem>& a, const std::shared_ptr<AssetItem>& b) {
            if (IsAssetFolder(*a) != IsAssetFolder(*b)) {
                return IsAssetFolder(*a);
            }

            return a->Name < b->Name;
        }
    );

    p_Item.FolderCount = std::ranges::count_if(p_Item.Children, [](const std::shared_ptr<AssetItem>& p_Child) {
        return IsAssetFolder(*p_Child);
    });

    for (const auto& s_Child : p_Item.Children) {
        SortAssetChildren(*s_Child);
    }
}

static std::shared_ptr<AssetItem> BuildAssetTree() {
    std::vector<std::pair<ZRuntimeResourceID, std::string>> s_Files;

    SDK()->EnumerateResourceIDs(
        "TEMP",
        1u << 3, // If bit 3 is not set, it means this file is not in HM3, so we skip it.
        [](const ZRuntimeResourceID& p_Hash,
            const ResourceIDInfo& p_Info,
            void* p_UserData) -> bool {
                auto& s_Files = *static_cast<
                    std::vector<std::pair<ZRuntimeResourceID, std::string>>*
                >(p_UserData);

                if (p_Info.m_Path && !FindUnspawnablePath(p_Info.m_Path)) {
                    s_Files.emplace_back(p_Hash, p_Info.m_Path);
                }

                return true;
        },
        &s_Files
    );

    if (s_Files.empty()) {
        return nullptr;
    }

    // Create a tree using BFS.
    std::map<std::string, std::shared_ptr<AssetItem>> s_PathToNode;
    const auto s_Root = std::make_shared<AssetItem>();

    s_Root->Name = "Root";

    for (const auto& [s_ResId, s_Path] : s_Files) {
        std::string s_CleanPath = s_Path;

        // Remove [assembly: prefix and ].pc_entitytype or ].pc_entitytemplate suffix
        if (s_CleanPath.starts_with("[assembly:")) {
            s_CleanPath = s_CleanPath.substr(std::string("[assembly:").length());
        }

        if (s_CleanPath.ends_with("].pc_entitytype")) {
            s_CleanPath = s_CleanPath.substr(0, s_CleanPath.length() - std::string("].pc_entitytype").length());
        }
        else if (s_CleanPath.ends_with("].pc_entitytemplate")) {
            s_CleanPath = s_CleanPath.substr(0, s_CleanPath.length() - std::string("].pc_entitytemplate").length());
        }

        // If it doesn't end in .entitytemplate or .prim, skip it.
        if (!s_CleanPath.ends_with(".entitytemplate") && !s_CleanPath.ends_with(".prim")) {
            continue;
        }

        // Split path into segments
        std::vector<std::string> s_Segments;
        std::string s_CurrentSegment;

        for (char c : s_CleanPath) {
            if (c == '/') {
                if (!s_CurrentSegment.empty()) {
                    s_Segments.push_back(s_CurrentSegment);
                    s_CurrentSegment.clear();
                }
            }
            else {
                s_CurrentSegment += c;
            }
        }

        if (!s_CurrentSegment.empty()) {
            s_Segments.push_back(s_CurrentSegment);
        }

        // Build tree path by path
        std::shared_ptr<AssetItem> s_CurrentParent = s_Root;
        std::string s_CurrentPath;

        for (size_t i = 0; i < s_Segments.size(); ++i) {
            if (!s_CurrentPath.empty()) {
                s_CurrentPath += "/";
            }
            s_CurrentPath += s_Segments[i];

            // Check if this path node already exists
            auto s_It = s_PathToNode.find(s_CurrentPath);
            std::shared_ptr<AssetItem> s_CurrentNode;

            if (s_It != s_PathToNode.end()) {
                s_CurrentNode = s_It->second;
            }
            else {
                // Create new node
                s_CurrentNode = std::make_shared<AssetItem>();
                s_CurrentNode->Name = s_Segments[i];

                s_CurrentNode->SearchName = Util::StringUtils::ToLowerCase(std::string(GetAssetItemLabel(*s_CurrentNode)));

                // If this is the last segment, assign the resource ID
                if (i == s_Segments.size() - 1) {
                    s_CurrentNode->ResId = s_ResId;
                    s_CurrentNode->Path = s_Path;
                }

                s_PathToNode[s_CurrentPath] = s_CurrentNode;

                s_CurrentParent->Children.push_back(s_CurrentNode);
            }

            s_CurrentParent = s_CurrentNode;
        }
    }

    SortAssetChildren(*s_Root);

    return s_Root;
}

static constexpr ImU32 c_NoThumbnailColor = IM_COL32(0x26, 0x27, 0x2c, 0xff);
static constexpr ImU32 c_PrimBadgeColor = IM_COL32(0x2e, 0x7d, 0x32, 0xff);
static constexpr ImU32 c_TemplateBadgeColor = IM_COL32(0x1f, 0x5f, 0xbf, 0xff);

// Font sizes in canvas pixels.
static constexpr float c_FolderTileIconFontSize = 24.f;
static constexpr float c_LargeIconFontSize = 18.f;
static constexpr float c_TypeBadgeFontSize = 9.f;

static std::string GetFolderPathString(const std::vector<const AssetItem*>& p_Path) {
    std::string s_Result;

    for (const AssetItem* s_Folder : p_Path) {
        if (!s_Result.empty()) {
            s_Result += "/";
        }

        s_Result += s_Folder->Name;
    }

    return s_Result;
}

static void CollectTemplates(
    const AssetItem& p_Folder, std::vector<std::pair<ZRuntimeResourceID, std::string>>& p_Templates
) {
    for (const auto& s_Item : p_Folder.Children) {
        if (IsAssetFolder(*s_Item)) {
            CollectTemplates(*s_Item, p_Templates);
        }
        else {
            p_Templates.emplace_back(*s_Item->ResId, s_Item->Path);
        }
    }
}

static void DrawAssetThumbnail(ImDrawList* p_DrawList, const ImVec2& p_Min, const ImVec2& p_Max, const ImGuiTexture* p_Texture) {
    if (p_Texture) {
        p_DrawList->AddImage(p_Texture->id, p_Min, p_Max);
        return;
    }

    static constexpr const char* c_NoThumbnail = "No thumbnail";

    const ImVec2 s_Size(p_Max.x - p_Min.x, p_Max.y - p_Min.y);
    const float s_WrapWidth = s_Size.x - ScaledSize(8.f);
    const ImVec2 s_TextSize = ImGui::CalcTextSize(c_NoThumbnail, nullptr, false, s_WrapWidth);

    p_DrawList->AddRectFilled(p_Min, p_Max, c_NoThumbnailColor);
    p_DrawList->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize(),
        ImVec2(p_Min.x + (s_Size.x - s_TextSize.x) * 0.5f, p_Min.y + (s_Size.y - s_TextSize.y) * 0.5f),
        ImGui::GetColorU32(c_MutedTextColor),
        c_NoThumbnail,
        nullptr,
        s_WrapWidth
    );
}

static bool IsPrimAsset(const AssetItem& p_Item) {
    return p_Item.Name.ends_with(".prim");
}

static void DrawTypeBadge(ImDrawList* p_DrawList, const ImVec2& p_ThumbMax, const AssetItem& p_Item) {
    const bool s_IsPrim = IsPrimAsset(p_Item);
    const char* s_Text = s_IsPrim ? "PRIM" : "TEMP";

    ImGui::PushFont(nullptr, FontSize(c_TypeBadgeFontSize));

    const ImVec2 s_TextSize = ImGui::CalcTextSize(s_Text);
    const ImVec2 s_Padding(ScaledSize(3.f), ScaledSize(1.f));
    const ImVec2 s_BadgeMin(
        p_ThumbMax.x - s_TextSize.x - s_Padding.x * 2.f,
        p_ThumbMax.y - s_TextSize.y - s_Padding.y * 2.f
    );

    p_DrawList->AddRectFilled(s_BadgeMin, p_ThumbMax, s_IsPrim ? c_PrimBadgeColor : c_TemplateBadgeColor);
    p_DrawList->AddText(
        ImVec2(s_BadgeMin.x + s_Padding.x, s_BadgeMin.y + s_Padding.y),
        IM_COL32_WHITE,
        s_Text
    );

    ImGui::PopFont();
}

static void DrawTileName(
    ImVec2 p_Pos, const float p_Width, const std::string_view p_Text, const size_t p_HighlightStart,
    const size_t p_HighlightEnd
) {
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
    ImFont* s_Font = ImGui::GetFont();
    const float s_FontSize = ImGui::GetFontSize();
    const float s_LineHeight = ImGui::GetTextLineHeight();

    const char* s_TextStart = p_Text.data();
    const char* s_TextEnd = s_TextStart + p_Text.size();
    const char* s_LineStart = s_TextStart;

    s_DrawList->PushClipRect(p_Pos, ImVec2(p_Pos.x + p_Width, p_Pos.y + s_LineHeight * 2.f), true);

    for (int s_Line = 0; s_Line < 2 && s_LineStart < s_TextEnd; ++s_Line) {
        const char* s_LineEnd = s_Line == 1
            ? s_TextEnd
            : s_Font->CalcWordWrapPosition(s_FontSize, s_LineStart, s_TextEnd, p_Width);

        if (s_LineEnd == s_LineStart) {
            ++s_LineEnd;
        }

        // Split the line where the highlight starts and ends.
        const char* s_HighlightStart = std::clamp(s_TextStart + p_HighlightStart, s_LineStart, s_LineEnd);
        const char* s_HighlightEnd = std::clamp(s_TextStart + p_HighlightEnd, s_LineStart, s_LineEnd);

        const std::pair<const char*, const char*> s_Pieces[] = {
            { s_LineStart, s_HighlightStart },
            { s_HighlightStart, s_HighlightEnd },
            { s_HighlightEnd, s_LineEnd },
        };

        float s_X = p_Pos.x;

        for (size_t i = 0; i < std::size(s_Pieces); ++i) {
            const auto [s_PieceStart, s_PieceEnd] = s_Pieces[i];

            if (s_PieceStart == s_PieceEnd) {
                continue;
            }

            const ImU32 s_Color = i == 1
                ? ImGui::GetColorU32(c_HighlightTextColor)
                : ImGui::GetColorU32(ImGuiCol_Text);

            s_DrawList->AddText(s_Font, s_FontSize, ImVec2(s_X, p_Pos.y), s_Color, s_PieceStart, s_PieceEnd);
            s_X += s_Font->CalcTextSizeA(s_FontSize, FLT_MAX, 0.f, s_PieceStart, s_PieceEnd).x;
        }

        p_Pos.y += s_LineHeight;
        s_LineStart = s_LineEnd;

        while (s_LineStart < s_TextEnd && *s_LineStart == ' ') {
            ++s_LineStart;
        }
    }

    s_DrawList->PopClipRect();
}

const AssetItem& AssetBrowser::GetCurrentFolder() const {
    return m_CurrentPath.empty() ? *m_Root : *m_CurrentPath.back();
}

bool AssetBrowser::IsSearching() const {
    return !m_SearchFilter.empty();
}

bool AssetBrowser::IsItemShown(const AssetItem& p_Item) const {
    if (IsAssetFolder(p_Item)) {
        return true;
    }

    return IsPrimAsset(p_Item) ? m_ShowPrims : m_ShowTemplates;
}

void AssetBrowser::UpdateAssetTree() {
    if (m_Root || !SDK()->AreResourceIDsLoaded()) {
        return;
    }

    if (!m_BuildFuture.valid()) {
        m_BuildFuture = std::async(std::launch::async, [] {
            return BuildAssetTree();
        });
    }

    if (m_BuildFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        m_Root = m_BuildFuture.get();

        Logger::Debug(
            "[Editor] Asset tree built successfully! Loaded {} root items.",
            m_Root->Children.size()
        );
    }
}

void AssetBrowser::UpdateTileGroups() {
    const AssetItem& s_CurrentFolder = GetCurrentFolder();

    if (m_TileGroupsQuery == m_SearchFilter && m_TileGroupsEverywhere == m_IsSearchEverywhere &&
        m_TileGroupsFolder == &s_CurrentFolder && m_TileGroupsShowTemplates == m_ShowTemplates &&
        m_TileGroupsShowPrims == m_ShowPrims) {
        return;
    }

    m_TileGroupsQuery = m_SearchFilter;
    m_TileGroupsLowerQuery = Util::StringUtils::ToLowerCase(m_SearchFilter);
    m_TileGroupsEverywhere = m_IsSearchEverywhere;
    m_TileGroupsFolder = &s_CurrentFolder;
    m_TileGroupsShowTemplates = m_ShowTemplates;
    m_TileGroupsShowPrims = m_ShowPrims;

    m_TileGroups.clear();
    m_AreTileRowsStale = true;

    const std::string s_CurrentFolderPath = GetFolderPathString(m_CurrentPath);

    if (!IsSearching()) {
        TileGroup s_Group { s_CurrentFolderPath.empty() ? m_Root->Name : s_CurrentFolderPath };

        for (const auto& s_Item : s_CurrentFolder.Children) {
            if (IsItemShown(*s_Item)) {
                s_Group.Items.push_back(s_Item.get());
            }
        }

        m_TileGroups.push_back(std::move(s_Group));
    }
    else if (m_IsSearchEverywhere) {
        CollectSearchResults(*m_Root, "");
    }
    else {
        CollectSearchResults(s_CurrentFolder, s_CurrentFolderPath);
    }

    m_TileCount = 0;

    for (const TileGroup& s_Group : m_TileGroups) {
        m_TileCount += s_Group.Items.size();
    }
}

void AssetBrowser::CollectSearchResults(const AssetItem& p_Folder, const std::string& p_FolderPath) {
    TileGroup s_Group { p_FolderPath.empty() ? m_Root->Name : p_FolderPath };

    for (size_t i = p_Folder.FolderCount; i < p_Folder.Children.size(); ++i) {
        const AssetItem& s_Template = *p_Folder.Children[i];

        if (IsItemShown(s_Template) && s_Template.SearchName.find(m_TileGroupsLowerQuery) != std::string::npos) {
            s_Group.Items.push_back(&s_Template);
        }
    }

    if (!s_Group.Items.empty()) {
        m_TileGroups.push_back(std::move(s_Group));
    }

    for (size_t i = 0; i < p_Folder.FolderCount; ++i) {
        const AssetItem& s_Subfolder = *p_Folder.Children[i];

        CollectSearchResults(s_Subfolder, p_FolderPath.empty() ? s_Subfolder.Name : p_FolderPath + "/" + s_Subfolder.Name);
    }
}

// Lay out the rows once and only draw the visible ones.
void AssetBrowser::UpdateTileRows(const int p_Columns, const float p_HeaderHeight, const float p_TileRowHeight) {
    if (!m_AreTileRowsStale && m_TileRowsColumns == p_Columns && m_TileRowsTileRowHeight == p_TileRowHeight) {
        return;
    }

    m_AreTileRowsStale = false;
    m_TileRowsColumns = p_Columns;
    m_TileRowsTileRowHeight = p_TileRowHeight;

    m_TileRows.clear();
    m_TileRowsHeight = 0.f;

    for (size_t s_GroupIndex = 0; s_GroupIndex < m_TileGroups.size(); ++s_GroupIndex) {
        const TileGroup& s_Group = m_TileGroups[s_GroupIndex];

        if (IsSearching()) {
            m_TileRows.push_back({ s_GroupIndex, 0, true, m_TileRowsHeight });
            m_TileRowsHeight += p_HeaderHeight;
        }

        if (s_Group.IsCollapsed) {
            continue;
        }

        for (size_t i = 0; i < s_Group.Items.size(); i += p_Columns) {
            m_TileRows.push_back({ s_GroupIndex, i, false, m_TileRowsHeight });
            m_TileRowsHeight += p_TileRowHeight;
        }
    }
}

void AssetBrowser::Draw(const char* p_WindowName) {
    UpdateAssetTree();

    m_NextPath.reset();
    m_IsGoingBack = false;
    m_HoveredItem = nullptr;

    if (BeginPanelWindow(p_WindowName, ImGuiWindowFlags_Docking)) {
        if (!m_Root) {
            ImGui::Text("Loading assets...");
        }
        else if (m_Root->Children.empty()) {
            ImGui::Text("Failed to load asset data.");
        }
        else {
            PushFrameHeight(20.f, 3.f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(5.f), ScaledSize(6.f)));

            DrawToolbar();
            UpdateTileGroups();

            const float s_FooterHeight = ScaledSize(24.f);
            const float s_BodyHeight = std::max(
                1.f, ImGui::GetContentRegionAvail().y - s_FooterHeight - ImGui::GetStyle().ItemSpacing.y
            );

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

            const float s_SplitterWidth = ImGui::GetStyle().ItemSpacing.x;
            const float s_MinColumnWidth = ScaledSize(100.f);
            const float s_MaxFolderTreeWidth = std::max(
                s_MinColumnWidth, ImGui::GetContentRegionAvail().x - s_SplitterWidth - s_MinColumnWidth
            );
            const float s_FolderTreeWidth = std::clamp(
                ScaledSize(m_FolderTreeWidth), s_MinColumnWidth, s_MaxFolderTreeWidth
            );

            DrawFolderTree(s_FolderTreeWidth, s_BodyHeight);

            ImGui::SameLine(0.f, 0.f);
            ImGui::InvisibleButton("##AssetBrowserSplitter", ImVec2(s_SplitterWidth, s_BodyHeight));

            const bool s_IsSplitterActive = ImGui::IsItemActive();

            if (s_IsSplitterActive) {
                const float s_NewWidth = std::clamp(
                    s_FolderTreeWidth + ImGui::GetIO().MouseDelta.x, s_MinColumnWidth, s_MaxFolderTreeWidth
                );

                m_FolderTreeWidth = s_NewWidth / ScaledSize(1.f);
            }

            if (s_IsSplitterActive || ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                const ImVec2 s_SplitterMin = ImGui::GetItemRectMin();
                const ImVec2 s_SplitterMax = ImGui::GetItemRectMax();
                const float s_LineX = (s_SplitterMin.x + s_SplitterMax.x) * 0.5f;

                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(s_LineX, s_SplitterMin.y),
                    ImVec2(s_LineX, s_SplitterMax.y),
                    ImGui::GetColorU32(s_IsSplitterActive ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered),
                    ScaledSize(2.f)
                );
            }

            ImGui::SameLine(0.f, 0.f);
            DrawContent(s_BodyHeight);

            ImGui::PopStyleColor();

            DrawFooter(s_FooterHeight);

            ImGui::PopStyleVar(2);

            if (m_HoveredItem) {
                DrawHoverPopup();
            }
        }
    }

    ImGui::End();

    if (m_HoveredItem) {
        SetHoverPreview(*m_HoveredItem->ResId, m_HoveredItem->Path);
    }
    else {
        ClearHoverPreview();
    }

    if (m_NextPath) {
        if (m_IsGoingBack) {
            m_History.pop_back();
        }
        else {
            m_History.push_back(m_CurrentPath);
        }

        m_CurrentPath = std::move(*m_NextPath);
        m_SearchFilter.clear();
    }
}

void AssetBrowser::DrawToolbar() {
    BeginPanelPadding("##AssetBrowserToolbar");

    const ImGuiStyle& s_Style = ImGui::GetStyle();
    const float s_FrameHeight = ImGui::GetFrameHeight();
    const float s_RowStartX = ImGui::GetCursorPosX();
    const float s_RowEndX = s_RowStartX + ImGui::GetContentRegionAvail().x;
    const float s_SearchWidth = ScaledSize(200.f);
    const float s_MinSearchWidth = ScaledSize(80.f);
    const float s_ScopeWidth = ScaledSize(104.f);
    const float s_FiltersWidth = s_FrameHeight * 2.f + s_Style.ItemInnerSpacing.x * 2.f +
        ImGui::CalcTextSize("TEMP").x + ImGui::CalcTextSize("PRIM").x + s_Style.ItemSpacing.x;
    const float s_ControlsWidth = s_FiltersWidth + s_SearchWidth + s_ScopeWidth + s_Style.ItemSpacing.x * 2.f;

    ImGui::BeginDisabled(m_History.empty());

    if (IconButton(ICON_MD_ARROW_BACK, "Back")) {
        m_NextPath = m_History.back();
        m_IsGoingBack = true;
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::BeginDisabled(m_CurrentPath.empty());

    if (IconButton(ICON_MD_ARROW_UPWARD, "Up one folder")) {
        m_NextPath = FolderPath(m_CurrentPath.begin(), m_CurrentPath.end() - 1);
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    DrawToolbarSeparator();
    ImGui::SameLine();

    const char* s_CrumbSeparator = "\xE2\x80\xBA";
    const char* s_Ellipsis = "\xE2\x80\xA6";
    const float s_CrumbSpacing = ScaledSize(4.f);
    const float s_SeparatorWidth = ImGui::CalcTextSize(s_CrumbSeparator).x + s_CrumbSpacing * 2.f;

    std::vector<float> s_CrumbWidths;
    s_CrumbWidths.push_back(ImGui::CalcTextSize(m_Root->Name.c_str()).x);

    for (const auto* s_Folder : m_CurrentPath) {
        s_CrumbWidths.push_back(ImGui::CalcTextSize(s_Folder->Name.c_str()).x);
    }

    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
    s_CrumbWidths.back() = ImGui::CalcTextSize(
        m_CurrentPath.empty() ? m_Root->Name.c_str() : m_CurrentPath.back()->Name.c_str()
    ).x;
    ImGui::PopFont();

    float s_CrumbsWidth = s_SeparatorWidth * static_cast<float>(m_CurrentPath.size());

    for (const float s_Width : s_CrumbWidths) {
        s_CrumbsWidth += s_Width;
    }

    if (IsSearching()) {
        s_CrumbsWidth += s_SeparatorWidth + ImGui::CalcTextSize("Search results").x;
    }

    const float s_CrumbsStartX = ImGui::GetCursorPosX();
    const float s_SingleRowCrumbsWidth = s_RowEndX - s_ControlsWidth - s_Style.ItemSpacing.x - s_CrumbsStartX;
    const bool s_IsWrapped = s_CrumbsWidth > s_SingleRowCrumbsWidth;
    const float s_CrumbsAvailWidth = s_IsWrapped ? s_RowEndX - s_CrumbsStartX : s_SingleRowCrumbsWidth;

    // Fold folders between into ellipsis when we don't have space.
    size_t s_HiddenCount = 0;

    while (s_CrumbsWidth > s_CrumbsAvailWidth && s_HiddenCount + 1 < m_CurrentPath.size()) {
        ++s_HiddenCount;
        s_CrumbsWidth -= s_CrumbWidths[s_HiddenCount] + s_SeparatorWidth;

        if (s_HiddenCount == 1) {
            s_CrumbsWidth += ImGui::CalcTextSize(s_Ellipsis).x + s_SeparatorWidth;
        }
    }

    const ImVec2 s_CrumbsMin = ImGui::GetCursorScreenPos();
    const float s_CrumbsMaxX = s_CrumbsMin.x + s_CrumbsAvailWidth;

    ImGui::PushClipRect(s_CrumbsMin, ImVec2(s_CrumbsMaxX, s_CrumbsMin.y + s_FrameHeight), true);

    const auto s_DrawCrumb = [&](const std::string& p_Label, const size_t p_Depth) {
        const bool s_IsLast = p_Depth == m_CurrentPath.size();

        ImGui::AlignTextToFramePadding();

        if (s_IsLast) {
            ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);

            const ImVec2 s_Pos = ImGui::GetCursorScreenPos();
            const float s_MaxWidth = s_CrumbsMaxX - s_Pos.x;

            if (ImGui::CalcTextSize(p_Label.c_str()).x > s_MaxWidth) {
                const ImVec2 s_TextPos(s_Pos.x, s_Pos.y + ImGui::GetStyle().FramePadding.y);

                ImGui::RenderTextEllipsis(
                    ImGui::GetWindowDrawList(),
                    s_TextPos,
                    ImVec2(s_CrumbsMaxX, s_TextPos.y + ImGui::GetFontSize()),
                    s_CrumbsMaxX,
                    p_Label.c_str(),
                    nullptr,
                    nullptr
                );
                ImGui::Dummy(ImVec2(s_MaxWidth, ImGui::GetFrameHeight()));
                ImGui::SetItemTooltip("%s", p_Label.c_str());
            }
            else {
                ImGui::TextUnformatted(p_Label.c_str());
            }

            ImGui::PopFont();

            return;
        }

        ImGui::PushID(static_cast<int>(p_Depth));

        if (DrawLinkText(p_Label)) {
            m_NextPath = FolderPath(m_CurrentPath.begin(), m_CurrentPath.begin() + p_Depth);
        }

        ImGui::PopID();
    };

    const auto s_DrawCrumbSeparator = [&] {
        ImGui::SameLine(0.f, s_CrumbSpacing);
        ImGui::TextColored(c_MutedTextColor, "%s", s_CrumbSeparator);
        ImGui::SameLine(0.f, s_CrumbSpacing);
    };

    s_DrawCrumb(m_Root->Name, 0);

    if (s_HiddenCount > 0) {
        s_DrawCrumbSeparator();

        if (DrawLinkText(s_Ellipsis)) {
            ImGui::OpenPopup("##HiddenCrumbs");
        }

        if (ImGui::BeginPopup("##HiddenCrumbs")) {
            for (size_t i = 0; i < s_HiddenCount; ++i) {
                ImGui::PushID(static_cast<int>(i));

                if (ImGui::Selectable(m_CurrentPath[i]->Name.c_str())) {
                    m_NextPath = FolderPath(m_CurrentPath.begin(), m_CurrentPath.begin() + i + 1);
                }

                ImGui::PopID();
            }

            ImGui::EndPopup();
        }
    }

    for (size_t i = s_HiddenCount; i < m_CurrentPath.size(); ++i) {
        s_DrawCrumbSeparator();
        s_DrawCrumb(m_CurrentPath[i]->Name, i + 1);
    }

    if (IsSearching()) {
        s_DrawCrumbSeparator();
        ImGui::TextColored(c_MutedTextColor, "Search results");
    }

    ImGui::PopClipRect();

    float s_SearchBoxWidth = s_SearchWidth;

    if (s_IsWrapped) {
        s_SearchBoxWidth = std::max(s_MinSearchWidth, s_SearchWidth + s_RowEndX - s_RowStartX - s_ControlsWidth);
        ImGui::SetCursorPosX(s_RowStartX);
    }
    else {
        ImGui::SameLine();
        ImGui::SetCursorPosX(s_RowEndX - s_ControlsWidth);
    }

    ImGui::Checkbox("TEMP", &m_ShowTemplates);
    ImGui::SetItemTooltip("Show templates");
    ImGui::SameLine();
    ImGui::Checkbox("PRIM", &m_ShowPrims);
    ImGui::SetItemTooltip("Show prims");

    ImGui::SameLine();

    const ImVec2 s_SearchPos = ImGui::GetCursorScreenPos();

    ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, ScaledSize(22.f));
    ImGui::SetNextItemWidth(s_SearchBoxWidth);
    ImGui::SetNextItemAllowOverlap();
    ImGui::InputTextWithHint("##AssetBrowserSearch", "Search assets", &m_SearchFilter);
    ImGui::PopStyleVar();

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(s_SearchPos.x + ScaledSize(4.f), s_SearchPos.y + (s_FrameHeight - ImGui::GetFontSize()) * 0.5f),
        ImGui::GetColorU32(c_MutedTextColor),
        ICON_MD_SEARCH
    );

    if (IsSearching()) {
        ImGui::SetCursorScreenPos(ImVec2(s_SearchPos.x + s_SearchBoxWidth - ScaledSize(21.f), s_SearchPos.y));

        if (GhostButton(ICON_MD_CLOSE, "##ClearAssetBrowserSearch", "Clear search")) {
            m_SearchFilter.clear();
        }
    }

    ImGui::SetCursorScreenPos(
        ImVec2(s_SearchPos.x + s_SearchBoxWidth + s_Style.ItemSpacing.x, s_SearchPos.y)
    );
    ImGui::SetNextItemWidth(s_ScopeWidth);

    if (ImGui::BeginCombo("##AssetBrowserSearchScope", m_IsSearchEverywhere ? "Everywhere" : "This folder")) {
        if (ImGui::Selectable("Everywhere", m_IsSearchEverywhere)) {
            m_IsSearchEverywhere = true;
        }

        if (ImGui::Selectable("This folder", !m_IsSearchEverywhere)) {
            m_IsSearchEverywhere = false;
        }

        ImGui::EndCombo();
    }

    EndPanelPadding();
}

void AssetBrowser::DrawFolderTree(const float p_Width, const float p_Height) {
    if (ImGui::BeginChild("##AssetBrowserFolders", ImVec2(p_Width, p_Height))) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

        const float s_Width = ImGui::GetContentRegionAvail().x;
        FolderPath s_RowPath;

        DrawPanelHeader("Folders", nullptr, s_Width);
        DrawFolderRows(*m_Root, s_RowPath, s_Width);

        ImGui::PopStyleVar();
    }

    ImGui::EndChild();
}

void AssetBrowser::DrawFolderRows(const AssetItem& p_Folder, FolderPath& p_RowPath, const float p_Width) {
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
    const float s_RowHeight = ScaledSize(18.f);
    const size_t s_Depth = p_RowPath.size();

    for (size_t i = 0; i < p_Folder.FolderCount; ++i) {
        const AssetItem& s_Item = *p_Folder.Children[i];

        p_RowPath.push_back(&s_Item);

        const bool s_IsOnPath = s_Depth < m_CurrentPath.size() && m_CurrentPath[s_Depth] == &s_Item;
        const bool s_IsSelected = s_IsOnPath && s_Depth + 1 == m_CurrentPath.size();
        const bool s_HasSubfolders = s_Item.FolderCount > 0;

        ImGui::PushID(&s_Item);

        const ImVec2 s_RowMin = ImGui::GetCursorScreenPos();

        if (ImGui::InvisibleButton("##FolderRow", ImVec2(p_Width, s_RowHeight))) {
            m_NextPath = p_RowPath;
        }

        if (ImGui::IsItemVisible()) {
            const ImVec2 s_RowMax(s_RowMin.x + p_Width, s_RowMin.y + s_RowHeight);

            if (s_IsSelected) {
                s_DrawList->AddRectFilled(s_RowMin, s_RowMax, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
            }
            else if (ImGui::IsItemHovered()) {
                s_DrawList->AddRectFilled(s_RowMin, s_RowMax, ImGui::GetColorU32(c_ListRowHoverColor));
            }

            const ImU32 s_TextColor = ImGui::GetColorU32(ImGuiCol_Text);
            const float s_TextY = s_RowMin.y + (s_RowHeight - ImGui::GetFontSize()) * 0.5f;
            float s_X = s_RowMin.x + ScaledSize(6.f + s_Depth * 14.f);

            if (s_HasSubfolders) {
                s_DrawList->AddText(
                    ImVec2(s_X, s_TextY),
                    s_TextColor,
                    s_IsOnPath ? ICON_MD_ARROW_DROP_DOWN : ICON_MD_ARROW_RIGHT
                );
            }

            s_X += ImGui::CalcTextSize(ICON_MD_ARROW_RIGHT).x + ScaledSize(5.f);

            const char* s_Icon = s_IsSelected ? ICON_MD_FOLDER_OPEN : ICON_MD_FOLDER;

            s_DrawList->AddText(ImVec2(s_X, s_TextY), s_TextColor, s_Icon);
            s_X += ImGui::CalcTextSize(s_Icon).x + ScaledSize(5.f);

            s_DrawList->AddText(ImVec2(s_X, s_TextY), s_TextColor, s_Item.Name.c_str());
        }

        ImGui::PopID();

        if (s_IsOnPath && s_HasSubfolders) {
            DrawFolderRows(s_Item, p_RowPath, p_Width);
        }

        p_RowPath.pop_back();
    }
}

void AssetBrowser::DrawContent(const float p_Height) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaledSize(6.f), ScaledSize(6.f)));

    if (ImGui::BeginChild("##AssetBrowserContent", ImVec2(0.f, p_Height), ImGuiChildFlags_AlwaysUseWindowPadding)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(8.f), ScaledSize(10.f)));

        const ImVec2 s_Spacing = ImGui::GetStyle().ItemSpacing;
        const float s_TileWidth = ScaledSize(static_cast<float>(m_TileSize));
        const float s_TileHeight = s_TileWidth + ScaledSize(4.f) + ImGui::GetTextLineHeight() * 2.f;
        const float s_TileRowHeight = s_TileHeight + s_Spacing.y;
        const int s_Columns = std::max(
            1, static_cast<int>((ImGui::GetContentRegionAvail().x + s_Spacing.x) / (s_TileWidth + s_Spacing.x))
        );

        UpdateTileRows(s_Columns, ScaledSize(24.f) + s_Spacing.y, s_TileRowHeight);

        if (m_TileGroups.empty()) {
            ImGui::TextColored(c_ErrorTextColor, "No results found.");
        }
        else if (m_TileCount == 0 && GetCurrentFolder().Children.empty()) {
            ImGui::TextColored(c_MutedTextColor, "This folder is empty.");
        }
        else if (m_TileCount == 0) {
            ImGui::TextColored(c_MutedTextColor, "No assets match the TEMP / PRIM filters.");
        }

        const ImVec2 s_Start = ImGui::GetCursorPos();
        const float s_ViewTop = ImGui::GetScrollY() - s_Start.y;
        const float s_ViewBottom = s_ViewTop + ImGui::GetWindowHeight();

        auto s_Row = std::ranges::upper_bound(m_TileRows, s_ViewTop - s_TileRowHeight, {}, &TileRow::Top);

        if (s_Row != m_TileRows.begin()) {
            --s_Row;
        }

        for (; s_Row != m_TileRows.end() && s_Row->Top < s_ViewBottom; ++s_Row) {
            TileGroup& s_Group = m_TileGroups[s_Row->Group];

            ImGui::SetCursorPos(ImVec2(s_Start.x, s_Start.y + s_Row->Top));

            if (s_Row->IsHeader) {
                ImGui::PushID(static_cast<int>(s_Row->Group));
                ImGui::SetNextItemOpen(!s_Group.IsCollapsed, ImGuiCond_Always);

                const bool s_IsOpen = DrawPropertySectionHeader(
                    fmt::format("{} {}##SearchGroup", ICON_MD_FOLDER, s_Group.Path).c_str(),
                    std::to_string(s_Group.Items.size())
                );

                if (s_IsOpen == s_Group.IsCollapsed) {
                    s_Group.IsCollapsed = !s_IsOpen;
                    m_AreTileRowsStale = true;
                }

                ImGui::PopID();
                continue;
            }

            const size_t s_End = std::min(s_Row->FirstItem + s_Columns, s_Group.Items.size());

            for (size_t i = s_Row->FirstItem; i < s_End; ++i) {
                if (i > s_Row->FirstItem) {
                    ImGui::SameLine();
                }

                DrawTile(s_Group, *s_Group.Items[i], s_TileWidth, s_TileHeight);
            }
        }

        // Reserve the full height so the scrollbar covers every row.
        ImGui::SetCursorPos(ImVec2(s_Start.x, s_Start.y + m_TileRowsHeight));
        ImGui::Dummy(ImVec2(0.f, 0.f));

        ImGui::PopStyleVar();
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void AssetBrowser::DrawTile(
    const TileGroup& p_Group, const AssetItem& p_Item, const float p_TileWidth, const float p_TileHeight) {
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
    const bool s_IsFolder = IsAssetFolder(p_Item);

    ImGui::PushID(&p_Item);

    const ImVec2 s_Min = ImGui::GetCursorScreenPos();
    const bool s_IsClicked = ImGui::InvisibleButton("##Tile", ImVec2(p_TileWidth, p_TileHeight));
    const bool s_IsHovered = ImGui::IsItemHovered();

    if (!s_IsFolder && ImGui::BeginDragDropSource()) {
        const AssetItem* s_Payload = &p_Item;
        ImGui::SetDragDropPayload(c_AssetItemPayloadType, &s_Payload, sizeof(s_Payload));

        const std::string_view s_Label = GetAssetItemLabel(p_Item);
        ImGui::TextUnformatted(s_Label.data(), s_Label.data() + s_Label.size());

        ImGui::EndDragDropSource();
    }

    DrawAssetItemContextMenu(p_Item);

    if (ImGui::IsItemVisible()) {
        const ImVec2 s_ThumbMax(s_Min.x + p_TileWidth, s_Min.y + p_TileWidth);

        if (s_IsFolder) {
            s_DrawList->AddRectFilled(s_Min, s_ThumbMax, ImGui::GetColorU32(ImGuiCol_Button));

            ImGui::PushFont(nullptr, FontSize(c_FolderTileIconFontSize));
            DrawCenteredText(s_DrawList, s_Min, s_ThumbMax, ImGui::GetColorU32(ImGuiCol_Text), ICON_MD_FOLDER);
            ImGui::PopFont();
        }
        else {
            const ImGuiTexture* s_Thumbnail = GetThumbnail(*p_Item.ResId, p_Item.Path);

            DrawAssetThumbnail(s_DrawList, s_Min, s_ThumbMax, s_Thumbnail);
            DrawTypeBadge(s_DrawList, s_ThumbMax, p_Item);
        }

        if (s_IsHovered) {
            const float s_Outline = ScaledSize(2.f);

            s_DrawList->AddRect(
                ImVec2(s_Min.x - s_Outline * 0.5f, s_Min.y - s_Outline * 0.5f),
                ImVec2(s_ThumbMax.x + s_Outline * 0.5f, s_ThumbMax.y + s_Outline * 0.5f),
                IM_COL32_WHITE,
                0.f,
                0,
                s_Outline
            );
        }

        size_t s_HighlightStart = 0;
        size_t s_HighlightEnd = 0;

        if (const size_t s_Match = IsSearching() ? p_Item.SearchName.find(m_TileGroupsLowerQuery) : std::string::npos;
            s_Match != std::string::npos) {
            s_HighlightStart = s_Match;
            s_HighlightEnd = s_Match + m_TileGroupsLowerQuery.size();
        }

        DrawTileName(
            ImVec2(s_Min.x + ScaledSize(1.f), s_ThumbMax.y + ScaledSize(4.f)),
            p_TileWidth - ScaledSize(2.f),
            GetAssetItemLabel(p_Item),
            s_HighlightStart,
            s_HighlightEnd
        );
    }

    ImGui::PopID();

    if (s_IsFolder && s_IsClicked) {
        m_NextPath = m_CurrentPath;
        m_NextPath->push_back(&p_Item);
    }

    // Hide the popup while dragging so it doesn't cover the game view.
    if (!s_IsFolder && s_IsHovered && !ImGui::GetDragDropPayload()) {
        m_HoveredItem = &p_Item;
        m_HoveredGroup = &p_Group;
        m_HoveredTileMin = s_Min;
    }
}

void AssetBrowser::DrawFooter(const float p_Height) {
    const AssetItem& s_CurrentFolder = GetCurrentFolder();
    const float s_FrameHeight = ImGui::GetFrameHeight();
    const ImVec2 s_FooterMin = ImGui::GetCursorScreenPos();
    const float s_FooterWidth = ImGui::GetContentRegionAvail().x;

    ImGui::GetWindowDrawList()->AddRectFilled(
        s_FooterMin,
        ImVec2(s_FooterMin.x + s_FooterWidth, s_FooterMin.y + p_Height),
        ImGui::GetColorU32(c_SectionHeaderColor)
    );

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(8.f), 0.f));
    ImGui::SetCursorScreenPos(
        ImVec2(s_FooterMin.x + ScaledSize(6.f), s_FooterMin.y + (p_Height - s_FrameHeight) * 0.5f)
    );

    std::string s_Summary;

    if (IsSearching()) {
        s_Summary = fmt::format(
            "{} results for \"{}\" in {}",
            m_TileCount,
            m_SearchFilter,
            m_IsSearchEverywhere ? "all folders" : s_CurrentFolder.Name
        );
    }
    else {
        s_Summary = fmt::format("{} items", s_CurrentFolder.Children.size() - s_CurrentFolder.FolderCount);

        if (s_CurrentFolder.FolderCount > 0) {
            s_Summary += fmt::format(" \xC2\xB7 {} folders", s_CurrentFolder.FolderCount);
        }

        s_Summary += " in " + s_CurrentFolder.Name;
    }

    const float s_MinSliderWidth = ScaledSize(48.f);
    const float s_MaxSliderWidth = ScaledSize(120.f);
    const float s_SmallIconWidth = ImGui::CalcTextSize(ICON_MD_PHOTO_SIZE_SELECT_SMALL).x;

    ImGui::PushFont(nullptr, FontSize(c_LargeIconFontSize));
    const float s_LargeIconWidth = ImGui::CalcTextSize(ICON_MD_PHOTO_SIZE_SELECT_LARGE).x;
    ImGui::PopFont();

    const float s_Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float s_ToolsWidthWithoutSlider = s_FrameHeight * 2.f + s_SmallIconWidth + s_LargeIconWidth +
        s_Spacing * 4.f;
    const float s_ToolsEndX = s_FooterMin.x + s_FooterWidth - ScaledSize(2.f);
    const float s_SummaryMaxX = s_ToolsEndX - s_ToolsWidthWithoutSlider - s_MinSliderWidth - s_Spacing;

    ImGui::PushClipRect(s_FooterMin, ImVec2(s_SummaryMaxX, s_FooterMin.y + p_Height), true);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(s_Summary.c_str());

    const size_t s_QueuedThumbnails = GetQueuedThumbnailCount();

    if (s_QueuedThumbnails > 0) {
        ImGui::SameLine();
        DrawToolbarSeparator();
        ImGui::SameLine();
        ImGui::TextColored(c_MutedTextColor, "Generating %zu thumbnail(s)", s_QueuedThumbnails);
    }

    ImGui::PopClipRect();

    const float s_SummaryEndX = ImGui::GetItemRectMax().x;
    const float s_SliderWidth = std::clamp(
        s_ToolsEndX - s_ToolsWidthWithoutSlider - s_SummaryEndX - s_Spacing, s_MinSliderWidth, s_MaxSliderWidth
    );

    ImGui::SameLine();
    ImGui::SetCursorScreenPos(
        ImVec2(
            s_ToolsEndX - s_ToolsWidthWithoutSlider - s_SliderWidth,
            s_FooterMin.y + (p_Height - s_FrameHeight) * 0.5f
        )
    );

    if (GhostButton(ICON_MD_DELETE, "##ClearThumbnails", "Clear thumbnails")) {
        ClearThumbnailCache();
    }

    ImGui::SameLine();

    if (GhostButton(ICON_MD_PHOTO_LIBRARY, "##GenerateThumbnails", "Generate all thumbnails")) {
        std::vector<std::pair<ZRuntimeResourceID, std::string>> s_Templates;

        CollectTemplates(*m_Root, s_Templates);
        QueueThumbnails(s_Templates);
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_MutedTextColor, ICON_MD_PHOTO_SIZE_SELECT_SMALL);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, ScaledSize(6.f));
    ImGui::SetNextItemWidth(s_SliderWidth);

    if (ImGui::SliderInt("##AssetBrowserTileSize", &m_TileSize, 40, 128, "Size %d")) {
        m_TileSize = std::clamp((m_TileSize + 2) / 4 * 4, 40, 128);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushFont(nullptr, FontSize(c_LargeIconFontSize));
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_MutedTextColor, ICON_MD_PHOTO_SIZE_SELECT_LARGE);
    ImGui::PopFont();

    ImGui::PopStyleVar();

    ImGui::SetCursorScreenPos(s_FooterMin);
    ImGui::Dummy(ImVec2(s_FooterWidth, p_Height));
}

void AssetBrowser::DrawHoverPopup() {
    ImGui::SetNextWindowPos(
        ImVec2(m_HoveredTileMin.x, m_HoveredTileMin.y - ScaledSize(8.f)), ImGuiCond_Always, ImVec2(0.f, 1.f)
    );
    ImGui::SetNextWindowSize(ImVec2(ScaledSize(224.f), 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaledSize(6.f), ScaledSize(6.f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(5.f), ScaledSize(5.f)));

    ImGui::BeginTooltip();

    const ImVec2 s_PreviewMin = ImGui::GetCursorScreenPos();
    const float s_PreviewSize = ImGui::GetContentRegionAvail().x;
    const ImVec2 s_PreviewMax(s_PreviewMin.x + s_PreviewSize, s_PreviewMin.y + s_PreviewSize);

    const ImGuiTexture* s_Texture = GetHoverPreviewTexture(*m_HoveredItem->ResId);

    if (!s_Texture) {
        s_Texture = GetThumbnail(*m_HoveredItem->ResId, m_HoveredItem->Path);
    }

    DrawAssetThumbnail(ImGui::GetWindowDrawList(), s_PreviewMin, s_PreviewMax, s_Texture);
    ImGui::Dummy(ImVec2(s_PreviewSize, s_PreviewSize));

    const std::string_view s_Label = GetAssetItemLabel(*m_HoveredItem);

    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextUnformatted(s_Label.data(), s_Label.data() + s_Label.size());
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, c_MutedTextColor);
    ImGui::TextUnformatted(m_HoveredGroup->Path.c_str());
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();

    ImGui::EndTooltip();

    ImGui::PopStyleVar(2);
}
