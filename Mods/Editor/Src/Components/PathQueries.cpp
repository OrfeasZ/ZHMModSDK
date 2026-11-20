#include "Editor.h"

#include <algorithm>
#include <format>
#include <ranges>

#include <imgui.h>

#include <Glacier/ZActor.h>
#include <Glacier/ZAnimatedActor.h>

#include "DebugLines.h"

namespace {
    using namespace DebugLines;

    constexpr uint32 c_AllObstaclesBlockageFlags = 0xFFFFFFFF;
    constexpr float c_PathQueryDedupeDistance = 0.25f;
    constexpr float c_PendingRequestMatchDistance = 0.001f;
    constexpr size_t c_MaxPathQueries = 32;
    constexpr size_t c_MaxPendingPathRequests = 64;
    constexpr float c_PathQueryLifetimeSeconds = 10.f;
    constexpr float c_PendingPathRequestLifetimeSeconds = 5.f;
    constexpr float c_BlockingObstacleMargin = 0.3f;
    constexpr float c_BlockingObstacleFillAlpha = 0.25f;

    const SVector4 c_PathColor(0.2f, 0.9f, 0.3f, 1.f);
    const SVector4 c_NoPathColor(1.f, 0.15f, 0.15f, 1.f);
    const SVector4 c_IslandPrecheckFailedColor(1.f, 0.2f, 0.9f, 1.f);
    const SVector4 c_InvalidLocationColor(1.f, 0.85f, 0.1f, 1.f);
    const SVector4 c_BlockingObstacleColor(1.f, 0.5f, 0.f, 1.f);

    ImVec4 ToImVec4(const SVector4& p_Color) {
        return ImVec4(p_Color.x, p_Color.y, p_Color.z, p_Color.w);
    }

    struct SPathRequestContext {
        bool m_IsTrackedActor = false;
        uint32 m_ObstacleBlockageFlags = 0;
        bool m_IslandPrecheckFailed = false;
    };

    thread_local SPathRequestContext* g_PathRequestContext = nullptr;
    thread_local const void* g_RequestingAnimatedActor = nullptr;

    template <typename T>
    class ScopedThreadLocal {
    public:
        ScopedThreadLocal(T& p_Slot, T p_Value) : m_Slot(p_Slot), m_Previous(p_Slot) {
            m_Slot = p_Value;
        }

        ~ScopedThreadLocal() {
            m_Slot = m_Previous;
        }

    private:
        T& m_Slot;
        T m_Previous;
    };

    bool AreWithinDistance(const SVector3& p_A, const SVector3& p_B, float p_Distance) {
        return (p_A - p_B).Length() <= p_Distance;
    }

    const ZPFLocation* GetLocationFromArea(const bfx::AreaHandle& p_Area) {
        return reinterpret_cast<const ZPFLocation*>(
            reinterpret_cast<const uint8*>(&p_Area) - offsetof(ZPFLocation, m_area)
        );
    }

    // RequestPath only marks a request ready up front when it rejects it without queueing a plan.
    bool WasPathRequestRejectedBeforePlanning(const TRefCountPtr<ZPFAsynchPathRequest>* p_Request) {
        return p_Request->m_pObject && p_Request->m_pObject->m_bReady;
    }

