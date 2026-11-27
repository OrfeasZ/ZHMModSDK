#include <Editor.h>

#include <cassert>

#include "Hooks.h"

#include <Glacier/ZSpatialEntity.h>

void Editor::QueueTask(std::function<void()> p_Task) {
    std::scoped_lock s_Lock(m_ChangeQueueMutex);
    m_ChangeQueue.push_back({ .Apply = std::move(p_Task) });
}

void Editor::QueueChange(const ChangeKey& p_Key, const ChangeMode p_Mode, std::function<void()> p_Apply) {
    std::scoped_lock s_Lock(m_ChangeQueueMutex);

    const bool s_IsPreview = p_Mode == ChangeMode::Preview;
    const auto s_QueuedPreview = std::ranges::find(m_ChangeQueue, p_Key, &QueuedChange::PreviewKey);

    if (s_QueuedPreview != m_ChangeQueue.end()) {
        if (s_IsPreview) {
            s_QueuedPreview->Apply = std::move(p_Apply);
            return;
        }

        m_ChangeQueue.erase(s_QueuedPreview);
    }

    m_ChangeQueue.push_back({
        .Apply = std::move(p_Apply),
        .PreviewKey = s_IsPreview ? std::optional(p_Key) : std::nullopt,
    });
}

void Editor::ProcessTasks() {
    m_UpdateThreadId = std::this_thread::get_id();

    std::vector<QueuedChange> s_Changes;

    {
        std::scoped_lock s_Lock(m_ChangeQueueMutex);
        s_Changes.swap(m_ChangeQueue);
    }

    for (const auto& s_Change : s_Changes) {
        s_Change.Apply();
    }
}

// Changes from the UI carry entity refs, so we drop the ones whose entity the game deleted
// since. Without a tree we can't tell.
bool Editor::IsEntityInTree(const ZEntityRef p_Entity) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    return !m_CachedEntityTree || m_CachedEntityTreeMap.contains(p_Entity);
}

std::optional<Editor::EditStart> Editor::TakeEditStart(const ChangeKey& p_Key) {
    const auto s_It = std::ranges::find(m_EditStarts, p_Key, &EditStart::Key);

    if (s_It == m_EditStarts.end()) {
        return std::nullopt;
    }

    EditStart s_Start = std::move(*s_It);
    m_EditStarts.erase(s_It);

    return s_Start;
}

void Editor::OnSetPropertyValue(
    ZEntityRef p_Entity,
    uint32_t p_PropertyId,
    const ZObjectRef& p_Value,
    std::optional<std::string> p_ClientId,
    const ChangeMode p_Mode
) {
    QueueChange(
        { p_Entity, p_PropertyId },
        p_Mode,
        [this, p_Entity, p_PropertyId, p_Value, p_ClientId, p_Mode] {
            SetPropertyValueInternal(p_Entity, p_PropertyId, p_Value, p_ClientId, p_Mode);
        }
    );
}

void Editor::SetPropertyValueInternal(
    ZEntityRef p_Entity,
    uint32_t p_PropertyId,
    const ZObjectRef& p_Value,
    std::optional<std::string> p_ClientId,
    const ChangeMode p_Mode
) {
    assert(std::this_thread::get_id() == m_UpdateThreadId);

    if (!IsEntityInTree(p_Entity)) {
        return;
    }

    const SPropertyData* s_Property = p_Entity->GetType()->FindProperty(p_PropertyId);

    if (!s_Property) {
        return;
    }

    const ChangeKey s_Key { p_Entity, p_PropertyId };

    // Physics, obstacles, clients and the history only see the final value of a drag.
    if (p_Mode == ChangeMode::Preview) {
        if (!std::ranges::contains(m_EditStarts, s_Key, &EditStart::Key)) {
            m_EditStarts.push_back({ .Key = s_Key, .Value = GetPropertyValue(p_Entity, s_Property) });
        }

        Hooks::SetPropertyValue->Call(p_Entity, p_PropertyId, p_Value, true);
        return;
    }

    std::optional<EditStart> s_Start = TakeEditStart(s_Key);
    const ZObjectRef s_OldValue = s_Start ? std::move(s_Start->Value) : GetPropertyValue(p_Entity, s_Property);

    Hooks::SetPropertyValue->Call(p_Entity, p_PropertyId, p_Value, true);
    TrackSpawnedEntityReference(p_Entity, p_PropertyId, p_Value);
    RefreshPathfinderObstacle(p_Entity, true);

    if (p_Mode == ChangeMode::Commit) {
        PushHistory({
            .Change = PropertyHistoryEntry {
                .Target = MakeHistoryTarget(p_Entity),
                .PropertyId = p_PropertyId,
                .OldValue = MakeHistoryValue(s_OldValue),
                .NewValue = MakeHistoryValue(p_Value),
            },
            .ClientId = p_ClientId,
        });
    }

    m_Server.OnEntityPropertySet(p_Entity, p_PropertyId, std::move(p_ClientId));
}

