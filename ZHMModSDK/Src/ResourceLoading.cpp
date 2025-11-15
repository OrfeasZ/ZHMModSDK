#include "ModSDK.h"

#include <Globals.h>
#include <Functions.h>
#include <Logging.h>
#include <zhmmodsdk_rs.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZResource.h>
#include <Util/StringUtils.h>
#include <ResourceLib_HM3.h>
#include <simdjson.h>
#include <filesystem>

static void WaitForResources() {
    while (!Globals::ResourceManager->DoneLoading()) {
        Logger::Debug("Waiting for resources to load (left: {})!", Globals::ResourceManager->m_nNumProcessing);
        Globals::ResourceManager->Update(true);
    }
}

void ModSDK::LoadResourceChunkMap() {
    if (m_ResourceIdToChunkMap.size() > 0) {
        return;
    }

    // Load resid -> chunk map.
    char s_ExePathStr[MAX_PATH];
    auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

    if (s_PathSize <= 0) {
        Logger::Error("Failed to load resource chunk map. Spawning entities from unloaded chunks will not work.");
        return;
    }

    const std::filesystem::path s_ExePath(s_ExePathStr);
    const auto s_ExeDir = s_ExePath.parent_path();

    Logger::Info("Loading resource chunk map (exe dir = {}). This might take a while...", s_ExeDir.string());

    const auto s_ResourceChunkMap = get_resource_chunk_map(s_ExeDir.string().c_str());

    if (!s_ResourceChunkMap) {
        Logger::Error("Failed to load resource chunk map. Spawning entities from unloaded chunks will not work.");
        return;
    }

    Logger::Debug("Resource chunk map loaded with {} entries.", s_ResourceChunkMap->entries.len);
    m_ResourceIdToChunkMap.reserve(s_ResourceChunkMap->entries.len);

    for (size_t i = 0; i < s_ResourceChunkMap->entries.len; ++i) {
        const auto& s_Entry = s_ResourceChunkMap->entries.ptr[i];
        const auto s_Rid = ZRuntimeResourceID(s_Entry.rid);
        m_ResourceIdToChunkMap[s_Rid].push_back(s_Entry.chunk_index);
    }

    free_resource_chunk_map(s_ResourceChunkMap);

    Logger::Info("Finished loading resource chunk map! Found {} entries.", m_ResourceIdToChunkMap.size());
}

bool ModSDK::IsChunkMounted(uint32_t p_ChunkIndex) {
    std::string s_ChunkName = std::format("chunk{}.rpkg", p_ChunkIndex);

    for (const auto& s_Package : (*Globals::ResourceContainer)->m_MountedPackages) {
        // If the package ends with the chunk name, it's mounted.
        if (Util::StringUtils::EndsWith(s_Package.c_str(), s_ChunkName)) {
            return true;
        }
    }

    return false;
}

void ModSDK::MountChunk(uint32_t p_ChunkIndex) {
    std::vector<IPackageManager::SPartitionInfo*> s_ChunksToMount;

    for (auto& s_Info : (*Globals::PackageManager)->m_aPartitionInfos) {
        if (s_Info->m_nIndex == p_ChunkIndex) {
            Logger::Debug("Found chunk {} info. Adding to mount queue.", s_Info->m_nIndex);
            s_ChunksToMount.push_back(s_Info);
        }
    }

    while (s_ChunksToMount.size() > 0) {
        const auto s_Info = s_ChunksToMount.back();

        if (IsChunkMounted(s_Info->m_nIndex)) {
            s_ChunksToMount.pop_back();
            continue;
        }

        if (s_Info->m_pParent) {
            if (!IsChunkMounted(s_Info->m_pParent->m_nIndex)) {
                Logger::Debug(
                    "Parent chunk {} of chunk {} is not mounted. Adding to mount queue.", s_Info->m_pParent->m_nIndex,
                    s_Info->m_nIndex
                );

                s_ChunksToMount.push_back(s_Info->m_pParent);
                continue;
            }
        }

        Logger::Info("Mounting chunk {}...", s_Info->m_nIndex);
        (*Globals::PackageManager)->MountResourcePackagesInPartition(s_Info, nullptr);
        s_ChunksToMount.pop_back();
        WaitForResources();
    }

    Logger::Debug("All requested chunks have been mounted.");
}