    bool DoesSegmentIntersectBox(
        const SVector3& p_Start, const SVector3& p_End, const SMatrix& p_WorldToBox, const SVector3& p_HalfSize
    ) {
        const float4 s_LocalStart = p_WorldToBox * float4(p_Start.x, p_Start.y, p_Start.z, 1.f);
        const float4 s_LocalEnd = p_WorldToBox * float4(p_End.x, p_End.y, p_End.z, 1.f);

        const float s_Start[3] = { s_LocalStart.x, s_LocalStart.y, s_LocalStart.z };
        const float s_End[3] = { s_LocalEnd.x, s_LocalEnd.y, s_LocalEnd.z };
        const float s_Extent[3] = { p_HalfSize.x, p_HalfSize.y, p_HalfSize.z };

        float s_Enter = 0.f;
        float s_Exit = 1.f;

        for (int i = 0; i < 3; ++i) {
            const float s_Delta = s_End[i] - s_Start[i];
            const float s_Limit = s_Extent[i] + c_BlockingObstacleMargin;

            if (std::abs(s_Delta) < 1e-6f) {
                if (std::abs(s_Start[i]) > s_Limit) {
                    return false;
                }

                continue;
            }

            float s_Near = (-s_Limit - s_Start[i]) / s_Delta;
            float s_Far = (s_Limit - s_Start[i]) / s_Delta;

            if (s_Near > s_Far) {
                std::swap(s_Near, s_Far);
            }

            s_Enter = (std::max)(s_Enter, s_Near);
            s_Exit = (std::min)(s_Exit, s_Far);

            if (s_Enter > s_Exit) {
                return false;
            }
        }

        return true;
    }

    std::string FormatPathQueryBlockageFlags(uint32 p_Flags) {
        if (p_Flags == c_AllObstaclesBlockageFlags) {
            return "All (0xFFFFFFFF)";
        }

        return std::format("0x{:X}", p_Flags);
    }
}

std::vector<SVector3> Editor::GetPolylinePoints(const bfx::PolylinePath* p_Path) {
    std::vector<SVector3> s_Points;

    if (!p_Path || !p_Path->m_segs.m_data || p_Path->m_segs.m_size <= 0) {
        return s_Points;
    }

    s_Points.reserve(p_Path->m_segs.m_size + 1);
    s_Points.push_back(p_Path->m_segs.m_data[0].m_startPos);

    for (int32 i = 0; i < p_Path->m_segs.m_size; ++i) {
        s_Points.push_back(p_Path->m_segs.m_data[i].m_endPos);
    }

    return s_Points;
}

std::vector<SVector3> Editor::PlanUnblockedPath(
    const bfx::AreaHandle& p_FromArea,
    const SVector3& p_FromPos,
    const bfx::AreaHandle& p_ToArea,
    const SVector3& p_ToPos,
    const bfx::PathSpec& p_PathSpec,
    const bfx::PathCreationOptions& p_Options
) {
    bfx::PathSpec s_PathSpec = p_PathSpec;
    s_PathSpec.m_obstacleBlockageFlags = 0;

    bfx::APIExclusiveAccessOb s_Lock;

    bfx::PolylinePath* s_Path = Hooks::bfx_CreatePolylinePath_Internal->CallOriginal(
        p_FromArea, p_FromPos, p_ToArea, p_ToPos, s_PathSpec, p_Options
    );

    std::vector<SVector3> s_Points = GetPolylinePoints(s_Path);

    if (s_Path) {
        s_Path->Release();
    }

    return s_Points;
}

void Editor::UpdatePathQueryActor() {
    if (!m_RecordPathQueries) {
        m_PathQueryAnimatedActor = nullptr;
        return;
    }

    ZActor* s_Actor = m_SelectedActor;

    {
        std::scoped_lock s_Lock(m_PathQueryMutex);

        if (s_Actor != m_PathQueryActor) {
            m_PathQueries.clear();
            m_PendingTrackedPathRequests.clear();
            m_PathQueryActor = s_Actor;
        }
    }

    m_PathQueryAnimatedActor = s_Actor ? s_Actor->m_pAnimatedActor : nullptr;

    ResolveBlockingObstacles();
}

const ZPFLocation* Editor::GetTrackedActorPathLocation() const {
    const ZAnimatedActor* s_AnimatedActor = m_PathQueryAnimatedActor.load();

    if (!s_AnimatedActor || !s_AnimatedActor->m_pNavState) {
        return nullptr;
    }

    return &s_AnimatedActor->m_pNavState->m_closestLocationOnNavmesh;
}

