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
#include "ZeroG/d3d12/D3D12PipelineRendering.hpp"
#include "ZeroG/d3d12/D3D12Textures.hpp"
#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

constexpr auto NUM_SWAP_CHAIN_BUFFERS = 3;

// D3D12 Context implementation
// ------------------------------------------------------------------------------------------------

class D3D12Context final : public IContext {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Context() = default;
	D3D12Context(const D3D12Context&) = delete;
	D3D12Context& operator= (const D3D12Context&) = delete;
	D3D12Context(D3D12Context&&) = delete;
	D3D12Context& operator= (D3D12Context&&) = delete;

	virtual ~D3D12Context() noexcept
	{
		mCommandQueueGraphicsPresent.flush();
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode init(ZgContextInitSettings& settings) noexcept
	{
		mLog = settings.logger;
		mAllocator = settings.allocator;
		mDebugMode = settings.debugMode;
		mWidth = settings.width;
		mHeight = settings.height;
		HWND hwnd = (HWND)settings.nativeWindowHandle;
		if (mWidth == 0 || mHeight == 0) return ZG_ERROR_INVALID_ARGUMENT;

		// Enable debug layers in debug mode
		if (settings.debugMode) {
			ComPtr<ID3D12Debug> debugInterface;
			if (D3D12_SUCC(mLog, D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
				debugInterface->EnableDebugLayer();
			}
			else {
				return ZG_ERROR_GENERIC;
			}
			ZG_INFO(mLog, "D3D12 debug mode enabled");
		}

		// Create DXGI factory
		ComPtr<IDXGIFactory7> dxgiFactory;
		{
			UINT flags = 0;
			if (settings.debugMode) flags |= DXGI_CREATE_FACTORY_DEBUG;
			if (D3D12_FAIL(mLog, CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory)))) {
				return ZG_ERROR_GENERIC;
			}
		}

		// Create DXGI adapter
		{
			// Iterate over all adapters and attempt to select the best one, current assumption
			// is that the device with the most amount of video memory is the best device
			ComPtr<IDXGIAdapter1> bestAdapter;
			SIZE_T bestAdapterVideoMemory = 0;
			for (UINT i = 0; true; i++) {

				// Get adapter, exit loop if no more adapters
				ComPtr<IDXGIAdapter1> adapter;
				if (dxgiFactory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) {
					break;
				}

				// Get adapter description
				DXGI_ADAPTER_DESC1 desc;
				CHECK_D3D12(mLog) adapter->GetDesc1(&desc);

				// Skip adapter if it is software renderer
				if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;

				// Skip adapter if it is not possible to create device with feature level 12.0
				if (D3D12_FAIL(mLog, D3D12CreateDevice(
					adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
					continue;
				}

				// Store best adapter if it has more video memory than the last one
				if (desc.DedicatedVideoMemory > bestAdapterVideoMemory) {
					bestAdapterVideoMemory = desc.DedicatedVideoMemory;
					bestAdapter = adapter;
				}
			}

			// Return error if not suitable device found
			if (bestAdapterVideoMemory == 0) return ZG_ERROR_NO_SUITABLE_DEVICE;

			// Convert device to DXGIAdapter4
			if (D3D12_FAIL(mLog, bestAdapter.As(&mDxgiAdapter))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			// Log some information about the choosen adapter
			DXGI_ADAPTER_DESC1 bestDesc;
			CHECK_D3D12(mLog) bestAdapter->GetDesc1(&bestDesc);
			ZG_INFO(mLog, "Description: %S\nVendor ID: %#x\nDevice ID: %u\nRevision: %u\n"
				"Dedicated video memory: %.2f GiB\nDedicated system memory: %.2f GiB\n"
				"Shared system memory: %.2f GiB",
				bestDesc.Description,
				uint32_t(bestDesc.VendorId),
				uint32_t(bestDesc.DeviceId),
				uint32_t(bestDesc.Revision),
				double(bestDesc.DedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0),
				double(bestDesc.DedicatedSystemMemory) / (1024.0 * 1024.0 * 1024.0),
				double(bestDesc.SharedSystemMemory) / (1024.0 * 1024.0 * 1024.0));
		}

		// Create device
		if (D3D12_FAIL(mLog, D3D12CreateDevice(
			mDxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}

		// Enable debug message in debug mode
		// TODO: Figure out how to make work with callback based logger.
		if (mDebugMode) {

			ComPtr<ID3D12InfoQueue> infoQueue;
			if (D3D12_FAIL(mLog, mDevice.As(&infoQueue))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}

		// Create residency manager
		// Latency: "NumberOfBufferedFrames * NumberOfCommandListSubmissionsPerFrame throughout
		// the execution of your app."
		// Hmm. Lets try 6 or something, seems to be default value.
		const uint32_t residencyManagerMaxLatency = 6;
		if (D3D12_FAIL(mLog, mResidencyManager.Initialize(
			mDevice.Get(), 0, mDxgiAdapter.Get(), residencyManagerMaxLatency))) {
			return ZG_ERROR_GENERIC;
		}

		// Allocate descriptors
		const uint32_t NUM_DESCRIPTORS = 1000000;
		ZG_INFO(mLog, "Attempting to allocate %u descriptors for the global ring buffer",
			NUM_DESCRIPTORS);
		{
			ZgErrorCode res = mGlobalDescriptorRingBuffer.create(
				*mDevice.Get(), mLog, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_DESCRIPTORS);
			if (res != ZG_SUCCESS) {
				ZG_ERROR(mLog, "Failed to allocate descriptors");
				return ZG_ERROR_GPU_OUT_OF_MEMORY;
			}
		}

		// Create command queue
		const uint32_t MAX_NUM_COMMAND_LISTS = 256;
		const uint32_t MAX_NUM_BUFFERS_PER_COMMAND_LIST = 256;
		ZgErrorCode res = mCommandQueueGraphicsPresent.create(
			mDevice,
			&mResidencyManager,
			&mGlobalDescriptorRingBuffer,
			MAX_NUM_COMMAND_LISTS,
			MAX_NUM_BUFFERS_PER_COMMAND_LIST,
			mLog,
			mAllocator);
		if (res != ZG_SUCCESS) return res;

		// Check if screen-tearing is allowed
		{
			BOOL tearingAllowed = FALSE;
			CHECK_D3D12(mLog) dxgiFactory->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingAllowed, sizeof(tearingAllowed));
			mAllowTearing = tearingAllowed != FALSE;
		}

		// Create swap chain
		{
			DXGI_SWAP_CHAIN_DESC1 desc = {};
			desc.Width = mWidth;
			desc.Height = mHeight;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Stereo = FALSE;
			desc.SampleDesc = { 1, 0 }; // No MSAA
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = NUM_SWAP_CHAIN_BUFFERS; // 3 buffers, TODO: 1-2 buffers for no-vsync?
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // Vsync? TODO: DXGI_SWAP_EFFECT_FLIP_DISCARD
			desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			desc.Flags = (mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

			ComPtr<IDXGISwapChain1> tmpSwapChain;
			if (D3D12_FAIL(mLog, dxgiFactory->CreateSwapChainForHwnd(
				mCommandQueueGraphicsPresent.commandQueue(), hwnd, &desc, nullptr, nullptr, &tmpSwapChain))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			if (D3D12_FAIL(mLog, tmpSwapChain.As(&mSwapChain))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}
		}

		// Disable Alt+Enter fullscreen toogle
		CHECK_D3D12(mLog) dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

		// Create swap chain descriptor heaps
		{
			// RTV descriptor heap
			D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
			rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvDesc.NumDescriptors = NUM_SWAP_CHAIN_BUFFERS;
			rtvDesc.NodeMask = 0;
			if (D3D12_FAIL(mLog, mDevice->CreateDescriptorHeap(
				&rtvDesc, IID_PPV_ARGS(&mSwapChainRtvDescriptorHeap)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			// The size of a RTV descriptor
			mDescriptorSizeRTV = mDevice->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// DSV descriptor heap
			D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
			dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvDesc.NumDescriptors = NUM_SWAP_CHAIN_BUFFERS;
			dsvDesc.NodeMask = 0;
			if (D3D12_FAIL(mLog, mDevice->CreateDescriptorHeap(
				&dsvDesc, IID_PPV_ARGS(&mSwapChainDsvDescriptorHeap)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			// The size of a DSV descriptor
			mDescriptorSizeDSV = mDevice->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		// Create swap chain framebuffers (RTVs and DSVs)
		mWidth = 0;
		mHeight = 0;
		this->swapchainResize(settings.width, settings.height);

		return ZG_SUCCESS;
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode swapchainResize(uint32_t width, uint32_t height) noexcept override final
	{
		if (mWidth == width && mHeight == height) return ZG_SUCCESS;
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Log that we are resizing the swap chain and then change the stored size
		bool initialCreation = false;
		if (mWidth == 0 && mHeight == 0) {
			ZG_INFO(mLog, "Creating swap chain framebuffers, size: %ux%u", width, height);
			initialCreation = true;
		}
		else {
			ZG_INFO(mLog, "Resizing swap chain framebuffers from %ux%u to %ux%u",
				mWidth, mHeight, width, height);
		}
		mWidth = width;
		mHeight = height;

		// Flush command queue so its safe to resize back buffers
		mCommandQueueGraphicsPresent.flush();

		if (!initialCreation) {
			// Release previous back buffers
			for (int i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {
				mBackBuffers[i].rtvResource.Reset();
			}

			// Resize swap chain's back buffers
			DXGI_SWAP_CHAIN_DESC desc = {};
			CHECK_D3D12(mLog) mSwapChain->GetDesc(&desc);
			CHECK_D3D12(mLog) mSwapChain->ResizeBuffers(
				NUM_SWAP_CHAIN_BUFFERS, width, height, desc.BufferDesc.Format, desc.Flags);
		}

		// Update current back buffer index
		mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex();

		// The first descriptor of the swap chain's descriptor heaps
		D3D12_CPU_DESCRIPTOR_HANDLE startOfRtvDescriptorHeap =
			mSwapChainRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE startOfDsvDescriptorHeap =
			mSwapChainDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// Create render target views (RTVs) for swap chain
		for (UINT i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

			// Get i:th back buffer from swap chain
			ComPtr<ID3D12Resource> backBufferRtv;
			CHECK_D3D12(mLog) mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBufferRtv));

			// Set width and height
			mBackBuffers[i].width = width;
			mBackBuffers[i].height = height;

			// Get the i:th RTV descriptor from the swap chain descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
			rtvDescriptor.ptr = startOfRtvDescriptorHeap.ptr + mDescriptorSizeRTV * i;
			mBackBuffers[i].rtvDescriptor = rtvDescriptor;

			// Create render target view for i:th backbuffer
			mDevice->CreateRenderTargetView(backBufferRtv.Get(), nullptr, rtvDescriptor);
			mBackBuffers[i].rtvResource = backBufferRtv;


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
			CHECK_D3D12(mLog) mDevice->CreateCommittedResource(
				&dsvHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&dsvResourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&optimizedClearValue,
				IID_PPV_ARGS(&backBufferDsv));
			
			// Get the i:th DSV descriptor from the swap chain descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};
			dsvDescriptor.ptr = startOfDsvDescriptorHeap.ptr + mDescriptorSizeDSV * i;
			mBackBuffers[i].dsvDescriptor = dsvDescriptor;

			// Create depth buffer view
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
			dsvViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvViewDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvViewDesc.Texture2D.MipSlice = 0;

			mDevice->CreateDepthStencilView(backBufferDsv.Get(), &dsvViewDesc, dsvDescriptor);
			mBackBuffers[i].dsvResource = backBufferDsv;
		}

		return ZG_SUCCESS;
	}

	ZgErrorCode swapchainCommandQueue(
		ICommandQueue** commandQueueOut) noexcept override final
	{
		*commandQueueOut = &mCommandQueueGraphicsPresent;
		return ZG_SUCCESS;
	}

	ZgErrorCode swapchainBeginFrame(
		zg::IFramebuffer** framebufferOut) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Retrieve current back buffer to be rendered to
		D3D12Framebuffer& backBuffer = mBackBuffers[mCurrentBackBufferIdx];

		// Create a small command list to insert the transition barrier for the back buffer
		ICommandList* barrierCommandList = nullptr;
		ZgErrorCode zgRes =
			mCommandQueueGraphicsPresent.beginCommandListRecording(&barrierCommandList);
		if (zgRes != ZG_SUCCESS) return zgRes;

		// Create barrier to transition back buffer into render target state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.rtvResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		reinterpret_cast<D3D12CommandList*>(barrierCommandList)->
			commandList->ResourceBarrier(1, &barrier);

		// Execute command list containing the barrier transition
		mCommandQueueGraphicsPresent.executeCommandList(barrierCommandList);

		// Return backbuffer
		*framebufferOut = &backBuffer;

		return ZG_SUCCESS;
	}

	ZgErrorCode swapchainFinishFrame() noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Retrieve current back buffer that has been rendered to
		D3D12Framebuffer& backBuffer = mBackBuffers[mCurrentBackBufferIdx];

		// Create a small command list to insert the transition barrier for the back buffer
		ICommandList* barrierCommandList = nullptr;
		ZgErrorCode zgRes =
			mCommandQueueGraphicsPresent.beginCommandListRecording(&barrierCommandList);
		if (zgRes != ZG_SUCCESS) return zgRes;

		// Create barrier to transition back buffer into present state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.rtvResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		reinterpret_cast<D3D12CommandList*>(barrierCommandList)->
			commandList->ResourceBarrier(1, &barrier);

		// Execute command list containing the barrier transition
		mCommandQueueGraphicsPresent.executeCommandList(barrierCommandList);

		// Signal the graphics present queue
		mSwapChainFenceValues[mCurrentBackBufferIdx] = mCommandQueueGraphicsPresent.signalOnGpu();

		// Present back buffer
		UINT vsync = 0; // TODO (MUST be 0 if DXGI_PRESENT_ALLOW_TEARING)
		CHECK_D3D12(mLog) mSwapChain->Present(vsync, mAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);

		// Get next back buffer index
		mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex();

		// Wait for the next back buffer to finish rendering so it's safe to use
		uint64_t nextBackBufferFenceValue = mSwapChainFenceValues[mCurrentBackBufferIdx];
		mCommandQueueGraphicsPresent.waitOnCpu(nextBackBufferFenceValue);

		return ZG_SUCCESS;
	}

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode pipelineRenderingCreateFromFileSPIRV(
		IPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoFileSPIRV& createInfo) noexcept override final
	{
		// Initialize DXC compiler if necessary
		// TODO: Provide our own allocator
		{
			std::lock_guard<std::mutex> lock(mContextMutex);
			if (mDxcLibrary == nullptr) {

				// Initialize DXC library
				HRESULT res = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mDxcLibrary));
				if (!SUCCEEDED(res)) return ZG_ERROR_GENERIC;

				// Initialize DXC compiler
				res = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mDxcCompiler));
				if (!SUCCEEDED(res)) {
					mDxcLibrary = nullptr;
					return ZG_ERROR_GENERIC;
				}
			}
		}
		
		// Create pipeline
		D3D12PipelineRendering* d3d12pipeline = nullptr;
		ZgErrorCode res = createPipelineRenderingFileSPIRV(
			&d3d12pipeline,
			signatureOut,
			createInfo,
			*mDxcLibrary.Get(),
			*mDxcCompiler.Get(),
			mLog,
			mAllocator,
			*mDevice.Get());
		if (res != ZG_SUCCESS) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgErrorCode pipelineRenderingCreateFromFileHLSL(
		IPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoFileHLSL& createInfo) noexcept override final
	{
		// Initialize DXC compiler if necessary
		// TODO: Provide our own allocator
		{
			std::lock_guard<std::mutex> lock(mContextMutex);
			if (mDxcLibrary == nullptr) {

				// Initialize DXC library
				HRESULT res = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mDxcLibrary));
				if (!SUCCEEDED(res)) return ZG_ERROR_GENERIC;

				// Initialize DXC compiler
				res = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mDxcCompiler));
				if (!SUCCEEDED(res)) {
					mDxcLibrary = nullptr;
					return ZG_ERROR_GENERIC;
				}
			}
		}
		
		// Create pipeline
		D3D12PipelineRendering* d3d12pipeline = nullptr;
		ZgErrorCode res = createPipelineRenderingFileHLSL(
			&d3d12pipeline,
			signatureOut,
			createInfo,
			*mDxcLibrary.Get(),
			*mDxcCompiler.Get(),
			mLog,
			mAllocator,
			*mDevice.Get());
		if (res != ZG_SUCCESS) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgErrorCode pipelineRenderingCreateFromSourceHLSL(
		IPipelineRendering** pipelineOut,
		ZgPipelineRenderingSignature* signatureOut,
		const ZgPipelineRenderingCreateInfoSourceHLSL& createInfo) noexcept override final
	{
		// Initialize DXC compiler if necessary
		// TODO: Provide our own allocator
		{
			std::lock_guard<std::mutex> lock(mContextMutex);
			if (mDxcLibrary == nullptr) {

				// Initialize DXC library
				HRESULT res = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mDxcLibrary));
				if (!SUCCEEDED(res)) return ZG_ERROR_GENERIC;

				// Initialize DXC compiler
				res = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mDxcCompiler));
				if (!SUCCEEDED(res)) {
					mDxcLibrary = nullptr;
					return ZG_ERROR_GENERIC;
				}
			}
		}
		
		// Create pipeline
		D3D12PipelineRendering* d3d12pipeline = nullptr;
		ZgErrorCode res = createPipelineRenderingSourceHLSL(
			&d3d12pipeline,
			signatureOut,
			createInfo,
			*mDxcLibrary.Get(),
			*mDxcCompiler.Get(),
			mLog,
			mAllocator,
			*mDevice.Get());
		if (res != ZG_SUCCESS) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgErrorCode pipelineRenderingRelease(
		IPipelineRendering* pipeline) noexcept override final
	{
		// TODO: Check if pipeline is currently in use? Lock?
		zgDelete<IPipelineRendering>(mAllocator, pipeline);
		return ZG_SUCCESS;
	}

	ZgErrorCode pipelineRenderingGetSignature(
		const IPipelineRendering* pipelineIn,
		ZgPipelineRenderingSignature* signatureOut) const noexcept override final
	{
		const D3D12PipelineRendering* pipeline =
			reinterpret_cast<const D3D12PipelineRendering*>(pipelineIn);
		*signatureOut = pipeline->signature;
		return ZG_SUCCESS;
	}

	// Memory methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode memoryHeapCreate(
		IMemoryHeap** memoryHeapOut,
		const ZgMemoryHeapCreateInfo& createInfo) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);
		return createMemoryHeap(
			mLog,
			mAllocator,
			*mDevice.Get(),
			&mResourceUniqueIdentifierCounter,
			mResidencyManager,
			reinterpret_cast<D3D12MemoryHeap**>(memoryHeapOut),
			createInfo);
	}

