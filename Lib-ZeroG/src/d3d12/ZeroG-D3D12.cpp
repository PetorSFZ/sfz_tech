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

#define ZG_DLL_EXPORT
#include "ZeroG.h"

#include <cmath>
#include <cstring>
#include <mutex>

#include <skipifzero_new.hpp>

// Must be first so we don't accidentally include system d3d12.h
#include "d3d12/D3D12Common.hpp"

#include <D3D12MemAlloc.h>

#include "common/Context.hpp"
#include "common/ErrorReporting.hpp"
#include "common/Logging.hpp"
#include "common/Matrices.hpp"

#include "d3d12/D3D12CommandList.hpp"
#include "d3d12/D3D12CommandQueue.hpp"
#include "d3d12/D3D12DescriptorRingBuffer.hpp"
#include "d3d12/D3D12Framebuffer.hpp"
#include "d3d12/D3D12Memory.hpp"
#include "d3d12/D3D12Pipelines.hpp"
#include "d3d12/D3D12Profiler.hpp"

// Implementation notes
// ------------------------------------------------------------------------------------------------

// D3D12's residency API is not supported, what will happen is instead that the app will likely
// crash if the memory budget is exceeded. All resources are "resident" always, from MakeResident()
// docs:
//
// "MakeResident is ref - counted, such that Evict must be called the same amount of times as
// MakeResident before Evict takes effect. Objects that support residency are made resident during
// creation, so a single Evict call will actually evict the object."

// D3D12 Agility SDK exports
// ------------------------------------------------------------------------------------------------

// Note: It seems this is not enough and must also be in the exe file of the application using
//       ZeroG. A bit annoying, but don't have a good solution to it for now.

// The version of the Agility SDK we are using, see https://devblogs.microsoft.com/directx/directx12agility/
extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }

// Specifies that D3D12Core.dll will be available in a directory called D3D12 next to the exe.
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

// Constants
// ------------------------------------------------------------------------------------------------

constexpr u32 NUM_SWAP_CHAIN_BUFFERS = 3;

// D3D12 Context State
// ------------------------------------------------------------------------------------------------

struct ZgContextState final {

	std::mutex contextMutex; // Access to the context is synchronized
	bool debugMode = false;
	bool dredAutoBreadcrumbs = false;

	// DXC compiler DLLs, lazily loaded if needed
	ComPtr<IDxcLibrary> dxcLibrary;
	ComPtr<IDxcCompiler> dxcCompiler;
	IDxcIncludeHandler* dxcIncludeHandler = nullptr;

	// Device
	ComPtr<IDXGIAdapter4> dxgiAdapter;
	ComPtr<ID3D12Device3> device;

	// D3D12 Memory Allocator
	D3D12MA::Allocator* d3d12Allocator = nullptr;

	// Debug info queue
	ComPtr<ID3D12InfoQueue> infoQueue;

	// Feature support
	ZgFeatureSupport featureSupport = {};

	// Static stats which don't change
	ZgStats staticStats = {};

	// Global descriptor ring buffers
	D3D12DescriptorRingBuffer globalDescriptorRingBuffer;

	// Command queues
	ZgCommandQueue commandQueuePresent;
	//ZgCommandQueue commandQueueAsyncCompute;
	ZgCommandQueue commandQueueCopy;

	// Swapchain and backbuffers
	u32 width = 0;
	u32 height = 0;
	ComPtr<IDXGISwapChain4> swapchain;
	ZgFramebuffer swapchainFramebuffers[NUM_SWAP_CHAIN_BUFFERS];
	u64 swapchainFenceValues[NUM_SWAP_CHAIN_BUFFERS] = {};
	int currentBackBufferIdx = 0;

	// Vsync settings
	bool allowTearing = false;
	bool vsyncEnabled = false;

	// Pipeline caching
	bool allowPipelineCaching = false;
	sfz::str320 pipelineCacheDir;

	// Memory
	std::atomic_uint64_t resourceUniqueIdentifierCounter = 1;
};

static ZgContextState* ctxState = nullptr;

// Statics
// ------------------------------------------------------------------------------------------------

static void* d3d12MemAllocAllocate(size_t size, size_t alignment, void* userData) noexcept
{
	(void)userData;
	SfzAllocator* allocator = getAllocator();
	return allocator->alloc(
		sfz_dbg("D3D12MemAlloc"), u64(size), sfz::max(u32(alignment), 32u));
}

static void d3d12MemAllocFree(void* memory, void* userData) noexcept
{
	(void)userData;
	SfzAllocator* allocator = getAllocator();
	allocator->dealloc(memory);
}

static D3D12MA::ALLOCATION_CALLBACKS getD3D12MemAllocAllocationCallbacks() noexcept
{
	D3D12MA::ALLOCATION_CALLBACKS callbacks = {};
	callbacks.pAllocate = d3d12MemAllocAllocate;
	callbacks.pFree = d3d12MemAllocFree;
	callbacks.pUserData = nullptr;
	return callbacks;
}

static void logDebugMessages(ZgContextState& state) noexcept
{
	if (!state.debugMode) return;

	SfzAllocator* allocator = getAllocator();

	// Log D3D12 messages in debug mode
	u64 numMessages = state.infoQueue->GetNumStoredMessages();
	for (u64 i = 0; i < numMessages; i++) {

		// Get the size of the message
		SIZE_T messageLength = 0;
		CHECK_D3D12 state.infoQueue->GetMessage(0, NULL, &messageLength);

		// Allocate space and get the message
		D3D12_MESSAGE* message =
			(D3D12_MESSAGE*)allocator->alloc(sfz_dbg("D3D12_MESSAGE"), messageLength);
		CHECK_D3D12 state.infoQueue->GetMessage(0, message, &messageLength);

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
		allocator->dealloc(message);
	}

	// Clear stored messages
	state.infoQueue->ClearStoredMessages();
}

// Not actually a static, forward declared in D3D12Common.hpp and called by the CHECK_D3D12 macros.
void dredCallback(HRESULT res)
{
	// Handle DRED errors
	if (ctxState->dredAutoBreadcrumbs && res == DXGI_ERROR_DEVICE_REMOVED) {
		ComPtr<ID3D12DeviceRemovedExtendedData> dred;
		CHECK_D3D12 ctxState->device->QueryInterface(IID_PPV_ARGS(&dred));
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dredAutoBreadcrumbsOutput = {};
		D3D12_DRED_PAGE_FAULT_OUTPUT dredPageFaultOutput = {};
		CHECK_D3D12 dred->GetAutoBreadcrumbsOutput(&dredAutoBreadcrumbsOutput);
		CHECK_D3D12 dred->GetPageFaultAllocationOutput(&dredPageFaultOutput);
		// TODO: Process and log DRED somehow??? For now, can at least open debugger here.
		sfz_assert(false);
	}
}

