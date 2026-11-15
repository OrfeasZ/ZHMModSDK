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
#include <charconv>
#include <Util/ResourceUtils.h>
#include <Util/ProcessUtils.h>

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

void ModSDK::UnmountChunk(uint32_t p_ChunkIndex, bool p_RemountChunksBelow) {
    if (!IsChunkMounted(p_ChunkIndex)) {
        return;
    }

    ZPackageManagerBase* s_PackageManagerBase = *Globals::PackageManager;
    uint32_t s_ParentChunkIndex = -1;
    bool s_UnmountParentChunk = false;
    bool s_IsPartitionFound = false;

    for (auto& s_PartitionInfo : s_PackageManagerBase->m_aPartitionInfos) {
        if (s_PartitionInfo->m_nIndex == p_ChunkIndex) {
            s_IsPartitionFound = true;

            if (s_PartitionInfo->m_pParent) {
                s_ParentChunkIndex = s_PartitionInfo->m_pParent->m_nIndex;
                s_UnmountParentChunk = true;
            }

            break;
        }
    }

    if (!s_IsPartitionFound) {
        Logger::Error("Can't unmount chunk {}: no matching partition found!", p_ChunkIndex);
        return;
    }

    if (s_ParentChunkIndex != -1) {
        for (auto& s_PartitionInfo : s_PackageManagerBase->m_aPartitionInfos) {
            if (s_PartitionInfo->m_nIndex == p_ChunkIndex) {
                continue;
            }

            if (!s_PartitionInfo->m_pParent) {
                continue;
            }

            if (s_PartitionInfo->m_pParent->m_nIndex != s_ParentChunkIndex) {
                continue;
            }

            if (IsChunkMounted(s_PartitionInfo->m_nIndex)) {
                s_UnmountParentChunk = false;
                break;
            }
        }
    }

    ZResourceContainer* s_ResourceContainer = *Globals::ResourceContainer;
    uint8_t s_PackageId = 0;

    for (size_t i = 0; i < s_ResourceContainer->m_MountedPackages.size(); ++i) {
        const ZString& s_MountedPackage = s_ResourceContainer->m_MountedPackages[i];
        auto s_ChunkIndex = Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_MountedPackage);

        if (!s_ChunkIndex) {
            continue;
        }

        if (s_UnmountParentChunk) {
            if (s_ChunkIndex == s_ParentChunkIndex) {
                s_PackageId = i;
                break;
            }
        }
        else if (s_ChunkIndex == p_ChunkIndex) {
            s_PackageId = i;
            break;
        }
    }

    std::unordered_set<uint32_t> s_ChunksToRemount;

    if (p_RemountChunksBelow) {
        for (size_t i = s_PackageId; i < s_ResourceContainer->m_MountedPackages.size(); ++i) {
            const ZString& s_MountedPackage = s_ResourceContainer->m_MountedPackages[i];
            auto s_ChunkIndex = Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_MountedPackage);

            if (!s_ChunkIndex) {
                continue;
            }

            if (s_ChunkIndex == p_ChunkIndex) {
                continue;
            }

            s_ChunksToRemount.insert(*s_ChunkIndex);
        }
    }

    for (size_t i = 0; i < s_ResourceContainer->m_firstPackageIndexPerMountedPartition.size(); ++i) {
        if (s_ResourceContainer->m_firstPackageIndexPerMountedPartition[i] == s_PackageId) {
            s_PackageManagerBase->UnmountPartitions(i);
            break;
        }
    }

    if (p_RemountChunksBelow) {
        for (uint32_t s_ChunkIndex : s_ChunksToRemount) {
            MountChunk(s_ChunkIndex);
        }
    }

    Logger::Info(
        "Unmounted chunk {}{}",
        p_ChunkIndex,
        p_RemountChunksBelow ? " and remounted chunks below" : ""
    );
}

const TArray<uint32_t>& ModSDK::GetChunkIndicesForRuntimeResourceId(const ZRuntimeResourceID& id) {
    if (m_ResourceIdToChunkMap.empty()) {
        LoadResourceChunkMap();
    }

    static const TArray<uint32_t> s_Empty;
    auto s_Iterator = m_ResourceIdToChunkMap.find(id);

    return s_Iterator != m_ResourceIdToChunkMap.end() ? s_Iterator->second : s_Empty;
}