bool Editor::IsTrackedActorRequest(const ZPFLocation& p_From) {
    const void* s_TrackedAnimatedActor = m_PathQueryAnimatedActor.load();

    if (!s_TrackedAnimatedActor) {
        return false;
    }

    if (g_RequestingAnimatedActor) {
        return g_RequestingAnimatedActor == s_TrackedAnimatedActor;
    }

    const ZPFLocation* s_TrackedLocation = GetTrackedActorPathLocation();

    return s_TrackedLocation
        && (s_TrackedLocation == &p_From
            || AreWithinDistance(s_TrackedLocation->m_pos, p_From.m_pos, c_PendingRequestMatchDistance));
}

void Editor::AddPendingTrackedPathRequest(const SVector3& p_From, const SVector3& p_To, uint32 p_ObstacleBlockageFlags) {
    const auto s_Now = std::chrono::steady_clock::now();

    std::scoped_lock s_Lock(m_PathQueryMutex);

    std::erase_if(m_PendingTrackedPathRequests, [&](const SPendingPathRequest& p_Request) {
        return std::chrono::duration<float>(s_Now - p_Request.m_RequestedAt).count() > c_PendingPathRequestLifetimeSeconds;
    });

    if (m_PendingTrackedPathRequests.size() >= c_MaxPendingPathRequests) {
        m_PendingTrackedPathRequests.erase(m_PendingTrackedPathRequests.begin());
    }

    m_PendingTrackedPathRequests.push_back({ p_From, p_To, p_ObstacleBlockageFlags, s_Now });
}

bool Editor::ConsumePendingTrackedPathRequest(
    const SVector3& p_From, const SVector3& p_To, uint32 p_ObstacleBlockageFlags
) {
    std::scoped_lock s_Lock(m_PathQueryMutex);

    const auto s_Match = std::ranges::find_if(m_PendingTrackedPathRequests, [&](const SPendingPathRequest& p_Request) {
        return p_Request.m_ObstacleBlockageFlags == p_ObstacleBlockageFlags
            && AreWithinDistance(p_Request.m_From, p_From, c_PendingRequestMatchDistance)
            && AreWithinDistance(p_Request.m_To, p_To, c_PendingRequestMatchDistance);
    });

    if (s_Match == m_PendingTrackedPathRequests.end()) {
        return false;
    }

    m_PendingTrackedPathRequests.erase(s_Match);
    return true;
}

void Editor::RecordPathQuery(
    const SVector3& p_From,
    const SVector3& p_To,
    uint32 p_ObstacleBlockageFlags,
    EPathQueryResult p_Result,
    bool p_FromBehaviorPrecheck,
    std::vector<SVector3>&& p_Points,
    std::vector<SVector3>&& p_UnblockedPoints
) {
    const auto s_Now = std::chrono::steady_clock::now();

    std::scoped_lock s_Lock(m_PathQueryMutex);

    auto s_Existing = std::ranges::find_if(m_PathQueries, [&](const SPathQueryRecord& p_Record) {
        return p_Record.m_ObstacleBlockageFlags == p_ObstacleBlockageFlags
            && AreWithinDistance(p_Record.m_From, p_From, c_PathQueryDedupeDistance)
            && AreWithinDistance(p_Record.m_To, p_To, c_PathQueryDedupeDistance);
    });

    if (s_Existing == m_PathQueries.end()) {
        if (m_PathQueries.size() >= c_MaxPathQueries) {
            m_PathQueries.erase(std::ranges::min_element(m_PathQueries, {}, &SPathQueryRecord::m_LastSeen));
        }

        s_Existing = m_PathQueries.emplace(m_PathQueries.end());
        s_Existing->m_ObstacleBlockageFlags = p_ObstacleBlockageFlags;
    }

    s_Existing->m_From = p_From;
    s_Existing->m_To = p_To;
    s_Existing->m_Result = p_Result;
    s_Existing->m_FromBehaviorPrecheck = p_FromBehaviorPrecheck;
    s_Existing->m_Points = std::move(p_Points);
    s_Existing->m_UnblockedPoints = std::move(p_UnblockedPoints);
    s_Existing->m_BlockingObstaclesResolved = false;
    s_Existing->m_HitCount++;
    s_Existing->m_LastSeen = s_Now;
}