	ZgErrorCode memoryHeapRelease(
		IMemoryHeap* memoryHeapIn) noexcept override final
	{
		// TODO: Check if any buffers still exist? Lock?

		// Stop tracking
		D3D12MemoryHeap* heap = reinterpret_cast<D3D12MemoryHeap*>(memoryHeapIn);
		mResidencyManager.EndTrackingObject(&heap->managedObject);

		zgDelete<IMemoryHeap>(mAllocator, heap);
		return ZG_SUCCESS;
	}

	ZgErrorCode bufferMemcpyTo(
		IBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept override final
	{
		D3D12Buffer& dstBuffer = *reinterpret_cast<D3D12Buffer*>(dstBufferInterface);
		if (dstBuffer.memoryHeap->memoryType != ZG_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;

		// Not gonna read from buffer
		D3D12_RANGE readRange = {};
		readRange.Begin = 0;
		readRange.End = 0;
		
		// Map buffer
		void* mappedPtr = nullptr;
		if (D3D12_FAIL(mLog, dstBuffer.resource->Map(0, &readRange, &mappedPtr))) {
			return ZG_ERROR_GENERIC;
		}

		// Memcpy to buffer
		memcpy(reinterpret_cast<uint8_t*>(mappedPtr) + bufferOffsetBytes, srcMemory, numBytes);

		// The range we memcpy'd to
		D3D12_RANGE writeRange = {};
		writeRange.Begin = bufferOffsetBytes;
		writeRange.End = writeRange.Begin + numBytes;

		// Unmap buffer
		dstBuffer.resource->Unmap(0, nullptr);// &writeRange);

		return ZG_SUCCESS;
	}

	// Texture methods
	// --------------------------------------------------------------------------------------------

	virtual ZgErrorCode texture2DGetAllocationInfo(
		ZgTexture2DAllocationInfo& allocationInfoOut,
		const ZgTexture2DCreateInfo& createInfo) noexcept override final
	{
		// Get resource desc
		D3D12_RESOURCE_DESC desc = createInfoToResourceDesc(createInfo);

		// Get allocation info
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = mDevice->GetResourceAllocationInfo(0, 1, &desc);

		// Return allocation info
		allocationInfoOut.sizeInBytes = (uint32_t)allocInfo.SizeInBytes;
		allocationInfoOut.alignmentInBytes = (uint32_t)allocInfo.Alignment;
		return ZG_SUCCESS;
	}

	ZgErrorCode textureHeapCreate(
		ITextureHeap** textureHeapOut,
		const ZgTextureHeapCreateInfo& createInfo) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);
		return createTextureHeap(
			mLog,
			mAllocator,
			*mDevice.Get(),
			&mResourceUniqueIdentifierCounter,
			mResidencyManager,
			reinterpret_cast<D3D12TextureHeap**>(textureHeapOut),
			createInfo);
	}

