#pragma once

// Reliable actor teleport.
//
// SetObjectToWorldMatrixFromEditor only moves the entity. An actor keeps other copies of its
// position - in its animated actor (ZActor+0x10B8), in the animation network and in the
// locomotion objects - and depending on its state the body stays where it was, or the actor is
// pulled back on the next frame.
//
// Before moving the entity, every copy of the position found in the actor's memory (the ZActor,
// the objects it points to, two levels deep, and the current behavior / act) is recorded, then
// moved along with it, keeping its small offset. No offset is hard-coded, so the search survives
// game updates.
//
// Actors pulled back to their place (measured in Paris with "Teleport all actors" on ~300 actors:
// 37 pulled back before, 291 teleported and staying after). Almost every actor runs an "act"
// (behavior BT_Act, entity ZActBehaviorEntity, state m_eState). Three causes:
//  1. A WALKING actor (act in PREPARING / MOVING) is snapped back onto its path by the animation's
//     path following; disabling the AI does not change that. We wait until it has arrived (act
//     RUNNING, or standing still if it has no act), 90 s at most, then try again.
//  2. An act with a target (m_rMoveToTransform, a marker in the level) realigns the actor on that
//     marker: the marker (and m_rOrientToTransform) is moved by the same offset.
//  3. A scripted pose (e.g. Dalia Margolis) still pulls it back: the act's pose is then disabled
//     ("Disable" pin of its ZChildNetworkActEntity, m_rAct) before trying again.
// Markers and poses are restored as soon as the actor leaves that act, so the level stays as it
// was and the actor resumes its routine (it may walk back later).
// Still impossible: the catwalk models in Paris (scripted show) and actors that never stop walking.
// A position is never forced frame after frame: fighting the game that way froze it.

#include <Windows.h>

#include <cmath>
#include <cstring>
#include <string>
#include <map>
#include <mutex>
#include <vector>

#include <Logging.h>
#include <Globals.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZBehavior.h>
#include <Glacier/ZEntity.h>
#include <Glacier/Hash.h>
#include <Glacier/ZMath.h>
#include <Glacier/ZSpatialEntity.h>

class ActorTeleporter {
public:
    // Call from the game update (frame update / editor task).
    void Queue(const ZEntityRef& p_Entity, void* p_Actor, const SMatrix& p_Target) {
        if (!p_Entity || !p_Actor) {
            return;
        }

        std::scoped_lock s_Lock(m_Mutex);

        for (auto& s_Pending : m_Pending) {
            if (s_Pending.Actor == p_Actor) {
                s_Pending = Pending { p_Entity, p_Actor, p_Target };
                return;
            }
        }

        m_Pending.push_back(Pending { p_Entity, p_Actor, p_Target });
    }

    void Update() {
        std::scoped_lock s_Lock(m_Mutex);

        if (++m_CacheAge >= 30) {
            m_CacheAge = 0;
            SharedCache().Clear();
        }

        for (auto s_It = m_Pending.begin(); s_It != m_Pending.end();) {
            if (Step(*s_It, m_Markers)) {
                s_It = m_Pending.erase(s_It);
            }
            else {
                ++s_It;
            }
        }

        // Every 15 frames: the comparison is cheap, but checking 200 markers every frame
        // is pointless.
        if (++m_RestoreTimer >= 15) {
            m_RestoreTimer = 0;
            RestoreMarkers();
        }
    }

    void Clear() {
        std::scoped_lock s_Lock(m_Mutex);
        m_Pending.clear();
        // Leaving the mission: the markers go away with the scene, nothing to restore.
        m_Markers.clear();
    }

private:
    struct Candidate {
        float* Address;
        int Region;
        uintptr_t Offset;
    };

    struct Region {
        uintptr_t Base;
        size_t Size;
        uintptr_t PointerOffset; // where the pointer was in the ZActor (0 = the ZActor itself)
        uintptr_t ChildOffset = ~0ull; // depth 2: where the second pointer was in the pointed object
    };

