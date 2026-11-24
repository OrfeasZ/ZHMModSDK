#include "SpawnSafety.h"

#include <algorithm>
#include <queue>

#include "Logging.h"

#include <Functions.h>
#include <Globals.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/SExternalReferences.h>
#include <Glacier/ZBoolConditionListenerManager.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZDistanceConditionManager.h>
#include <Glacier/ZInteractionManager.h>
#include <Glacier/ZLookAtTriggerManager.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZMassImpulse.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZWorldInventory.h>
#include <Glacier/ZAgentsStore.h>
#include <Glacier/ZSplashHintManager.h>

void ForEachEntityInTree(const ZEntityRef& p_Root, const std::function<void(ZEntityRef)>& p_Callback) {
    p_Callback(p_Root);

    std::queue<std::pair<ZEntityBlueprintFactoryBase*, ZEntityRef>> s_Queue;

    if (const auto s_RootFactory = p_Root.GetBlueprintFactory()) {
        s_Queue.emplace(s_RootFactory, p_Root);
    }

    while (!s_Queue.empty()) {
        const auto [s_Factory, s_Root] = s_Queue.front();
        s_Queue.pop();

        for (int64_t i = 0; i < s_Factory->GetSubEntitiesCount(); ++i) {
            const ZEntityRef s_SubEntity = s_Factory->GetSubEntity(s_Root.m_pObj, i);

            // Editor-only sub-entities are not created in the game.
            if (!s_SubEntity.m_pObj) {
                continue;
            }

            // Some aspect sub-entities have no entity at their address and read as a small
            // negative type pointer.
            const auto s_Type = reinterpret_cast<uintptr_t>(*s_SubEntity.m_pObj);

            if (s_Type < 0x10000 || s_Type > 0x7FFFFFFFFFFF) {
                continue;
            }

            p_Callback(s_SubEntity);

            if (const auto s_SubFactory = s_Factory->GetSubEntityBlueprint(i); s_SubFactory && s_SubFactory != s_Factory) {
                s_Queue.emplace(s_SubFactory, s_SubEntity);
            }
        }
    }
}

static constexpr std::string_view c_UnspawnableClasses[] = {
    // Activation reads player state that only the real hero has.
    "ZHitman5",
    // Activation needs a crowd set up in the scene.
    "ZCrowdEntity",
    // The ammo pickup check reads the inventory slot of every ammo action in the level, and
    // templates without an ammo type index outside the inventory.
    "ZHM5ItemAmmo",
    // Move the player to the spawn point.
    "ZHeroSpawn",
    "ZHeroSpawnBypass",
    // On delete, the outfit controller is destroyed after the actor and calls back into it.
    "ZActor",
    // Setting the actor property from an act's behavior promoter reads a null actor.
    "ZActorProxyEntity",
    // Replaces the level's reasoning grid config and leaves the actor manager's grid null.
    "ZReasoningGridConfigEntity",
};

struct UnspawnableTemplate {
    ZRuntimeResourceID m_ResourceId;
    std::string_view m_Name;
};

static constexpr UnspawnableTemplate c_UnspawnableTemplates[] = {
    // The render thread applies material overrides to an object without a constant buffer
    // and writes through a null pointer.
    { ZRuntimeResourceID(0x0092376377B372F1), "wrinklemapscinematics.entitytemplate" },

    // Rendering this removes the D3D device for some reason.
    { ZRuntimeResourceID(0x00A8FA0E1E636816), "tornado.prim" },
};

static constexpr std::string_view c_UnspawnablePaths[] = {
    // Don't allow spawning a second 47.
    "/characters/templates/hero/agent47/agent47.template?",
    // Rendering the river patches removes the D3D device for some reason.
    "/paris_fashionshow/water_mesh_a.wl2?",
};

std::optional<std::string_view> FindUnspawnablePath(std::string_view p_Path) {
    for (const auto s_Path : c_UnspawnablePaths) {
        if (p_Path.contains(s_Path)) {
            return s_Path;
        }
    }

    return std::nullopt;
}