void Editor::OnEntityTransformChange(
    ZEntityRef p_Entity, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId,
    const ChangeMode p_Mode
) {
    QueueChange(
        { p_Entity },
        p_Mode,
        [this, p_Entity, p_Transform, p_Relative, p_ClientId, p_Mode] {
            SetEntityTransformInternal(p_Entity, p_Transform, p_Relative, p_ClientId, p_Mode);
        }
    );
}

void Editor::SetEntityTransformInternal(
    ZEntityRef p_Entity, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId,
    const ChangeMode p_Mode
) {
    assert(std::this_thread::get_id() == m_UpdateThreadId);

    if (!IsEntityInTree(p_Entity)) {
        return;
    }

    auto* s_SpatialEntity = p_Entity.QueryInterface<ZSpatialEntity>();

    if (!s_SpatialEntity) {
        return;
    }

    const ChangeKey s_Key { p_Entity };
    SMatrix s_OldTransform = s_SpatialEntity->GetObjectToWorldMatrix();

    if (p_Mode == ChangeMode::Preview) {
        if (!std::ranges::contains(m_EditStarts, s_Key, &EditStart::Key)) {
            m_EditStarts.push_back({ .Key = s_Key, .Transform = s_OldTransform });
        }
    }
    else if (const auto s_Start = TakeEditStart(s_Key)) {
        s_OldTransform = s_Start->Transform;
    }

    if (!p_Relative) {
        s_SpatialEntity->SetObjectToWorldMatrixFromEditor(p_Transform);
    }
    else {
        SMatrix s_ParentTrans;

        // Get parent entity transform.
        if (s_SpatialEntity->m_eidParent.m_pInterfaceRef) {
            s_ParentTrans = s_SpatialEntity->m_eidParent.m_pInterfaceRef->GetObjectToWorldMatrix();
        }
        else if (p_Entity.GetLogicalParent() && p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()) {
            s_ParentTrans = p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
        }
        else if (p_Entity.GetOwningEntity() && p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()) {
            s_ParentTrans = p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
        }

        // Calculate world transform based on the provided relative transform.
        auto s_WorldTrans = p_Transform * s_ParentTrans;
        s_WorldTrans.Trans = s_ParentTrans.Trans + p_Transform.Trans;
        s_WorldTrans.Trans.w = 1.f;

        s_SpatialEntity->SetObjectToWorldMatrixFromEditor(s_WorldTrans);
    }

    if (p_Mode == ChangeMode::Preview) {
        return;
    }

    RefreshStaticPhysicsAndObstacles(p_Entity);

    if (p_Mode == ChangeMode::Commit) {
        PushHistory({
            .Change = TransformHistoryEntry {
                .Target = MakeHistoryTarget(p_Entity),
                .OldTransform = s_OldTransform,
                .NewTransform = s_SpatialEntity->GetObjectToWorldMatrix(),
            },
            .ClientId = p_ClientId,
        });
    }

    m_Server.OnEntityTransformChanged(p_Entity, std::move(p_ClientId));
}

