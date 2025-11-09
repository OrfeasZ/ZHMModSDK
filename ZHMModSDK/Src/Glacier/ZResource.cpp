#include "Glacier/ZResource.h"

#include "Functions.h"

ZResourcePtr::~ZResourcePtr() {
    if (m_nResourceIndex.val < 0)
        return;

    auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[m_nResourceIndex.val];

    if (InterlockedDecrement(&s_ResourceInfo.refCount) == 0 && s_ResourceInfo.resourceData)
        Functions::ZResourceManager_UninstallResource->Call(Globals::ResourceManager, m_nResourceIndex);
}