static std::optional<std::string_view> FindUnspawnableContent(IEntityFactory* p_Factory) {
    if (!p_Factory) {
        return std::nullopt;
    }

    if (p_Factory->IsTemplateEntityFactory()) {
        const auto s_TemplateFactory = static_cast<ZTemplateEntityFactory*>(p_Factory);

        for (const auto& s_Template : c_UnspawnableTemplates) {
            if (s_TemplateFactory->m_ridResource == s_Template.m_ResourceId) {
                return s_Template.m_Name;
            }
        }

        for (const auto s_SubFactory : s_TemplateFactory->m_pFactories) {
            if (const auto s_Class = FindUnspawnableContent(s_SubFactory)) {
                return s_Class;
            }
        }
    }
    else if (p_Factory->IsAspectEntityFactory()) {
        for (auto& s_SubFactory : static_cast<ZAspectEntityFactory*>(p_Factory)->m_factoryResources) {
            if (const auto s_Class = FindUnspawnableContent(s_SubFactory.GetResource())) {
                return s_Class;
            }
        }
    }
    else if (p_Factory->IsCppEntityFactory()) {
        const auto s_Blueprint = static_cast<ZCppEntityFactory*>(p_Factory)->m_blueprintResource.GetResource();

        if (s_Blueprint && s_Blueprint->m_pClassType) {
            const std::string_view s_Class = s_Blueprint->m_pClassType->type.pszTypeName;

            if (std::ranges::contains(c_UnspawnableClasses, s_Class)) {
                return s_Class;
            }
        }
    }

    return std::nullopt;
}

template <typename F>
static void ForEachRegisteredLookAtTrigger(F p_Callback) {
    for (const auto& s_Entry : Globals::LookAtTriggerManager->m_PendingTriggers) {
        p_Callback(s_Entry.first);
    }

    for (const auto& s_Entry : Globals::LookAtTriggerManager->m_Triggers) {
        p_Callback(s_Entry.first);
    }
}

std::unordered_set<ZEntityType**> GetRegisteredLookAtTriggers() {
    std::unordered_set<ZEntityType**> s_Triggers;

    ForEachRegisteredLookAtTrigger([&](const ZEntityRef& p_Trigger) {
        s_Triggers.insert(p_Trigger.m_pObj);
    });

    return s_Triggers;
}

size_t UnregisterLookAtTriggersIf(const std::function<bool(ZEntityType**)>& p_ShouldUnregister) {
    std::vector<ZEntityRef> s_Triggers;

    ForEachRegisteredLookAtTrigger([&](const ZEntityRef& p_Trigger) {
        if (p_ShouldUnregister(p_Trigger.m_pObj)) {
            s_Triggers.push_back(p_Trigger);
        }
    });

    for (auto& s_Trigger : s_Triggers) {
        Functions::ZLookAtTriggerManager_UnregisterLookAtTrigger->Call(Globals::LookAtTriggerManager, &s_Trigger);
    }

    return s_Triggers.size();
}

bool CanSpawnTemplate(IEntityFactory* p_Factory, const std::string& p_ItemName) {
    if (const auto s_Path = FindUnspawnablePath(p_ItemName)) {
        Logger::Warn("[Editor] Not spawning asset {}: items in {} can't be spawned.", p_ItemName, *s_Path);
        return false;
    }

    if (const auto s_Class = FindUnspawnableContent(p_Factory)) {
        Logger::Warn(
            "[Editor] Not spawning asset {}: it contains a {}, which crashes outside of a level setup.",
            p_ItemName,
            *s_Class
        );
        return false;
    }

    // The entity manager divides by the blueprint's alignment, which is 0 when the blueprint
    // isn't set up.
    uint32_t s_Size = 0;
    uint32_t s_Alignment = 0;
    int64_t s_Unknown = 0;

    if (const auto s_Blueprint = p_Factory->GetBlueprint()) {
        s_Blueprint->GetMemoryRequirements(s_Size, s_Alignment, s_Unknown);
    }

    if (s_Alignment == 0) {
        Logger::Warn("[Editor] Not spawning asset {}: its blueprint has no memory layout.", p_ItemName);
        return false;
    }

    return true;
}

static bool HasPendingMassImpulses(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    const auto s_IsTreeTarget = [&](const ZDelayedExplodeInfo& p_Info) {
        return p_TreeEntities.contains(p_Info.m_Target.m_pObj);
    };

    if (const auto s_Checker = *Globals::MassImpulseCollisionChecker) {
        for (const auto& s_Check : s_Checker->m_aCollisionChecks) {
            if (s_IsTreeTarget(s_Check.m_ExplodeInfo)) {
                return true;
            }
        }
    }

    if (const auto s_Exploder = *Globals::MassImpulseCharacterExploder) {
        return std::ranges::any_of(s_Exploder->m_aDelayedTargets, s_IsTreeTarget);
    }

    return false;
}