void Editor::ResolveBlockingObstacles() {
    if (!Globals::Pathfinder || !Globals::Pathfinder->m_obstacleManager) {
        return;
    }

    const auto* s_ObstacleManager = static_cast<const ZPFObstacleManagerDeprecated*>(
        Globals::Pathfinder->m_obstacleManager
    );

    std::scoped_lock s_Lock(m_PathQueryMutex);

    for (SPathQueryRecord& s_Record : m_PathQueries) {
        if (s_Record.m_BlockingObstaclesResolved) {
            continue;
        }

        s_Record.m_BlockingObstaclesResolved = true;
        s_Record.m_BlockingObstacles.clear();

        if (s_Record.m_UnblockedPoints.size() < 2) {
            continue;
        }

        const uint32 s_BlockingMask = s_Record.m_Result == EPathQueryResult::IslandPrecheckFailed
            ? c_AllObstaclesBlockageFlags
            : s_Record.m_ObstacleBlockageFlags;

        for (size_t i = 0; i < s_ObstacleManager->m_obstacles.size(); ++i) {
            const auto* s_Obstacle = static_cast<const ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep*>(
                s_ObstacleManager->m_obstacles[i].m_internal.GetTarget()
            );

            if (!s_Obstacle || (s_Obstacle->m_obstacleDef.m_blockageFlags & s_BlockingMask) == 0) {
                continue;
            }

            const SMatrix& s_Transform = s_Obstacle->m_obstacleDef.m_transform;
            const float4& s_HalfSize = s_Obstacle->m_obstacleDef.m_halfSize;
            const SVector3 s_BoxHalfSize(s_HalfSize.x, s_HalfSize.y, s_HalfSize.z);
            const SMatrix s_WorldToBox = s_Transform.Inverse();

            for (size_t j = 1; j < s_Record.m_UnblockedPoints.size(); ++j) {
                if (DoesSegmentIntersectBox(
                    s_Record.m_UnblockedPoints[j - 1], s_Record.m_UnblockedPoints[j], s_WorldToBox, s_BoxHalfSize
                )) {
                    s_Record.m_BlockingObstacles.push_back(
                        { s_Transform, s_BoxHalfSize, s_Obstacle->m_obstacleDef.m_blockageFlags }
                    );
                    break;
                }
            }
        }
    }
}