	ZgErrorCode textureHeapRelease(
		ITextureHeap* textureHeapIn) noexcept override final
	{
		// TODO: Check if any textures still exist? Lock?

		// Stop tracking
		D3D12TextureHeap* heap = reinterpret_cast<D3D12TextureHeap*>(textureHeapIn);
		mResidencyManager.EndTrackingObject(&heap->managedObject);

		zgDelete<ITextureHeap>(mAllocator, heap);
		return ZG_SUCCESS;
	}

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	std::mutex mContextMutex; // Access to the context is synchronized
	ZgLogger mLog = {};
	ZgAllocator mAllocator = {};
	bool mDebugMode = false;

	// DXC compiler DLLs, lazily loaded if needed
	ComPtr<IDxcLibrary> mDxcLibrary;
	ComPtr<IDxcCompiler> mDxcCompiler;

	// Device
	ComPtr<IDXGIAdapter4> mDxgiAdapter;
	ComPtr<ID3D12Device3> mDevice;
	
	// Residency manager
	D3DX12Residency::ResidencyManager mResidencyManager;

	// Global descriptor ring buffers
	D3D12DescriptorRingBuffer mGlobalDescriptorRingBuffer;

	// Command queues
	D3D12CommandQueue mCommandQueueGraphicsPresent;
	//D3D12CommandQueue mCommandQueueAsyncCompute;
	//D3D12CommandQueue mCommandQueueCopy;
	