    // Act marker moved (or pose disabled) for an actor, to restore when it leaves that act.
    struct MovedMarker {
        ZEntityRef Marker;
        SMatrix Original;
        ZEntityRef Act;
        void* Actor = nullptr;
        bool IsPose = false;
    };

    struct Pending {
        ZEntityRef Entity;
        void* Actor = nullptr;
        SMatrix Target;
        int Frames = 0;
        int Retries = 0;
        float4 Last {};
        float4 Origin {};
        // Walking actor: wait until it has arrived before trying again.
        bool WaitingStill = false;
        int StillFrames = 0;
        int WaitFrames = 0;
        int Attempts = 0;
        // Pulled back while in its pose: the next attempt disables the pose.
        bool DisablePose = false;
        std::vector<Region> Regions;
        std::vector<Candidate> Candidates;
    };

    static constexpr size_t c_ActorSize = 0x1400;
    static constexpr size_t c_ChildSize = 0x800;
    static constexpr size_t c_GrandChildSize = 0x400;
    static constexpr size_t c_MaxRegions = 4000;
    static constexpr size_t c_MaxCandidates = 300;
    // A walking actor has already moved on: its internal copies don't exactly match the origin.
    static constexpr float c_ScanTolerance = 0.75f;
    static constexpr float c_OriginTolerance = 1.0f;
    static constexpr int c_WatchFrames = 60;
    // Standing still = less than 2 cm per frame for a quarter of a second.
    static constexpr float c_StillStep = 0.02f;
    static constexpr int c_StillFrames = 15;
    static constexpr int c_MaxWaitFrames = 90 * 60;
    static constexpr int c_MaxAttempts = 4;
    static constexpr int c_MaxRetries = 1;
    // A jump of more than 2 m in one frame: the game put the actor back (it can't run that fast).
    static constexpr float c_PulledBack = 2.f;

    // Every checked read goes through a shared cache, cleared every 30 frames: in the HITMAN
    // process a VirtualQuery call costs several milliseconds, and checking 240 markers every
    // frame brought the game down to 3 FPS.
    static bool Readable(uintptr_t p_Address, size_t p_Size, bool p_Writable = false) {
        return SharedCache().Check(p_Address, p_Size, p_Writable);
    }

    // Safe reads, with a cache of the memory regions already seen: asking Windows page by page,
    // tens of thousands of times, froze the game for several seconds.
    class ReadableCache {
    public:
        void Clear() {
            m_Ranges.clear();
        }

        bool Check(uintptr_t p_Address, size_t p_Size, bool p_Writable = false) {
            if (p_Address < 0x10000 || p_Address > 0x7FFFFFFFFFFF) {
                return false;
            }

            uintptr_t s_Cursor = p_Address;
            const uintptr_t s_End = p_Address + p_Size;

            while (s_Cursor < s_End) {
                const Range* s_Range = Find(s_Cursor);

                if (!s_Range) {
                    MEMORY_BASIC_INFORMATION s_Info {};

                    if (!VirtualQuery(reinterpret_cast<void*>(s_Cursor), &s_Info, sizeof(s_Info))) {
                        return false;
                    }

                    const DWORD s_Ok = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
                    const bool s_IsReadable = s_Info.State == MEM_COMMIT &&
                        !(s_Info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (s_Info.Protect & s_Ok);
                    const bool s_IsWritable = s_IsReadable && (s_Info.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE));

                    const auto s_Base = reinterpret_cast<uintptr_t>(s_Info.BaseAddress);
                    s_Range = &m_Ranges.insert_or_assign(
                        s_Base, Range { s_Base, s_Base + s_Info.RegionSize, s_IsReadable, s_IsWritable }
                    ).first->second;
                }

                if (!s_Range->Readable || (p_Writable && !s_Range->Writable)) {
                    return false;
                }

                s_Cursor = s_Range->End;
            }

            return true;
        }

    private:
        struct Range {
            uintptr_t Begin;
            uintptr_t End;
            bool Readable;
            bool Writable;
        };

        const Range* Find(uintptr_t p_Address) const {
            auto s_It = m_Ranges.upper_bound(p_Address);

            if (s_It == m_Ranges.begin()) {
                return nullptr;
            }

            --s_It;

            return p_Address < s_It->second.End ? &s_It->second : nullptr;
        }