size_t ModSDK::GetResourcePackageCount(uint32_t p_ChunkIndex) {
    if (m_ChunkIndexToResourcePackageCount.empty()) {
        char s_ExePathStr[MAX_PATH] {};
        GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

        const auto s_RuntimeDirectory = std::filesystem::path(s_ExePathStr).parent_path().parent_path() / "Runtime";
        std::error_code s_Error;

        for (const auto& s_Entry : std::filesystem::directory_iterator(s_RuntimeDirectory, s_Error)) {
            const std::string s_Name = s_Entry.path().filename().string();

            if (s_Entry.path().extension() != ".rpkg" || !s_Name.starts_with("chunk")) {
                continue;
            }

            uint32_t s_ChunkIndex = 0;
            const auto s_Result = std::from_chars(s_Name.data() + 5, s_Name.data() + s_Name.size(), s_ChunkIndex);

            if (s_Result.ec == std::errc()) {
                ++m_ChunkIndexToResourcePackageCount[s_ChunkIndex];
            }
        }

        if (s_Error) {
            Logger::Error("Failed to list the resource packages in {}: {}.", s_RuntimeDirectory.string(), s_Error.message());
        }
    }

    const auto s_Iterator = m_ChunkIndexToResourcePackageCount.find(p_ChunkIndex);

    return s_Iterator != m_ChunkIndexToResourcePackageCount.end() ? s_Iterator->second : 0;
}

// The engine stores package IDs in a signed byte, which limits it to 128 mounted packages, and ZWwiseIOHookBlocking
// keeps one stream per package in a fixed array of 127. These patches read the IDs as unsigned bytes and give that
// array 255 slots, which raises the limit to 255 packages. 0xFF means "not mounted".
constexpr size_t c_SignedMaxResourcePackageCount = 127;
constexpr size_t c_UnsignedMaxResourcePackageCount = 255;

static bool FindCodePatches(std::vector<ModSDK::SCodePatch>& p_Patches) {
    bool s_AreAllFound = true;

    for (auto& s_Patch : p_Patches) {
        const auto s_Match = Util::ProcessUtils::SearchPattern(
            ModSDK::GetInstance()->GetModuleBase(),
            ModSDK::GetInstance()->GetSizeOfCode(),
            reinterpret_cast<const uint8_t*>(s_Patch.Pattern),
            s_Patch.Mask
        );

        if (s_Match == 0) {
            Logger::Warn("Could not find the code for patch '{}'.", s_Patch.Name);
            s_AreAllFound = false;
            continue;
        }

        s_Patch.Address = reinterpret_cast<uint8_t*>(s_Match + s_Patch.Offset);

        if (memcmp(s_Patch.Address, s_Patch.Original.data(), s_Patch.Original.size()) != 0) {
            Logger::Warn("The code for patch '{}' is not what was expected.", s_Patch.Name);
            s_AreAllFound = false;
        }
    }

    return s_AreAllFound;
}

static void ApplyCodePatches(const std::vector<ModSDK::SCodePatch>& p_Patches) {
    for (const auto& s_Patch : p_Patches) {
        DWORD s_OldProtect;
        VirtualProtect(s_Patch.Address, s_Patch.Patched.size(), PAGE_EXECUTE_READWRITE, &s_OldProtect);
        memcpy(s_Patch.Address, s_Patch.Patched.data(), s_Patch.Patched.size());
        VirtualProtect(s_Patch.Address, s_Patch.Patched.size(), s_OldProtect, &s_OldProtect);
    }
}