const TArray<uint32_t>& ModSDK::GetChunkIndicesForRuntimeResourceId(const ZRuntimeResourceID& id) {
    if (m_ResourceIdToChunkMap.empty()) {
        LoadResourceChunkMap();
    }

    static const TArray<uint32_t> s_Empty;
    auto s_Iterator = m_ResourceIdToChunkMap.find(id);

    return s_Iterator != m_ResourceIdToChunkMap.end() ? s_Iterator->second : s_Empty;
}

std::tuple<ZResourceIndex, ZRuntimeResourceID> ModSDK::LoadResourceFromBIN1(
    ResourceMem* p_ResourceMem, std::string_view p_MetaJson, std::function<void(ZResourcePending*)> p_Install
) {
    // Parse meta, create resource, and register references.
    simdjson::ondemand::parser s_Parser;
    const auto s_Json = simdjson::padded_string(p_MetaJson);
    simdjson::ondemand::document s_JsonMsg = s_Parser.iterate(s_Json);

    const std::string s_ResIdStr {std::string_view(s_JsonMsg["hash_value"])};
    const auto s_ResId = ZRuntimeResourceID::FromString(s_ResIdStr);

    Logger::Debug("Loading resource {}...", s_ResId);

    // Create a new resource index.
    ZResourceIndex s_Index;
    Functions::ZResourceContainer_AddResourceInternal->Call(*Globals::ResourceContainer, s_Index, s_ResId);

    Logger::Debug("Got resource index {} for rid {}.", s_Index.val, s_ResId);
    Logger::Debug("Collecting references from meta...");

    std::vector<std::pair<ZRuntimeResourceID, SResourceReferenceFlags>> s_References;
    for (auto s_Ref : s_JsonMsg["hash_reference_data"]) {
        const std::string s_Hash {std::string_view(s_Ref["hash"])};
        const std::string s_Flag {std::string_view(s_Ref["flag"])};

        const auto s_RefId = ZRuntimeResourceID::FromString(s_Hash);

        // Parse flag as hex byte.
        const SResourceReferenceFlags s_Flags {
            .flags = static_cast<uint8_t>(std::stoul(s_Flag, nullptr, 16))
        };

        s_References.emplace_back(s_RefId, s_Flags);
    }

    Logger::Debug("Found {} references!", s_References.size());

    if (s_References.size() > 0) {
        LoadResourceChunkMap();
    }

    // Make sure that the chunks these references are in are loaded.
    for (const auto& [s_RefId, _] : s_References) {
        if (!m_ResourceIdToChunkMap.contains(s_RefId)) {
            Logger::Error("Resource {} is not in any chunk!", s_RefId);
            continue;
        }

        const auto s_ChunkIndex = m_ResourceIdToChunkMap[s_RefId][0];

        if (!IsChunkMounted(s_ChunkIndex)) {
            Logger::Debug("Reference {} is in chunk {}. Mounting...", s_RefId, s_ChunkIndex);
            MountChunk(s_ChunkIndex);
        }
    }

    auto& s_ResInfo = (*Globals::ResourceContainer)->m_resources[s_Index.val];
    s_ResInfo.refCount = 99; // TODO: Fix.
    s_ResInfo.firstReferenceIndex = (*Globals::ResourceContainer)->m_references.size();
    s_ResInfo.numReferences = s_References.size();
    s_ResInfo.dataSize = p_ResourceMem->DataSize;
    s_ResInfo.compressedDataSize = p_ResourceMem->DataSize;
    s_ResInfo.dataOffset = 0;

    for (const auto& [s_RefId, s_RefFlags] : s_References) {
        Logger::Debug("Adding reference {} -> {} (flags = {:x}).", s_ResId, s_RefId, s_RefFlags.flags);
        Functions::ZResourceContainer_AddResourceReferenceInternal->Call(
            *Globals::ResourceContainer, s_RefId, s_RefFlags
        );
    }

    Functions::ZResourceContainer_AcquireReferences->Call(*Globals::ResourceContainer, s_Index);
    WaitForResources();

    Logger::Debug("All references loaded! Creating readers and installing resource...");

    auto* s_Buffer = static_cast<ZResourceDataBuffer*>((*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
        sizeof(ZResourceDataBuffer),
        alignof(ZResourceDataBuffer)
    ));

    new(s_Buffer) ZResourceDataBuffer();

    s_Buffer->m_pData = const_cast<void*>(p_ResourceMem->ResourceData);
    s_Buffer->m_nSize = p_ResourceMem->DataSize;
    s_Buffer->m_iRefCount = 69;
    s_Buffer->m_nCapacity = p_ResourceMem->DataSize;
    s_Buffer->m_bOwnsDataPtr = false;

    ZResourceDataPtr s_DataPtr {.m_pObject = s_Buffer};

    auto* s_Reader = static_cast<ZResourceReader*>((*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
        sizeof(ZResourceReader),
        alignof(ZResourceReader)
    ));

    Functions::ZResourceReader_ZResourceReader->Call(
        s_Reader,
        &s_Index,
        &s_DataPtr,
        p_ResourceMem->DataSize
    );

    ZResourcePending s_Pending {};
    s_Pending.m_pResource.m_nResourceIndex = s_Index.val;
    s_Pending.m_pResourceReader.m_pObject = s_Reader;

    // Increment m_nNumProcessing by 1 because installing will set the
    // resource status to valid, which will decrement m_nNumProcessing.
    InterlockedIncrement(&Globals::ResourceManager->m_nNumProcessing);
    p_Install(&s_Pending);

    // TODO: Free s_Reader, s_Buffer, etc.
    WaitForResources();

    return std::make_tuple(s_Index, s_ResId);
}