        std::map<uintptr_t, Range> m_Ranges;
    };

    static ReadableCache& SharedCache() {
        static ReadableCache s_Cache;
        return s_Cache;
    }

    static float Distance(const float* p_A, const float4& p_B) {
        const float s_X = p_A[0] - p_B.x;
        const float s_Y = p_A[1] - p_B.y;
        const float s_Z = p_A[2] - p_B.z;
        return std::sqrt(s_X * s_X + s_Y * s_Y + s_Z * s_Z);
    }

    static float Distance(const float4& p_A, const float4& p_B) {
        const float s_A[3] = { p_A.x, p_A.y, p_A.z };
        return Distance(s_A, p_B);
    }

    static void Scan(Pending& p_Pending) {
        const auto s_Start = GetTickCount64();
        const auto s_ActorBase = reinterpret_cast<uintptr_t>(p_Pending.Actor);
        ReadableCache& s_Cache = SharedCache();

        if (!s_Cache.Check(s_ActorBase, c_ActorSize)) {
            Logger::Warn("[Teleport] Actor memory not readable.");
            return;
        }

        p_Pending.Regions.push_back(Region { s_ActorBase, c_ActorSize, 0 });

        // The current act keeps its own copy of the position (an actor pulled back was put
        // back at its exact original place, not on the marker). These regions come before the
        // ZActor's: the region limit must not cut them.
        std::vector<std::pair<uintptr_t, size_t>> s_Roots;

        if (const int s_Index = ActorIndex(p_Pending.Actor); s_Index >= 0 && Globals::BehaviorService) {
            s_Roots.emplace_back(
                reinterpret_cast<uintptr_t>(&Globals::BehaviorService->m_aBehaviorStates[s_Index]),
                sizeof(ZBehaviorService::SBehaviorState)
            );
        }

        if (const SBehaviorBase* s_Current = CurrentBehavior(p_Pending.Actor)) {
            s_Roots.emplace_back(reinterpret_cast<uintptr_t>(s_Current), c_ChildSize);
        }

        if (const ZEntityRef s_Act = CurrentAct(p_Pending.Actor)) {
            s_Roots.emplace_back(reinterpret_cast<uintptr_t>(s_Act.GetEntity()), c_ChildSize);
        }

        for (const auto& [s_Root, s_Size] : s_Roots) {
            if (!s_Cache.Check(s_Root, s_Size)) {
                continue;
            }

            p_Pending.Regions.push_back(Region { s_Root, s_Size, 0 });

            for (uintptr_t s_Offset = 0; s_Offset + 8 <= s_Size; s_Offset += 8) {
                const auto s_Pointer = *reinterpret_cast<uintptr_t*>(s_Root + s_Offset);

                if ((s_Pointer & 7) != 0 || s_Pointer == s_Root || s_Pointer == s_ActorBase) {
                    continue;
                }

                if (!s_Cache.Check(s_Pointer, c_GrandChildSize)) {
                    continue;
                }

                p_Pending.Regions.push_back(Region { s_Pointer, c_GrandChildSize, 0 });
            }
        }

        // Objects the ZActor points to (depth 1).
        for (uintptr_t s_Offset = 0; s_Offset < c_ActorSize; s_Offset += 8) {
            const auto s_Pointer = *reinterpret_cast<uintptr_t*>(s_ActorBase + s_Offset);

            if ((s_Pointer & 7) != 0 || s_Pointer == s_ActorBase) {
                continue;
            }

            if (s_Pointer >= s_ActorBase && s_Pointer < s_ActorBase + c_ActorSize) {
                continue;
            }

            if (!s_Cache.Check(s_Pointer, c_ChildSize)) {
                continue;
            }

            p_Pending.Regions.push_back(Region { s_Pointer, c_ChildSize, s_Offset });
        }

        // Depth 2: objects pointed to by the ZActor's objects.
        const size_t s_FirstLevel = p_Pending.Regions.size();

        for (size_t r = 1; r < s_FirstLevel && p_Pending.Regions.size() < c_MaxRegions; ++r) {
            const auto s_Parent = p_Pending.Regions[r];

            for (uintptr_t s_Offset = 0; s_Offset < s_Parent.Size; s_Offset += 8) {
                const auto s_Pointer = *reinterpret_cast<uintptr_t*>(s_Parent.Base + s_Offset);

                if ((s_Pointer & 7) != 0 || s_Pointer == s_ActorBase || s_Pointer == s_Parent.Base) {
                    continue;
                }

                if (!s_Cache.Check(s_Pointer, c_GrandChildSize)) {
                    continue;
                }

                p_Pending.Regions.push_back(Region { s_Pointer, c_GrandChildSize, s_Parent.PointerOffset, s_Offset });

                if (p_Pending.Regions.size() >= c_MaxRegions) {
                    break;
                }
            }
        }

        for (int i = 0; i < static_cast<int>(p_Pending.Regions.size()); ++i) {
            const auto& s_Region = p_Pending.Regions[i];

            for (uintptr_t s_Offset = 0; s_Offset + 12 <= s_Region.Size; s_Offset += 4) {
                auto* s_Floats = reinterpret_cast<float*>(s_Region.Base + s_Offset);

                if (!std::isfinite(s_Floats[0]) || !std::isfinite(s_Floats[1]) || !std::isfinite(s_Floats[2])) {
                    continue;
                }

                if (Distance(s_Floats, p_Pending.Origin) > c_ScanTolerance) {
                    continue;
                }

                // The same copy can be seen from two overlapping regions.
                bool s_Known = false;

                for (const auto& s_Candidate : p_Pending.Candidates) {
                    if (s_Candidate.Address == s_Floats) {
                        s_Known = true;
                        break;
                    }
                }

                if (!s_Known && p_Pending.Candidates.size() < c_MaxCandidates) {
                    p_Pending.Candidates.push_back(Candidate { s_Floats, i, s_Offset });
                }
            }
        }

        Logger::Info(
            "[Teleport] {} memory regions scanned in {} ms, {} copies of the position found.",
            p_Pending.Regions.size(), GetTickCount64() - s_Start, p_Pending.Candidates.size()
        );
    }

