#pragma once
#include <cstdint>
#include <d3d12.h>

namespace Rendering
{
    class D3D12Device : public ID3D12Device10
    {
    public:
	    D3D12Device(ID3D12Device10* p_Target);
        virtual ~D3D12Device();

        HRESULT QueryInterface(const IID& riid, void** ppvObject) override;
        ULONG AddRef() override;
        ULONG Release() override;

	    // ID3D12Object interface
	    HRESULT SetPrivateData(const GUID& Name, UINT DataSize, const void* pData) override { return m_Target->SetPrivateData(Name, DataSize, pData); }
	    HRESULT SetPrivateDataInterface(const GUID& Name, const IUnknown* pUnknown) override { return m_Target->SetPrivateDataInterface(Name, pUnknown); }
	    HRESULT SetName(LPCWSTR Name) override { return m_Target->SetName(Name); }
	    HRESULT GetPrivateData(const GUID& Name, UINT* pDataSize, void* pData) override { return m_Target->GetPrivateData(Name, pDataSize, pData); }

	    // ID3D12Device interface
	    UINT GetNodeCount() override { return m_Target->GetNodeCount(); }
	    HRESULT CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC* pDesc, REFIID riid, void** ppCommandQueue) override { return m_Target->CreateCommandQueue(pDesc, riid, ppCommandQueue); }
	    HRESULT CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type, REFIID riid, void** ppCommandAllocator) override { return m_Target->CreateCommandAllocator(type, riid, ppCommandAllocator); }
	    HRESULT CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc, REFIID riid, void** ppPipelineState) override { return m_Target->CreateGraphicsPipelineState(pDesc, riid, ppPipelineState); }
	    HRESULT CreateComputePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc, REFIID riid, void** ppPipelineState) override { return m_Target->CreateComputePipelineState(pDesc, riid, ppPipelineState); }
	    HRESULT CreateCommandList(UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pCommandAllocator, ID3D12PipelineState* pInitialState, REFIID riid, void** ppCommandList) override { return m_Target->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid, ppCommandList); }
	    HRESULT CheckFeatureSupport(D3D12_FEATURE Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize) override { return m_Target->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize); }
	    HRESULT CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC* pDescriptorHeapDesc, REFIID riid, void** ppvHeap) override { return m_Target->CreateDescriptorHeap(pDescriptorHeapDesc, riid, ppvHeap); }
	    UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType) override { return m_Target->GetDescriptorHandleIncrementSize(DescriptorHeapType); }
	    HRESULT CreateRootSignature(UINT nodeMask, const void* pBlobWithRootSignature, SIZE_T blobLengthInBytes, REFIID riid, void** ppvRootSignature) override { return m_Target->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid, ppvRootSignature); }
	    void CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateConstantBufferView(pDesc, DestDescriptor); }
	    void CreateShaderResourceView(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateShaderResourceView(pResource, pDesc, DestDescriptor); }
	    void CreateUnorderedAccessView(ID3D12Resource* pResource, ID3D12Resource* pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, DestDescriptor); }
	    void CreateRenderTargetView(ID3D12Resource* pResource, const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateRenderTargetView(pResource, pDesc, DestDescriptor); }
	    void CreateDepthStencilView(ID3D12Resource* pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateDepthStencilView(pResource, pDesc, DestDescriptor); }
	    void CreateSampler(const D3D12_SAMPLER_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateSampler(pDesc, DestDescriptor); }
	    void CopyDescriptors(UINT NumDestDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts, const UINT* pDestDescriptorRangeSizes, UINT NumSrcDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts, const UINT* pSrcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) override { return m_Target->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, DescriptorHeapsType); }
	    void CopyDescriptorsSimple(UINT NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) override { return m_Target->CopyDescriptorsSimple(NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType); }
	    D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(UINT visibleMask, UINT numResourceDescs, const D3D12_RESOURCE_DESC* pResourceDescs) override { return m_Target->GetResourceAllocationInfo(visibleMask, numResourceDescs, pResourceDescs); }
	    D3D12_HEAP_PROPERTIES GetCustomHeapProperties(UINT nodeMask, D3D12_HEAP_TYPE heapType) override { return m_Target->GetCustomHeapProperties(nodeMask, heapType); }
	    HRESULT CreateCommittedResource(const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riidResource, void** ppvResource) override { return m_Target->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, riidResource, ppvResource); }
	    HRESULT CreateHeap(const D3D12_HEAP_DESC* pDesc, REFIID riid, void** ppvHeap) override { return m_Target->CreateHeap(pDesc, riid, ppvHeap); }
	    HRESULT CreatePlacedResource(ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riid, void** ppvResource) override { return m_Target->CreatePlacedResource(pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, riid, ppvResource); }
	    HRESULT CreateReservedResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riid, void** ppvResource) override { return m_Target->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, riid, ppvResource); }
	    HRESULT CreateSharedHandle(ID3D12DeviceChild* pObject, const SECURITY_ATTRIBUTES* pAttributes, DWORD Access, LPCWSTR Name, HANDLE* pHandle) override { return m_Target->CreateSharedHandle(pObject, pAttributes, Access, Name, pHandle); }
	    HRESULT OpenSharedHandle(HANDLE NTHandle, REFIID riid, void** ppvObj) override { return m_Target->OpenSharedHandle(NTHandle, riid, ppvObj); }
	    HRESULT OpenSharedHandleByName(LPCWSTR Name, DWORD Access, HANDLE* pNTHandle) override { return m_Target->OpenSharedHandleByName(Name, Access, pNTHandle); }
	    HRESULT MakeResident(UINT NumObjects, ID3D12Pageable* const* ppObjects) override { return m_Target->MakeResident(NumObjects, ppObjects); }
	    HRESULT Evict(UINT NumObjects, ID3D12Pageable* const* ppObjects) override { return m_Target->Evict(NumObjects, ppObjects); }
	    HRESULT CreateFence(UINT64 InitialValue, D3D12_FENCE_FLAGS Flags, REFIID riid, void** ppFence) override { return m_Target->CreateFence(InitialValue, Flags, riid, ppFence); }
	    HRESULT GetDeviceRemovedReason() override { return m_Target->GetDeviceRemovedReason(); }
	    void GetCopyableFootprints(const D3D12_RESOURCE_DESC* pResourceDesc, UINT FirstSubresource, UINT NumSubresources, UINT64 BaseOffset, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pNumRows, UINT64* pRowSizeInBytes, UINT64* pTotalBytes) override { return m_Target->GetCopyableFootprints(pResourceDesc, FirstSubresource, NumSubresources, BaseOffset, pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes); }
	    HRESULT CreateQueryHeap(const D3D12_QUERY_HEAP_DESC* pDesc, REFIID riid, void** ppvHeap) override { return m_Target->CreateQueryHeap(pDesc, riid, ppvHeap); }
	    HRESULT SetStablePowerState(BOOL Enable) override { return m_Target->SetStablePowerState(Enable); }
	    HRESULT CreateCommandSignature(const D3D12_COMMAND_SIGNATURE_DESC* pDesc, ID3D12RootSignature* pRootSignature, REFIID riid, void** ppvCommandSignature) override { return m_Target->CreateCommandSignature(pDesc, pRootSignature, riid, ppvCommandSignature); }
	    void GetResourceTiling(ID3D12Resource* pTiledResource, UINT* pNumTilesForEntireResource, D3D12_PACKED_MIP_INFO* pPackedMipDesc, D3D12_TILE_SHAPE* pStandardTileShapeForNonPackedMips, UINT* pNumSubresourceTilings, UINT FirstSubresourceTilingToGet, D3D12_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips) override { return m_Target->GetResourceTiling(pTiledResource, pNumTilesForEntireResource, pPackedMipDesc, pStandardTileShapeForNonPackedMips, pNumSubresourceTilings, FirstSubresourceTilingToGet, pSubresourceTilingsForNonPackedMips); }
	    LUID GetAdapterLuid() override { return m_Target->GetAdapterLuid(); }

	    // ID3D12Device1 interface
	    HRESULT CreatePipelineLibrary(const void* pLibraryBlob, SIZE_T BlobLength, REFIID riid, void** ppPipelineLibrary) override { return m_Target->CreatePipelineLibrary(pLibraryBlob, BlobLength, riid, ppPipelineLibrary); }
	    HRESULT SetEventOnMultipleFenceCompletion(ID3D12Fence* const* ppFences, const UINT64* pFenceValues, UINT NumFences, D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags, HANDLE hEvent) override { return m_Target->SetEventOnMultipleFenceCompletion(ppFences, pFenceValues, NumFences, Flags, hEvent); }
	    HRESULT SetResidencyPriority(UINT NumObjects, ID3D12Pageable* const* ppObjects, const D3D12_RESIDENCY_PRIORITY* pPriorities) override { return m_Target->SetResidencyPriority(NumObjects, ppObjects, pPriorities); }

	    // ID3D12Device2 interface
	    HRESULT CreatePipelineState(const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc, REFIID riid, void** ppPipelineState) override { return m_Target->CreatePipelineState(pDesc, riid, ppPipelineState); }

	    // ID3D12Device3 interface
	    HRESULT OpenExistingHeapFromAddress(const void* pAddress, REFIID riid, void** ppvHeap) override { return m_Target->OpenExistingHeapFromAddress(pAddress, riid, ppvHeap); }
	    HRESULT OpenExistingHeapFromFileMapping(HANDLE hFileMapping, REFIID riid, void** ppvHeap) override { return m_Target->OpenExistingHeapFromFileMapping(hFileMapping, riid, ppvHeap); }
	    HRESULT EnqueueMakeResident(D3D12_RESIDENCY_FLAGS Flags, UINT NumObjects, ID3D12Pageable* const* ppObjects, ID3D12Fence* pFenceToSignal, UINT64 FenceValueToSignal) override { return m_Target->EnqueueMakeResident(Flags, NumObjects, ppObjects, pFenceToSignal, FenceValueToSignal); }

	    // ID3D12Device4 interface
	    HRESULT CreateCommandList1(UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_LIST_FLAGS flags, REFIID riid, void** ppCommandList) override { return m_Target->CreateCommandList1(nodeMask, type, flags, riid, ppCommandList); }
	    HRESULT CreateProtectedResourceSession(const D3D12_PROTECTED_RESOURCE_SESSION_DESC* pDesc, REFIID riid, void** ppSession) override { return m_Target->CreateProtectedResourceSession(pDesc, riid, ppSession); }
	    HRESULT CreateCommittedResource1(const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, REFIID riidResource, void** ppvResource) override { return m_Target->CreateCommittedResource1(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, pProtectedSession, riidResource, ppvResource); }
	    HRESULT CreateHeap1(const D3D12_HEAP_DESC* pDesc, ID3D12ProtectedResourceSession* pProtectedSession, REFIID riid, void** ppvHeap) override { return m_Target->CreateHeap1(pDesc, pProtectedSession, riid, ppvHeap); }
	    HRESULT CreateReservedResource1(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, REFIID riid, void** ppvResource) override { return m_Target->CreateReservedResource1(pDesc, InitialState, pOptimizedClearValue, pProtectedSession, riid, ppvResource); }
	    D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo1(UINT visibleMask, UINT numResourceDescs, const D3D12_RESOURCE_DESC* pResourceDescs, D3D12_RESOURCE_ALLOCATION_INFO1* pResourceAllocationInfo1) override { return m_Target->GetResourceAllocationInfo1(visibleMask, numResourceDescs, pResourceDescs, pResourceAllocationInfo1); }

	    // ID3D12Device5 interface
	    HRESULT CreateLifetimeTracker(ID3D12LifetimeOwner* pOwner, REFIID riid, void** ppvTracker) override { return m_Target->CreateLifetimeTracker(pOwner, riid, ppvTracker); }
	    void RemoveDevice() override { return m_Target->RemoveDevice(); }
	    HRESULT EnumerateMetaCommands(UINT* pNumMetaCommands, D3D12_META_COMMAND_DESC* pDescs) override { return m_Target->EnumerateMetaCommands(pNumMetaCommands, pDescs); }
	    HRESULT EnumerateMetaCommandParameters(REFGUID CommandId, D3D12_META_COMMAND_PARAMETER_STAGE Stage, UINT* pTotalStructureSizeInBytes, UINT* pParameterCount, D3D12_META_COMMAND_PARAMETER_DESC* pParameterDescs) override { return m_Target->EnumerateMetaCommandParameters(CommandId, Stage, pTotalStructureSizeInBytes, pParameterCount, pParameterDescs); }
	    HRESULT CreateMetaCommand(REFGUID CommandId, UINT NodeMask, const void* pCreationParametersData, SIZE_T CreationParametersDataSizeInBytes, REFIID riid, void** ppMetaCommand) override { return m_Target->CreateMetaCommand(CommandId, NodeMask, pCreationParametersData, CreationParametersDataSizeInBytes, riid, ppMetaCommand); }
	    HRESULT CreateStateObject(const D3D12_STATE_OBJECT_DESC* pDesc, REFIID riid, void** ppStateObject) override { return m_Target->CreateStateObject(pDesc, riid, ppStateObject); }
	    void GetRaytracingAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo) override { return m_Target->GetRaytracingAccelerationStructurePrebuildInfo(pDesc, pInfo); }
	    D3D12_DRIVER_MATCHING_IDENTIFIER_STATUS CheckDriverMatchingIdentifier(D3D12_SERIALIZED_DATA_TYPE SerializedDataType, const D3D12_SERIALIZED_DATA_DRIVER_MATCHING_IDENTIFIER* pIdentifierToCheck) override { return m_Target->CheckDriverMatchingIdentifier(SerializedDataType, pIdentifierToCheck); }

	    // ID3D12Device6 interface
	    HRESULT SetBackgroundProcessingMode(D3D12_BACKGROUND_PROCESSING_MODE Mode, D3D12_MEASUREMENTS_ACTION MeasurementsAction, HANDLE hEventToSignalUponCompletion, BOOL* pbFurtherMeasurementsDesired) override { return m_Target->SetBackgroundProcessingMode(Mode, MeasurementsAction, hEventToSignalUponCompletion, pbFurtherMeasurementsDesired); }

	    // ID3D12Device7 interface
	    HRESULT AddToStateObject(const D3D12_STATE_OBJECT_DESC* pAddition, ID3D12StateObject* pStateObjectToGrowFrom, REFIID riid, void** ppNewStateObject) override { return m_Target->AddToStateObject(pAddition, pStateObjectToGrowFrom, riid, ppNewStateObject); }
	    HRESULT CreateProtectedResourceSession1(const D3D12_PROTECTED_RESOURCE_SESSION_DESC1* pDesc, REFIID riid, void** ppSession) override { return m_Target->CreateProtectedResourceSession1(pDesc, riid, ppSession); }

	    // ID3D12Device8 interface
	    D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo2(UINT visibleMask, UINT numResourceDescs, const D3D12_RESOURCE_DESC1* pResourceDescs, D3D12_RESOURCE_ALLOCATION_INFO1* pResourceAllocationInfo1) override { return m_Target->GetResourceAllocationInfo2(visibleMask, numResourceDescs, pResourceDescs, pResourceAllocationInfo1); }
	    HRESULT CreateCommittedResource2(const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, REFIID riidResource, void** ppvResource) override { return m_Target->CreateCommittedResource2(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, pProtectedSession, riidResource, ppvResource); }
	    HRESULT CreatePlacedResource1(ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riid, void** ppvResource) override { return m_Target->CreatePlacedResource1(pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, riid, ppvResource); }
	    void CreateSamplerFeedbackUnorderedAccessView(ID3D12Resource* pTargetedResource, ID3D12Resource* pFeedbackResource, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) override { return m_Target->CreateSamplerFeedbackUnorderedAccessView(pTargetedResource, pFeedbackResource, DestDescriptor); }
	    void GetCopyableFootprints1(const D3D12_RESOURCE_DESC1* pResourceDesc, UINT FirstSubresource, UINT NumSubresources, UINT64 BaseOffset, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pNumRows, UINT64* pRowSizeInBytes, UINT64* pTotalBytes) override { return m_Target->GetCopyableFootprints1(pResourceDesc, FirstSubresource, NumSubresources, BaseOffset, pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes); }

	    // ID3D12Device9 interface
	    HRESULT CreateShaderCacheSession(const D3D12_SHADER_CACHE_SESSION_DESC* pDesc, REFIID riid, void** ppvSession) override { return m_Target->CreateShaderCacheSession(pDesc, riid, ppvSession); }
	    HRESULT ShaderCacheControl(D3D12_SHADER_CACHE_KIND_FLAGS Kinds, D3D12_SHADER_CACHE_CONTROL_FLAGS Control) override { return m_Target->ShaderCacheControl(Kinds, Control); }
	    HRESULT CreateCommandQueue1(const D3D12_COMMAND_QUEUE_DESC* pDesc, REFIID CreatorID, REFIID riid, void** ppCommandQueue) override { return m_Target->CreateCommandQueue1(pDesc, CreatorID, riid, ppCommandQueue); }

	    // ID3D12Device10 interface
	    HRESULT CreateCommittedResource3(const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, UINT32 NumCastableFormats, DXGI_FORMAT* pCastableFormats, REFIID riidResource, void** ppvResource) override { return m_Target->CreateCommittedResource3(pHeapProperties, HeapFlags, pDesc, InitialLayout, pOptimizedClearValue, pProtectedSession, NumCastableFormats, pCastableFormats, riidResource, ppvResource); }
	    HRESULT CreatePlacedResource2(ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, UINT32 NumCastableFormats, DXGI_FORMAT* pCastableFormats, REFIID riid, void** ppvResource) override { return m_Target->CreatePlacedResource2(pHeap, HeapOffset, pDesc, InitialLayout, pOptimizedClearValue, NumCastableFormats, pCastableFormats, riid, ppvResource); }
	    HRESULT CreateReservedResource2(const D3D12_RESOURCE_DESC* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, UINT32 NumCastableFormats, DXGI_FORMAT* pCastableFormats, REFIID riid, void** ppvResource) override { return m_Target->CreateReservedResource2(pDesc, InitialLayout, pOptimizedClearValue, pProtectedSession, NumCastableFormats, pCastableFormats, riid, ppvResource); }

    private:
	    ID3D12Device10* m_Target;
        volatile ULONG m_RefCount = 0;
    };
}