void ModSDK::PatchResourcePackageLimit() {
    std::vector<SCodePatch> s_Patches {
        {
            "ZResourceContainer::MountPackages package ID",
            "\x44\x0F\xBE\xB5\x88\x00\x00\x00",
            "xxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZDirectResourceStreamer package ID",
            "\x4C\x0F\xBE\x74\x0A\x3E\x48\x8B\x4E\x78",
            "xxxxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZDirectResourceStreamer thread package ID",
            "\x48\x0F\xBE\x94\x24\xC0\x00\x00\x00\x3A\x94\x24",
            "xxxxxxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZFileSystemDevice package ID",
            "\x48\x0F\xBE\x7C\x0A\x3E\x40\x80\xFF\xFF",
            "xxxxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZResourceContainer::FindPackage package ID",
            "\x4C\x0F\xBE\x4C\x03\x3E\x45\x84\xC9\x0F\x88",
            "xxxxxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZResourceContainer::FindPackage sign check",
            "\x4C\x0F\xBE\x4C\x03\x3E\x45\x84\xC9\x0F\x88",
            "xxxxxxxxxxx",
            9,
            {0x0F, 0x88},
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90},
        },
        {
            "ZOverlappedDevice package ID",
            "\x4C\x0F\xBE\x74\x29\x3E\x45\x85\xF6\x0F\x88",
            "xxxxxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZWwiseIOHookBlocking::Open package ID",
            "\x48\x0F\xBE\x44\x0A\x3E\x0F\xB6\x74\x0A\x3F",
            "xxxxxxxxxxx",
            2,
            {0xBE},
            {0xB6},
        },
        {
            "ZWwiseIOHookBlocking::Open sign check",
            "\x84\xC0\x0F\x88\x00\x00\x00\x00\x48\x8B\x5C\xC5\x18",
            "xxxx????xxxxx",
            0,
            {0x84, 0xC0, 0x0F, 0x88},
            {0x3C, 0xFF, 0x0F, 0x84},
        },
        {
            "ZWwiseIOHookBlocking allocation size",
            "\xBA\x10\x04\x00\x00\x48\x8B\x48\x10\x48\x8B\x01\xFF\x50\x38\x48\x8B\xD8\x48\x85\xC0\x74\x00\x48\x8D\x05",
            "xxxxxxxxxxxxxxxxxxxxxx?xxx",
            1,
            {0x10, 0x04},
            {0x10, 0x08},
        },
    };

    m_WwiseDestructorPatches = {
        {
            "ZWwiseIOHookBlocking destructor end",
            "\x48\x8D\xBE\x10\x04\x00\x00\x48\x3B\xDF",
            "xxxxxxxxxx",
            3,
            {0x10, 0x04},
            {0x10, 0x08},
        },
        {
            "ZWwiseIOHookBlocking free size",
            "\xBA\x10\x04\x00\x00\x48\x8B\xCE\xE8\x00\x00\x00\x00\x48\x8B\xC6\x48\x83\xC4\x40\x5E\xC3",
            "xxxxxxxxx????xxxxxxxxx",
            1,
            {0x10, 0x04},
            {0x10, 0x08},
        },
    };

    const bool s_ArePatchesFound = FindCodePatches(s_Patches);

    if (!FindCodePatches(m_WwiseDestructorPatches) || !s_ArePatchesFound) {
        Logger::Warn("Could not raise the resource package limit. At most {} packages can be mounted.", c_SignedMaxResourcePackageCount);
        return;
    }

    ApplyCodePatches(s_Patches);
    Hooks::ZWwiseIOHookBlocking_InitPackageStreams->AddDetour(this, &ModSDK::ZWwiseIOHookBlocking_InitPackageStreams);
}

size_t ModSDK::GetMaxResourcePackageCount() {
    return m_IsResourcePackageLimitRaised ? c_UnsignedMaxResourcePackageCount : c_SignedMaxResourcePackageCount;
}