    // Record the copies of the position, move the entity, then the copies along with it.
    static void Apply(Pending& p_Pending, ZSpatialEntity* p_Spatial, std::vector<MovedMarker>& p_Markers) {
        p_Pending.Origin = p_Spatial->GetObjectToWorldMatrix().Trans;
        p_Pending.Regions.clear();
        p_Pending.Candidates.clear();

        if (p_Pending.DisablePose) {
            DisableActPose(p_Pending, p_Markers);
        }

        MoveActMarkers(p_Pending, p_Markers);
        Scan(p_Pending);

        p_Spatial->SetObjectToWorldMatrixFromEditor(p_Pending.Target);

        const float s_DeltaX = p_Pending.Target.Trans.x - p_Pending.Origin.x;
        const float s_DeltaY = p_Pending.Target.Trans.y - p_Pending.Origin.y;
        const float s_DeltaZ = p_Pending.Target.Trans.z - p_Pending.Origin.z;

        int s_Moved = 0;

        for (const auto& s_Candidate : p_Pending.Candidates) {
            // The entity already updated its own copies: do not move them twice.
            if (Distance(s_Candidate.Address, p_Pending.Origin) >= c_OriginTolerance) {
                continue;
            }

            if (!Readable(reinterpret_cast<uintptr_t>(s_Candidate.Address), 12, true)) {
                continue;
            }

            s_Candidate.Address[0] += s_DeltaX;
            s_Candidate.Address[1] += s_DeltaY;
            s_Candidate.Address[2] += s_DeltaZ;
            ++s_Moved;
        }

        Logger::Info(
            "[Teleport] Actor moved {:.1f} m; {} other copies of its position moved with it.",
            Distance(p_Pending.Origin, p_Pending.Target.Trans), s_Moved
        );
    }

