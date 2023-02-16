#include <Editor.h>

#include <Glacier/EntityFactory.h>
#include <Glacier/ZModule.h>

#include "Logging.h"

bool HasChildEntity(ZEntityRef p_Entity, ZEntityRef p_ChildEntity, IEntityBlueprintFactory* p_Factory, ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity)
{
    for (int i = 0; i < p_BrickFactory->GetSubEntitiesCount(); ++i)
    {
        const ZEntityRef s_SubEntity = p_BrickFactory->GetSubEntity(p_BrickEntity.m_pEntity, i);

        if (s_SubEntity.GetLogicalParent() == p_Entity)
        {
            if (s_SubEntity == p_ChildEntity)
                return true;

            if (HasChildEntity(s_SubEntity, p_ChildEntity, p_BrickFactory->GetSubEntityBlueprint(i), p_BrickFactory, p_BrickEntity))
                return true;
        }
    }

    if (p_Factory && p_Factory->GetSubEntitiesCount() > 0)
    {
        const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Factory);
        const ZEntityRef s_RootSubEntity = s_BpFactory->GetSubEntity(p_Entity.m_pEntity, s_BpFactory->m_rootEntityIndex);

        return HasChildEntity(
            s_RootSubEntity,
            p_ChildEntity,
            s_BpFactory->GetSubEntityBlueprint(s_BpFactory->m_rootEntityIndex),
            s_BpFactory,
            p_Entity
        );
    }

    return false;
}

void Editor::RenderEntity(int p_Index, ZEntityRef p_Entity, uint64_t p_EntityId, IEntityBlueprintFactory* p_Factory, ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity)
{
    const auto s_EntityTypeName = p_Entity.GetEntity()->GetType()->m_pInterfaces->operator[](0).m_pTypeId->typeInfo()->m_pTypeName;
    const auto s_EntityText = fmt::format("{}::{} ({:08x})", p_BrickFactory->m_pTemplateEntityBlueprint ? p_BrickFactory->m_pTemplateEntityBlueprint->subEntities[p_Index].entityName.c_str() : "<noname>", s_EntityTypeName, p_EntityId);

    ImGuiTreeNodeFlags s_Flags = ImGuiTreeNodeFlags_SpanFullWidth;

    bool s_HasChildren = p_Factory && p_Factory->GetSubEntitiesCount() > 0;

    if (!s_HasChildren)
    {
        for (int i = 0; i < p_BrickFactory->GetSubEntitiesCount(); ++i)
        {
            const ZEntityRef s_SubEntity = p_BrickFactory->GetSubEntity(p_BrickEntity.m_pEntity, i);

            if (s_SubEntity.GetLogicalParent() == p_Entity)
            {
                s_HasChildren = true;
                break;
            }
        }
    }

    if (!s_HasChildren)
        s_Flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;

    if (m_SelectedEntity == p_Entity)
    {
        s_Flags |= ImGuiTreeNodeFlags_Selected;

        if (m_ShouldScrollToEntity)
        {
            ImGui::SetScrollHereX();
            ImGui::SetScrollHereY();
        }
    }
    else if (m_ShouldScrollToEntity && m_SelectedEntity && m_SelectedEntity.IsAnyParent(p_Entity))
    {
        ImGui::SetNextTreeNodeOpen(true);
    }

    if (ImGui::TreeNodeEx(s_EntityText.c_str(), s_Flags))
    {
        if (ImGui::IsItemClicked())
        {
            Logger::Debug("Setting selected entity to {}", fmt::ptr(p_Entity.GetEntity()));
            m_SelectedEntity = p_Entity;
        }

        if (s_HasChildren)
        {
            for (int i = 0; i < p_BrickFactory->GetSubEntitiesCount(); ++i)
            {
                const ZEntityRef s_SubEntity = p_BrickFactory->GetSubEntity(p_BrickEntity.m_pEntity, i);

                if (s_SubEntity.GetLogicalParent() == p_Entity)
                {
                    RenderEntity(i, s_SubEntity, p_BrickFactory->GetSubEntityId(i), p_BrickFactory->GetSubEntityBlueprint(i), p_BrickFactory, p_BrickEntity);
                }
            }

            if (p_Factory && p_Factory->GetSubEntitiesCount() > 0)
            {
                const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Factory);
                const ZEntityRef s_RootSubEntity = s_BpFactory->GetSubEntity(p_Entity.m_pEntity, s_BpFactory->m_rootEntityIndex);

                RenderEntity(
                    s_BpFactory->m_rootEntityIndex,
                    s_RootSubEntity,
                    s_BpFactory->GetSubEntityId(s_BpFactory->m_rootEntityIndex),
                    s_BpFactory->GetSubEntityBlueprint(s_BpFactory->m_rootEntityIndex),
                    s_BpFactory,
                    p_Entity
                );
            }

            ImGui::TreePop();
        }
    }
}

void Editor::RenderBrick(ZEntityRef p_Entity)
{
    const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(p_Entity.GetBlueprintFactory());
    const auto s_SubEntityCount = s_BpFactory->GetSubEntitiesCount();

    //ImGui::Text(fmt::format("Brick Entity = {}, id = {:x}, bp = {}, sub = {}", fmt::ptr(p_Entity.GetEntity()), p_Entity->GetType()->m_nEntityId, fmt::ptr(s_BpFactory), s_SubEntityCount).c_str());

    if (s_SubEntityCount == 0)
        return;

    for (int i = 0; i < s_SubEntityCount; ++i)
    {
        const ZEntityRef s_SubEntity = s_BpFactory->GetSubEntity(p_Entity.m_pEntity, i);

        if (!s_SubEntity)
            continue;

        if (i == s_BpFactory->m_rootEntityIndex || !s_SubEntity.GetLogicalParent())
        {
            RenderEntity(
                i,
                s_SubEntity,
                s_BpFactory->GetSubEntityId(i),
                s_BpFactory->GetSubEntityBlueprint(i),
                s_BpFactory,
                p_Entity
            );
        }
    }
}

void Editor::DrawEntityTree()
{
    ImGui::SetNextWindowPos({ 0, 110 });
    ImGui::SetNextWindowSize({ 700, ImGui::GetIO().DisplaySize.y - 110 });
    ImGui::Begin("Entities", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (s_SceneCtx && s_SceneCtx->m_pScene)
    {
        std::string s_PreviewLabel = "No bricks loaded";

        if (s_SceneCtx->m_aLoadedBricks.size() > 0)
            s_PreviewLabel = fmt::format("{:016X}", s_SceneCtx->m_aLoadedBricks[m_SelectedBrickIndex].runtimeResourceID.GetID());

        if (ImGui::BeginCombo("Bricks", s_PreviewLabel.c_str()))
        {
            for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i)
            {
                const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];
                const auto s_Selected = i == m_SelectedBrickIndex;

                if (ImGui::Selectable(fmt::format("{:016X}", s_Brick.runtimeResourceID.GetID()).c_str(), s_Selected))
                    m_SelectedBrickIndex = i;

                if (s_Selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        if (s_SceneCtx->m_aLoadedBricks.size() > 0)
        {
            RenderBrick(s_SceneCtx->m_aLoadedBricks[m_SelectedBrickIndex].entityRef);
        }
    }

    ImGui::End();

    if (m_ShouldScrollToEntity)
        m_ShouldScrollToEntity = false;
}

