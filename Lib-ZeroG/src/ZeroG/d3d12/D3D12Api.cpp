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

// CHECK_D3D12 macro
// ------------------------------------------------------------------------------------------------

// Checks result (HRESULT) from D3D call and log if not success, returns result unmodified
#define CHECK_D3D12 (CheckD3D12Impl(__FILE__, __LINE__)) %

// Checks result (HRESULT) from D3D call and log if not success, converts result to bool
#define CHECK_D3D12_SUCCEEDED(result) (CheckD3D12Impl(__FILE__, __LINE__).succeeded((result)))

static const char* d3d12HresultToString(HRESULT result) noexcept
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
		if (result == S_OK) return S_OK;
		printf("D3D12_ERROR: %s\n", d3d12HresultToString(result));
		return result;
	}

	bool succeeded(HRESULT result) noexcept
	{
		if (result == S_OK) return true;
		printf("D3D12_ERROR: %s\n", d3d12HresultToString(result));
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

	}

	// State methods
	// --------------------------------------------------------------------------------------------

	ZgErrorCode init(ZgAllocator& allocator, bool debugMode) noexcept
	{
		mAllocator = allocator;
		mDebugMode = debugMode;

		// Enable debug layers in debug mode
		if (debugMode) {
			ComPtr<ID3D12Debug> debugInterface;
			if (CHECK_D3D12_SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
				debugInterface->EnableDebugLayer();
			}
			else {
				return ZG_ERROR_GENERIC;
			}
		}
		
		// Create DXGI factory
		ComPtr<IDXGIFactory4> dxgiFactory;
		{
			UINT flags = 0;
			if (debugMode) flags |= DXGI_CREATE_FACTORY_DEBUG;
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
				if (dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
					break;
				}

				// Get adapter description
				DXGI_ADAPTER_DESC1 desc;
				CHECK_D3D12 adapter->GetDesc1(&desc);

				// Skip adapter if it is software renderer
				if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;

				// Skip adapter if it is not possible to create device with feature level 12.0
				if (!SUCCEEDED(D3D12CreateDevice(
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
		
		return ZG_SUCCESS;
	}

	// Private members
	// --------------------------------------------------------------------------------------------
private:
	ZgAllocator mAllocator = {};
	bool mDebugMode = false;
};

// D3D12 API
// ------------------------------------------------------------------------------------------------

ZgErrorCode createD3D12Backend(
	Api** apiOut,
	void* windowHandle,
	ZgAllocator& allocator,
	bool debugMode) noexcept
{
	// Allocate and create D3D12 backend
	D3D12Api* api = zgNew<D3D12Api>(allocator, "D3D12 backend");

	// Initialize backend, return nullptr if init failed
	ZgErrorCode initRes = api->init(allocator, debugMode);
	if (initRes != ZG_SUCCESS)
	{
		zgDelete(allocator, api);
		return initRes;
	}

	*apiOut = api;
	return ZG_SUCCESS;
}

} // namespace zg
