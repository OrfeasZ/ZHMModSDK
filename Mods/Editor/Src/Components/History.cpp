#include <Editor.h>

#include "Logging.h"

#include <Functions.h>
#include <Globals.h>
#include <Glacier/SExternalReferences.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZScene.h>
#include <Glacier/ZSpatialEntity.h>

Editor::HistoryTarget Editor::MakeHistoryTarget(const ZEntityRef p_Entity) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    for (const auto& [s_Root, s_SpawnedEntity] : m_SpawnedEntities) {
        if (!s_SpawnedEntity.Tree.Entities.contains(p_Entity.m_pObj)) {
            continue;
        }

        HistoryTarget s_Target { .SpawnedRootId = s_Root->GetType()->m_nEntityID };
        size_t s_Index = 0;

        ForEachEntityInTree(s_Root, [&](const ZEntityRef p_TreeEntity) {
            if (p_TreeEntity == p_Entity) {
                s_Target.TreeIndex = s_Index;
            }

            ++s_Index;
        });

        return s_Target;
    }

    return { .Entity = p_Entity };
}

ZEntityRef Editor::ResolveHistoryTarget(const HistoryTarget& p_Target) {
    if (!p_Target.SpawnedRootId) {
        return IsEntityInTree(p_Target.Entity) ? p_Target.Entity : ZEntityRef();
    }

    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    for (const ZEntityRef s_Root : m_SpawnedEntities | std::views::keys) {
        if (s_Root->GetType()->m_nEntityID != *p_Target.SpawnedRootId) {
            continue;
        }

        ZEntityRef s_Entity;
        size_t s_Index = 0;

        ForEachEntityInTree(s_Root, [&](const ZEntityRef p_TreeEntity) {
            if (s_Index++ == p_Target.TreeIndex) {
                s_Entity = p_TreeEntity;
            }
        });

        return s_Entity;
    }

    return {};
}

Editor::HistoryValue Editor::MakeHistoryValue(const ZObjectRef& p_Value) {
    HistoryValue s_Value { .Value = p_Value };

    const IType* s_TypeInfo = p_Value.GetTypeID()->GetTypeInfo();

    if (!s_TypeInfo->IsEntity()) {
        return s_Value;
    }

    const ZEntityRef s_EntityRef = std::string_view(s_TypeInfo->pszTypeName) == "ZEntityRef"
        ? *static_cast<ZEntityRef*>(p_Value.GetData())
        : static_cast<TEntityRef<void>*>(p_Value.GetData())->m_entityRef;

    if (s_EntityRef) {
        s_Value.EntityRef = MakeHistoryTarget(s_EntityRef);
    }

    return s_Value;
}

std::optional<ZObjectRef> Editor::ResolveHistoryValue(const HistoryValue& p_Value) {
    if (!p_Value.EntityRef) {
        return p_Value.Value;
    }

    const ZEntityRef s_EntityRef = ResolveHistoryTarget(*p_Value.EntityRef);

    if (!s_EntityRef) {
        return std::nullopt;
    }

    try {
        return MakeEntityRefValue(p_Value.Value.GetTypeID(), s_EntityRef);
    }
    catch (const std::exception&) {
        return std::nullopt;
    }
}

Editor::SpawnInfo Editor::MakeSpawnInfo(const SpawnedEntity& p_SpawnedEntity) {
    SpawnInfo s_Spawn {
        .Factory = *p_SpawnedEntity.Factory,
        .EntityId = p_SpawnedEntity.Entity->GetType()->m_nEntityID,
        .IsAsset = p_SpawnedEntity.IsAsset,
    };

    if (const auto s_It = m_EntityNames.find(p_SpawnedEntity.Entity); s_It != m_EntityNames.end()) {
        s_Spawn.Name = s_It->second;
    }

    if (const auto s_SpatialEntity = p_SpawnedEntity.Entity.QueryInterface<ZSpatialEntity>()) {
        s_Spawn.Transform = s_SpatialEntity->GetObjectToWorldMatrix();
    }

    return s_Spawn;
}

ZEntityRef Editor::SpawnEntityInternal(const SpawnInfo& p_Spawn) {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        return {};
    }

    ZEntityRef s_Entity;
    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        s_Entity,
        p_Spawn.Name,
        p_Spawn.Factory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        p_Spawn.EntityId
    );

    if (!s_Entity) {
        Logger::Error("[Editor] Failed to spawn entity {:016x}.", p_Spawn.EntityId);
        return {};
    }

    if (const auto s_SpatialEntity = s_Entity.QueryInterface<ZSpatialEntity>()) {
        if (p_Spawn.IsAsset) {
            s_Entity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);
        }

        s_SpatialEntity->SetObjectToWorldMatrixFromEditor(p_Spawn.Transform);
    }

    RefreshStaticPhysicsAndObstacles(s_Entity);

    SpawnedEntity s_SpawnedEntity {
        .Entity = s_Entity,
        .IsAsset = p_Spawn.IsAsset,
        .Tree = CollectSpawnedTree(s_Entity),
    };
    s_SpawnedEntity.Factory.emplace(p_Spawn.Factory);

    AddSpawnedEntity(std::move(s_SpawnedEntity), p_Spawn.Name, std::nullopt);

    return s_Entity;
}

void Editor::PushHistory(HistoryEntry p_Entry) {
    std::scoped_lock s_Lock(m_ChangeQueueMutex);

    m_UndoHistory.push_back(std::move(p_Entry));
    m_RedoHistory.clear();

    while (m_UndoHistory.size() > static_cast<size_t>(m_MaxHistorySize)) {
        m_UndoHistory.pop_front();
    }
}

