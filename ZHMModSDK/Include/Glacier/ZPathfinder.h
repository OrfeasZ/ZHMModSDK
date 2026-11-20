#pragma once

#include "Enums.h"
#include "ZBuffer.h"
#include "TSharedPointer.h"
#include "ZSharedPointerTarget.h"
#include "ZEntity.h"
#include "ZResource.h"
#include "ZSpatialEntity.h"

struct SNavPowerResource;

class IPFObstacleManager {
public:
    virtual ~IPFObstacleManager() = 0;
};

class IPFObstacleInternal : public ZSharedPointerTarget {};

enum EPFObstacleState {
    ePFOS_CreationRequested = 0x0,
    ePFOS_CreationFailed = 0x1,
    ePFOS_Created = 0x2,
    ePFOS_DestructionRequested = 0x3,
    ePFOS_Destroyed = 0x4,
};

enum EPFObstacleClient {
    ePFOC_Unspecified = 0x0,
    ePFOC_CrowdActivitySpotEntity = 0x1,
    ePFOC_PFObstacleEntity = 0x2,
    ePFOC_VIPThreat = 0x3,
    ePFOC_Combat = 0x4,
    ePFOC_Door = 0x5,
    ePFOC_Max = 0x6,
};

namespace bfx {
    class CriticalSection {
    public:
        CriticalSection() { InitializeCriticalSection(&m_criticalSection); }
        ~CriticalSection() { DeleteCriticalSection(&m_criticalSection); }

        void Enter() {
            EnterCriticalSection(&m_criticalSection);
        }

        void Leave() {
            LeaveCriticalSection(&m_criticalSection);
        }

    private:
        CriticalSection(const CriticalSection& rhs);

    private:
        CRITICAL_SECTION m_criticalSection;
    };

    class APICriticalSection : public CriticalSection {};

    class SystemInstance {
    public:
        PAD(0x70);
        APICriticalSection* m_pGlobalCS; // 0x70
    };

    class APIExclusiveAccessOb {
    public:
        APIExclusiveAccessOb() {
            if (Globals::NavPowerSystemInstance && *Globals::NavPowerSystemInstance) {
                m_pGlobalCS = (*Globals::NavPowerSystemInstance)->m_pGlobalCS;
            }

            if (m_pGlobalCS) {
                m_pGlobalCS->Enter();
            }
        }

        ~APIExclusiveAccessOb() {
            if (m_pGlobalCS) {
                m_pGlobalCS->Leave();
            }
        }

    private:
        APICriticalSection* m_pGlobalCS = nullptr;
    };

    class APISharedAccessOb : public APIExclusiveAccessOb {};

    enum ObstacleBehavior {
        OBSTACLE_BEHAVIOR_AUTOMATIC = 0x0,
        OBSTACLE_BEHAVIOR_NOEFFECT_PENALTY = 0x1,
        OBSTACLE_BEHAVIOR_NOEFFECT_IMPASSABLE = 0x2,
        OBSTACLE_BEHAVIOR_PENALTY_IMPASSABLE = 0x3,
        NUM_OBSTACLE_BEHAVIORS = 0x4,
    };

    struct ObstacleDat {
        uint32 m_layerMask;
        ObstacleBehavior m_obstacleBehavior;
        float m_penaltyMult;
        uint32 m_obstacleBlockageFlags;
        uint64 m_userData;
        const char* m_obstacleName;
    };

    class HandleTargetBase;

    class HandleProxy {
    public:
        HandleTargetBase* GetTarget() { return m_pTarget; }

        HandleTargetBase* m_pTarget;
        uint32 m_refCount : 24;
        int32 m_state : 4;
        uint32 m_handleMode : 2;
    };

    class HandleTargetBase {
    public:
        virtual ~HandleTargetBase() = 0;

        HandleProxy* m_pProxy;
    };

    class Planner;

    class ObstacleImpl : HandleTargetBase {
    public:
        ObstacleDat& GetObstacleDat() { return m_obstacleDat; }

        Planner* m_pPlanner;
        int m_obID;
        uint32 m_replayID;
        ObstacleImpl* m_pNext;
        ObstacleImpl* m_pPrev;
        ObstacleDat m_obstacleDat;
        uint32 m_applicationLayerMask;
    };

    template <class T>
    class Handle {
    public:
        T* operator*() { return m_pProxy ? (T*)m_pProxy->GetTarget() : NULL; }

        HandleProxy* m_pProxy;
    };

    class ObstacleHandle {
    public:
        ObstacleDat GetObstacleDat() const {
            //APISharedAccessOb ob;
            Handle<ObstacleImpl>* pImpl = (Handle<ObstacleImpl>*) this;
            ObstacleImpl* pObstacle = **pImpl;

            if (pObstacle)
                return pObstacle->GetObstacleDat();

            return ObstacleDat();
        }

        void* m_pProxy;
    };

    class AreaHandle {
    public:
        void* m_handleImpl;
        void* m_pSpace;
    };

    template <class T>
    struct Array {
        T* m_data;
        int32 m_size;
        int32 m_cap;
        uint32 m_tag;
    };

    enum BlockageMode : uint32 {
        BLOCKED_IF_ANY_MATCH = 0,
        BLOCKED_IF_ALL_MATCH = 1,
    };

