#pragma once

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
    ePFOS_CreationRequested    = 0x0,
    ePFOS_CreationFailed       = 0x1,
    ePFOS_Created              = 0x2,
    ePFOS_DestructionRequested = 0x3,
    ePFOS_Destroyed            = 0x4,
};

enum EPFObstacleClient {
    ePFOC_Unspecified             = 0x0,
    ePFOC_CrowdActivitySpotEntity = 0x1,
    ePFOC_PFObstacleEntity        = 0x2,
    ePFOC_VIPThreat               = 0x3,
    ePFOC_Combat                  = 0x4,
    ePFOC_Door                    = 0x5,
    ePFOC_Max                     = 0x6,
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
        APICriticalSection* m_pGlobalCS; // 0x70
    };

    class APIExclusiveAccessOb {
    public:
        APIExclusiveAccessOb() {
            if (Globals::NavPowerSystemInstance) {
                Globals::NavPowerSystemInstance->m_pGlobalCS->Enter();
            }
        }

        ~APIExclusiveAccessOb() {
            if (Globals::NavPowerSystemInstance) {
                Globals::NavPowerSystemInstance->m_pGlobalCS->Leave();
            }
        }
    };

    class APISharedAccessOb : public APIExclusiveAccessOb {};

    enum ObstacleBehavior {
        OBSTACLE_BEHAVIOR_AUTOMATIC           = 0x0,
        OBSTACLE_BEHAVIOR_NOEFFECT_PENALTY    = 0x1,
        OBSTACLE_BEHAVIOR_NOEFFECT_IMPASSABLE = 0x2,
        OBSTACLE_BEHAVIOR_PENALTY_IMPASSABLE  = 0x3,
        NUM_OBSTACLE_BEHAVIORS                = 0x4,
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
        T* operator*() { return m_pProxy ? (T*) m_pProxy->GetTarget() : NULL; }

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
        ZPFObstacleManagerDeprecated::SObstacleDef m_obstacleDef;
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

class ZPathfinderConfiguration : public ZEntityImpl {
public:
    TResourcePtr<ZBuffer> m_NavpowerResourceID; // 0x18
    TEntityRef<ZSpatialEntity> m_rPivot; // 0x20
};

struct SNavPowerResource {
    ZPathfinderConfiguration* m_PathfinderConfiguration; // 0x0
    void* m_pNavpowerResource; // 0x8
    uint64 m_nNavpowerResourceSize; // 0x10
    char* m_pNavpowerResourceCopy; // 0x18
};

class ZPathfinder : public IComponentInterface {
public:
    PAD(0x38);
    TArray<SNavPowerResource> m_NavPowerResources; // 0x40
    PAD(0x20);
    IPFObstacleManager* m_obstacleManager; // 0x78
};