// The situation manager calls into deleted situations. Drama roles register theirs again when
// they deactivate, so we deactivate the whole tree first and then remove the situations.
static void RemoveSituations(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    const auto s_SituationType = (*Globals::TypeRegistry)->GetTypeID("ISituation");

    if (!s_SituationType) {
        return;
    }

    std::vector<TEntityRef<ISituation>> s_Situations;

    for (const auto s_EntityObj : p_TreeEntities) {
        const ZEntityRef s_Entity(s_EntityObj);

        if (const auto s_Situation = s_Entity.QueryInterface<ISituation>(s_SituationType)) {
            auto& s_SituationRef = s_Situations.emplace_back();
            s_SituationRef.m_entityRef = s_Entity;
            s_SituationRef.m_pInterfaceRef = s_Situation;
        }
    }

    if (s_Situations.empty()) {
        return;
    }

    constexpr auto c_Activated = static_cast<uint32_t>(ZEntityImpl::EEntityFlags::ENTITYFLAG_ACTIVATED);

    for (const auto s_EntityObj : p_TreeEntities) {
        const auto s_Entity = ZEntityRef(s_EntityObj).GetEntity();

        // The activation manager only deactivates an entity on delete while this flag is set.
        if (s_Entity->m_nEntityFlags & c_Activated) {
            s_Entity->Deactivate(0);
            s_Entity->m_nEntityFlags &= ~c_Activated;
        }
    }

    for (const auto& s_Situation : s_Situations) {
        Functions::ZSituationManager_RemoveSituation->Call(&Globals::ActorManager->m_situationManager, s_Situation);
    }
}

SpawnedTree CollectSpawnedTree(const ZEntityRef& p_Root) {
    SpawnedTree s_Tree;

    ForEachEntityInTree(p_Root, [&](ZEntityRef p_Entity) {
        s_Tree.Entities.insert(p_Entity.m_pObj);

        if (const auto s_ItemSpawner = p_Entity.QueryInterface<ZItemSpawner>()) {
            s_Tree.ItemSpawners.push_back(s_ItemSpawner);
        }
    });

    return s_Tree;
}

bool IsTreeInUse(const SpawnedTree& p_Tree) {
    return HasPendingMassImpulses(p_Tree.Entities) || std::ranges::any_of(p_Tree.ItemSpawners, &ZItemSpawner::IsPreparing);
}

// The delete deactivates value entities after the look-at triggers unregister, which registers
// them again. So unregister every trigger after the delete. It only compares pointers.
static std::vector<ZEntityRef> FindLookAtTriggers(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    std::vector<ZEntityRef> s_Triggers;
    const auto s_LookAtTriggerType = (*Globals::TypeRegistry)->GetTypeID("ILookAtTriggerEntity");

    if (!s_LookAtTriggerType) {
        return s_Triggers;
    }

    for (const auto s_EntityObj : p_TreeEntities) {
        const ZEntityRef s_Entity(s_EntityObj);

        if (s_Entity.QueryInterface(s_LookAtTriggerType)) {
            s_Triggers.push_back(s_Entity);
        }
    }

    return s_Triggers;
}

static void UnregisterLookAtTriggers(
    std::vector<ZEntityRef>& p_Triggers, const std::unordered_set<ZEntityType**>& p_TreeEntities
) {
    for (auto& s_Trigger : p_Triggers) {
        Functions::ZLookAtTriggerManager_UnregisterLookAtTrigger->Call(Globals::LookAtTriggerManager, &s_Trigger);
    }

    UnregisterLookAtTriggersIf([&](ZEntityType** p_Trigger) {
        return p_TreeEntities.contains(p_Trigger);
    });
}

// Bool condition listeners never unregister, and a later spawn that reuses a condition's address
// calls into the dead listener. Removing them after the delete only compares pointers.
static std::unordered_set<IBoolCondition*> FindBoolConditions(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    std::unordered_set<IBoolCondition*> s_Conditions;
    const auto s_BoolConditionType = (*Globals::TypeRegistry)->GetTypeID("IBoolCondition");

    if (!s_BoolConditionType) {
        return s_Conditions;
    }

    for (const auto s_EntityObj : p_TreeEntities) {
        if (const auto s_Condition = ZEntityRef(s_EntityObj).QueryInterface(s_BoolConditionType)) {
            s_Conditions.insert(static_cast<IBoolCondition*>(s_Condition));
        }
    }

    return s_Conditions;
}

