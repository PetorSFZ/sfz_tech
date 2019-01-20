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
#include "ZeroG/d3d12/D3D12Memory.hpp"
#include "ZeroG/d3d12/D3D12PipelineRendering.hpp"
#include "ZeroG/util/CpuAllocation.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

constexpr auto NUM_SWAP_CHAIN_BUFFERS = 3;

static D3D12_HEAP_TYPE bufferMemoryTypeToD3D12HeapType(ZgBufferMemoryType type) noexcept
{
	switch (type) {
	case ZG_BUFFER_MEMORY_TYPE_UPLOAD: return D3D12_HEAP_TYPE_UPLOAD;
	case ZG_BUFFER_MEMORY_TYPE_DOWNLOAD: return D3D12_HEAP_TYPE_READBACK;
	case ZG_BUFFER_MEMORY_TYPE_DEVICE: return D3D12_HEAP_TYPE_DEFAULT;
	}
	// TODO: Handle error (undefined)
	return D3D12_HEAP_TYPE_DEFAULT;
}

// D3D12 Context implementation
// ------------------------------------------------------------------------------------------------

class D3D12Context final : public IContext {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Context() noexcept = default;
	D3D12Context(const D3D12Context&) = delete;
	D3D12Context& operator= (const D3D12Context&) = delete;
	D3D12Context(D3D12Context&&) = delete;
	D3D12Context& operator= (D3D12Context&&) = delete;