void Editor::DrawPathQueries(IRenderer* p_Renderer) {
    if (!m_RecordPathQueries) {
        return;
    }

    const auto s_Now = std::chrono::steady_clock::now();

    std::scoped_lock s_Lock(m_PathQueryMutex);

    for (const SPathQueryRecord& s_Record : m_PathQueries) {
        const float s_Age = std::chrono::duration<float>(s_Now - s_Record.m_LastSeen).count();

        if (s_Age > c_PathQueryLifetimeSeconds) {
            continue;
        }

        const float s_Alpha = (std::max)(0.2f, 1.f - s_Age / c_PathQueryLifetimeSeconds);

        if (s_Record.m_Result == EPathQueryResult::Path) {
            const SVector4 s_Color = WithAlpha(c_PathColor, s_Alpha);
            DrawPolyline(p_Renderer, s_Record.m_Points, s_Color);
            DrawEndMarker(p_Renderer, s_Record.m_To, s_Color);
            continue;
        }

        SVector4 s_Color;

        switch (s_Record.m_Result) {
            case EPathQueryResult::IslandPrecheckFailed:
                s_Color = WithAlpha(c_IslandPrecheckFailedColor, s_Alpha);
                break;
            case EPathQueryResult::InvalidLocation:
                s_Color = WithAlpha(c_InvalidLocationColor, s_Alpha);
                break;
            default:
                s_Color = WithAlpha(c_NoPathColor, s_Alpha);
                break;
        }

        if (s_Record.m_UnblockedPoints.size() >= 2) {
            DrawDashedPolyline(p_Renderer, s_Record.m_UnblockedPoints, s_Color);
        }
        else {
            DrawDashedLine(p_Renderer, s_Record.m_From, s_Record.m_To, s_Color);
        }

        DrawEndMarker(p_Renderer, s_Record.m_To, s_Color);

        const SVector4 s_ObstacleColor = WithAlpha(c_BlockingObstacleColor, s_Alpha);
        const SVector4 s_ObstacleFillColor = WithAlpha(c_BlockingObstacleColor, s_Alpha * c_BlockingObstacleFillAlpha);

        for (const SBlockingObstacle& s_Obstacle : s_Record.m_BlockingObstacles) {
            const SVector3 s_Min(-s_Obstacle.m_HalfSize.x, -s_Obstacle.m_HalfSize.y, -s_Obstacle.m_HalfSize.z);

            p_Renderer->DrawBoundingQuads3D(s_Min, s_Obstacle.m_HalfSize, s_Obstacle.m_Transform, s_ObstacleFillColor);
            p_Renderer->DrawOBB3D(s_Min, s_Obstacle.m_HalfSize, s_Obstacle.m_Transform, s_ObstacleColor);
        }
    }
}