static void UnregisterBoolConditionListeners(
    const std::unordered_set<IBoolCondition*>& p_TreeConditions, const std::unordered_set<ZEntityType**>& p_TreeEntities
) {
    std::vector<std::pair<TEntityRef<IBoolCondition>, TEntityRef<IBoolConditionListener>>> s_DeadListeners;

    for (auto& [s_Condition, s_Listeners] : Globals::BoolConditionListenerManager->m_Listeners) {
        const bool s_IsTreeCondition = p_TreeConditions.contains(s_Condition);

        for (const auto& s_Listener : s_Listeners) {
            if (s_IsTreeCondition || p_TreeEntities.contains(s_Listener.m_pObj)) {
                auto& [s_ConditionRef, s_ListenerRef] = s_DeadListeners.emplace_back();
                s_ConditionRef.m_pInterfaceRef = s_Condition;
                s_ListenerRef.m_entityRef = s_Listener;
            }
        }
    }

    for (const auto& [s_ConditionRef, s_ListenerRef] : s_DeadListeners) {
        Functions::ZBoolConditionListenerManager_UnregisterListener->Call(
            Globals::BoolConditionListenerManager,
            s_ConditionRef,
            s_ListenerRef
        );
    }
}

// A spawner's item outlives the delete, and the save system later crashes reading its spawner,
// owner and transform parent. So destroy every item that refers to the tree first.
static void DestroyItems(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    std::vector<TEntityRef<IItemBase>> s_TreeItems;

    for (const auto& s_Item : Globals::WorldInventory->m_aItems) {
        const auto s_HM5Item = s_Item.m_entityRef.QueryInterface<ZHM5Item>();
        const auto s_Spatial = s_Item.m_entityRef.QueryInterface<ZSpatialEntity>();

        const bool s_HasTreeSpawner = s_HM5Item && p_TreeEntities.contains(s_HM5Item->m_rSpawner.m_pObj);
        const bool s_HasTreeOwner = s_HM5Item && p_TreeEntities.contains(s_HM5Item->m_pOwner.m_pObj);
        const bool s_HasTreeParent =
            s_Spatial && p_TreeEntities.contains(s_Spatial->m_eidParent.m_entityRef.m_pObj);

        if (s_HasTreeSpawner || s_HasTreeOwner || s_HasTreeParent) {
            s_TreeItems.push_back(s_Item);
        }
    }

    for (const auto& s_Item : s_TreeItems) {
        Functions::ZWorldInventory_DestroyItem->Call(Globals::WorldInventory, s_Item);
    }
}

// The splash hint manager keeps reading the current hint, and hiding one fires its pins, so
// hide the tree's hints before the delete.
static void HideSplashHints(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    const auto s_HintManager = Globals::SplashHintManager->m_pInterfaceRef;

    if (!s_HintManager) {
        return;
    }

    std::vector<TEntityRef<ZSplashHintController>> s_DeadControllers;
    auto& s_Queue = s_HintManager->m_QueuedControllers;

    // The queued ones go first, so hiding the current one doesn't show one of them.
    for (auto* s_Node = s_Queue.m_pNext; s_Node != &s_Queue; s_Node = s_Node->m_pNext) {
        if (p_TreeEntities.contains(s_Node->m_rController.m_entityRef.m_pObj)) {
            s_DeadControllers.push_back(s_Node->m_rController);
        }
    }

    if (p_TreeEntities.contains(s_HintManager->m_rCurrentController.m_entityRef.m_pObj)) {
        s_DeadControllers.push_back(s_HintManager->m_rCurrentController);
    }

    for (auto& s_Controller : s_DeadControllers) {
        Functions::ZSplashHintManager_HideController->Call(s_HintManager, s_Controller);
    }
}

// The interaction manager keeps the tree's subactions until its next rebuild and reads them
// before that. It skips entries without an entity.
static void ClearInteractions(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    for (auto& s_Interaction : Globals::InteractionManager->m_aInteractions) {
        if (p_TreeEntities.contains(s_Interaction.m_Entity.m_pObj)) {
            s_Interaction.m_Entity = {};
            s_Interaction.m_pInterface = nullptr;
        }
    }
}

// Drama roles can add their situation back to the registration changes during the delete.
static void RemoveSituationRegistrationChanges(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    auto& s_RegistrationChanges = Globals::ActorManager->m_situationManager.m_aRegistrationChanges;
    const auto s_DeadChanges = std::ranges::remove_if(s_RegistrationChanges, [&](const TEntityRef<ISituation>& p_Situation) {
        return p_TreeEntities.contains(p_Situation.m_entityRef.m_pObj);
    });

    for (size_t i = 0; i < s_DeadChanges.size(); ++i) {
        s_RegistrationChanges.pop_back();
    }
}