bool ModSDK::LoadQnEntity(
    const ZString& p_Json,
    TResourcePtr<ZTemplateEntityBlueprintFactory>& p_BlueprintFactoryOut,
    TResourcePtr<ZTemplateEntityFactory>& p_FactoryOut
) {
    Logger::Debug("Converting QN entity JSON to RT JSON and meta...");

    const auto s_QnData = convert_qn_entity(p_Json.c_str());

    if (!s_QnData) {
        Logger::Error("Failed to convert QN entity to RT.");
        return false;
    }

    Logger::Debug("Converted from QN to RT! Generating BIN1 resources from RT JSON...");

    const auto s_ResourceTempMem = HM3_GetGeneratorForResource("TEMP")->FromJsonStringToResourceMem(
        s_QnData->factory_json,
        strlen(s_QnData->factory_json),
        false
    );

    const auto s_ResourceTbluMem = HM3_GetGeneratorForResource("TBLU")->FromJsonStringToResourceMem(
        s_QnData->blueprint_json,
        strlen(s_QnData->blueprint_json),
        false
    );

    std::string s_FactoryMetaJson {s_QnData->factory_meta_json};
    std::string s_BlueprintMetaJson {s_QnData->blueprint_meta_json};

    // Free the QN data.
    free_qn_converted_data(s_QnData);

    if (!s_ResourceTbluMem || !s_ResourceTempMem) {
        Logger::Error("Failed to generate editor resources.");

        return false;
    }

    Logger::Debug("Creating TBLU resource...");

    auto [s_TbluIndex, s_TbluId] = LoadResourceFromBIN1(
        s_ResourceTbluMem, s_BlueprintMetaJson,
        [](ZResourcePending* r) { Functions::ZTemplateBlueprintInstaller_Install->Call(nullptr, r); }
    );

    Logger::Debug("TBLU rid = {}, index = {}", s_TbluId, s_TbluIndex.val);

    Logger::Debug("Creating TEMP resource...");

    auto [s_TempIndex, s_TempId] = LoadResourceFromBIN1(
        s_ResourceTempMem, s_FactoryMetaJson,
        [](ZResourcePending* r) { Functions::ZTemplateInstaller_Install->Call(nullptr, r); }
    );

    Logger::Debug("TEMP rid = {}, index = {}", s_TempId, s_TempIndex.val);

    Globals::ResourceManager->GetResourcePtr(p_BlueprintFactoryOut, s_TbluId, 0);

    if (!p_BlueprintFactoryOut) {
        Logger::Error("Could not get created TBLU resource.");
        return false;
    }

    Globals::ResourceManager->GetResourcePtr(p_FactoryOut, s_TempId, 0);

    if (!p_FactoryOut) {
        Logger::Error("Could not get created TEMP resource.");
        return false;
    }

    return true;
}
