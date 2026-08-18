#include "Editor.h"

#include <filesystem>

#include <directx/d3dx12.h>

#include <DirectXTex.h>

#include <IconsMaterialDesign.h>

#include <Glacier/ZComponent.h>

#include <Logging.h>
#include <Util/StringUtils.h>

void Editor::DrawBoxReflectionsWindow(const bool p_HasFocus) {
    if (!p_HasFocus || !m_ShowBoxReflectionsWindow) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_IsWindowExpanded = ImGui::Begin(ICON_MD_VIEW_IN_AR " Box reflections", &m_ShowBoxReflectionsWindow);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (!s_IsWindowExpanded) {
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
        return;
    }

    if (ImGui::Checkbox("Enable box reflection cache", &m_EnableBoxReflectionCache)) {
        (*Globals::ComponentManager)->m_pApplication->SetOption(
            "EnableBoxReflectionCache",
            m_EnableBoxReflectionCache ? "1" : "0"
        );
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Must be enabled before loading a scene to take effect.");
    }

    if (!m_CachedEntityTree || !m_CachedEntityTree->Entity) {
        if (ImGui::Button("Build entity tree")) {
            UpdateEntities();
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
        return;
    }

    const auto& s_BoxReflections = Globals::RenderGraphManager->m_BoxReflections;

    if (s_BoxReflections.empty()) {
        ImGui::TextUnformatted("No box reflections.");

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
        return;
    }

    static char s_OutputFolder[2048] { "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Output folder");
    ImGui::SameLine();

    ImGui::InputText("##OutputFolder", s_OutputFolder, sizeof(s_OutputFolder));

    if (ImGui::Button("Export all cubemaps")) {
        if (!ExportAllBoxReflectionCubemaps(s_OutputFolder, false)) {
            Logger::Error("[Editor] Failed to export all box reflection cubemaps.");
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Generate box reflection cache resource (BOXC)")) {
        if (!GenerateBoxReflectionCacheResource(s_OutputFolder)) {
            Logger::Error("[Editor] Failed to generate box reflection cache resource.");
        }
    }

    ImGui::Separator();

    static char s_EntityName[2048] { "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Entity name");
    ImGui::SameLine();

    ImGui::InputText("##EntityName", s_EntityName, sizeof(s_EntityName));

    ImGui::BeginChild(
        "BoxReflectionList",
        ImVec2(300, 0),
        true,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    for (size_t i = 0; i < s_BoxReflections.size(); ++i) {
        const auto s_BoxReflectionGraphNode = s_BoxReflections[i];

        if (!s_BoxReflectionGraphNode || !s_BoxReflectionGraphNode->m_pRenderableEntity) {
            continue;
        }

        ZEntityRef s_EntityRef;
        s_BoxReflectionGraphNode->m_pRenderableEntity->GetID(s_EntityRef);

        std::string s_BoxReflectionEntityName;

        {
            std::shared_lock s_TreeLock(m_CachedEntityTreeMutex);

            const auto s_Iterator = m_CachedEntityTreeMap.find(s_EntityRef);

            if (s_Iterator != m_CachedEntityTreeMap.end()) {
                s_BoxReflectionEntityName = s_Iterator->second->Name;
            }
        }

        if (!Util::StringUtils::FindSubstring(s_BoxReflectionEntityName, s_EntityName)) {
            continue;
        }

        const std::string s_BoxReflectionLabel = std::format("{}###{}", s_BoxReflectionEntityName, i);

        const bool s_IsSelected = m_SelectedBoxReflectionGraphNode == s_BoxReflectionGraphNode;

        if (ImGui::Selectable(s_BoxReflectionLabel.c_str(), s_IsSelected)) {
            m_SelectedBoxReflectionGraphNode = s_BoxReflectionGraphNode;

            UpdateBoxReflectionPreview(m_SelectedBoxReflectionGraphNode);
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    if (!m_SelectedBoxReflectionGraphNode || !m_SelectedBoxReflectionGraphNode->m_pRenderableEntity) {
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
        return;
    }

    ZEntityRef s_SelectedEntity;
    m_SelectedBoxReflectionGraphNode->m_pRenderableEntity->GetID(s_SelectedEntity);

    ImGui::BeginGroup();
    ImGui::BeginChild("BoxReflectionDetails", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

    const float s_FaceSize = static_cast<float>(Globals::RenderManager->m_pSharedResources->m_nBoxReflectionResolution);

    ImGui::BeginGroup();

    ImGui::TextUnformatted("Cubemap");

    DrawBoxReflectionCross(m_BoxReflectionPreview.m_ImGuiTextures, s_FaceSize);

    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();

    ImGui::TextUnformatted("Diffuse cubemap");

    DrawBoxReflectionCross(m_BoxReflectionPreview.m_DiffuseImGuiTextures, s_FaceSize);

    ImGui::EndGroup();

    ImGui::Separator();

    if (ImGui::Button("Select in entity tree")) {
        OnSelectEntity(
            s_SelectedEntity,
            true,
            std::nullopt
        );
    }

    if (ImGui::Button("Export cubemap")) {
        ZRenderTexture2D* s_Texture = nullptr;
        uint32_t s_CubeIndex = 0;

        if (GetBoxReflectionTexture(m_SelectedBoxReflectionGraphNode, false, s_Texture, s_CubeIndex)) {
            const auto s_OutputFilePath = GetBoxReflectionExportPath(
                m_SelectedBoxReflectionGraphNode,
                s_OutputFolder,
                false
            );

            if (!ExportBoxReflectionCubemap(s_Texture->m_pResource, s_CubeIndex, s_OutputFilePath)) {
                Logger::Error(
                    "[Editor] Failed to export box reflection cubemap for entity with ID {}.",
                    s_SelectedEntity->GetType()->m_nEntityID
                );
            }
        }
    }

    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

bool Editor::UpdateBoxReflectionPreview(ZRenderGraphNodeBoxReflection* p_BoxReflectionGraphNode) {
    ClearBoxReflectionPreview();

    ZRenderTexture2D* s_Texture = nullptr;
    uint32_t s_CubeIndex = 0;

    if (!GetBoxReflectionTexture(
        p_BoxReflectionGraphNode,
        false,
        s_Texture,
        s_CubeIndex
    )) {
        return false;
    }

    if (!UpdateBoxReflectionCubemapPreview(
        s_Texture,
        s_CubeIndex,
        m_BoxReflectionPreview.m_Textures,
        m_BoxReflectionPreview.m_ImGuiTextures
    )) {
        ClearBoxReflectionPreview();

        return false;
    }

    ZRenderTexture2D* s_DiffuseTexture = nullptr;
    uint32_t s_DiffuseCubeIndex = 0;

    if (!GetBoxReflectionTexture(
        p_BoxReflectionGraphNode,
        true,
        s_DiffuseTexture,
        s_DiffuseCubeIndex
    )) {
        ClearBoxReflectionPreview();

        return false;
    }

    if (!UpdateBoxReflectionCubemapPreview(
        s_DiffuseTexture,
        s_DiffuseCubeIndex,
        m_BoxReflectionPreview.m_DiffuseTextures,
        m_BoxReflectionPreview.m_DiffuseImGuiTextures
    )) {
        ClearBoxReflectionPreview();

        return false;
    }

    m_BoxReflectionPreview.m_BoxReflectionId = p_BoxReflectionGraphNode->m_nId;

    return true;
}

bool Editor::UpdateBoxReflectionCubemapPreview(
    ZRenderTexture2D* p_SourceTexture,
    uint32_t p_CubeIndex,
    std::array<ScopedD3DRef<ID3D12Resource>, 6>& p_OutTextures,
    std::array<ImGuiTexture, 6>& p_OutImGuiTextures
) {
    if (!p_SourceTexture || !p_SourceTexture->m_pResource) {
        return false;
    }

    const auto s_SourceResource = p_SourceTexture->m_pResource;
    const auto s_SourceDescription = s_SourceResource->GetDesc();

    constexpr size_t s_FaceCount = 6;

    const size_t s_FirstArraySlice =
        static_cast<size_t>(p_CubeIndex) * s_FaceCount;

    if (s_FirstArraySlice + s_FaceCount > s_SourceDescription.DepthOrArraySize) {
        return false;
    }

    DirectX::ScratchImage s_CapturedImage;

    constexpr D3D12_RESOURCE_STATES s_SourceState =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    const HRESULT s_Result = DirectX::CaptureTexture(
        Globals::RenderManager->m_pDevice->m_pCommandQueue,
        s_SourceResource,
        true,
        s_CapturedImage,
        s_SourceState,
        s_SourceState
    );

    if (FAILED(s_Result)) {
        return false;
    }

    for (size_t i = 0; i < s_FaceCount; ++i) {
        const size_t s_ArraySlice = s_FirstArraySlice + i;

        const auto s_Image = s_CapturedImage.GetImage(
            0,
            s_ArraySlice,
            0
        );

        if (!s_Image) {
            return false;
        }

        if (!CreateBoxReflectionFaceTexture(
            *s_Image,
            p_OutTextures[i],
            p_OutImGuiTextures[i]
        )) {
            return false;
        }
    }

    return true;
}

void Editor::ClearBoxReflectionPreview() {
    const auto s_ClearTextures = [](
        auto& p_Textures,
        auto& p_ImGuiTextures
    ) {
        for (size_t i = 0; i < p_Textures.size(); ++i) {
            if (p_ImGuiTextures[i].id) {
                SDK()->DestroyImGuiTexture(
                    p_Textures[i],
                    p_ImGuiTextures[i]
                );

                p_ImGuiTextures[i] = {};
            }

            p_Textures[i] = nullptr;
        }
        };

    s_ClearTextures(
        m_BoxReflectionPreview.m_Textures,
        m_BoxReflectionPreview.m_ImGuiTextures
    );

    s_ClearTextures(
        m_BoxReflectionPreview.m_DiffuseTextures,
        m_BoxReflectionPreview.m_DiffuseImGuiTextures
    );

    m_BoxReflectionPreview.m_BoxReflectionId = SIZE_MAX;
}

void Editor::DrawBoxReflectionCross(
    const std::array<ImGuiTexture, 6>& p_Textures,
    float p_FaceSize
) {
    const ImVec2 s_StartPosition = ImGui::GetCursorScreenPos();

    const auto s_DrawFace = [&](size_t p_Index, float p_X, float p_Y) {
        const auto& s_Texture = p_Textures[p_Index];

        if (!s_Texture.id) {
            return;
        }

        ImGui::SetCursorScreenPos({
            s_StartPosition.x + p_X * p_FaceSize,
            s_StartPosition.y + p_Y * p_FaceSize
        });

        ImGui::Image(
            s_Texture.id,
            ImVec2(p_FaceSize, p_FaceSize)
        );
        };

    //         +Y
    //     -X  +Z  +X  -Z
    //         -Y

    s_DrawFace(2, 1.f, 0.f); // +Y

    s_DrawFace(1, 0.f, 1.f); // -X
    s_DrawFace(4, 1.f, 1.f); // +Z
    s_DrawFace(0, 2.f, 1.f); // +X
    s_DrawFace(5, 3.f, 1.f); // -Z

    s_DrawFace(3, 1.f, 2.f); // -Y

    ImGui::SetCursorScreenPos({
        s_StartPosition.x,
        s_StartPosition.y + p_FaceSize * 3.f
    });

    ImGui::Dummy(ImVec2(p_FaceSize * 4.f, 0.f));
}

bool Editor::CreateBoxReflectionFaceTexture(
    const DirectX::Image& p_Image,
    ScopedD3DRef<ID3D12Resource>& p_OutTexture,
    ImGuiTexture& p_OutImGuiTexture
) {
    DirectX::TexMetadata s_Metadata {};
    s_Metadata.width = p_Image.width;
    s_Metadata.height = p_Image.height;
    s_Metadata.depth = 1;
    s_Metadata.arraySize = 1;
    s_Metadata.mipLevels = 1;
    s_Metadata.miscFlags = 0;
    s_Metadata.miscFlags2 = 0;
    s_Metadata.format = p_Image.format;
    s_Metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

    DirectX::Blob s_DDSData;

    const HRESULT s_Result = DirectX::SaveToDDSMemory(
        &p_Image,
        1,
        s_Metadata,
        DirectX::DDS_FLAGS_NONE,
        s_DDSData
    );

    if (FAILED(s_Result)) {
        return false;
    }

    return SDK()->CreateDDSTextureFromMemory(
        s_DDSData.GetBufferPointer(),
        s_DDSData.GetBufferSize(),
        p_OutTexture,
        p_OutImGuiTexture
    );
}

bool Editor::ExportAllBoxReflectionCubemaps(
    const std::filesystem::path& p_OutputFolder,
    bool p_Diffuse
) {
    const auto s_RenderSharedResources = Globals::RenderManager->m_pSharedResources;
    const auto s_RenderGraphManager = Globals::RenderGraphManager;

    if (!s_RenderSharedResources || !s_RenderGraphManager) {
        return false;
    }

    const auto& s_BoxReflections = s_RenderGraphManager->m_BoxReflections;

    const uint32_t s_ReflectionCount = static_cast<uint32_t>(s_BoxReflections.size());

    if (!s_ReflectionCount) {
        return true;
    }

    const std::filesystem::path s_OutputFolder = p_OutputFolder.empty()
        ? "box_reflections"
        : p_OutputFolder;

    std::filesystem::create_directories(s_OutputFolder);

    const uint32_t s_CubemapsPerChunk =
        s_RenderSharedResources->m_nBoxReflectionMaxCubeMaps /
        s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks;

    for (uint32_t i = 0; i < s_ReflectionCount; ++i) {
        const auto s_BoxReflectionId = s_BoxReflections[i]->m_nId;

        const uint32_t s_Chunk =
            s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks == 2 && s_BoxReflectionId >= s_CubemapsPerChunk
            ? 1
            : 0;

        const uint32_t s_CubeIndex =
            s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks == 2 && s_BoxReflectionId >= s_CubemapsPerChunk
            ? s_BoxReflectionId - s_CubemapsPerChunk
            : s_BoxReflectionId;

        ZRenderTexture2D* s_Texture = p_Diffuse
            ? s_RenderSharedResources->m_pBoxReflectionDiffuseCubeTexture[s_Chunk]
            : s_RenderSharedResources->m_pBoxReflectionCubeTexture[s_Chunk];

        if (!s_Texture || !s_Texture->m_pResource) {
            continue;
        }

        const auto s_Path = GetBoxReflectionExportPath(
            s_BoxReflections[i],
            s_OutputFolder,
            p_Diffuse
        );

        if (!ExportBoxReflectionCubemap(
            s_Texture->m_pResource,
            s_CubeIndex,
            s_Path)) {
            Logger::Error("[Editor] Failed to export box reflection cubemap {}.", i);
        }
    }

    return true;
}

bool Editor::ExportBoxReflectionCubemap(
    ID3D12Resource* p_Resource,
    uint32_t p_CubeIndex,
    const std::filesystem::path& p_OutputFolder
) {
    if (!p_Resource || !Globals::RenderManager->m_pDevice->m_pCommandQueue) {
        return false;
    }

    const auto s_ParentFolder = p_OutputFolder.parent_path();

    if (!s_ParentFolder.empty()) {
        std::filesystem::create_directories(s_ParentFolder);
    }

    const D3D12_RESOURCE_DESC s_ResourceDesc = p_Resource->GetDesc();

    DirectX::ScratchImage s_CapturedImage;

    constexpr D3D12_RESOURCE_STATES s_State =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    HRESULT s_Result = DirectX::CaptureTexture(
        Globals::RenderManager->m_pDevice->m_pCommandQueue,
        p_Resource,
        false,
        s_CapturedImage,
        s_State,
        s_State
    );

    if (FAILED(s_Result)) {
        Logger::Error("[Editor] CaptureTexture failed with HRESULT 0x{:08X}.", static_cast<uint32_t>(s_Result));
        return false;
    }

    const DirectX::TexMetadata& s_SourceMetadata = s_CapturedImage.GetMetadata();

    constexpr uint32_t s_FaceCount = 6;

    const uint32_t s_FirstArraySlice = p_CubeIndex * s_FaceCount;

    if (s_FirstArraySlice + s_FaceCount > s_SourceMetadata.arraySize) {
        return false;
    }

    DirectX::ScratchImage s_CubeImage;

    s_Result = s_CubeImage.InitializeCube(
        s_SourceMetadata.format,
        s_SourceMetadata.width,
        s_SourceMetadata.height,
        1,
        s_SourceMetadata.mipLevels
    );

    if (FAILED(s_Result)) {
        return false;
    }

    for (uint32_t s_Face = 0; s_Face < s_FaceCount; ++s_Face) {
        const uint32_t s_SourceArraySlice = s_FirstArraySlice + s_Face;

        for (uint32_t s_Mip = 0; s_Mip < s_SourceMetadata.mipLevels; ++s_Mip) {
            const DirectX::Image* s_SourceImage = s_CapturedImage.GetImage(s_Mip, s_SourceArraySlice, 0);
            const DirectX::Image* s_DestinationImage = s_CubeImage.GetImage(s_Mip, s_Face, 0);

            if (!s_SourceImage || !s_DestinationImage) {
                return false;
            }

            if (s_SourceImage->slicePitch != s_DestinationImage->slicePitch) {
                return false;
            }

            std::memcpy(
                s_DestinationImage->pixels,
                s_SourceImage->pixels,
                s_SourceImage->slicePitch
            );
        }
    }

    s_Result = DirectX::SaveToDDSFile(
        s_CubeImage.GetImages(),
        s_CubeImage.GetImageCount(),
        s_CubeImage.GetMetadata(),
        DirectX::DDS_FLAGS_NONE,
        p_OutputFolder.c_str()
    );

    if (FAILED(s_Result)) {
        Logger::Error("[Editor] SaveToDDSFile failed with HRESULT 0x{:08X}.", static_cast<uint32_t>(s_Result));
        return false;
    }

    return true;
}

bool Editor::GetBoxReflectionTexture(
    ZRenderGraphNodeBoxReflection* p_BoxReflectionGraphNode,
    bool p_Diffuse,
    ZRenderTexture2D*& p_OutTexture,
    uint32_t& p_OutCubeIndex
) {
    const auto s_RenderSharedResources = Globals::RenderManager->m_pSharedResources;
    const auto s_RenderGraphManager = Globals::RenderGraphManager;

    if (!s_RenderSharedResources || !s_RenderGraphManager || !p_BoxReflectionGraphNode) {
        return false;
    }

    const auto& s_BoxReflections = s_RenderGraphManager->m_BoxReflections;

    const uint32_t s_CubemapsPerChunk =
        s_RenderSharedResources->m_nBoxReflectionMaxCubeMaps /
        s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks;

    const bool s_SecondChunk =
        s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks == 2 &&
        p_BoxReflectionGraphNode->m_nId >= s_CubemapsPerChunk;

    const uint32_t s_Chunk = s_SecondChunk ? 1 : 0;

    p_OutCubeIndex = s_SecondChunk
        ? p_BoxReflectionGraphNode->m_nId - s_CubemapsPerChunk
        : p_BoxReflectionGraphNode->m_nId;

    p_OutTexture = p_Diffuse
        ? s_RenderSharedResources->m_pBoxReflectionDiffuseCubeTexture[s_Chunk]
        : s_RenderSharedResources->m_pBoxReflectionCubeTexture[s_Chunk];

    return p_OutTexture && p_OutTexture->m_pResource;
}

std::filesystem::path Editor::GetBoxReflectionExportPath(
    const ZRenderGraphNodeBoxReflection* p_BoxReflection,
    const std::filesystem::path& p_OutputFolder,
    bool p_Diffuse
) {
    const std::filesystem::path s_OutputFolder = p_OutputFolder.empty()
        ? "box_reflections"
        : p_OutputFolder;

    ZEntityRef s_EntityRef;
    p_BoxReflection->m_pRenderableEntity->GetID(s_EntityRef);

    std::string s_EntityName;

    {
        std::shared_lock s_TreeLock(m_CachedEntityTreeMutex);

        const auto s_Iterator = m_CachedEntityTreeMap.find(s_EntityRef);

        if (s_Iterator != m_CachedEntityTreeMap.end()) {
            s_EntityName = s_Iterator->second->Name;
        }
    }

    if (s_EntityName.empty()) {
        s_EntityName = "box_reflection";
    }

    std::ranges::replace(s_EntityName, ' ', '_');

    return s_OutputFolder /
        std::format(
            "{}{}.dds",
            s_EntityName,
            p_Diffuse ? "_diffuse" : ""
        );
}

bool Editor::GenerateBoxReflectionCacheResource(const std::filesystem::path& p_OutputFolder) {
    const auto s_RenderSharedResources = Globals::RenderManager->m_pSharedResources;
    const auto s_RenderGraphManager = Globals::RenderGraphManager;
    const auto s_CommandQueue = Globals::RenderManager->m_pDevice->m_pCommandQueue;

    if (!s_RenderSharedResources || !s_RenderGraphManager || !s_CommandQueue) {
        return false;
    }

    const auto& s_BoxReflections = s_RenderGraphManager->m_BoxReflections;

    if (s_BoxReflections.empty()) {
        return false;
    }

    const std::filesystem::path s_OutputFolder = p_OutputFolder.empty()
        ? "."
        : p_OutputFolder;

    std::filesystem::create_directories(s_OutputFolder);

    const auto s_OutputFilePath =
        s_OutputFolder / "box_reflections.BOXC";

    std::ofstream s_File(s_OutputFilePath, std::ios::binary);

    if (!s_File.is_open()) {
        return false;
    }

    // A chunk contains up to 341 cubemaps (2046 array slices / 6 faces).
    std::array<DirectX::ScratchImage, 2> s_BC6HImages;
    std::array<DirectX::ScratchImage, 2> s_R11G11B10Images;
    std::array<bool, 2> s_ChunkCaptured {};

    constexpr D3D12_RESOURCE_STATES s_State =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    const uint32_t s_BoxReflectionCount = static_cast<uint32_t>(s_BoxReflections.size());

    s_File.write(reinterpret_cast<const char*>(&s_BoxReflectionCount), sizeof(s_BoxReflectionCount));

    const uint32_t s_CubemapsPerChunk =
        s_RenderSharedResources->m_nBoxReflectionMaxCubeMaps /
        s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks;

    for (uint32_t i = 0; i < s_BoxReflectionCount; ++i) {
        const auto s_BoxReflectionId = s_BoxReflections[i]->m_nId;

        const uint32_t s_Chunk =
            s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks == 2 && s_BoxReflectionId >= s_CubemapsPerChunk
            ? 1
            : 0;

        const uint32_t s_CubeIndex =
            s_RenderSharedResources->m_nBoxReflectionCubeRenderTargetChunks == 2 && s_BoxReflectionId >= s_CubemapsPerChunk
            ? s_BoxReflectionId - s_CubemapsPerChunk
            : s_BoxReflectionId;

        if (!s_ChunkCaptured[s_Chunk]) {
            ZRenderTexture2D* s_BC6HTexture = s_RenderSharedResources->m_pBoxReflectionCubeTexture[s_Chunk];
            ZRenderTexture2D* s_R11G11B10Texture = s_RenderSharedResources->m_pBoxReflectionDiffuseCubeTexture[s_Chunk];

            if (!s_BC6HTexture ||
                !s_BC6HTexture->m_pResource ||
                !s_R11G11B10Texture ||
                !s_R11G11B10Texture->m_pResource) {
                return false;
            }

            HRESULT s_Result = DirectX::CaptureTexture(
                s_CommandQueue,
                s_BC6HTexture->m_pResource,
                false,
                s_BC6HImages[s_Chunk],
                s_State,
                s_State
            );

            if (FAILED(s_Result)) {
                Logger::Error(
                    "[Editor] Failed to capture BC6H box reflection texture (HRESULT 0x{:08X}).",
                    static_cast<uint32_t>(s_Result)
                );

                return false;
            }

            s_Result = DirectX::CaptureTexture(
                s_CommandQueue,
                s_R11G11B10Texture->m_pResource,
                false,
                s_R11G11B10Images[s_Chunk],
                s_State,
                s_State
            );

            if (FAILED(s_Result)) {
                Logger::Error(
                    "[Editor] Failed to capture R11G11B10 box reflection texture (HRESULT 0x{:08X}).",
                    static_cast<uint32_t>(s_Result)
                );

                return false;
            }

            s_ChunkCaptured[s_Chunk] = true;
        }

        const auto s_BoxReflection = s_BoxReflections[i];

        if (!s_BoxReflection || !s_BoxReflection->m_pRenderableEntity) {
            return false;
        }

        const auto& s_ObjectToWorld = s_BoxReflection->m_pRenderableEntity->GetObjectToWorldMatrix();

        const SVector3 s_Position {
            s_ObjectToWorld.Trans.x,
            s_ObjectToWorld.Trans.y,
            s_ObjectToWorld.Trans.z
        };

        const uint32_t s_BC6HSize = static_cast<uint32_t>(CalculateCubemapSize(s_BC6HImages[s_Chunk], s_CubeIndex));
        const uint32_t s_R11G11B10Size = static_cast<uint32_t>(CalculateCubemapSize(s_R11G11B10Images[s_Chunk], s_CubeIndex));
        const uint32_t s_DataSize = s_BC6HSize + s_R11G11B10Size;

        s_File.write(reinterpret_cast<const char*>(&s_Position), sizeof(s_Position));
        s_File.write(reinterpret_cast<const char*>(&s_DataSize), sizeof(s_DataSize));

        const auto& s_BC6HImage = s_BC6HImages[s_Chunk];
        const auto& s_BC6HMetadata = s_BC6HImage.GetMetadata();

        constexpr uint32_t s_FaceCount = 6;

        for (uint32_t s_Mip = 0; s_Mip < s_BC6HMetadata.mipLevels; ++s_Mip) {
            for (uint32_t s_Face = 0; s_Face < s_FaceCount; ++s_Face) {
                const uint32_t s_ArraySlice = s_CubeIndex * s_FaceCount + s_Face;

                const DirectX::Image* s_Image =
                    s_BC6HImage.GetImage(
                        s_Mip,
                        s_ArraySlice,
                        0
                    );

                if (!s_Image) {
                    return false;
                }

                s_File.write(
                    reinterpret_cast<const char*>(s_Image->pixels),
                    s_Image->slicePitch
                );
            }
        }

        const auto& s_R11G11B10Image = s_R11G11B10Images[s_Chunk];
        const auto& s_R11G11B10Metadata =
            s_R11G11B10Image.GetMetadata();

        for (uint32_t s_Mip = 0; s_Mip < s_R11G11B10Metadata.mipLevels; ++s_Mip) {
            for (uint32_t s_Face = 0; s_Face < s_FaceCount; ++s_Face) {
                const uint32_t s_ArraySlice = s_CubeIndex * s_FaceCount + s_Face;

                const DirectX::Image* s_Image =
                    s_R11G11B10Image.GetImage(
                        s_Mip,
                        s_ArraySlice,
                        0
                    );

                if (!s_Image) {
                    return false;
                }

                s_File.write(
                    reinterpret_cast<const char*>(s_Image->pixels),
                    s_Image->slicePitch
                );
            }
        }

        if (!s_File) {
            return false;
        }
    }

    return true;
}

size_t Editor::CalculateCubemapSize(const DirectX::ScratchImage& p_Image, uint32_t p_CubeIndex) {
    const auto& s_Metadata = p_Image.GetMetadata();

    constexpr uint32_t s_FaceCount = 6;

    size_t s_Size = 0;

    for (size_t s_Mip = 0; s_Mip < s_Metadata.mipLevels; ++s_Mip) {
        for (size_t s_Face = 0; s_Face < s_FaceCount; ++s_Face) {
            const auto s_Image = p_Image.GetImage(
                s_Mip,
                p_CubeIndex * s_FaceCount + s_Face,
                0
            );

            if (!s_Image) {
                return size_t { 0 };
            }

            s_Size += s_Image->slicePitch;
        }
    }

    return s_Size;
}