// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "ZeroG/d3d12/D3D12Backend.hpp"

#include <mutex>

#include "ZeroG/d3d12/D3D12CommandList.hpp"
#include "ZeroG/d3d12/D3D12CommandQueue.hpp"
#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/d3d12/D3D12DescriptorRingBuffer.hpp"
#include "ZeroG/d3d12/D3D12Framebuffer.hpp"
#include "ZeroG/d3d12/D3D12Memory.hpp"
#include "ZeroG/d3d12/D3D12Pipelines.hpp"
#include "ZeroG/d3d12/D3D12Profiler.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

constexpr uint32_t NUM_SWAP_CHAIN_BUFFERS = 3;

// D3D12 Backend State
// ------------------------------------------------------------------------------------------------

// We keep a separate state in order to create an easy way to control the order things are
// destroyed in. E.g., we would like to destroy everything but the absolute minimal required in
// order to check check for dangling objects using ReportLiveObjects.
struct D3D12BackendState final {

	// DXC compiler DLLs, lazily loaded if needed
	ComPtr<IDxcLibrary> dxcLibrary;
	ComPtr<IDxcCompiler> dxcCompiler;
	IDxcIncludeHandler* dxcIncludeHandler = nullptr;

	// Device
	ComPtr<IDXGIAdapter4> dxgiAdapter;
	ComPtr<ID3D12Device3> device;
	
	// Debug info queue
	ComPtr<ID3D12InfoQueue> infoQueue;

	// Static stats which don't change
	ZgStats staticStats = {};

	// Residency manager
	D3DX12Residency::ResidencyManager residencyManager;

	// Global descriptor ring buffers
	D3D12DescriptorRingBuffer globalDescriptorRingBuffer;

	// Command queues
	D3D12CommandQueue commandQueuePresent;
	//D3D12CommandQueue commandQueueAsyncCompute;
	D3D12CommandQueue commandQueueCopy;
	
	// Swapchain and backbuffers
	uint32_t width = 0;
	uint32_t height = 0;
	ComPtr<IDXGISwapChain4> swapchain;
	D3D12Framebuffer swapchainFramebuffers[NUM_SWAP_CHAIN_BUFFERS];
	uint64_t swapchainFenceValues[NUM_SWAP_CHAIN_BUFFERS] = {};
	int currentBackBufferIdx = 0;

	// Vsync settings
	bool allowTearing = false;
	bool vsyncEnabled = false;

	// Memory
	std::atomic_uint64_t resourceUniqueIdentifierCounter = 1;
};

// D3D12 Backend implementation
// ------------------------------------------------------------------------------------------------

class D3D12Backend final : public ZgBackend {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Backend() = default;
	D3D12Backend(const D3D12Backend&) = delete;
	D3D12Backend& operator= (const D3D12Backend&) = delete;
	D3D12Backend(D3D12Backend&&) = delete;
	D3D12Backend& operator= (D3D12Backend&&) = delete;