	// Swapchain and backbuffers
	uint32_t mWidth = 0;
	uint32_t mHeight = 0;
	ComPtr<IDXGISwapChain4> mSwapChain;
	ComPtr<ID3D12DescriptorHeap> mSwapChainRtvDescriptorHeap;
	uint32_t mDescriptorSizeRTV = 0;
	ComPtr<ID3D12DescriptorHeap> mSwapChainDsvDescriptorHeap;
	uint32_t mDescriptorSizeDSV = 0;
	D3D12Framebuffer mBackBuffers[NUM_SWAP_CHAIN_BUFFERS];
	uint64_t mSwapChainFenceValues[NUM_SWAP_CHAIN_BUFFERS] = {};
	int mCurrentBackBufferIdx = 0;

	bool mAllowTearing = false;

	// Memory
	std::atomic_uint64_t mResourceUniqueIdentifierCounter = 1;
};

// D3D12 API
// ------------------------------------------------------------------------------------------------

ZgErrorCode createD3D12Backend(IContext** contextOut, ZgContextInitSettings& settings) noexcept
{
	// Allocate and create D3D12 backend
	D3D12Context* context = zgNew<D3D12Context>(settings.allocator, "D3D12 Context");

	// Initialize backend, return nullptr if init failed
	ZgErrorCode initRes = context->init(settings);
	if (initRes != ZG_SUCCESS)
	{
		zgDelete(settings.allocator, context);
		return initRes;
	}

	*contextOut = context;
	return ZG_SUCCESS;
}

} // namespace zg