	virtual ~D3D12Context() noexcept
	{
		mCommandQueue.flush();
	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode init(ZgContextInitSettings& settings) noexcept
	{
		mAllocator = settings.allocator;
		mDebugMode = settings.debugMode;
		mWidth = uint32_t(settings.width);
		mHeight = uint32_t(settings.height);
		HWND hwnd = (HWND)settings.nativeWindowHandle;
		if (mWidth == 0 || mHeight == 0) return ZG_ERROR_INVALID_ARGUMENT;

		// Enable debug layers in debug mode
		if (settings.debugMode) {
			ComPtr<ID3D12Debug> debugInterface;
			if (CHECK_D3D12_SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
				debugInterface->EnableDebugLayer();
			}
			else {
				return ZG_ERROR_GENERIC;
			}
		}

		// Create DXGI factory
		ComPtr<IDXGIFactory7> dxgiFactory;
		{
			UINT flags = 0;
			if (settings.debugMode) flags |= DXGI_CREATE_FACTORY_DEBUG;
			if (!CHECK_D3D12_SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory)))) {
				return ZG_ERROR_GENERIC;
			}
		}

		// Create DXGI adapter
		ComPtr<IDXGIAdapter4> dxgiAdapter;
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
				CHECK_D3D12 adapter->GetDesc1(&desc);

				// Skip adapter if it is software renderer
				if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;

				// Skip adapter if it is not possible to create device with feature level 12.0
				if (!CHECK_D3D12_SUCCEEDED(D3D12CreateDevice(
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
			if (!CHECK_D3D12_SUCCEEDED(bestAdapter.As(&dxgiAdapter))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}
		}

		// Create device
		if (!CHECK_D3D12_SUCCEEDED(D3D12CreateDevice(
			dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)))) {
			return ZG_ERROR_NO_SUITABLE_DEVICE;
		}

		// Enable debug message in debug mode
		// TODO: Figure out how to make work with callback based logger.
		if (mDebugMode) {

			ComPtr<ID3D12InfoQueue> infoQueue;
			if (!CHECK_D3D12_SUCCEEDED(mDevice.As(&infoQueue))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}

		// Create command queue
		const uint32_t MAX_NUM_COMMAND_LISTS = 128;
		ZgErrorCode res = mCommandQueue.init(mDevice, MAX_NUM_COMMAND_LISTS);
		if (res != ZG_SUCCESS) return res;

		// Check if screen-tearing is allowed
		{
			BOOL tearingAllowed = FALSE;
			CHECK_D3D12 dxgiFactory->CheckFeatureSupport(
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
			if (!CHECK_D3D12_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(
				mCommandQueue.commandQueue(), hwnd, &desc, nullptr, nullptr, &tmpSwapChain))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}

			if (!CHECK_D3D12_SUCCEEDED(tmpSwapChain.As(&mSwapChain))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}
		}

		// Disable Alt+Enter fullscreen toogle
		CHECK_D3D12 dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

		// Create swap chain descriptor heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = NUM_SWAP_CHAIN_BUFFERS; // Same as number of swap chain buffers, TODO: how decide
			desc.NodeMask = 0;

			if (!CHECK_D3D12_SUCCEEDED(mDevice->CreateDescriptorHeap(
				&desc, IID_PPV_ARGS(&mSwapChainDescriptorHeap)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}
		}

		// Create render target views (RTVs) for swap chain
		{
			// The size of an RTV descriptor
			mDescriptorSizeRTV = mDevice->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// The first descriptor of the swap chain descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE startOfDescriptorHeap =
				mSwapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

			for (UINT i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

				// Get i:th back buffer from swap chain
				ComPtr<ID3D12Resource> backBuffer;
				if (!CHECK_D3D12_SUCCEEDED(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)))) {
					return ZG_ERROR_NO_SUITABLE_DEVICE;
				}

				// Get the i:th descriptor from the swap chain descriptor heap
				D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {};
				descriptor.ptr = startOfDescriptorHeap.ptr + mDescriptorSizeRTV * i;

				// Create render target view for i:th backbuffer
				mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, descriptor);
				mBackBuffers[i] = backBuffer;
			}
		}

		// Create command allocators
		for (UINT i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {
			// TODO: This is tied to the type of queue (e.g. direct queue, copy queue).
			//       Figure out where to place this later.
			if (!CHECK_D3D12_SUCCEEDED(mDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator[i])))) {
				return ZG_ERROR_GENERIC;
			}
		}



		// Create command list
		// TODO: Think about this
		{
			if (!CHECK_D3D12_SUCCEEDED(mDevice->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				mCommandAllocator[0].Get(),
				nullptr,
				IID_PPV_ARGS(&mCommandList)))) {
				return ZG_ERROR_GENERIC;
			}

			// TODO: Why?
			CHECK_D3D12 mCommandList->Close();
		}

		

		return ZG_SUCCESS;
	}

	// Context methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode resize(uint32_t width, uint32_t height) noexcept override final
	{
		if (mWidth == width && mHeight == height) return ZG_SUCCESS;
		std::lock_guard<std::mutex> lock(mContextMutex);

		mWidth = width;
		mHeight = height;

		// Flush command queue so its safe to resize back buffers
		mCommandQueue.flush();

		// Release previous back buffers
		for (int i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {
			mBackBuffers[i].Reset();
		}

		// Resize swap chain's back buffers
		DXGI_SWAP_CHAIN_DESC desc = {};
		CHECK_D3D12 mSwapChain->GetDesc(&desc);
		CHECK_D3D12 mSwapChain->ResizeBuffers(
			NUM_SWAP_CHAIN_BUFFERS, width, height, desc.BufferDesc.Format, desc.Flags);

		// Update current back buffer index
		mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex();

		// The first descriptor of the swap chain descriptor heap
		D3D12_CPU_DESCRIPTOR_HANDLE startOfDescriptorHeap =
			mSwapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// Create render target views (RTVs) for swap chain
		for (UINT i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++) {

			// Get i:th back buffer from swap chain
			ComPtr<ID3D12Resource> backBuffer;
			CHECK_D3D12 mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

			// Get the i:th descriptor from the swap chain descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {};
			descriptor.ptr = startOfDescriptorHeap.ptr + mDescriptorSizeRTV * i;

			// Create render target view for i:th backbuffer
			mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, descriptor);
			mBackBuffers[i] = backBuffer;
		}

		return ZG_SUCCESS;
	}

	// Pipeline methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode pipelineCreate(
		IPipelineRendering** pipelineOut,
		const ZgPipelineRenderingCreateInfo& createInfo) noexcept override final
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
		ZgErrorCode res = createPipelineRendering(
			&d3d12pipeline,
			createInfo,
			*mDxcLibrary.Get(),
			*mDxcCompiler.Get(),
			mAllocator,
			*mDevice.Get(),
			mContextMutex);
		if (!CHECK_D3D12_SUCCEEDED(res)) return res;
		
		*pipelineOut = d3d12pipeline;
		return res;
	}

	ZgErrorCode pipelineRelease(IPipelineRendering* pipeline) noexcept override final
	{
		// TODO: Check if pipeline is currently in use? Lock?
		zgDelete<IPipelineRendering>(mAllocator, pipeline);
		return ZG_SUCCESS;
	}

	// Memory methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode bufferCreate(
		IBuffer** bufferOut,
		const ZgBufferCreateInfo& createInfo) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);

		// Create heap
		ComPtr<ID3D12Heap> heap;
		{
			bool allowAtomics = createInfo.bufferMemoryType == ZG_BUFFER_MEMORY_TYPE_DEVICE;

			D3D12_HEAP_DESC desc = {};
			desc.SizeInBytes = createInfo.sizeInBytes;
			desc.Properties.Type = bufferMemoryTypeToD3D12HeapType(createInfo.bufferMemoryType);
			desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			desc.Properties.CreationNodeMask = 0; // No multi-GPU support
			desc.Properties.VisibleNodeMask = 0; // No multi-GPU support
			desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS |
				(allowAtomics ? D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS(0));

			// Create heap
			if (!CHECK_D3D12_SUCCEEDED(mDevice->CreateHeap(&desc, IID_PPV_ARGS(&heap)))) {
				return ZG_ERROR_GPU_OUT_OF_MEMORY;
			}
		}
		
		// Create placed resource
		ComPtr<ID3D12Resource> resource;
		{
			bool allowUav = createInfo.bufferMemoryType == ZG_BUFFER_MEMORY_TYPE_DEVICE;

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			desc.Width = createInfo.sizeInBytes;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags =
				allowUav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS(0);

			// Create placed resource
			if (!CHECK_D3D12_SUCCEEDED(mDevice->CreatePlacedResource(
				heap.Get(),
				0,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&resource)))) {
				return ZG_ERROR_GPU_OUT_OF_MEMORY;
			}
		}

		// Allocate buffer
		D3D12Buffer* buffer =
			zgNew<D3D12Buffer>(mAllocator, "ZeroG - D3D12Buffer");

		// Copy stuff
		buffer->memoryType = createInfo.bufferMemoryType;
		buffer->heap = heap;
		buffer->resource = resource;

		// Return buffer
		*bufferOut = buffer;
		return ZG_SUCCESS;
	}

	ZgErrorCode bufferRelease(IBuffer* buffer) noexcept override final
	{
		// TODO: Check if buffer is currently in use? Lock?
		zgDelete<IBuffer>(mAllocator, buffer);
		return ZG_SUCCESS;
	}

	ZgErrorCode bufferMemcpyTo(
		IBuffer* dstBufferInterface,
		uint64_t bufferOffsetBytes,
		const uint8_t* srcMemory,
		uint64_t numBytes) noexcept override final
	{
		D3D12Buffer& dstBuffer = *reinterpret_cast<D3D12Buffer*>(dstBufferInterface);
		if (dstBuffer.memoryType != ZG_BUFFER_MEMORY_TYPE_UPLOAD) return ZG_ERROR_INVALID_ARGUMENT;

		// Not gonna read from buffer
		D3D12_RANGE readRange = {};
		readRange.Begin = 0;
		readRange.End = 0;
		
		// Map buffer
		void* mappedPtr = nullptr;
		if (!CHECK_D3D12_SUCCEEDED(dstBuffer.resource->Map(0, &readRange, &mappedPtr))) {
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

	// Command queue
	// --------------------------------------------------------------------------------------------

	ZgErrorCode getCommandQueue(ICommandQueue** commandQueueOut) noexcept override final
	{
		*commandQueueOut = &mCommandQueue;
		return ZG_SUCCESS;
	}

	// Experiments
	// --------------------------------------------------------------------------------------------

	ZgErrorCode renderExperiment(
		IBuffer* bufferIn,
		IPipelineRendering* pipelineIn,
		ICommandList* commandListIn) noexcept override final
	{
		std::lock_guard<std::mutex> lock(mContextMutex);
		
		// Cast input to D3D12
		D3D12Buffer& vertexBuffer = *reinterpret_cast<D3D12Buffer*>(bufferIn);
		D3D12PipelineRendering& pipeline = *reinterpret_cast<D3D12PipelineRendering*>(pipelineIn);
		D3D12CommandList& commandList = *reinterpret_cast<D3D12CommandList*>(commandListIn);

		// Get resources for current target back buffer
		ID3D12Resource& backBuffer = *mBackBuffers[mCurrentBackBufferIdx].Get();
		ID3D12CommandAllocator& commandAllocator = *mCommandAllocator[mCurrentBackBufferIdx].Get();
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferDescriptor;
		backBufferDescriptor.ptr =
			mSwapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr +
			mDescriptorSizeRTV * mCurrentBackBufferIdx;

		// Reset command allocator and command list
		CHECK_D3D12 commandAllocator.Reset();
		CHECK_D3D12 mCommandList->Reset(&commandAllocator, nullptr);

		// Create barrier to transition back buffer into render target state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			&backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mCommandList->ResourceBarrier(1, &barrier);

		// Clear screen
		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		mCommandList->ClearRenderTargetView(backBufferDescriptor, clearColor, 0, nullptr);

	


		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(float) * 6; // TODO: Don't hardcode
		vertexBufferView.SizeInBytes = vertexBufferView.StrideInBytes * 3; // TODO: Don't hardcode


		// Set viewport
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = float(mWidth);
		viewport.Height = float(mHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		mCommandList->RSSetViewports(1, &viewport);

		// Set scissor rects
		D3D12_RECT scissorRect = {};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = LONG_MAX;
		scissorRect.bottom = LONG_MAX;
		mCommandList->RSSetScissorRects(1, &scissorRect);

		// Set render target descriptor
		mCommandList->OMSetRenderTargets(1, &backBufferDescriptor, FALSE, nullptr);

		// Set pipeline
		mCommandList->SetPipelineState(pipeline.pipelineState.Get());
		mCommandList->SetGraphicsRootSignature(pipeline.rootSignature.Get());

		// Set vertex buffer
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

		// Draw
		mCommandList->DrawInstanced(3, 1, 0, 0);

		



		// Create barrier to transition back buffer into present state
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			&backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCommandList->ResourceBarrier(1, &barrier);

		// Finish command list
		CHECK_D3D12 mCommandList->Close();

		// Execute command list
		ID3D12CommandList* cmdListPtr = mCommandList.Get();
		mCommandQueue.commandQueue()->ExecuteCommandLists(1, &cmdListPtr);

		// Signal the command queue
		mSwapChainFenceValues[mCurrentBackBufferIdx] = mCommandQueue.signalOnGpu();

		// Present back buffer
		UINT vsync = 0; // TODO (MUST be 0 if DXGI_PRESENT_ALLOW_TEARING)
		CHECK_D3D12 mSwapChain->Present(vsync, mAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);

		// Get next back buffer index
		mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex();

		// Wait for the next back buffer to finish rendering
		uint64_t nextBackBufferFenceValue = mSwapChainFenceValues[mCurrentBackBufferIdx];
		mCommandQueue.waitOnCpu(nextBackBufferFenceValue);

		return ZG_SUCCESS;
	}

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	std::mutex mContextMutex; // Access to the context is synchronized
	ZgAllocator mAllocator = {};
	bool mDebugMode = false;

	// DXC compiler DLLs, lazily loaded if needed
	ComPtr<IDxcLibrary> mDxcLibrary;
	ComPtr<IDxcCompiler> mDxcCompiler;

	ComPtr<ID3D12Device3> mDevice;
	D3D12CommandQueue mCommandQueue;
	
	//ComPtr<ID3D12CommandQueue> mCopyQueue;
	ComPtr<IDXGISwapChain4> mSwapChain;

	uint32_t mWidth = 0;
	uint32_t mHeight = 0;

	ComPtr<ID3D12DescriptorHeap> mSwapChainDescriptorHeap;
	ComPtr<ID3D12Resource> mBackBuffers[NUM_SWAP_CHAIN_BUFFERS];
	ComPtr<ID3D12CommandAllocator> mCommandAllocator[NUM_SWAP_CHAIN_BUFFERS];
	uint64_t mSwapChainFenceValues[NUM_SWAP_CHAIN_BUFFERS] = {};

	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	int mCurrentBackBufferIdx = 0;

	uint32_t mDescriptorSizeRTV = 0;
	bool mAllowTearing = false;
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
