#include "Glacier/ZActor.h"
#include "Functions.h"

ZString ZActor::GetActorName() const {
    static TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID) {
        const auto s_RepositoryData = static_cast<THashMap<
            ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.
                GetResourceData());

        ZEntityRef s_EntityRef;

        GetID(s_EntityRef);

        ZRepositoryID s_RepoId = s_EntityRef.GetProperty<ZRepositoryID>("RepositoryId").Get();
        auto s_Iterator = s_RepositoryData->find(s_RepoId);

        if (s_Iterator != s_RepositoryData->end()) {
            ZDynamicObject s_DynamicObject = s_Iterator->second;
            TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject.As<TArray<
                SDynamicObjectKeyValuePair>>();

            for (auto& s_Entry : *s_Entries) {
                if (s_Entry.sKey == "Name") {
                    return *s_Entry.value.As<ZString>();
                }
            }
        }
    }

    return m_sActorName;
}