void Editor::DrawSelectedActorPathQueries() {
    bool s_RecordPathQueries = m_RecordPathQueries;

    if (ImGui::Checkbox("Draw path queries", &s_RecordPathQueries)) {
        m_RecordPathQueries = s_RecordPathQueries;
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear path queries")) {
        std::scoped_lock s_Lock(m_PathQueryMutex);
        m_PathQueries.clear();
    }

    if (!m_RecordPathQueries) {
        return;
    }

    ImGui::TextColored(ToImVec4(c_PathColor), "Path");
    ImGui::SameLine();
    ImGui::TextColored(ToImVec4(c_NoPathColor), "No path");
    ImGui::SameLine();
    ImGui::TextColored(ToImVec4(c_IslandPrecheckFailedColor), "Island pre-check failed");
    ImGui::SameLine();
    ImGui::TextColored(ToImVec4(c_InvalidLocationColor), "Off navmesh");
    ImGui::SameLine();
    ImGui::TextColored(ToImVec4(c_BlockingObstacleColor), "Blocking obstacle");

    if (!ImGui::BeginTable("PathQueries", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
        return;
    }

    ImGui::TableSetupColumn("Mask");
    ImGui::TableSetupColumn("Source");
    ImGui::TableSetupColumn("Result");
    ImGui::TableSetupColumn("Hits");
    ImGui::TableSetupColumn("Age");
    ImGui::TableHeadersRow();

    const auto s_Now = std::chrono::steady_clock::now();

    const auto s_FormatBlockage = [](const SPathQueryRecord& p_Record) -> std::string {
        if (p_Record.m_UnblockedPoints.empty()) {
            return "no route even ignoring obstacles";
        }

        if (!p_Record.m_BlockingObstaclesResolved) {
            return "resolving obstacles...";
        }

        if (p_Record.m_BlockingObstacles.empty()) {
            return "blocked, no obstacle found on route";
        }

        std::string s_Text = std::format("{} obstacle(s):", p_Record.m_BlockingObstacles.size());

        for (const SBlockingObstacle& s_Obstacle : p_Record.m_BlockingObstacles) {
            s_Text += std::format(" 0x{:X}", s_Obstacle.m_BlockageFlags);
        }

        return s_Text;
    };

    std::scoped_lock s_Lock(m_PathQueryMutex);

    for (const SPathQueryRecord& s_Record : m_PathQueries) {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(FormatPathQueryBlockageFlags(s_Record.m_ObstacleBlockageFlags).c_str());

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(s_Record.m_FromBehaviorPrecheck ? "Behavior" : "Request");

        ImGui::TableNextColumn();

        switch (s_Record.m_Result) {
            case EPathQueryResult::Path:
                ImGui::Text("%zu points", s_Record.m_Points.size());
                break;
            case EPathQueryResult::NoPath:
                ImGui::Text("No path: %s", s_FormatBlockage(s_Record).c_str());
                break;
            case EPathQueryResult::IslandPrecheckFailed:
                ImGui::Text("Island pre-check: %s", s_FormatBlockage(s_Record).c_str());
                break;
            case EPathQueryResult::InvalidLocation:
                ImGui::TextUnformatted("Start or end off navmesh");
                break;
        }

        ImGui::TableNextColumn();
        ImGui::Text("%u", s_Record.m_HitCount);

        ImGui::TableNextColumn();
        ImGui::Text("%.1fs", std::chrono::duration<float>(s_Now - s_Record.m_LastSeen).count());
    }

    ImGui::EndTable();
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    bfx::PolylinePath*,
    bfx_CreatePolylinePath_Internal,
    const bfx::AreaHandle& p_FromArea,
    const SVector3& p_FromPos,
    const bfx::AreaHandle& p_ToArea,
    const SVector3& p_ToPos,
    const bfx::PathSpec& p_PathSpec,
    const bfx::PathCreationOptions& p_Options
) {
    if (!m_RecordPathQueries) {
        return HookResult<bfx::PolylinePath*>(HookAction::Continue());
    }

    const uint32 s_BlockageFlags = p_PathSpec.m_obstacleBlockageFlags;

    // Planning usually runs on a NavPower worker, so the requesting actor is matched from RequestPath's queue.
    if (!ConsumePendingTrackedPathRequest(p_FromPos, p_ToPos, s_BlockageFlags)) {
        return HookResult<bfx::PolylinePath*>(HookAction::Continue());
    }

    bfx::PolylinePath* s_Path = p_Hook->CallOriginal(p_FromArea, p_FromPos, p_ToArea, p_ToPos, p_PathSpec, p_Options);

    std::vector<SVector3> s_Points = GetPolylinePoints(s_Path);
    std::vector<SVector3> s_UnblockedPoints;

    if (s_Points.empty() && s_BlockageFlags != 0) {
        s_UnblockedPoints = PlanUnblockedPath(p_FromArea, p_FromPos, p_ToArea, p_ToPos, p_PathSpec, p_Options);
    }

    RecordPathQuery(
        p_FromPos,
        p_ToPos,
        s_BlockageFlags,
        s_Points.empty() ? EPathQueryResult::NoPath : EPathQueryResult::Path,
        false,
        std::move(s_Points),
        std::move(s_UnblockedPoints)
    );

    return HookResult<bfx::PolylinePath*>(HookAction::Return(), s_Path);
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    bool,
    bfx_IsAreaReachableFromArea,
    const bfx::AreaHandle& p_FromArea,
    const bfx::AreaHandle& p_ToArea
) {
    const bool s_Connected = p_Hook->CallOriginal(p_FromArea, p_ToArea);

    if (s_Connected || !m_RecordPathQueries) {
        return HookResult<bool>(HookAction::Return(), s_Connected);
    }

    // Every caller embeds the area handles in a ZPFLocation, so the positions sit just before them.
    const ZPFLocation* s_From = GetLocationFromArea(p_FromArea);
    const ZPFLocation* s_To = GetLocationFromArea(p_ToArea);

    uint32 s_BlockageFlags = c_AllObstaclesBlockageFlags;
    bool s_FromBehaviorPrecheck = false;

    if (SPathRequestContext* s_Context = g_PathRequestContext) {
        if (!s_Context->m_IsTrackedActor) {
            return HookResult<bool>(HookAction::Return(), s_Connected);
        }

        s_Context->m_IslandPrecheckFailed = true;
        s_BlockageFlags = s_Context->m_ObstacleBlockageFlags;
        ConsumePendingTrackedPathRequest(s_From->m_pos, s_To->m_pos, s_BlockageFlags);
    }
    else if (GetTrackedActorPathLocation() == s_From) {
        s_FromBehaviorPrecheck = true;
    }
    else {
        return HookResult<bool>(HookAction::Return(), s_Connected);
    }

    std::vector<SVector3> s_UnblockedPoints = PlanUnblockedPath(
        p_FromArea, s_From->m_pos, p_ToArea, s_To->m_pos, bfx::PathSpec {}, bfx::PathCreationOptions {}
    );

    RecordPathQuery(
        s_From->m_pos,
        s_To->m_pos,
        s_BlockageFlags,
        EPathQueryResult::IslandPrecheckFailed,
        s_FromBehaviorPrecheck,
        {},
        std::move(s_UnblockedPoints)
    );

    return HookResult<bool>(HookAction::Return(), s_Connected);
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    TRefCountPtr<ZPFAsynchPathRequest>*,
    ZPathfinder_RequestPath,
    ZPathfinder* th,
    TRefCountPtr<ZPFAsynchPathRequest>* p_Result,
    const ZPFLocation& p_From,
    const ZPFLocation& p_To,
    float32 p_PathSharingPenalty,
    uint32 p_ObstacleBlockageFlags,
    float32 p_MaxCost,
    bool p_PostProcess,
    IPathModifier* p_PathModifier
) {
    if (!m_RecordPathQueries) {
        return HookResult<TRefCountPtr<ZPFAsynchPathRequest>*>(HookAction::Continue());
    }

    SPathRequestContext s_Context { IsTrackedActorRequest(p_From), p_ObstacleBlockageFlags };

    if (s_Context.m_IsTrackedActor) {
        AddPendingTrackedPathRequest(p_From.m_pos, p_To.m_pos, p_ObstacleBlockageFlags);
    }

    TRefCountPtr<ZPFAsynchPathRequest>* s_Result;

    {
        ScopedThreadLocal<SPathRequestContext*> s_Scope(g_PathRequestContext, &s_Context);
        s_Result = p_Hook->CallOriginal(
            th, p_Result, p_From, p_To, p_PathSharingPenalty, p_ObstacleBlockageFlags, p_MaxCost, p_PostProcess,
            p_PathModifier
        );
    }

    if (s_Context.m_IsTrackedActor
        && !s_Context.m_IslandPrecheckFailed
        && WasPathRequestRejectedBeforePlanning(p_Result)
        && ConsumePendingTrackedPathRequest(p_From.m_pos, p_To.m_pos, p_ObstacleBlockageFlags)) {
        RecordPathQuery(
            p_From.m_pos, p_To.m_pos, p_ObstacleBlockageFlags, EPathQueryResult::InvalidLocation, false, {}, {}
        );
    }

    return HookResult<TRefCountPtr<ZPFAsynchPathRequest>*>(HookAction::Return(), s_Result);
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    void,
    ZMoveAnimProgram_AsyncRequestPath,
    ZMoveAnimProgram* th,
    const ZPFLocation& p_StartLocation,
    const TArray<ZPFLocation>& p_Waypoints
) {
    if (!m_RecordPathQueries) {
        return HookResult<void>(HookAction::Continue());
    }

    ScopedThreadLocal<const void*> s_Scope(g_RequestingAnimatedActor, th->m_pAnimatedActor);
    p_Hook->CallOriginal(th, p_StartLocation, p_Waypoints);

    return HookResult<void>(HookAction::Return());
}