static ZgResult init(const ZgContextInitSettings& settings) noexcept
{
	// Initialize members
	ctxState = sfz_new<ZgContextState>(getAllocator(), sfz_dbg("ZgContextState"));
	ctxState->debugMode = settings.d3d12.debugMode;

	// Initialize part of state
	ctxState->width = settings.width;
	ctxState->height = settings.height;
	HWND hwnd = (HWND)settings.nativeHandle;
	if (ctxState->width == 0 || ctxState->height == 0) return ZG_ERROR_INVALID_ARGUMENT;

	// Initialize DXC compiler
	// TODO: Provide our own allocator
	sfz_assert(ctxState->dxcLibrary == nullptr);
	{
		// Initialize DXC library
		HRESULT res = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&ctxState->dxcLibrary));
		if (!SUCCEEDED(res)) return ZG_ERROR_GENERIC;

		// Initialize DXC compiler
		res = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&ctxState->dxcCompiler));
		if (!SUCCEEDED(res)) {
			ctxState->dxcLibrary = nullptr;
			return ZG_ERROR_GENERIC;
		}

		// Create include handler
		res = ctxState->dxcLibrary->CreateIncludeHandler(&ctxState->dxcIncludeHandler);
		if (!SUCCEEDED(res)) {
			ctxState->dxcLibrary = nullptr;
			ctxState->dxcCompiler = nullptr;
			return ZG_ERROR_GENERIC;
		}
	}

	// Enable debug layers in debug mode
	if (settings.d3d12.debugMode) {

		// Get debug interface
		ComPtr<ID3D12Debug1> debugInterface;
		if (D3D12_FAIL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
			return ZG_ERROR_GENERIC;
		}

		// Enable debug layer and GPU based validation
		debugInterface->EnableDebugLayer();
		ZG_INFO("D3D12 debug mode enabled");

		// Enable GPU based debug mode if requested
		if (settings.d3d12.debugModeGpuBased) {
			debugInterface->SetEnableGPUBasedValidation(TRUE);
			ZG_INFO("D3D12 GPU based debug mode enabled");
		}
	}

	// Enable DRED Auto-Breadcrumbs if requested
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
	if (settings.d3d12.enableDredAutoBreadcrumbs) {
		if (D3D12_FAIL(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)))) {
			return ZG_ERROR_GENERIC;
		}

		// Turn on auto-breadcrumbs and page fault reporting.
		dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

		ctxState->dredAutoBreadcrumbs = true;
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
			ctxState->dxgiAdapter,
			ctxState->device);
		if (res != ZG_SUCCESS) return res;
	}
	else {
		ZgResult res = createHighPerformanceDevice(
			dxgiFactory,
			ctxState->dxgiAdapter,
			ctxState->device);
		if (res != ZG_SUCCESS) return res;
	}

	// Initialize D3D12 Memory Allocator
	{
		D3D12MA::ALLOCATOR_DESC desc = {};
		desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE; // D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;
		desc.pDevice = ctxState->device.Get();
		desc.PreferredBlockSize = 0; // 0 == Default, 256 MiB
		D3D12MA::ALLOCATION_CALLBACKS callbacks = getD3D12MemAllocAllocationCallbacks();
		desc.pAllocationCallbacks = &callbacks;
		desc.pAdapter = ctxState->dxgiAdapter.Get();

		sfz_assert(ctxState->d3d12Allocator == nullptr);
		HRESULT hr = D3D12MA::CreateAllocator(&desc, &ctxState->d3d12Allocator);
		if (D3D12_FAIL(hr)) {
			return ZG_ERROR_GENERIC;
		}
	}

	// Store some info about device in stats
	{
		// Set some information about choosen adapter in static stats
		DXGI_ADAPTER_DESC1 desc;
		CHECK_D3D12 ctxState->dxgiAdapter->GetDesc1(&desc);
		snprintf(
			ctxState->featureSupport.deviceDescription,
			sizeof(ctxState->featureSupport.deviceDescription),
			"%S", desc.Description);
		ctxState->staticStats.dedicatedGpuMemoryBytes = desc.DedicatedVideoMemory;
		ctxState->staticStats.dedicatedCpuMemoryBytes = desc.DedicatedSystemMemory;
		ctxState->staticStats.sharedCpuMemoryBytes = desc.SharedSystemMemory;
	}

	// Feature support
	{
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
		// "before calling the function initialize the HighestShaderModel field to the highest
		// shader model that your application understands."
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
		ctxState->featureSupport.shaderModel = [](D3D_SHADER_MODEL model) {
			switch (model) {
			case D3D_SHADER_MODEL_5_1: return ZG_SHADER_MODEL_UNDEFINED;
			case D3D_SHADER_MODEL_6_0: return ZG_SHADER_MODEL_6_0;
			case D3D_SHADER_MODEL_6_1: return ZG_SHADER_MODEL_6_1;
			case D3D_SHADER_MODEL_6_2: return ZG_SHADER_MODEL_6_2;
			case D3D_SHADER_MODEL_6_3: return ZG_SHADER_MODEL_6_3;
			case D3D_SHADER_MODEL_6_4: return ZG_SHADER_MODEL_6_4;
			case D3D_SHADER_MODEL_6_5: return ZG_SHADER_MODEL_6_5;
			case D3D_SHADER_MODEL_6_6: return ZG_SHADER_MODEL_6_6;
			}
			return ZG_SHADER_MODEL_UNDEFINED;
		}(shaderModel.HighestShaderModel);

		D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

		const char* resourceBindingTier = [](D3D12_RESOURCE_BINDING_TIER tier) {
			switch (tier) {
			case D3D12_RESOURCE_BINDING_TIER_1: return "Tier 1";
			case D3D12_RESOURCE_BINDING_TIER_2: return "Tier 2";
			case D3D12_RESOURCE_BINDING_TIER_3: return "Tier 3";
			}
			return "";
		}(options.ResourceBindingTier);
		snprintf(
			ctxState->featureSupport.resourceBindingTier, 
			sizeof(ctxState->featureSupport.resourceBindingTier),
			"%s", resourceBindingTier);

		const char* resourceHeapTier = [](D3D12_RESOURCE_HEAP_TIER tier) {
			switch (tier) {
			case D3D12_RESOURCE_HEAP_TIER_1: return "Tier 1";
			case D3D12_RESOURCE_HEAP_TIER_2: return "Tier 2";
			}
			return "";
		}(options.ResourceHeapTier);
		snprintf(
			ctxState->featureSupport.resourceHeapTier,
			sizeof(ctxState->featureSupport.resourceHeapTier),
			"%s", resourceHeapTier);

		// Note: Might need to update these if more tiers or shading models are added
		ctxState->featureSupport.shaderDynamicResources =
			(shaderModel.HighestShaderModel == D3D_SHADER_MODEL_6_6 &&
			options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3) ? ZG_TRUE : ZG_FALSE;

		D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));

		ctxState->featureSupport.waveOps = options1.WaveOps ? ZG_TRUE : ZG_FALSE;
		ctxState->featureSupport.waveMinLaneCount = options1.WaveLaneCountMin;
		ctxState->featureSupport.waveMaxLaneCount = options1.WaveLaneCountMax;
		ctxState->featureSupport.gpuTotalLaneCount = options1.TotalLaneCount;

		D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));

		ctxState->featureSupport.shader16bitOps =
			options4.Native16BitShaderOpsSupported ? ZG_TRUE : ZG_FALSE;

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

		ctxState->featureSupport.raytracing =
			options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED ? ZG_TRUE : ZG_FALSE;
		const char* raytracingTier = [](D3D12_RAYTRACING_TIER tier) {
			switch (tier) {
			case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: return "None";
			case D3D12_RAYTRACING_TIER_1_0: return "Tier 1.0";
			case D3D12_RAYTRACING_TIER_1_1: return "Tier 1.1";
			}
			return "";
		}(options5.RaytracingTier);
		snprintf(
			ctxState->featureSupport.raytracingTier,
			sizeof(ctxState->featureSupport.raytracingTier),
			"%s", raytracingTier);

		D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));

		ctxState->featureSupport.variableShadingRate =
			options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED ? ZG_TRUE : ZG_FALSE;
		const char* vrsTier = [](D3D12_VARIABLE_SHADING_RATE_TIER tier) {
			switch (tier) {
			case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: return "None";
			case D3D12_VARIABLE_SHADING_RATE_TIER_1: return "Tier 1";
			case D3D12_VARIABLE_SHADING_RATE_TIER_2: return "Tier 2";
			}
			return "";
		}(options6.VariableShadingRateTier);
		snprintf(
			ctxState->featureSupport.variableShadingRateTier,
			sizeof(ctxState->featureSupport.variableShadingRateTier),
			"%s", vrsTier);
		ctxState->featureSupport.variableShadingRateTileSize = options6.ShadingRateImageTileSize;

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
		CHECK_D3D12 ctxState->device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

		ctxState->featureSupport.meshShaders =
			options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED ? ZG_TRUE : ZG_FALSE;
	}

	// Enable debug message in debug mode
	if (ctxState->debugMode) {

		if (D3D12_FAIL(ctxState->device->QueryInterface(IID_PPV_ARGS(&ctxState->infoQueue)))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}

		// Break on corruption and error messages
		CHECK_D3D12 ctxState->infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		CHECK_D3D12 ctxState->infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

		// Log initial messages
		logDebugMessages(*ctxState);
	}

	// Allocate descriptors
	const u32 NUM_DESCRIPTORS = 1000000;
	ZG_INFO("Attempting to allocate %u descriptors for the global ring buffer",
		NUM_DESCRIPTORS);
	{
		ZgResult res = ctxState->globalDescriptorRingBuffer.create(
			*ctxState->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_DESCRIPTORS);
		if (res != ZG_SUCCESS) {
			ZG_ERROR("Failed to allocate descriptors");
			return ZG_ERROR_GPU_OUT_OF_MEMORY;
		}
	}

	// Create command queue
	ZgResult res = ctxState->commandQueuePresent.create(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		ctxState->device,
		&ctxState->globalDescriptorRingBuffer);
	if (res != ZG_SUCCESS) return res;

	// Create copy queue
	res = ctxState->commandQueueCopy.create(
		D3D12_COMMAND_LIST_TYPE_COPY,
		ctxState->device,
		&ctxState->globalDescriptorRingBuffer);
	if (res != ZG_SUCCESS) return res;


	// Check if screen-tearing is allowed
	{
		BOOL tearingAllowed = FALSE;
		CHECK_D3D12 dxgiFactory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingAllowed, sizeof(tearingAllowed));
		ctxState->allowTearing = tearingAllowed != FALSE;
	}
	ctxState->vsyncEnabled = settings.vsync != ZG_FALSE;

	// Create swap chain
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = ctxState->width;
		desc.Height = ctxState->height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Stereo = FALSE;
		desc.SampleDesc = { 1, 0 }; // No MSAA
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = NUM_SWAP_CHAIN_BUFFERS; // 3 buffers, TODO: 1-2 buffers for no-vsync?
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.Flags = (ctxState->allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

		ComPtr<IDXGISwapChain1> tmpSwapChain;
		if (D3D12_FAIL(dxgiFactory->CreateSwapChainForHwnd(
			ctxState->commandQueuePresent.mCommandQueue.Get(), hwnd, &desc, nullptr, nullptr, &tmpSwapChain))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}

		if (D3D12_FAIL(tmpSwapChain.As(&ctxState->swapchain))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}
	}

	// Disable Alt+Enter fullscreen toogle
	CHECK_D3D12 dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	// Perform early hacky initializiation of the D3D12 framebuffers to prepare them for
	// swapchain use
	// TODO: Unify this with the more general case somehow?
	for (u32 i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

		ZgFramebuffer& framebuffer = ctxState->swapchainFramebuffers[i];

		// Mark framebuffer as swapchain framebuffer
		// TODO: Hacky hack, consider attempting to unify with general use case
		framebuffer.swapchainFramebuffer = true;

		// Create render target descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.NumDescriptors = 1;
		rtvDesc.NodeMask = 0;
		if (D3D12_FAIL(ctxState->device->CreateDescriptorHeap(
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
		if (D3D12_FAIL(ctxState->device->CreateDescriptorHeap(
			&dsvDesc, IID_PPV_ARGS(&framebuffer.descriptorHeapDSV)))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}

		// Set depth buffer available and descriptor
		framebuffer.hasDepthBuffer = true;
		framebuffer.depthBufferDescriptor =
			framebuffer.descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart();
	}

	// Create swap chain framebuffers (RTVs and DSVs)
	ctxState->width = 0;
	ctxState->height = 0;

	// Store pipeline caching settings
	ctxState->allowPipelineCaching = settings.autoCachePipelines == ZG_TRUE ? true : false;
	if (ctxState->allowPipelineCaching) {
		sfz_assert(settings.autoCachePipelinesDir != nullptr);
		ctxState->pipelineCacheDir.appendf("%s", settings.autoCachePipelinesDir);
		ZG_INFO("Pipeline auto-cache enabled in dir: \"%s\"", ctxState->pipelineCacheDir.str());
	}

	return ZG_SUCCESS;
}

static ZgResult swapchainResize(ZgContextState& state, u32 width, u32 height) noexcept
{
	if (state.width == width && state.height == height) return ZG_SUCCESS;
	std::lock_guard<std::mutex> lock(state.contextMutex);

	// Log that we are resizing the swap chain and then change the stored size
	bool initialCreation = false;
	if (state.width == 0 && state.height == 0) {
		ZG_INFO("Creating swap chain framebuffers, size: %ux%u", width, height);
		initialCreation = true;
	}
	else {
		ZG_INFO("Resizing swap chain framebuffers from %ux%u to %ux%u",
			state.width, state.height, width, height);
	}
	state.width = width;
	state.height = height;

	// Flush command queue so its safe to resize back buffers
	[[maybe_unused]] ZgResult flushRes = state.commandQueuePresent.flush();

	if (!initialCreation) {
		// Release previous back buffers
		for (int i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {
			state.swapchainFramebuffers[i].swapchain.renderTarget.Reset();
			state.swapchainFramebuffers[i].swapchain.depthBuffer.Reset();
		}

		// Resize swap chain's back buffers
		DXGI_SWAP_CHAIN_DESC desc = {};
		CHECK_D3D12 state.swapchain->GetDesc(&desc);
		CHECK_D3D12 state.swapchain->ResizeBuffers(
			NUM_SWAP_CHAIN_BUFFERS, width, height, desc.BufferDesc.Format, desc.Flags);
	}

	// Update current back buffer index
	state.currentBackBufferIdx = state.swapchain->GetCurrentBackBufferIndex();

	// Create render target views (RTVs) for swap chain
	for (u32 i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

		// Get i:th back buffer from swap chain
		ComPtr<ID3D12Resource> backBufferRtv;
		CHECK_D3D12 state.swapchain->GetBuffer(i, IID_PPV_ARGS(&backBufferRtv));

		// Set width and height
		state.swapchainFramebuffers[i].width = width;
		state.swapchainFramebuffers[i].height = height;

		// Get the i:th RTV descriptor from the swap chain descriptor heap
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor =
			state.swapchainFramebuffers[i].renderTargetDescriptors[0];

		// Create render target view for i:th backbuffer
		state.device->CreateRenderTargetView(backBufferRtv.Get(), nullptr, rtvDescriptor);
		state.swapchainFramebuffers[i].swapchain.renderTarget = backBufferRtv;

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
		CHECK_D3D12 state.device->CreateCommittedResource(
			&dsvHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&dsvResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimizedClearValue,
			IID_PPV_ARGS(&backBufferDsv));

		// Get the i:th DSV descriptor from the swap chain descriptor heap
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor =
			state.swapchainFramebuffers[i].depthBufferDescriptor;

		// Create depth buffer view
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
		dsvViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvViewDesc.Texture2D.MipSlice = 0;

		state.device->CreateDepthStencilView(backBufferDsv.Get(), &dsvViewDesc, dsvDescriptor);
		state.swapchainFramebuffers[i].swapchain.depthBuffer = backBufferDsv;
		state.swapchainFramebuffers[i].depthBufferOptimalClearValue = ZG_OPTIMAL_CLEAR_VALUE_ONE;
	}

	logDebugMessages(state);
	return ZG_SUCCESS;
}

// Version information
// ------------------------------------------------------------------------------------------------

ZG_API u32 zgApiLinkedVersion(void)
{
	return ZG_COMPILED_API_VERSION;
}

// Backends
// ------------------------------------------------------------------------------------------------

ZG_API ZgBackendType zgBackendCompiledType(void)
{
	return ZG_BACKEND_D3D12;
}

// Results
// ------------------------------------------------------------------------------------------------

ZG_API const char* zgResultToString(ZgResult result)
{
	switch (result) {
	case ZG_SUCCESS: return "ZG_SUCCESS";

	case ZG_WARNING_GENERIC: return "ZG_WARNING_GENERIC";
	case ZG_WARNING_UNIMPLEMENTED: return "ZG_WARNING_UNIMPLEMENTED";
	case ZG_WARNING_ALREADY_INITIALIZED: return "ZG_WARNING_ALREADY_INITIALIZED";

	case ZG_ERROR_GENERIC: return "ZG_ERROR_GENERIC";
	case ZG_ERROR_CPU_OUT_OF_MEMORY: return "ZG_ERROR_CPU_OUT_OF_MEMORY";
	case ZG_ERROR_GPU_OUT_OF_MEMORY: return "ZG_ERROR_GPU_OUT_OF_MEMORY";
	case ZG_ERROR_NO_SUITABLE_DEVICE: return "ZG_ERROR_NO_SUITABLE_DEVICE";
	case ZG_ERROR_INVALID_ARGUMENT: return "ZG_ERROR_INVALID_ARGUMENT";
	case ZG_ERROR_SHADER_COMPILE_ERROR: return "ZG_ERROR_SHADER_COMPILE_ERROR";
	case ZG_ERROR_OUT_OF_COMMAND_LISTS: return "ZG_ERROR_OUT_OF_COMMAND_LISTS";
	case ZG_ERROR_INVALID_COMMAND_LIST_STATE: return "ZG_ERROR_INVALID_COMMAND_LIST_STATE";
	}
	return "<UNKNOWN RESULT>";
}

// Buffer
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgBufferCreate(
	ZgBuffer** bufferOut,
	const ZgBufferDesc* desc)
{
	return createBuffer(
		*bufferOut, *desc, ctxState->d3d12Allocator, &ctxState->resourceUniqueIdentifierCounter);
}

ZG_API void zgBufferDestroy(
	ZgBuffer* buffer)
{
	if (buffer == nullptr) return;
	sfz_delete(getAllocator(), buffer);
}

ZG_API ZgResult zgBufferMemcpyUpload(
	ZgBuffer* dstBuffer,
	u64 dstBufferOffsetBytes,
	const void* srcMemory,
	u64 numBytes)
{
	return bufferMemcpyUpload(*dstBuffer, dstBufferOffsetBytes, srcMemory, numBytes);
}

ZG_API ZgResult zgBufferMemcpyDownload(
	void* dstMemory,
	ZgBuffer* srcBuffer,
	u64 srcBufferOffsetBytes,
	u64 numBytes)
{
	return bufferMemcpyDownload(*srcBuffer, srcBufferOffsetBytes, dstMemory, numBytes);
}

// Textures
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgTextureCreate(
	ZgTexture** textureOut,
	const ZgTextureDesc* desc)
{
	return createTexture(
		*textureOut, *desc, *ctxState->device.Get(), ctxState->d3d12Allocator, &ctxState->resourceUniqueIdentifierCounter);
}

ZG_API void zgTextureDestroy(
	ZgTexture* texture)
{
	if (texture == nullptr) return;
	sfz_delete(getAllocator(), texture);
}

ZG_API u32 zgTextureSizeInBytes(
	const ZgTexture* texture)
{
	if (texture == nullptr) return 0;
	return u32(texture->totalSizeInBytes);
}

// Pipeline Compute
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgPipelineComputeCreateFromFileHLSL(
	ZgPipelineCompute** pipelineOut,
	const ZgPipelineComputeDesc* desc,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(desc == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	return createPipelineComputeFileHLSL(
		pipelineOut,
		*desc,
		*compileSettings,
		*ctxState->dxcLibrary.Get(),
		*ctxState->dxcCompiler.Get(),
		ctxState->dxcIncludeHandler,
		*ctxState->device.Get(),
		ctxState->allowPipelineCaching ? ctxState->pipelineCacheDir.str() : nullptr);
}

ZG_API void zgPipelineComputeDestroy(
	ZgPipelineCompute* pipeline)
{
	sfz_delete(getAllocator(), pipeline);
}

ZG_API void zgPipelineComputeGetBindingsSignature(
	const ZgPipelineCompute* pipeline,
	ZgPipelineBindingsSignature* bindingsSignatureOut)
{
	*bindingsSignatureOut = pipeline->bindingsSignature.toZgSignature();
}

ZG_API void zgPipelineComputeGetGroupDimensions(
	const ZgPipelineCompute* pipeline,
	u32* groupDimXOut,
	u32* groupDimYOut,
	u32* groupDimZOut)
{
	if (groupDimXOut != nullptr) *groupDimXOut = pipeline->groupDimX;
	if (groupDimYOut != nullptr) *groupDimYOut = pipeline->groupDimY;
	if (groupDimZOut != nullptr) *groupDimZOut = pipeline->groupDimZ;
}

// Pipeline Render
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgPipelineRenderCreateFromFileHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(compileSettings->shaderModel == ZG_SHADER_MODEL_UNDEFINED, "Must specify shader model");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");
	return createPipelineRenderFileHLSL(
		pipelineOut,
		*createInfo,
		*compileSettings,
		*ctxState->dxcLibrary.Get(),
		*ctxState->dxcCompiler.Get(),
		ctxState->dxcIncludeHandler,
		*ctxState->device.Get(),
		ctxState->allowPipelineCaching ? ctxState->pipelineCacheDir.str() : nullptr);
}

ZG_API ZgResult zgPipelineRenderCreateFromSourceHLSL(
	ZgPipelineRender** pipelineOut,
	const ZgPipelineRenderDesc* createInfo,
	const ZgPipelineCompileSettingsHLSL* compileSettings)
{
	ZG_ARG_CHECK(createInfo == nullptr, "");
	ZG_ARG_CHECK(compileSettings == nullptr, "");
	ZG_ARG_CHECK(pipelineOut == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->vertexShaderEntry == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShader == nullptr, "");
	ZG_ARG_CHECK(createInfo->pixelShaderEntry == nullptr, "");
	ZG_ARG_CHECK(compileSettings->shaderModel == ZG_SHADER_MODEL_UNDEFINED, "Must specify shader model");
	ZG_ARG_CHECK(createInfo->numVertexAttributes >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex attributes specified");
	ZG_ARG_CHECK(createInfo->numVertexBufferSlots >= ZG_MAX_NUM_VERTEX_ATTRIBUTES, "Too many vertex buffers specified");
	ZG_ARG_CHECK(createInfo->numPushConstants >= ZG_MAX_NUM_CONSTANT_BUFFERS, "Too many push constants specified");
	return createPipelineRenderSourceHLSL(
		pipelineOut,
		*createInfo,
		*compileSettings,
		*ctxState->dxcLibrary.Get(),
		*ctxState->dxcCompiler.Get(),
		ctxState->dxcIncludeHandler,
		*ctxState->device.Get(),
		ctxState->allowPipelineCaching ? ctxState->pipelineCacheDir.str() : nullptr);
}

ZG_API void zgPipelineRenderDestroy(
	ZgPipelineRender* pipeline)
{
	sfz_delete(getAllocator(), pipeline);
}

ZG_API void zgPipelineRenderGetSignature(
	const ZgPipelineRender* pipeline,
	ZgPipelineRenderSignature* signatureOut)
{
	*signatureOut = pipeline->renderSignature;
}

// Framebuffer
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFramebufferCreate(
	ZgFramebuffer** framebufferOut,
	const ZgFramebufferDesc* desc)
{
	ZG_ARG_CHECK(framebufferOut == nullptr, "");
	ZG_ARG_CHECK(desc == nullptr, "");
	ZG_ARG_CHECK(desc->numRenderTargets > ZG_MAX_NUM_RENDER_TARGETS, "Too many render targets");
	return createFramebuffer(
		*ctxState->device.Get(),
		framebufferOut,
		*desc);
}

ZG_API void zgFramebufferDestroy(
	ZgFramebuffer* framebuffer)
{
	if (framebuffer == nullptr) return;
	if (framebuffer->swapchainFramebuffer) return;
	sfz_delete(getAllocator(), framebuffer);
}

ZG_API ZgResult zgFramebufferGetResolution(
	const ZgFramebuffer* framebuffer,
	u32* widthOut,
	u32* heightOut)
{
	return framebuffer->getResolution(*widthOut, *heightOut);
}

// Profiler
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgProfilerCreate(
	ZgProfiler** profilerOut,
	const ZgProfilerDesc* desc)
{
	return d3d12CreateProfiler(
		*ctxState->device.Get(),
		ctxState->d3d12Allocator,
		&ctxState->resourceUniqueIdentifierCounter,
		profilerOut,
		*desc);
}

ZG_API void zgProfilerDestroy(
	ZgProfiler* profiler)
{
	if (profiler == nullptr) return;
	sfz_delete(getAllocator(), profiler);
}

ZG_API ZgResult zgProfilerGetMeasurement(
	ZgProfiler* profiler,
	u64 measurementId,
	f32* measurementMsOut)
{
	ZG_ARG_CHECK(profiler == nullptr, "");
	ZG_ARG_CHECK(measurementMsOut == nullptr, "");
	return profiler->getMeasurement(measurementId, *measurementMsOut);
}

// Fence
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgFenceCreate(
	ZgFence** fenceOut)
{
	*fenceOut = sfz_new<ZgFence>(getAllocator(), sfz_dbg("ZgFence"));
	return ZG_SUCCESS;
}

ZG_API void zgFenceDestroy(
	ZgFence* fence)
{
	if (fence == nullptr) return;
	sfz_delete(getAllocator(), fence);
}

ZG_API ZgResult zgFenceReset(
	ZgFence* fence)
{
	return fence->reset();
}

ZG_API ZgResult zgFenceCheckIfSignaled(
	const ZgFence* fence,
	ZgBool* fenceSignaledOut)
{
	bool fenceSignaled = false;
	ZgResult res = fence->checkIfSignaled(fenceSignaled);
	*fenceSignaledOut = fenceSignaled ? ZG_TRUE : ZG_FALSE;
	return res;
}

ZG_API ZgResult zgFenceWaitOnCpuBlocking(
	const ZgFence* fence)
{
	return fence->waitOnCpuBlocking();
}

// Command list
// ------------------------------------------------------------------------------------------------

ZG_API ZgResult zgCommandListBeginEvent(
	ZgCommandList* commandList,
	const char* name,
	const f32* optionalRgbaColor)
{
	return commandList->beginEvent(name, optionalRgbaColor);
}

ZG_API ZgResult zgCommandListEndEvent(
	ZgCommandList* commandList)
{
	return commandList->endEvent();
}

ZG_API ZgResult zgCommandListMemcpyBufferToBuffer(
	ZgCommandList* commandList,
	ZgBuffer* dstBuffer,
	u64 dstBufferOffsetBytes,
	ZgBuffer* srcBuffer,
	u64 srcBufferOffsetBytes,
	u64 numBytes)
{
	ZG_ARG_CHECK(numBytes == 0, "Can't copy zero bytes");
	return commandList->memcpyBufferToBuffer(
		dstBuffer,
		dstBufferOffsetBytes,
		srcBuffer,
		srcBufferOffsetBytes,
		numBytes);
}

ZG_API ZgResult zgCommandListMemcpyToTexture(
	ZgCommandList* commandList,
	ZgTexture* dstTexture,
	u32 dstTextureMipLevel,
	const ZgImageViewConstCpu* srcImageCpu,
	ZgBuffer* tempUploadBuffer)
{
	ZG_ARG_CHECK(srcImageCpu->data == nullptr, "");
	ZG_ARG_CHECK(srcImageCpu->width == 0, "");
	ZG_ARG_CHECK(srcImageCpu->height == 0, "");
	ZG_ARG_CHECK(srcImageCpu->pitchInBytes < srcImageCpu->width, "");
	ZG_ARG_CHECK(dstTextureMipLevel >= ZG_MAX_NUM_MIPMAPS, "Invalid target mip level");
	return commandList->memcpyToTexture(
		dstTexture,
		dstTextureMipLevel,
		*srcImageCpu,
		tempUploadBuffer);
}

ZG_API ZgResult zgCommandListEnableQueueTransitionBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer)
{
	return commandList->enableQueueTransitionBuffer(buffer);
}

ZG_API ZgResult zgCommandListEnableQueueTransitionTexture(
	ZgCommandList* commandList,
	ZgTexture* texture)
{
	return commandList->enableQueueTransitionTexture(texture);
}

ZG_API ZgResult zgCommandListSetPushConstant(
	ZgCommandList* commandList,
	u32 shaderRegister,
	const void* data,
	u32 dataSizeInBytes)
{
	ZG_ARG_CHECK(data == nullptr, "");
	return commandList->setPushConstant(shaderRegister, data, dataSizeInBytes);
}

ZG_API ZgResult zgCommandListSetPipelineBindings(
	ZgCommandList* commandList,
	const ZgPipelineBindings* bindings)
{
	return commandList->setPipelineBindings(*bindings);
}

ZG_API ZgResult zgCommandListSetPipelineCompute(
	ZgCommandList* commandList,
	ZgPipelineCompute* pipeline)
{
	return commandList->setPipelineCompute(pipeline);
}

ZG_API ZgResult zgCommandListUnorderedBarrierBuffer(
	ZgCommandList* commandList,
	ZgBuffer* buffer)
{
	return commandList->unorderedBarrierBuffer(buffer);
}

ZG_API ZgResult zgCommandListUnorderedBarrierTexture(
	ZgCommandList* commandList,
	ZgTexture* texture)
{
	return commandList->unorderedBarrierTexture(texture);
}

ZG_API ZgResult zgCommandListUnorderedBarrierAll(
	ZgCommandList* commandList)
{
	return commandList->unorderedBarrierAll();
}

ZG_API ZgResult zgCommandListDispatchCompute(
	ZgCommandList* commandList,
	u32 groupCountX,
	u32 groupCountY,
	u32 groupCountZ)
{
	return commandList->dispatchCompute(groupCountX, groupCountY, groupCountZ);
}

ZG_API ZgResult zgCommandListSetPipelineRender(
	ZgCommandList* commandList,
	ZgPipelineRender* pipeline)
{
	return commandList->setPipelineRender(pipeline);
}

ZG_API ZgResult zgCommandListSetFramebuffer(
	ZgCommandList* commandList,
	ZgFramebuffer* framebuffer,
	const ZgRect* optionalViewport,
	const ZgRect* optionalScissor)
{
	return commandList->setFramebuffer(framebuffer, optionalViewport, optionalScissor);
}

ZG_API ZgResult zgCommandListSetFramebufferViewport(
	ZgCommandList* commandList,
	const ZgRect* viewport)
{
	return commandList->setFramebufferViewport(*viewport);
}

ZG_API ZgResult zgCommandListSetFramebufferScissor(
	ZgCommandList* commandList,
	const ZgRect* scissor)
{
	return commandList->setFramebufferScissor(*scissor);
}


ZG_API ZgResult zgCommandListClearRenderTargetOptimal(
	ZgCommandList* commandList,
	u32 renderTargetIdx)
{
	return commandList->clearRenderTargetOptimal(renderTargetIdx);
}

ZG_API ZgResult zgCommandListClearRenderTargets(
	ZgCommandList* commandList,
	f32 red,
	f32 green,
	f32 blue,
	f32 alpha)
{
	return commandList->clearRenderTargets(red, green, blue, alpha);
}

ZG_API ZgResult zgCommandListClearRenderTargetsOptimal(
	ZgCommandList* commandList)
{
	return commandList->clearRenderTargetsOptimal();
}

ZG_API ZgResult zgCommandListClearDepthBuffer(
	ZgCommandList* commandList,
	f32 depth)
{
	return commandList->clearDepthBuffer(depth);
}

ZG_API ZgResult zgCommandListClearDepthBufferOptimal(
	ZgCommandList* commandList)
{
	return commandList->clearDepthBufferOptimal();
}

ZG_API ZgResult zgCommandListSetIndexBuffer(
	ZgCommandList* commandList,
	ZgBuffer* indexBuffer,
	ZgIndexBufferType type)
{
	return commandList->setIndexBuffer(indexBuffer, type);
}

ZG_API ZgResult zgCommandListSetVertexBuffer(
	ZgCommandList* commandList,
	u32 vertexBufferSlot,
	ZgBuffer* vertexBuffer)
{
	return commandList->setVertexBuffer(
		vertexBufferSlot, vertexBuffer);
}

ZG_API ZgResult zgCommandListDrawTriangles(
	ZgCommandList* commandList,
	u32 startVertexIndex,
	u32 numVertices)
{
	ZG_ARG_CHECK((numVertices % 3) != 0, "Odd number of vertices");
	return commandList->drawTriangles(startVertexIndex, numVertices);
}

ZG_API ZgResult zgCommandListDrawTrianglesIndexed(
	ZgCommandList* commandList,
	u32 startIndex,
	u32 numIndices)
{
	ZG_ARG_CHECK((numIndices % 3) != 0, "Odd number of indices");
	return commandList->drawTrianglesIndexed(startIndex, numIndices);
}

ZG_API ZgResult zgCommandListProfileBegin(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	u64* measurementIdOut)
{
	ZG_ARG_CHECK(profiler == nullptr, "");
	ZG_ARG_CHECK(measurementIdOut == nullptr, "");
	return commandList->profileBegin(profiler, *measurementIdOut);
}

ZG_API ZgResult zgCommandListProfileEnd(
	ZgCommandList* commandList,
	ZgProfiler* profiler,
	u64 measurementId)
{
	ZG_ARG_CHECK(profiler == nullptr, "");

	// Get command queue timestamp frequency
	u64 timestampTicksPerSecond = 0;
	if (commandList->commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
		bool success = D3D12_SUCC(ctxState->commandQueuePresent.mCommandQueue.Get()->
			GetTimestampFrequency(&timestampTicksPerSecond));
		if (!success) return ZG_ERROR_GENERIC;
	}
	/*else if (commandList->commandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {

	}*/
	else {
		return ZG_ERROR_INVALID_ARGUMENT;
	}

	return commandList->profileEnd(profiler, measurementId, timestampTicksPerSecond);
}

// Command queue
// ------------------------------------------------------------------------------------------------

ZG_API ZgCommandQueue* zgCommandQueueGetPresentQueue(void)
{
	return &ctxState->commandQueuePresent;
}

ZG_API ZgCommandQueue* zgCommandQueueGetCopyQueue(void)
{
	return &ctxState->commandQueueCopy;
}

ZG_API ZgResult zgCommandQueueSignalOnGpu(
	ZgCommandQueue* commandQueue,
	ZgFence* fenceToSignal)
{
	return commandQueue->signalOnGpu(*fenceToSignal);
}

ZG_API ZgResult zgCommandQueueWaitOnGpu(
	ZgCommandQueue* commandQueue,
	const ZgFence* fence)
{
	return commandQueue->waitOnGpu(*fence);
}

ZG_API ZgResult zgCommandQueueFlush(
	ZgCommandQueue* commandQueue)
{
	return commandQueue->flush();
}

ZG_API ZgResult zgCommandQueueBeginCommandListRecording(
	ZgCommandQueue* commandQueue,
	ZgCommandList** commandListOut)
{
	return commandQueue->beginCommandListRecording(commandListOut);
}

ZG_API ZgResult zgCommandQueueExecuteCommandList(
	ZgCommandQueue* commandQueue,
	ZgCommandList* commandList)
{
	return commandQueue->executeCommandList(commandList);
}

// Context
// ------------------------------------------------------------------------------------------------

ZG_API ZgBool zgContextAlreadyInitialized(void)
{
	return ctxState == nullptr ? ZG_FALSE : ZG_TRUE;
}

ZG_API ZgResult zgContextInit(const ZgContextInitSettings* settings)
{
	// Can't use ZG_ARG_CHECK() here because logger is not yet initialized
	if (settings == nullptr) return ZG_ERROR_INVALID_ARGUMENT;
	if (zgContextAlreadyInitialized() == ZG_TRUE) return ZG_WARNING_ALREADY_INITIALIZED;

	ZgContext tmpContext;

	// Set default logger if none is specified
	bool usingDefaultLogger = settings->logger.log == nullptr;
	if (usingDefaultLogger) tmpContext.logger = getDefaultLogger();
	else tmpContext.logger = settings->logger;

	// Set allocator if specified, otherwise standard allocator
	bool usingDefaultAllocator = settings->allocator == nullptr;
	if (usingDefaultAllocator) tmpContext.allocator = sfz::createStandardAllocator();
	else tmpContext.allocator = *settings->allocator; 

	// Set temporary context (without API backend). Required so rest of initialization can allocate
	// memory and log.
	setContext(tmpContext);

	// Log which logger is used
	if (usingDefaultLogger) ZG_INFO("zgContextInit(): Using default logger (printf)");
	else ZG_INFO("zgContextInit(): Using user-provided logger");

	// Log which allocator is used
	if (usingDefaultAllocator) ZG_INFO("zgContextInit(): Using default allocator");
	else ZG_INFO("zgContextInit(): Using user-provided allocator");

	// Create D3D12 backend
	{
		// Initialize backend, return nullptr if init failed
		ZgResult initRes = init(*settings);
		if (initRes != ZG_SUCCESS) {
			sfz_delete(getAllocator(), ctxState);
			ctxState = nullptr;
			ZG_ERROR("zgContextInit(): Could not create D3D12 backend, exiting.");
			return initRes;
		}

		initRes = swapchainResize(*ctxState, settings->width, settings->height);
		if (initRes != ZG_SUCCESS) {
			sfz_delete(getAllocator(), ctxState);
			ctxState = nullptr;
			ZG_ERROR("zgContextInit(): Could not create D3D12 swapchain, exiting.");
			return initRes;
		}
	}

	// Set context
	setContext(tmpContext);
	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextDeinit(void)
{
	if (zgContextAlreadyInitialized() == ZG_FALSE) return ZG_SUCCESS;

	ZgContext& ctx = getContext();

	// Delete context
	{
		// Flush command queues
		[[maybe_unused]] ZgResult flushRes1 = ctxState->commandQueuePresent.flush();
		[[maybe_unused]] ZgResult flushRes2 = ctxState->commandQueueCopy.flush();

		// Release include handler
		// TODO: Probably correct...?
		if (ctxState->dxcIncludeHandler != nullptr) {
			ctxState->dxcIncludeHandler->Release();
			ctxState->dxcIncludeHandler = nullptr;
		}

		// Log debug messages
		logDebugMessages(*ctxState);

		// Get debug device for report live objects in debug mode
		ComPtr<ID3D12DebugDevice1> debugDevice;
		bool debugMode = ctxState->debugMode;
		if (debugMode) {
			CHECK_D3D12 ctxState->device->QueryInterface(IID_PPV_ARGS(&debugDevice));
		}

		// Destroy D3D12MemoryAllocator
		ctxState->d3d12Allocator->Release();

		// Delete most state
		sfz_delete(getAllocator(), ctxState);
		ctxState = nullptr;

		// Report live objects
		if (debugMode) {
			CHECK_D3D12 debugDevice->ReportLiveDeviceObjects(
				D3D12_RLDO_FLAGS(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
		}
	}

	// Reset context
	ctx = {};
	ctx.allocator = sfz::createStandardAllocator();

	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextSwapchainResize(
	u32 width,
	u32 height)
{
	return swapchainResize(*ctxState, width, height);
}

ZG_API ZgResult zgContextSwapchainSetVsync(
	ZgBool vsync)
{
	ctxState->vsyncEnabled = vsync != ZG_FALSE;
	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextSwapchainBeginFrame(
	ZgFramebuffer** framebufferOut,
	ZgProfiler* profiler,
	u64* measurementIdOut)
{
	std::lock_guard<std::mutex> lock(ctxState->contextMutex);

	// Retrieve current back buffer to be rendered to
	ZgFramebuffer& backBuffer = ctxState->swapchainFramebuffers[ctxState->currentBackBufferIdx];

	// Create a small command list to insert the transition barrier for the back buffer
	ZgCommandList* barrierCommandList = nullptr;
	ZgResult zgRes =
		ctxState->commandQueuePresent.beginCommandListRecording(&barrierCommandList);
	if (zgRes != ZG_SUCCESS) return zgRes;

	// Begin Frame event
	[[maybe_unused]] ZgResult ignore1 = barrierCommandList->beginEvent("Frame", nullptr);

	// Create barrier to transition back buffer into render target state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer.swapchain.renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	barrierCommandList->commandList->ResourceBarrier(1, &barrier);

	// Insert profiling begin call if a profiler is specified
	if (profiler != nullptr) {
		ZgResult res = barrierCommandList->profileBegin(profiler, *measurementIdOut);
		sfz_assert(res == ZG_SUCCESS);
	}

	// Execute command list containing the barrier transition
	[[maybe_unused]] ZgResult ignore2 = ctxState->commandQueuePresent.executeCommandList(barrierCommandList);

	// Return backbuffer
	*framebufferOut = &backBuffer;

	logDebugMessages(*ctxState);
	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextSwapchainFinishFrame(
	ZgProfiler* profiler,
	u64 measurementId)
{
	std::lock_guard<std::mutex> lock(ctxState->contextMutex);

	// Retrieve current back buffer that has been rendered to
	ZgFramebuffer& backBuffer = ctxState->swapchainFramebuffers[ctxState->currentBackBufferIdx];

	// Create a small command list to insert the transition barrier for the back buffer
	ZgCommandList* barrierCommandList = nullptr;
	ZgResult zgRes =
		ctxState->commandQueuePresent.beginCommandListRecording(&barrierCommandList);
	if (zgRes != ZG_SUCCESS) return zgRes;

	// Create barrier to transition back buffer into present state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer.swapchain.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	barrierCommandList->commandList->ResourceBarrier(1, &barrier);

	// Finish profiling if a profiler is specified
	if (profiler != nullptr) {

		// Get command queue timestamp frequency
		u64 timestampTicksPerSecond = 0;
		bool success = D3D12_SUCC(ctxState->commandQueuePresent.mCommandQueue->
			GetTimestampFrequency(&timestampTicksPerSecond));
		sfz_assert(success);

		ZgResult res = barrierCommandList->profileEnd(profiler, measurementId, timestampTicksPerSecond);
		sfz_assert(res == ZG_SUCCESS);
	}

	// End Frame event
	[[maybe_unused]] ZgResult ignore3 = barrierCommandList->endEvent();

	// Execute command list containing the barrier transition
	[[maybe_unused]] ZgResult ignore4 = ctxState->commandQueuePresent.executeCommandList(barrierCommandList);

	// Signal the graphics present queue
	ctxState->swapchainFenceValues[ctxState->currentBackBufferIdx] =
		ctxState->commandQueuePresent.signalOnGpuInternal();

	// Present back buffer
	{
		UINT vsync = 0;
		UINT flags = 0;
		if (ctxState->vsyncEnabled) {
			vsync = 1;
		}
		else if (ctxState->allowTearing) {
			// vsync MUST be 0 if we use the DXGI_PRESENT_ALLOW_TEARING flag
			flags = DXGI_PRESENT_ALLOW_TEARING;
		}
		CHECK_D3D12 ctxState->swapchain->Present(vsync, flags);
	}

	// Get next back buffer index
	ctxState->currentBackBufferIdx = ctxState->swapchain->GetCurrentBackBufferIndex();

	// Wait for the next back buffer to finish rendering so it's safe to use
	u64 nextBackBufferFenceValue = ctxState->swapchainFenceValues[ctxState->currentBackBufferIdx];
	ctxState->commandQueuePresent.waitOnCpuInternal(nextBackBufferFenceValue);

	logDebugMessages(*ctxState);
	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextGetStats(ZgStats* statsOut)
{
	ZG_ARG_CHECK(statsOut == nullptr, "");

	// First set the static stats which don't change
	*statsOut = ctxState->staticStats;

	// Query information about "local" memory from DXGI
	// Local memory is "the fastest" for the GPU
	DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo = {};
	CHECK_D3D12 ctxState->dxgiAdapter->QueryVideoMemoryInfo(
		0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo);

	// Query information about "non-local" memory from DXGI
	DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfoNonLocal = {};
	CHECK_D3D12 ctxState->dxgiAdapter->QueryVideoMemoryInfo(
		0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memoryInfoNonLocal);

	// Set memory info stats
	statsOut->memoryBudgetBytes = memoryInfo.Budget;
	statsOut->memoryUsageBytes = memoryInfo.CurrentUsage;
	statsOut->nonLocalBugetBytes = memoryInfoNonLocal.Budget;
	statsOut->nonLocalUsageBytes = memoryInfoNonLocal.CurrentUsage;

	return ZG_SUCCESS;
}

ZG_API ZgResult zgContextGetFeatureSupport(ZgFeatureSupport* featureSupportOut)
{
	ZG_ARG_CHECK(featureSupportOut == nullptr, "");
	*featureSupportOut = ctxState->featureSupport;
	return ZG_SUCCESS;
}