// Deactivating the tree early fires pins that activate its distance conditions again, and
// the manager's next update calls into them.
static void ClearDistanceConditionChanges(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    for (auto& s_Change : Globals::DistanceConditionManager->m_aPendingChanges) {
        if (s_Change.m_bActivated && p_TreeEntities.contains(s_Change.m_Entity.m_pObj)) {
            s_Change.m_Entity = {};
            s_Change.m_pCondition = nullptr;
        }
    }
}

// Actors keep noticed setpiece distractions until they notice one again, and the rebuild copies
// the distraction table rows, which still hold the tree. Readers skip entries without an entity.
static void ClearSetpieceDistractions(const std::unordered_set<ZEntityType**>& p_TreeEntities) {
    if (*Globals::SetpieceDistractionTables) {
        for (auto* s_Table : {
                 (**Globals::SetpieceDistractionTables)->m_pSoundDistractions,
                 (**Globals::SetpieceDistractionTables)->m_pVisualDistractions
             }) {
            if (!s_Table) {
                continue;
            }

            for (auto& s_Row : s_Table->m_aRows) {
                if (p_TreeEntities.contains(s_Row.m_Entity.m_pObj)) {
                    s_Row.m_Entity = {};
                }
            }
        }
    }

    if (*Globals::AgentsStore) {
        for (auto& s_Agent : (**Globals::AgentsStore)->m_pData->m_aAgents) {
            for (auto* s_Distractions : {
                     &s_Agent.m_aCurrentSetpieceVisualDistractions,
                     &s_Agent.m_aCurrentSetpieceSoundDistractions
                 }) {
                for (auto& s_Entity : *s_Distractions) {
                    if (p_TreeEntities.contains(s_Entity.m_pObj)) {
                        s_Entity = {};
                    }
                }
            }
        }
    }
}

void EntityTreeDeleter::Delete(const ZEntityRef& p_Root, const SpawnedTree& p_Tree) {
    const auto& s_TreeEntities = p_Tree.Entities;

    auto s_LookAtTriggers = FindLookAtTriggers(s_TreeEntities);
    const auto s_BoolConditions = FindBoolConditions(s_TreeEntities);

    DestroyItems(s_TreeEntities);
    HideSplashHints(s_TreeEntities);
    RemoveSituations(s_TreeEntities);

    Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, p_Root, {});

    UnregisterLookAtTriggers(s_LookAtTriggers, s_TreeEntities);

    {
        std::scoped_lock s_Lock(m_Mutex);

        for (const auto s_Entity : s_TreeEntities) {
            m_SpawnedTreeEntities.erase(s_Entity);
        }

        m_DeletedTreeEntities.insert(s_TreeEntities.begin(), s_TreeEntities.end());
        m_DeletedTreeCheckFrames = 60;
    }

    ClearInteractions(s_TreeEntities);
    RemoveSituationRegistrationChanges(s_TreeEntities);
    ClearDistanceConditionChanges(s_TreeEntities);
    ClearSetpieceDistractions(s_TreeEntities);
    UnregisterBoolConditionListeners(s_BoolConditions, s_TreeEntities);
}

void EntityTreeDeleter::Update() {
    std::scoped_lock s_Lock(m_Mutex);

    if (m_DeletedTreeCheckFrames == 0) {
        return;
    }

    const size_t s_TriggerCount = UnregisterLookAtTriggersIf([&](ZEntityType** p_Trigger) {
        return m_DeletedTreeEntities.contains(p_Trigger);
    });

    if (s_TriggerCount > 0) {
        Logger::Warn(
            "[Editor] Unregistered {} look-at trigger(s) of a deleted entity tree that registered again in a later frame.",
            s_TriggerCount
        );
    }

    if (--m_DeletedTreeCheckFrames == 0) {
        m_DeletedTreeEntities.clear();
    }
}

void EntityTreeDeleter::OnTreeSpawned(const SpawnedTree& p_Tree) {
    std::scoped_lock s_Lock(m_Mutex);

    for (const auto s_Entity : p_Tree.Entities) {
        m_DeletedTreeEntities.erase(s_Entity);
    }

    m_SpawnedTreeEntities.insert(p_Tree.Entities.begin(), p_Tree.Entities.end());
}

bool EntityTreeDeleter::IsInSpawnedTree(ZEntityType** p_Entity) {
    std::scoped_lock s_Lock(m_Mutex);
    return m_SpawnedTreeEntities.contains(p_Entity);
}