	virtual ~D3D12Backend() noexcept
	{
		// Flush command queues
		mState->commandQueuePresent.flush();
		mState->commandQueueCopy.flush();

		// Release include handler
		// TODO: Probably correct...?
		if (mState->dxcIncludeHandler != nullptr) {
			mState->dxcIncludeHandler->Release();
			mState->dxcIncludeHandler = nullptr;
		}

		// Destroy residency manager (which apparently has to be done manually...)
		mState->residencyManager.Destroy();

		// Log debug messages
		logDebugMessages();

		// Get debug device for report live objects in debug mode
		ComPtr<ID3D12DebugDevice1> debugDevice;
		if (mDebugMode) {
			CHECK_D3D12 mState->device->QueryInterface(IID_PPV_ARGS(&debugDevice));
		}

		// Delete most state
		getAllocator()->deleteObject(mState);

		// Report live objects
		if (mDebugMode) {
			CHECK_D3D12 debugDevice->ReportLiveDeviceObjects(
				D3D12_RLDO_FLAGS(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
		}
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgResult init(const ZgContextInitSettings& settings) noexcept
	{
		// Initialize members
		mDebugMode = settings.d3d12.debugMode;
		mState = getAllocator()->newObject<D3D12BackendState>(sfz_dbg("D3D12BackendState"));
		
		// Initialize part of state
		mState->width = settings.width;
		mState->height = settings.height;
		HWND hwnd = (HWND)settings.nativeHandle;
		if (mState->width == 0 || mState->height == 0) return ZG_ERROR_INVALID_ARGUMENT;

		// Enable debug layers in debug mode
		if (settings.d3d12.debugMode) {
			
			// Get debug interface
			ComPtr<ID3D12Debug1> debugInterface;
			if (D3D12_FAIL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
				return ZG_ERROR_GENERIC;
			}
			
			// Enable debug layer and GPU based validation
			debugInterface->EnableDebugLayer();
			debugInterface->SetEnableGPUBasedValidation(TRUE);

			ZG_INFO("D3D12 debug mode enabled");
		}

		// Create DXGI factory
		ComPtr<IDXGIFactory6> dxgiFactory;
		{
			UINT flags = 0;
			if (settings.d3d12.debugMode) flags |= DXGI_CREATE_FACTORY_DEBUG;
			if (D3D12_FAIL(CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory)))) {
				return ZG_ERROR_GENERIC;
			}
		}

		// Log available D3D12 devices
		d3d12LogAvailableDevices(dxgiFactory);

		// Create DXGI adapter and device. Software renderer if requested, otherwise high-performance
		if (settings.d3d12.useSoftwareRenderer) {
			ZgResult res = createSoftwareDevice(
				dxgiFactory,
				mState->dxgiAdapter,
				mState->device);
			if (res != ZG_SUCCESS) return res;
		}
		else {
			ZgResult res = createHighPerformanceDevice(
				dxgiFactory,
				mState->dxgiAdapter,
				mState->device);
			if (res != ZG_SUCCESS) return res;
		}

		// Store some info about device in stats
		{
			// Set some information about choosen adapter in static stats
			DXGI_ADAPTER_DESC1 desc;
			CHECK_D3D12 mState->dxgiAdapter->GetDesc1(&desc);
			snprintf(
				mState->staticStats.deviceDescription,
				sizeof(mState->staticStats.deviceDescription),
				"%S", desc.Description);
			mState->staticStats.dedicatedGpuMemoryBytes = desc.DedicatedVideoMemory;
			mState->staticStats.dedicatedCpuMemoryBytes = desc.DedicatedSystemMemory;
			mState->staticStats.sharedCpuMemoryBytes = desc.SharedSystemMemory;
		}

		// Enable debug message in debug mode
		if (mDebugMode) {

			if (D3D12_FAIL(mState->device->QueryInterface(IID_PPV_ARGS(&mState->infoQueue)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			// Break on corruption and error messages
			CHECK_D3D12 mState->infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			CHECK_D3D12 mState->infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

			// Log initial messages
			logDebugMessages();
		}

		// Create residency manager
		// Latency: "NumberOfBufferedFrames * NumberOfCommandListSubmissionsPerFrame throughout
		// the execution of your app."
		// Hmm. Lets try 128 or something
		const uint32_t residencyManagerMaxLatency = 128;
		if (D3D12_FAIL(mState->residencyManager.Initialize(
			mState->device.Get(), 0, mState->dxgiAdapter.Get(), residencyManagerMaxLatency))) {
			return ZG_ERROR_GENERIC;
		}

		// Allocate descriptors
		const uint32_t NUM_DESCRIPTORS = 1000000;
		ZG_INFO("Attempting to allocate %u descriptors for the global ring buffer",
			NUM_DESCRIPTORS);
		{
			ZgResult res = mState->globalDescriptorRingBuffer.create(
				*mState->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_DESCRIPTORS);
			if (res != ZG_SUCCESS) {
				ZG_ERROR("Failed to allocate descriptors");
				return ZG_ERROR_GPU_OUT_OF_MEMORY;
			}
		}

		// Create command queue
		const uint32_t MAX_NUM_COMMAND_LISTS_SWAPCHAIN_QUEUE = 256;
		const uint32_t MAX_NUM_BUFFERS_PER_COMMAND_LIST_SWAPCHAIN_QUEUE = 1024;
		ZgResult res = mState->commandQueuePresent.create(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mState->device,
			&mState->residencyManager,
			&mState->globalDescriptorRingBuffer,
			MAX_NUM_COMMAND_LISTS_SWAPCHAIN_QUEUE,
			MAX_NUM_BUFFERS_PER_COMMAND_LIST_SWAPCHAIN_QUEUE);
		if (res != ZG_SUCCESS) return res;

		// Create copy queue
		const uint32_t MAX_NUM_COMMAND_LISTS_COPY_QUEUE = 128;
		const uint32_t MAX_NUM_BUFFERS_PER_COMMAND_LIST_COPY_QUEUE = 1024;
		res = mState->commandQueueCopy.create(
			D3D12_COMMAND_LIST_TYPE_COPY,
			mState->device,
			&mState->residencyManager,
			&mState->globalDescriptorRingBuffer,
			MAX_NUM_COMMAND_LISTS_COPY_QUEUE,
			MAX_NUM_BUFFERS_PER_COMMAND_LIST_COPY_QUEUE);
		if (res != ZG_SUCCESS) return res;
		

		// Check if screen-tearing is allowed
		{
			BOOL tearingAllowed = FALSE;
			CHECK_D3D12 dxgiFactory->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingAllowed, sizeof(tearingAllowed));
			mState->allowTearing = tearingAllowed != FALSE;
		}
		mState->vsyncEnabled = settings.vsync != ZG_FALSE;

		// Create swap chain
		{
			DXGI_SWAP_CHAIN_DESC1 desc = {};
			desc.Width = mState->width;
			desc.Height = mState->height;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Stereo = FALSE;
			desc.SampleDesc = { 1, 0 }; // No MSAA
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = NUM_SWAP_CHAIN_BUFFERS; // 3 buffers, TODO: 1-2 buffers for no-vsync?
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			desc.Flags = (mState->allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

			ComPtr<IDXGISwapChain1> tmpSwapChain;
			if (D3D12_FAIL(dxgiFactory->CreateSwapChainForHwnd(
				mState->commandQueuePresent.commandQueue(), hwnd, &desc, nullptr, nullptr, &tmpSwapChain))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			if (D3D12_FAIL(tmpSwapChain.As(&mState->swapchain))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}
		}

		// Disable Alt+Enter fullscreen toogle
		CHECK_D3D12 dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

		// Perform early hacky initializiation of the D3D12 framebuffers to prepare them for
		// swapchain use
		// TODO: Unify this with the more general case somehow?
		for (uint32_t i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

			D3D12Framebuffer& framebuffer = mState->swapchainFramebuffers[i];
			
			// Mark framebuffer as swapchain framebuffer
			// TODO: Hacky hack, consider attempting to unify with general use case
			framebuffer.swapchainFramebuffer = true;

			// Create render target descriptor heap
			D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
			rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvDesc.NumDescriptors = 1;
			rtvDesc.NodeMask = 0;
			if (D3D12_FAIL(mState->device->CreateDescriptorHeap(
				&rtvDesc, IID_PPV_ARGS(&framebuffer.descriptorHeapRTV)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			// Set number of render targets and descriptor
			framebuffer.numRenderTargets = 1;
			framebuffer.renderTargetDescriptors[0] =
				framebuffer.descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();

			// Create depth buffer descriptor heap
			D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
			dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvDesc.NumDescriptors = 1;
			dsvDesc.NodeMask = 0;
			if (D3D12_FAIL(mState->device->CreateDescriptorHeap(
				&dsvDesc, IID_PPV_ARGS(&framebuffer.descriptorHeapDSV)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			// Set depth buffer available and descriptor
			framebuffer.hasDepthBuffer = true;
			framebuffer.depthBufferDescriptor =
				framebuffer.descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart();
		}

		// Create swap chain framebuffers (RTVs and DSVs)
		mState->width = 0;
		mState->height = 0;
		this->swapchainResize(settings.width, settings.height);

		logDebugMessages();
		return ZG_SUCCESS;
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	ZgResult swapchainResize(uint32_t width, uint32_t height) noexcept override final
	{
		if (mState->width == width && mState->height == height) return ZG_SUCCESS;
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Log that we are resizing the swap chain and then change the stored size
		bool initialCreation = false;
		if (mState->width == 0 && mState->height == 0) {
			ZG_INFO("Creating swap chain framebuffers, size: %ux%u", width, height);
			initialCreation = true;
		}
		else {
			ZG_INFO("Resizing swap chain framebuffers from %ux%u to %ux%u",
				mState->width, mState->height, width, height);
		}
		mState->width = width;
		mState->height = height;

		// Flush command queue so its safe to resize back buffers
		mState->commandQueuePresent.flush();

		if (!initialCreation) {
			// Release previous back buffers
			for (int i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {
				mState->swapchainFramebuffers[i].swapchain.renderTarget.Reset();
				mState->swapchainFramebuffers[i].swapchain.depthBuffer.Reset();
			}

			// Resize swap chain's back buffers
			DXGI_SWAP_CHAIN_DESC desc = {};
			CHECK_D3D12 mState->swapchain->GetDesc(&desc);
			CHECK_D3D12 mState->swapchain->ResizeBuffers(
				NUM_SWAP_CHAIN_BUFFERS, width, height, desc.BufferDesc.Format, desc.Flags);
		}

		// Update current back buffer index
		mState->currentBackBufferIdx = mState->swapchain->GetCurrentBackBufferIndex();

		// Create render target views (RTVs) for swap chain
		for (uint32_t i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

			// Get i:th back buffer from swap chain
			ComPtr<ID3D12Resource> backBufferRtv;
			CHECK_D3D12 mState->swapchain->GetBuffer(i, IID_PPV_ARGS(&backBufferRtv));

			// Set width and height
			mState->swapchainFramebuffers[i].width = width;
			mState->swapchainFramebuffers[i].height = height;

			// Get the i:th RTV descriptor from the swap chain descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor =
				mState->swapchainFramebuffers[i].renderTargetDescriptors[0];

			// Create render target view for i:th backbuffer
			mState->device->CreateRenderTargetView(backBufferRtv.Get(), nullptr, rtvDescriptor);
			mState->swapchainFramebuffers[i].swapchain.renderTarget = backBufferRtv;

			// Create the depth buffer
			D3D12_HEAP_PROPERTIES dsvHeapProperties = {};
			dsvHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			dsvHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			dsvHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

			D3D12_RESOURCE_DESC dsvResourceDesc = {};
			dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			dsvResourceDesc.Alignment = 0;
			dsvResourceDesc.Width = width;
			dsvResourceDesc.Height = height;
			dsvResourceDesc.DepthOrArraySize = 1;
			dsvResourceDesc.MipLevels = 0;
			dsvResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvResourceDesc.SampleDesc.Count = 1;
			dsvResourceDesc.SampleDesc.Quality = 0;
			dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE optimizedClearValue = {};
			optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			optimizedClearValue.DepthStencil.Depth = 1.0f;
			optimizedClearValue.DepthStencil.Stencil = 0;

			ComPtr<ID3D12Resource> backBufferDsv;
			CHECK_D3D12 mState->device->CreateCommittedResource(
				&dsvHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&dsvResourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&optimizedClearValue,
				IID_PPV_ARGS(&backBufferDsv));
			
			// Get the i:th DSV descriptor from the swap chain descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor =
				mState->swapchainFramebuffers[i].depthBufferDescriptor;

			// Create depth buffer view
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
			dsvViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvViewDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvViewDesc.Texture2D.MipSlice = 0;

			mState->device->CreateDepthStencilView(backBufferDsv.Get(), &dsvViewDesc, dsvDescriptor);
			mState->swapchainFramebuffers[i].swapchain.depthBuffer = backBufferDsv;
			mState->swapchainFramebuffers[i].depthBufferOptimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_ONE;
		}

		logDebugMessages();
		return ZG_SUCCESS;
	}

	ZgResult setVsync(
		bool vsync) noexcept override final
	{
		mState->vsyncEnabled = vsync;
		return ZG_SUCCESS;
	}

	ZgResult swapchainBeginFrame(
		ZgFramebuffer** framebufferOut,
		ZgProfiler* profiler,
		uint64_t* measurementIdOut) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Retrieve current back buffer to be rendered to
		D3D12Framebuffer& backBuffer = mState->swapchainFramebuffers[mState->currentBackBufferIdx];

		// Create a small command list to insert the transition barrier for the back buffer
		ZgCommandList* barrierCommandList = nullptr;
		ZgResult zgRes =
			mState->commandQueuePresent.beginCommandListRecording(&barrierCommandList);
		if (zgRes != ZG_SUCCESS) return zgRes;

		// Create barrier to transition back buffer into render target state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.swapchain.renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		reinterpret_cast<D3D12CommandList*>(barrierCommandList)->
			commandList->ResourceBarrier(1, &barrier);

		// Insert profiling begin call if a profiler is specified
		if (profiler != nullptr) {
			ZgResult res = barrierCommandList->profileBegin(profiler, *measurementIdOut);
			sfz_assert(res == ZG_SUCCESS);
		}

		// Execute command list containing the barrier transition
		mState->commandQueuePresent.executeCommandList(barrierCommandList);

		// Return backbuffer
		*framebufferOut = &backBuffer;

		logDebugMessages();
		return ZG_SUCCESS;
	}

	ZgResult swapchainFinishFrame(
		ZgProfiler* profiler,
		uint64_t measurementId) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Retrieve current back buffer that has been rendered to
		D3D12Framebuffer& backBuffer = mState->swapchainFramebuffers[mState->currentBackBufferIdx];

		// Create a small command list to insert the transition barrier for the back buffer
		ZgCommandList* barrierCommandList = nullptr;
		ZgResult zgRes =
			mState->commandQueuePresent.beginCommandListRecording(&barrierCommandList);
		if (zgRes != ZG_SUCCESS) return zgRes;

		// Create barrier to transition back buffer into present state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.swapchain.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		reinterpret_cast<D3D12CommandList*>(barrierCommandList)->
			commandList->ResourceBarrier(1, &barrier);

		// Finish profiling if a profiler is specified
		if (profiler != nullptr) {
			ZgResult res = barrierCommandList->profileEnd(profiler, measurementId);
			sfz_assert(res == ZG_SUCCESS);
		}

		// Execute command list containing the barrier transition
		mState->commandQueuePresent.executeCommandList(barrierCommandList);

		// Signal the graphics present queue
		mState->swapchainFenceValues[mState->currentBackBufferIdx] =
			mState->commandQueuePresent.signalOnGpuInternal();

		// Present back buffer
		{
			UINT vsync = 0;
			UINT flags = 0;
			if (mState->vsyncEnabled) {
				vsync = 1;
			}
			else if (mState->allowTearing) {
				// vsync MUST be 0 if we use the DXGI_PRESENT_ALLOW_TEARING flag
				flags = DXGI_PRESENT_ALLOW_TEARING;
			}
			CHECK_D3D12 mState->swapchain->Present(vsync, flags);
		}

		// Get next back buffer index
		mState->currentBackBufferIdx = mState->swapchain->GetCurrentBackBufferIndex();

		// Wait for the next back buffer to finish rendering so it's safe to use
		uint64_t nextBackBufferFenceValue = mState->swapchainFenceValues[mState->currentBackBufferIdx];
		mState->commandQueuePresent.waitOnCpuInternal(nextBackBufferFenceValue);

		logDebugMessages();
		return ZG_SUCCESS;
	}

	ZgResult fenceCreate(ZgFence** fenceOut) noexcept override final
	{
		*fenceOut = getAllocator()->newObject<D3D12Fence>(sfz_dbg("D3D12Fence"));
		return ZG_SUCCESS;
	}

	// Stats
	// --------------------------------------------------------------------------------------------

	ZgResult getStats(ZgStats& statsOut) noexcept override final
	{
		// First set the static stats which don't change
		statsOut = mState->staticStats;

		// Query information about "local" memory from DXGI
		// Local memory is "the fastest" for the GPU
		DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo = {};
		CHECK_D3D12 mState->dxgiAdapter->QueryVideoMemoryInfo(
			0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo);

		// Query information about "non-local" memory from DXGI
		DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfoNonLocal = {};
		CHECK_D3D12 mState->dxgiAdapter->QueryVideoMemoryInfo(
			0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memoryInfoNonLocal);

		// Set memory info stats
		statsOut.memoryBudgetBytes = memoryInfo.Budget;
		statsOut.memoryUsageBytes = memoryInfo.CurrentUsage;
		statsOut.nonLocalBugetBytes = memoryInfoNonLocal.Budget;
		statsOut.nonLocalUsageBytes = memoryInfoNonLocal.CurrentUsage;

		return ZG_SUCCESS;
	}

	// Pipeline compute methods
	// --------------------------------------------------------------------------------------------

	ZgResult pipelineComputeCreateFromFileHLSL(
		ZgPipelineCompute** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineComputeSignature* computeSignatureOut,
		const ZgPipelineComputeCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept override final
	{
		// Initialize DXC compiler if necessary
		{
			ZgResult res = initializeDxcCompiler();
			if (res != ZG_SUCCESS) return res;
		}

		// Create pipeline
		D3D12PipelineCompute* d3d12pipeline = nullptr;
		ZgResult res = createPipelineComputeFileHLSL(
			&d3d12pipeline,
			bindingsSignatureOut,
			computeSignatureOut,
			createInfo,
			compileSettings,
			*mState->dxcLibrary.Get(),
			*mState->dxcCompiler.Get(),
			mState->dxcIncludeHandler,
			*mState->device.Get());
		if (res != ZG_SUCCESS) return res;

		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgResult pipelineComputeRelease(
		ZgPipelineCompute* pipeline) noexcept override final
	{
		getAllocator()->deleteObject(pipeline);
		return ZG_SUCCESS;
	}

	// Pipeline rendeer methods
	// --------------------------------------------------------------------------------------------

	ZgResult pipelineRenderCreateFromFileSPIRV(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo) noexcept override final
	{
		// Initialize DXC compiler if necessary
		{
			ZgResult res = initializeDxcCompiler();
			if (res != ZG_SUCCESS) return res;
		}
		
		// Create pipeline
		D3D12PipelineRender* d3d12pipeline = nullptr;
		ZgResult res = createPipelineRenderFileSPIRV(
			&d3d12pipeline,
			bindingsSignatureOut,
			renderSignatureOut,
			createInfo,
			*mState->dxcLibrary.Get(),
			*mState->dxcCompiler.Get(),
			mState->dxcIncludeHandler,
			*mState->device.Get());
		if (res != ZG_SUCCESS) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgResult pipelineRenderCreateFromFileHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept override final
	{
		// Initialize DXC compiler if necessary
		{
			ZgResult res = initializeDxcCompiler();
			if (res != ZG_SUCCESS) return res;
		}
		
		// Create pipeline
		D3D12PipelineRender* d3d12pipeline = nullptr;
		ZgResult res = createPipelineRenderFileHLSL(
			&d3d12pipeline,
			bindingsSignatureOut,
			renderSignatureOut,
			createInfo,
			compileSettings,
			*mState->dxcLibrary.Get(),
			*mState->dxcCompiler.Get(),
			mState->dxcIncludeHandler,
			*mState->device.Get());
		if (res != ZG_SUCCESS) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgResult pipelineRenderCreateFromSourceHLSL(
		ZgPipelineRender** pipelineOut,
		ZgPipelineBindingsSignature* bindingsSignatureOut,
		ZgPipelineRenderSignature* renderSignatureOut,
		const ZgPipelineRenderCreateInfo& createInfo,
		const ZgPipelineCompileSettingsHLSL& compileSettings) noexcept override final
	{
		// Initialize DXC compiler if necessary
		{
			ZgResult res = initializeDxcCompiler();
			if (res != ZG_SUCCESS) return res;
		}
		
		// Create pipeline
		D3D12PipelineRender* d3d12pipeline = nullptr;
		ZgResult res = createPipelineRenderSourceHLSL(
			&d3d12pipeline,
			bindingsSignatureOut,
			renderSignatureOut,
			createInfo,
			compileSettings,
			*mState->dxcLibrary.Get(),
			*mState->dxcCompiler.Get(),
			mState->dxcIncludeHandler,
			*mState->device.Get());
		if (res != ZG_SUCCESS) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgResult pipelineRenderRelease(
		ZgPipelineRender* pipeline) noexcept override final
	{
		// TODO: Check if pipeline is currently in use? Lock?
		getAllocator()->deleteObject(pipeline);
		return ZG_SUCCESS;
	}

	// Memory methods
	// --------------------------------------------------------------------------------------------

	ZgResult memoryHeapCreate(
		ZgMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);
		return createMemoryHeap(
			*mState->device.Get(),
			&mState->resourceUniqueIdentifierCounter,
			mState->residencyManager,
			reinterpret_cast<D3D12MemoryHeap**>(memoryHeapOut),
			createInfo);
	}

	ZgResult memoryHeapRelease(
		ZgMemoryHeap* memoryHeapIn) noexcept override final
	{
		// TODO: Check if any buffers still exist? Lock?

		// Stop tracking
		D3D12MemoryHeap* heap = static_cast<D3D12MemoryHeap*>(memoryHeapIn);
		mState->residencyManager.EndTrackingObject(&heap->managedObject);

		getAllocator()->deleteObject(heap);
		return ZG_SUCCESS;
	}

	// Texture methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final
	{
		// Get resource desc
		D3D12_RESOURCE_DESC desc = createInfoToResourceDesc(createInfo);

		// Get allocation info
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = mState->device->GetResourceAllocationInfo(0, 1, &desc);

		// Return allocation info
		allocationInfoOut.sizeInBytes = (uint32_t)allocInfo.SizeInBytes;
		allocationInfoOut.alignmentInBytes = (uint32_t)allocInfo.Alignment;
		return ZG_SUCCESS;
	}

	// Framebuffer methods
	// --------------------------------------------------------------------------------------------

	virtual ZgResult framebufferCreate(
		ZgFramebuffer** framebufferOut,
		const ZgFramebufferCreateInfo& createInfo) noexcept override final
	{
		return createFramebuffer(
			*mState->device.Get(),
			reinterpret_cast<D3D12Framebuffer**>(framebufferOut),
			createInfo);
	}

	virtual void framebufferRelease(
		ZgFramebuffer* framebuffer) noexcept override final
	{
		if (reinterpret_cast<D3D12Framebuffer*>(framebuffer)->swapchainFramebuffer) return;
		getAllocator()->deleteObject(framebuffer);
	}

	// CommandQueue methods
	// --------------------------------------------------------------------------------------------

	ZgResult getPresentQueue(ZgCommandQueue** presentQueueOut) noexcept override final
	{
		*presentQueueOut = &mState->commandQueuePresent;
		return ZG_SUCCESS;
	}

	ZgResult getCopyQueue(ZgCommandQueue** copyQueueOut) noexcept override final
	{
		*copyQueueOut = &mState->commandQueueCopy;
		return ZG_SUCCESS;
	}

	// Profiler methods
	// --------------------------------------------------------------------------------------------

	ZgResult profilerCreate(
		ZgProfiler** profilerOut,
		const ZgProfilerCreateInfo& createInfo) noexcept override final
	{
		D3D12Profiler* profiler = nullptr;
		ZgResult res = d3d12CreateProfiler(
			*mState->device.Get(),
			&mState->resourceUniqueIdentifierCounter,
			mState->residencyManager,
			&profiler,
			createInfo);
		if (res != ZG_SUCCESS) return res;
		*profilerOut = profiler;
		return ZG_SUCCESS;
	}

	void profilerRelease(
		ZgProfiler* profilerIn) noexcept override final
	{
		getAllocator()->deleteObject(profilerIn);
	}

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	ZgResult initializeDxcCompiler() noexcept
	{
		// Initialize DXC compiler if necessary
		// TODO: Provide our own allocator
		std::lock_guard<std::mutex> lock(mContextMutex);
		if (mState->dxcLibrary == nullptr) {

			// Initialize DXC library
			HRESULT res = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mState->dxcLibrary));
			if (!SUCCEEDED(res)) return ZG_ERROR_GENERIC;

			// Initialize DXC compiler
			res = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mState->dxcCompiler));
			if (!SUCCEEDED(res)) {
				mState->dxcLibrary = nullptr;
				return ZG_ERROR_GENERIC;
			}

			// Create include handler
			res = mState->dxcLibrary->CreateIncludeHandler(&mState->dxcIncludeHandler);
			if (!SUCCEEDED(res)) {
				mState->dxcLibrary = nullptr;
				mState->dxcCompiler = nullptr;
				return ZG_ERROR_GENERIC;
			}
		}
		return ZG_SUCCESS;
	}

	void logDebugMessages() noexcept
	{
		if (!mDebugMode) return;

		sfz::Allocator* allocator = getAllocator();

		// Log D3D12 messages in debug mode
		uint64_t numMessages = mState->infoQueue->GetNumStoredMessages();
		for (uint64_t i = 0; i < numMessages; i++) {

			// Get the size of the message
			SIZE_T messageLength = 0;
			CHECK_D3D12 mState->infoQueue->GetMessage(0, NULL, &messageLength);

			// Allocate space and get the message
			D3D12_MESSAGE* message =
				(D3D12_MESSAGE*)allocator->allocate(sfz_dbg("D3D12_MESSAGE"), messageLength);
			CHECK_D3D12 mState->infoQueue->GetMessage(0, message, &messageLength);

			// Log message
			switch (message->Severity) {
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			case D3D12_MESSAGE_SEVERITY_ERROR:
				ZG_ERROR("D3D12Message: %s", message->pDescription);
				break;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				ZG_WARNING("D3D12Message: %s", message->pDescription);
				break;
			case D3D12_MESSAGE_SEVERITY_INFO:
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
				ZG_INFO("D3D12Message: %s", message->pDescription);
				break;
			}

			// Deallocate message
			allocator->deallocate(message);
		}

		// Clear stored messages
		mState->infoQueue->ClearStoredMessages();
	}
	
	// Private members
	// --------------------------------------------------------------------------------------------

	std::mutex mContextMutex; // Access to the context is synchronized
	bool mDebugMode = false;
	
	D3D12BackendState* mState = nullptr;
};

// D3D12 API
// ------------------------------------------------------------------------------------------------

ZgResult createD3D12Backend(ZgBackend** backendOut, const ZgContextInitSettings& settings) noexcept
{
	// Allocate and create D3D12 backend
	D3D12Backend* backend = getAllocator()->newObject<D3D12Backend>(sfz_dbg("D3D12Backend"));

	// Initialize backend, return nullptr if init failed
	ZgResult initRes = backend->init(settings);
	if (initRes != ZG_SUCCESS)
	{
		getAllocator()->deleteObject(backend);
		return initRes;
	}

	*backendOut = backend;
	return ZG_SUCCESS;
}

} // namespace zg