void Editor::SetEntityNameInternal(
    ZEntityRef p_Entity, const std::string& p_Name, std::optional<std::string> p_ClientId, const ChangeMode p_Mode
) {
    assert(std::this_thread::get_id() == m_UpdateThreadId);

    std::string s_OldName;

    {
        std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

        if (const auto s_It = m_EntityNames.find(p_Entity); s_It != m_EntityNames.end()) {
            s_OldName = s_It->second;
        }

        m_EntityNames[p_Entity] = p_Name;
    }

    if (p_Mode == ChangeMode::Commit) {
        PushHistory({
            .Change = NameHistoryEntry {
                .Target = MakeHistoryTarget(p_Entity),
                .OldName = std::move(s_OldName),
                .NewName = p_Name,
            },
            .ClientId = p_ClientId,
        });
    }

    m_Server.OnEntityNameChanged(p_Entity, std::move(p_ClientId));
}

void Editor::CommitPropertyEdit() {
    OnSetPropertyValue(
        m_PropertyEdit->Key.Entity, *m_PropertyEdit->Key.PropertyId, m_PropertyEdit->Value, std::nullopt
    );

    QueueEditCommitted(*m_PropertyEdit);
}

void Editor::CommitGizmoEdit(const ChangeMode p_Mode) {
    auto& s_Edit = *m_GizmoEdit;

    if (p_Mode == ChangeMode::Revert) {
        s_Edit.Transform = s_Edit.StartTransform;
    }

    ZEntityRef s_Entity = s_Edit.Key.Entity;

    // The scale gizmo edits the primitive scale property instead of the transform.
    if (s_Edit.Key.PropertyId) {
        OnSetPropertyValue(
            s_Entity, *s_Edit.Key.PropertyId, ZObjectRef::From<SVector3>(s_Edit.Transform.GetScale()), std::nullopt,
            p_Mode
        );

        // Toggling physics off and on again makes the collision pick up the new scale.
        QueueTask([this, s_Entity]() mutable {
            if (!IsEntityInTree(s_Entity) || s_Entity.GetProperty<bool>("m_bRemovePhysics").Get()) {
                return;
            }

            s_Entity.SetProperty<bool>("m_bRemovePhysics", true);
            s_Entity.SetProperty<bool>("m_bRemovePhysics", false);
        });
    }
    else {
        OnEntityTransformChange(s_Entity, s_Edit.Transform, false, std::nullopt, p_Mode);
    }

    QueueEditCommitted(s_Edit);
}

// Queued after the commit, so the UI drops the edit only once the update thread has applied it.
void Editor::QueueEditCommitted(ActiveEdit& p_Edit) {
    p_Edit.CommitSerial = ++m_LastCommitSerial;

    QueueTask([this, s_Serial = p_Edit.CommitSerial] {
        m_AppliedCommitSerial = s_Serial;
    });
}

// A drag whose widget wasn't drawn this frame lost its release, so we commit it here. A
// committed edit stays until the update thread has applied it.
void Editor::UpdateActiveEdits() {
    // The entities of a cleared scene are gone, so their edits are dropped instead.
    if (m_ShouldResetEdits.exchange(false)) {
        m_PropertyEdit.reset();
        m_GizmoEdit.reset();
        return;
    }

    UpdateActiveEdit(m_PropertyEdit, [this] { CommitPropertyEdit(); });
    UpdateActiveEdit(m_GizmoEdit, [this] { CommitGizmoEdit(ChangeMode::Commit); });
}

void Editor::UpdateActiveEdit(std::optional<ActiveEdit>& p_Edit, const std::function<void()>& p_Commit) {
    if (!p_Edit) {
        return;
    }

    if (p_Edit->IsCommitted()) {
        if (m_AppliedCommitSerial >= p_Edit->CommitSerial) {
            p_Edit.reset();
        }
    }
    else if (!p_Edit->IsDrawn) {
        p_Commit();
    }
    else {
        p_Edit->IsDrawn = false;
    }
}