    struct PathSpec {
        BlockageMode m_obstacleMode = BLOCKED_IF_ANY_MATCH;
        uint32 m_obstacleBlockageFlags = 0;
        uint32 m_areaUsageFlags = 0xEFFFFFFF;
        uint32 m_areaPenaltyFlags = 0xFFFFFFFF;
        uint32 m_linkUsageFlags = 0xFFFFFFFF;
        bool m_usePathSharingPenalty = false;
        float32 m_pathSharingPenalty = 0.f;
        float32 m_maxPathSharingPenalty = 0.f;
        float32 m_maxSearchDist = 10000.f;
    };

    static_assert(sizeof(PathSpec) == 0x24);

    struct PathCreationOptions {
        bool m_performInitialNavProbe = true;
        bool m_performAdditionalSmoothing = false;
        bool m_forceFirstPosOntoNavGraph = false;
    };

    class Space;

    struct PathSegmentImpl {
        Space* m_pSpace;
        uint32 m_type;
        SVector3 m_startPos;
        SVector3 m_endPos;
        SVector3 m_normal;
        uint64 m_userData;
        void* m_areaHandle;
        void* m_linkHandle;
    };

    static_assert(sizeof(PathSegmentImpl) == 0x48);

    class PolylinePath : public HandleTargetBase {
    public:
        void Release() {
            using DeletingDestructor_t = void(*)(PolylinePath*, uint64);
            (**reinterpret_cast<DeletingDestructor_t**>(this))(this, 1);
        }

        Space* m_pSpace; // 0x10
        Array<PathSegmentImpl> m_segs; // 0x18
        uint32 m_pathID; // 0x30
    };
}

class ZPFObstacleHandle;

class ZPFObstacleManagerDeprecated : public IPFObstacleManager {
public:
    struct SObstacleDef {
        SMatrix m_transform;
        float4 m_halfSize;
        float32 m_penalty;
        uint32 m_blockageFlags;
        bfx::ObstacleHandle m_handle;
    };

    class ZPFObstacleInternalDep : public IPFObstacleInternal {
    public:
        SObstacleDef m_obstacleDef;
        EPFObstacleState m_state;
        EPFObstacleClient m_debugSource;
        bool m_bJobStarted : 1;
        bool m_bDestroyOnCreated : 1;
    };

    TArray<ZPFObstacleHandle> m_obstacles;
};

class ZPFObstacleHandle {
public:
    SMatrix GetTransform() const {
        return static_cast<const ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep*>(m_internal.GetTarget())->
            m_obstacleDef.m_transform;
    }

    float4 GetHalfSize() const {
        return static_cast<const ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep*>(m_internal.GetTarget())->
            m_obstacleDef.m_halfSize;
    }

    TSharedPointer<IPFObstacleInternal> m_internal;
};

class ZPFObstacleEntity : public ZBoundedEntity {
public:
    PAD(0x08); // 0xB8
    SVector3 m_vGlobalSize; // 0xC0
    float32 m_fPenaltyMultiplier; // 0xCC
    uint32 m_nObstacleBlockageFlags; // 0xD0
    bool m_bEnabled; // 0xD4
    ZResourcePtr m_pHelper; // 0xD8
    ZPFObstacleHandle m_obstacle; // 0xE0
};

class ZActorFlagPFObstacleEntity : public ZPFObstacleEntity {
public:
    uint32 GetEffectiveFlags() const {
        if (m_nObstacleBlockageFlags != 0)
            return m_nObstacleBlockageFlags;

        return 1u << static_cast<uint32>(m_eActorObstacleBlockage);
    }

    PAD(0x08); // 0xE8
    EActorObstacleBlockage m_eActorObstacleBlockage; // 0xF0
};

class ZPathfinderConfiguration : public ZEntityImpl {
public:
    TResourcePtr<ZBuffer> m_NavpowerResourceID; // 0x18
    TEntityRef<ZSpatialEntity> m_rPivot; // 0x20
    bool m_bAllowMultipleInclusionVolumes; // 0x30
};

struct SLoadedNavMesh {
    ZPathfinderConfiguration* m_PathfinderConfiguration; // 0x0
    void* m_pNavpowerResource; // 0x8
    uint64 m_nNavpowerResourceSize; // 0x10
    char* m_pNavpowerResourceCopy; // 0x18
};

class ZPathfinder : public IComponentInterface {
public:
    PAD(0x38);
    TArray<SLoadedNavMesh> m_aLoadedNavMeshes; // 0x40
    PAD(0x20);
    IPFObstacleManager* m_obstacleManager; // 0x78
};

#pragma pack(push, 4)

class ZPFLocation {
public:
    SVector3 m_pos;
    bfx::AreaHandle m_area;
    bool m_bMapped;
};

#pragma pack(pop)

static_assert(offsetof(ZPFLocation, m_area) == 0xC);
static_assert(sizeof(ZPFLocation) == 0x20);

class ZPFPath;

enum EPathRequestStatus : uint32 {
    ePRS_Initialize = 0,
    ePRS_Pending = 1,
    ePRS_Done = 2,
};

class ZPFAsynchPathRequest {
public:
    int32 m_nRefCount;
    bool m_bReady; // 0x4
    ZPFLocation m_lFrom; // 0x8
    ZPFLocation m_lTo; // 0x28
    ZPFPath* m_pathResult; // 0x48
    uint32 m_nNodeKeyTo; // 0x50
    float32 m_fMaxCost; // 0x54
    EPathRequestStatus m_ePathRequestStatus; // 0x58
};

static_assert(offsetof(ZPFAsynchPathRequest, m_ePathRequestStatus) == 0x58);
static_assert(sizeof(ZPFAsynchPathRequest) == 0x60);
