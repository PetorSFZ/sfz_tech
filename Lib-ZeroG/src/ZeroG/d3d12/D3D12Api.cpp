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

#include "ZeroG/d3d12/D3D12Api.hpp"

// Windows.h
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h> // ComPtr
//#include <Winerror.h>

// D3D12 headers
#include <d3d12.h>
#pragma comment (lib, "d3d12.lib")
#include <dxgi1_6.h>
#pragma comment (lib, "dxgi.lib")
//#pragma comment (lib, "dxguid.lib")

// ZeroG headers
#include "ZeroG/CpuAllocation.hpp"

namespace zg {

using Microsoft::WRL::ComPtr;

// Statics
// ------------------------------------------------------------------------------------------------

constexpr auto NUM_SWAP_CHAIN_BUFFERS = 3;

static const char* stripFilePath(const char* file) noexcept
{
	const char* strippedFile1 = std::strrchr(file, '\\');
	const char* strippedFile2 = std::strrchr(file, '/');
	if (strippedFile1 == nullptr && strippedFile2 == nullptr) {
		return file;
	}
	else if (strippedFile2 == nullptr) {
		return strippedFile1 + 1;
	}
	else {
		return strippedFile2 + 1;
	}
}

static D3D12_RESOURCE_BARRIER createBarrierTransition(
	ID3D12Resource& resource,
	D3D12_RESOURCE_STATES stateBefore,
	D3D12_RESOURCE_STATES stateAfter,
	uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
	D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = flags;
	barrier.Transition.pResource = &resource;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;
	barrier.Transition.Subresource = subresource;
	return barrier;
}

// CHECK_D3D12 macro
// ------------------------------------------------------------------------------------------------

// Checks result (HRESULT) from D3D call and log if not success, returns result unmodified
#define CHECK_D3D12 (CheckD3D12Impl(__FILE__, __LINE__)) %

// Checks result (HRESULT) from D3D call and log if not success, converts result to bool
#define CHECK_D3D12_SUCCEEDED(result) (CheckD3D12Impl(__FILE__, __LINE__).succeeded((result)))

static const char* resultToString(HRESULT result) noexcept
{
	switch (result) {
	case DXGI_ERROR_ACCESS_DENIED: return "DXGI_ERROR_ACCESS_DENIED";
	case DXGI_ERROR_ACCESS_LOST: return "DXGI_ERROR_ACCESS_LOST";
	case DXGI_ERROR_ALREADY_EXISTS: return "DXGI_ERROR_ALREADY_EXISTS";
	case DXGI_ERROR_CANNOT_PROTECT_CONTENT: return "DXGI_ERROR_CANNOT_PROTECT_CONTENT";
	case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
	case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
	case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
	case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
	case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
	case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
	case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA";
	case DXGI_ERROR_NAME_ALREADY_EXISTS: return "DXGI_ERROR_NAME_ALREADY_EXISTS";
	case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE";
	case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
	case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND";
	case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
	case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY";
	case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE: return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
	case DXGI_ERROR_SDK_COMPONENT_MISSING: return "DXGI_ERROR_SDK_COMPONENT_MISSING";
	case DXGI_ERROR_SESSION_DISCONNECTED: return "DXGI_ERROR_SESSION_DISCONNECTED";
	case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED";
	case DXGI_ERROR_WAIT_TIMEOUT: return "DXGI_ERROR_WAIT_TIMEOUT";
	case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING";

	//case D3D12_ERROR_FILE_NOT_FOUND: return "D3D12_ERROR_FILE_NOT_FOUND";
	//case D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
	//case D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
	
	case E_FAIL: return "E_FAIL";
	case E_INVALIDARG: return "E_INVALIDARG";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	case E_NOTIMPL: return "E_NOTIMPL";
	case S_FALSE: return "S_FALSE";
	case S_OK: return "S_OK";
	}
	return "UNKNOWN";
}

struct CheckD3D12Impl final {
	const char* file;
	int line;

	CheckD3D12Impl() = delete;
	CheckD3D12Impl(const char* file, int line) noexcept : file(file), line(line) {}

	HRESULT operator% (HRESULT result) noexcept
	{
		if (SUCCEEDED(result)) return result;
		printf("%s:%i: D3D12 error: %s\n", stripFilePath(file), line, resultToString(result));
		return result;
	}

	bool succeeded(HRESULT result) noexcept
	{
		if (SUCCEEDED(result)) return true;
		printf("%s:%i: D3D12 error: %s\n", stripFilePath(file), line, resultToString(result));
		return false;
	}
};

// D3D12 API implementation
// ------------------------------------------------------------------------------------------------

class D3D12Api : public Api {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Api() noexcept = default;
	D3D12Api(const D3D12Api&) = delete;
	D3D12Api& operator= (const D3D12Api&) = delete;
	D3D12Api(D3D12Api&&) = delete;
	D3D12Api& operator= (D3D12Api&&) = delete;