// Returns false when the entry's entity is gone, so the caller drops it and tries the next one.
bool Editor::ApplyHistoryEntry(const HistoryEntry& p_Entry, const bool p_IsUndo) {
    if (const auto* s_Change = std::get_if<PropertyHistoryEntry>(&p_Entry.Change)) {
        const ZEntityRef s_Entity = ResolveHistoryTarget(s_Change->Target);
        const auto s_Value = ResolveHistoryValue(p_IsUndo ? s_Change->OldValue : s_Change->NewValue);

        if (!s_Entity || !s_Value) {
            Logger::Warn("[Editor] Skipping a property change whose entity no longer exists.");
            return false;
        }

        SetPropertyValueInternal(s_Entity, s_Change->PropertyId, *s_Value, std::nullopt, ChangeMode::Revert);
        return true;
    }

    if (const auto* s_Change = std::get_if<TransformHistoryEntry>(&p_Entry.Change)) {
        const ZEntityRef s_Entity = ResolveHistoryTarget(s_Change->Target);

        if (!s_Entity) {
            Logger::Warn("[Editor] Skipping a transform change whose entity no longer exists.");
            return false;
        }

        SetEntityTransformInternal(
            s_Entity, p_IsUndo ? s_Change->OldTransform : s_Change->NewTransform, false, std::nullopt,
            ChangeMode::Revert
        );
        return true;
    }

    if (const auto* s_Change = std::get_if<NameHistoryEntry>(&p_Entry.Change)) {
        const ZEntityRef s_Entity = ResolveHistoryTarget(s_Change->Target);

        if (!s_Entity) {
            Logger::Warn("[Editor] Skipping a rename whose entity no longer exists.");
            return false;
        }

        SetEntityNameInternal(
            s_Entity, p_IsUndo ? s_Change->OldName : s_Change->NewName, std::nullopt, ChangeMode::Revert
        );
        return true;
    }

    const auto s_SpawnChange = std::get_if<SpawnHistoryEntry>(&p_Entry.Change);
    const auto s_DestroyChange = std::get_if<DestroyHistoryEntry>(&p_Entry.Change);

    const SpawnInfo& s_Spawn = s_SpawnChange ? s_SpawnChange->Spawn : s_DestroyChange->Spawn;
    const ZEntityRef s_Existing = ResolveHistoryTarget({ .SpawnedRootId = s_Spawn.EntityId });

    // Undoing a spawn or redoing a destroy deletes the entity, the other two spawn it again.
    if (static_cast<bool>(s_SpawnChange) == p_IsUndo) {
        if (!s_Existing) {
            Logger::Warn("[Editor] Skipping the delete of entity {:016x}, which no longer exists.", s_Spawn.EntityId);
            return false;
        }

        DestroyEntityInternal(s_Existing, std::nullopt, ChangeMode::Revert);
        return true;
    }

    if (s_Existing) {
        Logger::Warn("[Editor] Skipping the spawn of entity {:016x}, which already exists.", s_Spawn.EntityId);
        return false;
    }

    if (!SpawnEntityInternal(s_Spawn)) {
        return false;
    }

    // The entity comes back as its factory made it, so we apply its earlier edits again.
    if (s_DestroyChange) {
        ReplayHistory(s_Spawn.EntityId);

        if (const ZEntityRef s_Entity = ResolveHistoryTarget({ .SpawnedRootId = s_Spawn.EntityId })) {
            SetEntityTransformInternal(s_Entity, s_Spawn.Transform, false, std::nullopt, ChangeMode::Revert);
        }
    }

    return true;
}

void Editor::ReplayHistory(const uint64_t p_SpawnedRootId) {
    std::vector<HistoryEntry> s_Entries;

    {
        std::scoped_lock s_Lock(m_ChangeQueueMutex);

        for (const auto& s_Entry : m_UndoHistory) {
            const HistoryTarget* s_Target = nullptr;

            if (const auto* s_Change = std::get_if<PropertyHistoryEntry>(&s_Entry.Change)) {
                s_Target = &s_Change->Target;
            }
            else if (const auto* s_Change = std::get_if<TransformHistoryEntry>(&s_Entry.Change)) {
                s_Target = &s_Change->Target;
            }

            if (s_Target && s_Target->SpawnedRootId == p_SpawnedRootId) {
                s_Entries.push_back(s_Entry);
            }
        }
    }

    for (const auto& s_Entry : s_Entries) {
        ApplyHistoryEntry(s_Entry, false);
    }
}

void Editor::Undo() {
    StepHistory(m_UndoHistory, m_RedoHistory, true);
}

void Editor::Redo() {
    StepHistory(m_RedoHistory, m_UndoHistory, false);
}

void Editor::StepHistory(std::deque<HistoryEntry>& p_From, std::deque<HistoryEntry>& p_To, const bool p_IsUndo) {
    while (true) {
        std::optional<HistoryEntry> s_Entry;

        {
            std::scoped_lock s_Lock(m_ChangeQueueMutex);

            if (p_From.empty()) {
                return;
            }

            s_Entry.emplace(std::move(p_From.back()));
            p_From.pop_back();
        }

        if (ApplyHistoryEntry(*s_Entry, p_IsUndo)) {
            std::scoped_lock s_Lock(m_ChangeQueueMutex);
            p_To.push_back(std::move(*s_Entry));
            return;
        }
    }
}

void Editor::HandleUndoKeys(const bool p_HasFocus) {
    // ImGui handles Ctrl+Z in text fields itself.
    if (!p_HasFocus || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() || ImGuizmo::IsUsing()) {
        return;
    }

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
        QueueTask([this] { Undo(); });
    }
    else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y) ||
        ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) {
        QueueTask([this] { Redo(); });
    }
}