    static int ActorIndex(void* p_Actor) {
        if (!Globals::ActorManager || !Globals::NextActorId) {
            return -1;
        }

        for (size_t i = 0; i < *Globals::NextActorId; ++i) {
            if (Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef == p_Actor) {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    static bool InGameImage(uintptr_t p_Address) {
        const auto s_Base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        const auto* s_Dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(s_Base);
        const auto* s_Nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(s_Base + s_Dos->e_lfanew);
        return p_Address >= s_Base && p_Address < s_Base + s_Nt->OptionalHeader.SizeOfImage;
    }

    // True if p_Entity looks like a game entity: vtable inside the executable, readable type
    // description and interface list. Check this before any call that reads it
    // (HasInterface, QueryInterface): any other value would crash the game.
    static bool LooksLikeEntity(uintptr_t p_Entity) {
        if (!p_Entity || (p_Entity & 7) || !Readable(p_Entity, 0x18)) {
            return false;
        }

        if (!InGameImage(*reinterpret_cast<const uintptr_t*>(p_Entity))) {
            return false;
        }

        auto s_TypeSlot = p_Entity + 8;
        auto s_RawType = *reinterpret_cast<const uintptr_t*>(s_TypeSlot);

        if (s_RawType & 1) {
            s_TypeSlot = s_TypeSlot + (static_cast<intptr_t>(s_RawType) >> 1);

            if (!Readable(s_TypeSlot, 8)) {
                return false;
            }

            s_RawType = *reinterpret_cast<const uintptr_t*>(s_TypeSlot);
        }

        if (!Readable(s_RawType, sizeof(ZEntityType))) {
            return false;
        }

        const auto* s_Type = reinterpret_cast<const ZEntityType*>(s_RawType);
        const auto s_Interfaces = reinterpret_cast<uintptr_t>(s_Type->m_pInterfaceData);

        if (!Readable(s_Interfaces, 0x18)) {
            return false;
        }

        const auto s_Begin = *reinterpret_cast<const uintptr_t*>(s_Interfaces);
        const auto s_End = *reinterpret_cast<const uintptr_t*>(s_Interfaces + 8);

        return s_End >= s_Begin && s_End - s_Begin <= 64 * sizeof(SInterfaceData) &&
            (s_End == s_Begin || Readable(s_Begin, s_End - s_Begin));
    }

    // The actor's current behavior, checked readable; nullptr otherwise.
    static const SBehaviorBase* CurrentBehavior(void* p_Actor) {
        const int s_Index = ActorIndex(p_Actor);

        if (s_Index < 0 || !Globals::BehaviorService) {
            return nullptr;
        }

        const SBehaviorBase* s_Current = Globals::BehaviorService->m_aBehaviorStates[s_Index].m_pCurrentBehavior;

        if (!s_Current || !Readable(reinterpret_cast<uintptr_t>(s_Current), 0x10)) {
            return nullptr;
        }

        return s_Current;
    }

    // The actor's current act (ZActBehaviorEntity), empty otherwise. For the BT_Act behavior
    // (the only one observed), the behavior service keeps the entity reference at +0x8;
    // everything is checked before use.
    static ZEntityRef CurrentAct(void* p_Actor) {
        const SBehaviorBase* s_Current = CurrentBehavior(p_Actor);

        if (!s_Current || s_Current->eBehaviorType != ECompiledBehaviorType::BT_Act) {
            return {};
        }

        const auto s_Obj = *reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(s_Current) + 8);

        if (!LooksLikeEntity(s_Obj - 8)) {
            return {};
        }

        // The type is looked up once: HasInterface looked it up by name on every call, and
        // checking the markers cost 32 ms per frame.
        static STypeID* s_ActType = *Globals::TypeRegistry
            ? (*Globals::TypeRegistry)->GetTypeID("ZActBehaviorEntity") : nullptr;

        if (!s_ActType) {
            return {};
        }

        ZEntityRef s_Act(reinterpret_cast<ZEntityType**>(s_Obj));

        for (const auto& s_Interface : *s_Act->GetType()->m_pInterfaceData) {
            if (s_Interface.m_Type == s_ActType) {
                return s_Act;
            }
        }

        return {};
    }

    // Address of a property of the act (id = CRC32 of its name), 0 if absent.
    static uintptr_t ActProperty(const ZEntityRef& p_Act, const char* p_Property, size_t p_Size) {
        const auto* s_Type = p_Act->GetType();

        if (!s_Type || !s_Type->m_pPropertyData) {
            return 0;
        }

        const uint32_t s_Id = Hash::Crc32(p_Property, strlen(p_Property));

        for (const auto& s_Property : *s_Type->m_pPropertyData) {
            if (s_Property.m_nPropertyID != s_Id) {
                continue;
            }

            const auto s_Address = reinterpret_cast<uintptr_t>(p_Act.m_pObj) + s_Property.m_nPropertyOffset;
            return Readable(s_Address, p_Size) ? s_Address : 0;
        }

        return 0;
    }

    // The entity referenced by a TEntityRef property of the act.
    static ZEntityRef ActEntity(const ZEntityRef& p_Act, const char* p_Property) {
        const uintptr_t s_Address = ActProperty(p_Act, p_Property, 8);

        if (!s_Address) {
            return {};
        }

        const auto s_Obj = *reinterpret_cast<const uintptr_t*>(s_Address);

        if (!s_Obj || !LooksLikeEntity(s_Obj - 8)) {
            return {};
        }

        return ZEntityRef(reinterpret_cast<ZEntityType**>(s_Obj));
    }

    // The marker referenced by a TEntityRef<ZSpatialEntity> property of the act.
    static ZEntityRef ActMarker(const ZEntityRef& p_Act, const char* p_Property) {
        const ZEntityRef s_Marker = ActEntity(p_Act, p_Property);
        return s_Marker && s_Marker.QueryInterface<ZSpatialEntity>() ? s_Marker : ZEntityRef {};
    }

    // True if the actor has arrived: act in its pose (RUNNING), or no act at all.
    static bool ActRunningOrNone(void* p_Actor) {
        const ZEntityRef s_Act = CurrentAct(p_Actor);

        if (!s_Act) {
            return true;
        }

        const uintptr_t s_State = ActProperty(s_Act, "m_eState", 4);

        return !s_State || *reinterpret_cast<const int32_t*>(s_State) ==
            static_cast<int32_t>(ZActBehaviorEntity_EState::RUNNING);
    }

    // Disable the current act's pose (remembered, to enable it again when the actor leaves it).
    static void DisableActPose(Pending& p_Pending, std::vector<MovedMarker>& p_Markers) {
        const ZEntityRef s_Act = CurrentAct(p_Pending.Actor);
        const ZEntityRef s_Pose = s_Act ? ActEntity(s_Act, "m_rAct") : ZEntityRef {};

        if (!s_Pose) {
            return;
        }

        for (const auto& s_Known : p_Markers) {
            if (s_Known.IsPose && s_Known.Marker == s_Pose) {
                return;
            }
        }

        s_Pose.SignalInputPin("Disable");
        p_Markers.push_back(MovedMarker { s_Pose, {}, s_Act, p_Pending.Actor, true });
        Logger::Info("[Teleport] {}: scripted pose cancelled for this attempt.", Describe(p_Pending.Actor));
    }

    // Move the current act's markers by the same offset as the actor.
    static void MoveActMarkers(Pending& p_Pending, std::vector<MovedMarker>& p_Markers) {
        const ZEntityRef s_Act = CurrentAct(p_Pending.Actor);

        if (!s_Act) {
            return;
        }

        // From the actor's position at the time of this attempt: if it walked between two
        // attempts, it may be in another act, with another marker.
        const float s_DeltaX = p_Pending.Target.Trans.x - p_Pending.Origin.x;
        const float s_DeltaY = p_Pending.Target.Trans.y - p_Pending.Origin.y;
        const float s_DeltaZ = p_Pending.Target.Trans.z - p_Pending.Origin.z;

        for (const char* s_Name : { "m_rMoveToTransform", "m_rOrientToTransform" }) {
            const ZEntityRef s_Marker = ActMarker(s_Act, s_Name);

            if (!s_Marker) {
                continue;
            }

            auto* s_Spatial = s_Marker.QueryInterface<ZSpatialEntity>();
            const MovedMarker* s_Known = nullptr;

            for (const auto& s_Moved : p_Markers) {
                if (!s_Moved.IsPose && s_Moved.Marker == s_Marker) {
                    s_Known = &s_Moved;
                    break;
                }
            }

            // A marker already moved starts again from its original place: never moved twice.
            const SMatrix s_Original = s_Known ? s_Known->Original : s_Spatial->GetObjectToWorldMatrix();

            if (!s_Known) {
                p_Markers.push_back(MovedMarker { s_Marker, s_Original, s_Act, p_Pending.Actor });
            }

            SMatrix s_Moved = s_Original;
            s_Moved.Trans.x += s_DeltaX;
            s_Moved.Trans.y += s_DeltaY;
            s_Moved.Trans.z += s_DeltaZ;
            s_Spatial->SetObjectToWorldMatrixFromEditor(s_Moved);

            Logger::Info("[Teleport] {}: act marker {} moved with the actor.", Describe(p_Pending.Actor), s_Name);
        }
    }

    // Restore everything touched for an actor: pose enabled again, markers back in place.
    static void UndoFor(void* p_Actor, std::vector<MovedMarker>& p_Markers) {
        for (auto s_It = p_Markers.begin(); s_It != p_Markers.end();) {
            if (s_It->Actor != p_Actor) {
                ++s_It;
                continue;
            }

            if (s_It->IsPose) {
                s_It->Marker.SignalInputPin("Enable");
            }
            else if (auto* s_Spatial = s_It->Marker.QueryInterface<ZSpatialEntity>()) {
                s_Spatial->SetObjectToWorldMatrixFromEditor(s_It->Original);
            }

            s_It = p_Markers.erase(s_It);
        }
    }

    // Put each marker back as soon as its actor is no longer in that act.
    void RestoreMarkers() {
        for (auto s_It = m_Markers.begin(); s_It != m_Markers.end();) {
            // Fast: nothing is validated, the current act reference is compared to the one we
            // recorded (pointers kept up to date by the game itself).
            const int s_Index = ActorIndex(s_It->Actor);
            const SBehaviorBase* s_Current = s_Index >= 0 && Globals::BehaviorService
                ? Globals::BehaviorService->m_aBehaviorStates[s_Index].m_pCurrentBehavior : nullptr;
            const bool s_SameAct = s_Current && s_Current->eBehaviorType == ECompiledBehaviorType::BT_Act &&
                *reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(s_Current) + 8) ==
                reinterpret_cast<uintptr_t>(s_It->Act.m_pObj);

            if (s_SameAct) {
                ++s_It;
                continue;
            }

            if (s_It->IsPose) {
                s_It->Marker.SignalInputPin("Enable");
                Logger::Info("[Teleport] {} left the act: its pose is enabled again.", Describe(s_It->Actor));
            }
            else if (auto* s_Spatial = s_It->Marker.QueryInterface<ZSpatialEntity>()) {
                s_Spatial->SetObjectToWorldMatrixFromEditor(s_It->Original);
                Logger::Info("[Teleport] {} left the act: its marker is back in place.", Describe(s_It->Actor));
            }

            s_It = m_Markers.erase(s_It);
        }
    }

    // "name [behavior]" of an actor, for the log.
    static std::string Describe(void* p_Actor) {
        auto* s_Actor = static_cast<ZActor*>(p_Actor);
        int s_Behavior = -1;
        const int s_Index = ActorIndex(p_Actor);

        if (s_Index >= 0 && Globals::BehaviorService) {
            const SBehaviorBase* s_Current = Globals::BehaviorService->m_aBehaviorStates[s_Index].m_pCurrentBehavior;
            s_Behavior = s_Current ? static_cast<int>(s_Current->eBehaviorType) : -1;
        }

        return std::string(s_Actor->m_sActorName.c_str()) + " [behavior " + std::to_string(s_Behavior) + "]";
    }

    // Returns true when the watch is over.
    static bool Step(Pending& p_Pending, std::vector<MovedMarker>& p_Markers) {
        auto* s_Spatial = p_Pending.Entity.QueryInterface<ZSpatialEntity>();

        if (!s_Spatial) {
            return true;
        }

        if (p_Pending.WaitingStill) {
            // Arrived: act in its pose (a short stop on the way does not count) and still.
            const float4 s_Now = s_Spatial->GetObjectToWorldMatrix().Trans;
            const bool s_Still = Distance(s_Now, p_Pending.Last) < c_StillStep && ActRunningOrNone(p_Pending.Actor);
            p_Pending.StillFrames = s_Still ? p_Pending.StillFrames + 1 : 0;
            p_Pending.Last = s_Now;

            if (++p_Pending.WaitFrames > c_MaxWaitFrames) {
                Logger::Warn(
                    "[Teleport] PULLED BACK {}: still walking after {} s; giving up.", Describe(p_Pending.Actor),
                    c_MaxWaitFrames / 60
                );
                UndoFor(p_Pending.Actor, p_Markers);
                return true;
            }

            if (p_Pending.StillFrames < c_StillFrames) {
                return false;
            }

            Logger::Info("[Teleport] {} has arrived; trying again.", Describe(p_Pending.Actor));
            p_Pending.WaitingStill = false;
            p_Pending.Retries = 0;
            p_Pending.Frames = 0;
        }

        if (p_Pending.Frames == 0) {
            Apply(p_Pending, s_Spatial, p_Markers);
            p_Pending.Last = p_Pending.Target.Trans;
            p_Pending.Frames = 1;
            return false;
        }

        const float4 s_Position = s_Spatial->GetObjectToWorldMatrix().Trans;
        const float s_FromTarget = Distance(s_Position, p_Pending.Target.Trans);
        const float s_Jump = Distance(s_Position, p_Pending.Last);

        p_Pending.Last = s_Position;

        if (s_Jump > c_PulledBack) {
            if (p_Pending.Retries < c_MaxRetries) {
                Logger::Info("[Teleport] The game put {} back {:.1f} m away; trying once more.", Describe(p_Pending.Actor), s_FromTarget);
                ++p_Pending.Retries;
                Apply(p_Pending, s_Spatial, p_Markers);
                p_Pending.Last = p_Pending.Target.Trans;
                p_Pending.Frames = 1;
                return false;
            }

            if (p_Pending.Attempts < c_MaxAttempts) {
                ++p_Pending.Attempts;

                // In its pose and still pulled back: scripted pose, disable it for the next attempt.
                if (ActRunningOrNone(p_Pending.Actor) && !p_Pending.DisablePose) {
                    p_Pending.DisablePose = true;
                    p_Pending.Retries = 0;
                    p_Pending.Frames = 0;
                    return false;
                }

                // Walking: its path following snaps it back onto its path (disabling the AI does
                // not change that, the animation carries it). Wait until it has arrived.
                p_Pending.WaitingStill = true;
                p_Pending.StillFrames = 0;
                p_Pending.WaitFrames = 0;
                Logger::Info(
                    "[Teleport] {} is walking back to its path; will retry when it arrives.", Describe(p_Pending.Actor)
                );
                return false;
            }

            // Giving up (catwalk and other scripted scenes): nothing may stay modified,
            // otherwise a model would walk the catwalk without its pose.
            Logger::Warn(
                "[Teleport] PULLED BACK {}: the game keeps putting this actor back; giving up ({:.1f} m away).",
                Describe(p_Pending.Actor), s_FromTarget
            );
            UndoFor(p_Pending.Actor, p_Markers);
            return true;
        }

        if (++p_Pending.Frames < c_WatchFrames) {
            return false;
        }

        Logger::Info("[Teleport] Done: actor {:.2f} m from target after {} frames.", s_FromTarget, c_WatchFrames);
        return true;
    }

    std::mutex m_Mutex;
    std::vector<Pending> m_Pending;
    std::vector<MovedMarker> m_Markers;
    int m_RestoreTimer = 0;
    int m_CacheAge = 0;
};