	virtual ~D3D12Api() noexcept
	{
		flushCommandQueue();

		// Destroy Command Queue Fence Event
		CloseHandle(mCommandQueueFenceEvent);
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
		if (mWidth == 0 || mHeight == 0) return ZG_ERROR_INVALID_PARAMS;

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
		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // TODO: D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
			desc.NodeMask = 0;

			if (!CHECK_D3D12_SUCCEEDED(
				mDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&mCommandQueue)))) {
				return ZG_ERROR_NO_SUITABLE_DEVICE;
			}
		}

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
				mCommandQueue.Get(), hwnd, &desc, nullptr, nullptr, &tmpSwapChain))) {
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
		
		// Create command queue fence
		if (!CHECK_D3D12_SUCCEEDED(mDevice->CreateFence(
			mCommandQueueFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCommandQueueFence)))) {
			return ZG_ERROR_GENERIC;
		}

		// Create command queue fence event
		mCommandQueueFenceEvent = ::CreateEvent(NULL, false, false, NULL);

		return ZG_SUCCESS;
	}

	// API methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode resize(uint32_t width, uint32_t height) noexcept override final
	{
		if (mWidth == width && mHeight == height) return ZG_SUCCESS;

		mWidth = width;
		mHeight = height;

		// Flush command queue so its safe to resize back buffers
		flushCommandQueue();

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

	ZgErrorCode renderExperiment() noexcept override final
	{
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
		D3D12_RESOURCE_BARRIER barrier = createBarrierTransition(
			backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mCommandList->ResourceBarrier(1, &barrier);

		// Clear screen
		float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		mCommandList->ClearRenderTargetView(backBufferDescriptor, clearColor, 0, nullptr);

		// Create barrier to transition back buffer into present state
		barrier = createBarrierTransition(
			backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCommandList->ResourceBarrier(1, &barrier);

		// Finish command list
		CHECK_D3D12 mCommandList->Close();

		// Execute command list
		ID3D12CommandList* cmdListPtr = mCommandList.Get();
		mCommandQueue->ExecuteCommandLists(1, &cmdListPtr);

		// Signal the command queue
		mSwapChainFenceValues[mCurrentBackBufferIdx] = signalCommandQueueGpu();

		// Present back buffer
		UINT vsync = 0; // TODO (MUST be 0 if DXGI_PRESENT_ALLOW_TEARING)
		CHECK_D3D12 mSwapChain->Present(vsync, mAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);

		// Get next back buffer index
		mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex();

		// Wait for the next back buffer to finish rendering
		uint64_t nextBackBufferFenceValue = mSwapChainFenceValues[mCurrentBackBufferIdx];
		waitCommandQueueCpu(nextBackBufferFenceValue);

		return ZG_SUCCESS;
	}

	// Methods
	// --------------------------------------------------------------------------------------------

	uint64_t signalCommandQueueGpu() noexcept
	{
		CHECK_D3D12 mCommandQueue->Signal(mCommandQueueFence.Get(), mCommandQueueFenceValue);
		return mCommandQueueFenceValue++;
	}

	void waitCommandQueueCpu(uint64_t fenceValue) noexcept
	{
		if (mCommandQueueFence->GetCompletedValue() < fenceValue) {
			CHECK_D3D12 mCommandQueueFence->SetEventOnCompletion(
				fenceValue, mCommandQueueFenceEvent);
			// TODO: Don't wait forever
			::WaitForSingleObject(mCommandQueueFenceEvent, INFINITE);
		}
	}

	void flushCommandQueue() noexcept
	{
		uint64_t fenceValue = mCommandQueueFenceValue;
		signalCommandQueueGpu();
		waitCommandQueueCpu(fenceValue);
	}

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	ZgAllocator mAllocator = {};
	bool mDebugMode = false;

	ComPtr<ID3D12Device5> mDevice;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	//ComPtr<ID3D12CommandQueue> mCopyQueue;
	ComPtr<IDXGISwapChain4> mSwapChain;

	uint32_t mWidth = 0;
	uint32_t mHeight = 0;

	ComPtr<ID3D12DescriptorHeap> mSwapChainDescriptorHeap;
	ComPtr<ID3D12Resource> mBackBuffers[NUM_SWAP_CHAIN_BUFFERS];
	ComPtr<ID3D12CommandAllocator> mCommandAllocator[NUM_SWAP_CHAIN_BUFFERS];
	uint64_t mSwapChainFenceValues[NUM_SWAP_CHAIN_BUFFERS] = {};

	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	ComPtr<ID3D12Fence> mCommandQueueFence;
	uint64_t mCommandQueueFenceValue = 0;
	HANDLE mCommandQueueFenceEvent = nullptr;

	int mCurrentBackBufferIdx = 0;

	uint32_t mDescriptorSizeRTV = 0;
	bool mAllowTearing = false;
};

// D3D12 API
// ------------------------------------------------------------------------------------------------

ZgErrorCode createD3D12Backend(Api** apiOut, ZgContextInitSettings& settings) noexcept
{
	// Allocate and create D3D12 backend
	D3D12Api* api = zgNew<D3D12Api>(settings.allocator, "D3D12 backend");

	// Initialize backend, return nullptr if init failed
	ZgErrorCode initRes = api->init(settings);
	if (initRes != ZG_SUCCESS)
	{
		zgDelete(settings.allocator, api);
		return initRes;
	}

	*apiOut = api;
	return ZG_SUCCESS;
}

} // namespace zg