DEFINE_DETOUR_WITH_CONTEXT(ModSDK, void*, ZWwiseIOHookBlocking_InitPackageStreams, void** th) {
    p_Hook->CallOriginal(th);

    // The original clears the first 127 slots, of the 255 the patched allocation has, so
    // we need to clear the rest.
    std::fill(th + c_SignedMaxResourcePackageCount, th + c_UnsignedMaxResourcePackageCount, nullptr);

    ApplyCodePatches(m_WwiseDestructorPatches);
    m_IsResourcePackageLimitRaised = true;

    Logger::Info("Raised the resource package limit to {}.", c_UnsignedMaxResourcePackageCount);

    return HookResult<void*>(HookAction::Return(), th);
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

    std::string_view s_TypeStr = std::string_view(s_JsonMsg["hash_resource_type"]);

    uint32_t s_Type =
        (static_cast<uint32_t>(static_cast<uint8_t>(s_TypeStr[0])) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(s_TypeStr[1])) << 16)
        | (static_cast<uint32_t>(static_cast<uint8_t>(s_TypeStr[2])) << 8) | (static_cast<uint32_t>(static_cast<uint8_t>(s_TypeStr[3])));

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
    s_ResInfo.dataOffset = 0;
    s_ResInfo.dataSize = p_ResourceMem->DataSize;
    s_ResInfo.compressedDataSize = p_ResourceMem->DataSize;
    s_ResInfo.refCount = 2; // One reference held by the output TResourcePtr, one by ZResourcePending during installation.
    s_ResInfo.firstReferenceIndex = (*Globals::ResourceContainer)->m_references.size();
    s_ResInfo.numReferences = s_References.size();
    s_ResInfo.resourceType = s_Type;

    for (const auto& [s_RefId, s_RefFlags] : s_References) {
        Logger::Debug("Adding reference {} -> {} (flags = {:x}).", s_ResId, s_RefId, s_RefFlags.flags);
        Functions::ZResourceContainer_AddResourceReferenceInternal->Call(
            *Globals::ResourceContainer, s_RefId, s_RefFlags
        );
    }

    Functions::ZResourceContainer_AcquireResourceReferences->Call(*Globals::ResourceContainer, s_Index);
    WaitForResources();

    Logger::Debug("All references loaded! Creating readers and installing resource...");

    auto* s_Buffer = static_cast<ZResourceDataBuffer*>((*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
        sizeof(ZResourceDataBuffer),
        alignof(ZResourceDataBuffer)
    ));

    new(s_Buffer) ZResourceDataBuffer();

    s_Buffer->m_pData = const_cast<void*>(p_ResourceMem->ResourceData);
    s_Buffer->m_nSize = p_ResourceMem->DataSize;
    s_Buffer->m_nCapacity = p_ResourceMem->DataSize;
    s_Buffer->m_bOwnsDataPtr = false;

    ZResourceDataPtr s_DataPtr(s_Buffer);

    auto* s_Reader = static_cast<ZResourceReader*>((*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
        sizeof(ZResourceReader),
        alignof(ZResourceReader)
    ));

    Functions::ZResourceReader_ZResourceReader->Call(
        s_Reader,
        s_Index,
        std::move(s_DataPtr),
        p_ResourceMem->DataSize
    );

    ZResourcePending s_Pending {};
    s_Pending.m_pResource.m_nResourceIndex = s_Index.val;
    s_Pending.m_pResourceReader = ZResourceReaderPtr(s_Reader);

    // Increment m_nNumProcessing by 1 because installing will set the
    // resource status to valid, which will decrement m_nNumProcessing.
    InterlockedIncrement(&Globals::ResourceManager->m_nNumProcessing);
    p_Install(&s_Pending);

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
        [&p_BlueprintFactoryOut, s_ResourceTbluMem](ZResourcePending* r) {
        const bool s_IsInstalled = Functions::ZTemplateBlueprintInstaller_Install->Call(nullptr, r);

        if (s_IsInstalled) {
            p_BlueprintFactoryOut.m_nResourceIndex.val = r->m_pResource.m_nResourceIndex.val;
        }

        HM3_GetGeneratorForResource("TBLU")->FreeResourceMem(s_ResourceTbluMem);
    });

    Logger::Debug("TBLU rid = {}, index = {}", s_TbluId, s_TbluIndex.val);

    Logger::Debug("Creating TEMP resource...");

    auto [s_TempIndex, s_TempId] = LoadResourceFromBIN1(
        s_ResourceTempMem, s_FactoryMetaJson,
        [&p_FactoryOut, s_ResourceTempMem](ZResourcePending* r) {
        const bool s_IsInstalled = Functions::ZTemplateInstaller_Install->Call(nullptr, r);

        if (s_IsInstalled) {
            p_FactoryOut.m_nResourceIndex.val = r->m_pResource.m_nResourceIndex.val;
        }

        HM3_GetGeneratorForResource("TEMP")->FreeResourceMem(s_ResourceTempMem);
    });

    Logger::Debug("TEMP rid = {}, index = {}", s_TempId, s_TempIndex.val);

    return true;
}
