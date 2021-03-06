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

#pragma once

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

// Windows.h
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h> // ComPtr
//#include <Winerror.h>

// D3D12 headers
#include <D3D12AgilitySDK/d3d12.h>
#pragma comment (lib, "d3d12.lib")
#include <dxgi1_6.h>
#pragma comment (lib, "dxgi.lib")
//#pragma comment (lib, "dxguid.lib")
#include <D3D12AgilitySDK/d3d12shader.h>

// DXGI Debug
//#include <dxgidebug.h>
//#pragma comment (lib, "dxguid.lib")

// DXC compiler
#include <dxc/dxcapi.h>

// D3DX12 library
#include "d3dx12.h"

#include "common/Logging.hpp"

 using Microsoft::WRL::ComPtr;

using sfz::ArrayLocal;
using sfz::str320;

// TextureFormats conversion
// ------------------------------------------------------------------------------------------------

inline DXGI_FORMAT zgToDxgiTextureFormat(ZgTextureFormat format) noexcept
{
	switch (format) {
	case ZG_TEXTURE_FORMAT_R_U8_UNORM: return DXGI_FORMAT_R8_UNORM;
	case ZG_TEXTURE_FORMAT_RG_U8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
	case ZG_TEXTURE_FORMAT_RGBA_U8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;

	case ZG_TEXTURE_FORMAT_R_F16: return DXGI_FORMAT_R16_FLOAT;
	case ZG_TEXTURE_FORMAT_RG_F16: return DXGI_FORMAT_R16G16_FLOAT;
	case ZG_TEXTURE_FORMAT_RGBA_F16: return DXGI_FORMAT_R16G16B16A16_FLOAT;

	case ZG_TEXTURE_FORMAT_R_F32: return DXGI_FORMAT_R32_FLOAT;
	case ZG_TEXTURE_FORMAT_RG_F32: return DXGI_FORMAT_R32G32_FLOAT;
	case ZG_TEXTURE_FORMAT_RGBA_F32: return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case ZG_TEXTURE_FORMAT_DEPTH_F32: return DXGI_FORMAT_D32_FLOAT;

	default:
		break;
	}

	sfz_assert(false);
	return DXGI_FORMAT_UNKNOWN;
}

// HRESULT toString
// ------------------------------------------------------------------------------------------------

inline const char* resultToString(HRESULT result) noexcept
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

	case S_OK: return "S_OK";
	case E_NOTIMPL: return "E_NOTIMPL";
	case E_NOINTERFACE: return "E_NOINTERFACE";
	case E_POINTER: return "E_POINTER";
	case E_ABORT: return "E_ABORT";
	case E_FAIL: return "E_FAIL";
	case E_UNEXPECTED: return "E_UNEXPECTED";
	case E_ACCESSDENIED: return "E_ACCESSDENIED";
	case E_HANDLE: return "E_HANDLE";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	case E_INVALIDARG: return "E_INVALIDARG";
	case S_FALSE: return "S_FALSE";
	}
	return "UNKNOWN";
}

// Helper functions
// ------------------------------------------------------------------------------------------------

inline bool utf8ToWide(WCHAR* wideOut, uint32_t numWideChars, const char* utf8In) noexcept
{
	// TODO: Unsure if 6th paramter should be num chars or num bytes =/
	int res = MultiByteToWideChar(CP_UTF8, 0, utf8In, -1, wideOut, numWideChars);
	return res != 0;
}

// Checks result (HRESULT) from D3D call and log if not success, returns result unmodified
#define CHECK_D3D12 (CheckD3D12Impl(__FILE__, __LINE__)) %

// Checks result (HRESULT) from D3D call and log if not success, true on success
#define D3D12_SUCC(result) \
	(CheckD3D12Impl(__FILE__, __LINE__).succeeded((result)))

// Checks result (HRESULT) from D3D call and log if not success, true on failure
#define D3D12_FAIL(result) \
	(!CheckD3D12Impl(__FILE__, __LINE__).succeeded((result)))

void dredCallback(HRESULT res);

struct CheckD3D12Impl final {
	const char* file = nullptr;
	int line = 0;

	CheckD3D12Impl() = delete;
	CheckD3D12Impl(const char* file, int line) noexcept
		: file(file), line(line) {}

	HRESULT operator% (HRESULT result) noexcept
	{
		if (SUCCEEDED(result)) return result;
		dredCallback(result);
		logWrapper(file, line, ZG_LOG_LEVEL_ERROR,
			"D3D12 error: %s\n", resultToString(result));
		return result;
	}

	bool succeeded(HRESULT result) noexcept
	{
		if (SUCCEEDED(result)) return true;
		dredCallback(result);
		logWrapper(file, line, ZG_LOG_LEVEL_ERROR,
			"D3D12 error: %s\n", resultToString(result));
		return false;
	}
};

inline void setDebugName(ID3D12Resource* resource, const char* name) noexcept
{
	// Small hack to fix D3D12 bug with debug name shorter than 4 chars
	char tmpBuffer[256] = {};
	snprintf(tmpBuffer, 256, "zg__%s", name);

	// Convert to wide
	WCHAR tmpBufferWide[256] = {};
	utf8ToWide(tmpBufferWide, 256, tmpBuffer);

	// Set debug name
	CHECK_D3D12 resource->SetName(tmpBufferWide);
}


// Device creation functions
// ------------------------------------------------------------------------------------------------

inline void d3d12LogAvailableDevices(ComPtr<IDXGIFactory6>& dxgiFactory) noexcept
{
	for (uint32_t i = 0; true; i++) {

		// Get adapter, exit loop if no more adapters
		ComPtr<IDXGIAdapter1> adapter;
		if (dxgiFactory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;

		//dxgiFactory->EnumAdapterByGpuPreference

		// Get adapter description
		DXGI_ADAPTER_DESC1 desc;
		CHECK_D3D12 adapter->GetDesc1(&desc);

		// Log description
		ZG_INFO("Adapter: %u\nDescription: %S\nVendor ID: %#x\nDevice ID: %u\nRevision: %u\n"
			"Dedicated video memory: %.2f GiB\nDedicated system memory: %.2f GiB\n"
			"Shared system memory: %.2f GiB",
			i,
			desc.Description,
			uint32_t(desc.VendorId),
			uint32_t(desc.DeviceId),
			uint32_t(desc.Revision),
			double(desc.DedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0),
			double(desc.DedicatedSystemMemory) / (1024.0 * 1024.0 * 1024.0),
			double(desc.SharedSystemMemory) / (1024.0 * 1024.0 * 1024.0));

	}
}

inline ZgResult createHighPerformanceDevice(
	ComPtr<IDXGIFactory6>& dxgiFactory,
	ComPtr<IDXGIAdapter4>& adapterOut,
	ComPtr<ID3D12Device3>& deviceOut) noexcept
{
	// Create high-performance adapter
	bool adapterSuccess = D3D12_SUCC(
		dxgiFactory->EnumAdapterByGpuPreference(
		0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapterOut)));
	if (!adapterSuccess) return ZG_ERROR_NO_SUITABLE_DEVICE;

	// Log adapter description
	DXGI_ADAPTER_DESC1 desc;
	CHECK_D3D12 adapterOut->GetDesc1(&desc);
	ZG_INFO("Using adapter: %S", desc.Description);

	// Create device
	bool deviceSuccess = D3D12_SUCC(D3D12CreateDevice(
		adapterOut.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&deviceOut)));
	if (!deviceSuccess) return ZG_ERROR_NO_SUITABLE_DEVICE;

	return ZG_SUCCESS;
}

inline ZgResult createSoftwareDevice(
	ComPtr<IDXGIFactory6>& dxgiFactory,
	ComPtr<IDXGIAdapter4>& adapterOut,
	ComPtr<ID3D12Device3>& deviceOut) noexcept
{
	// Find software adapter
	bool foundPixAdapter = false;
	ComPtr<IDXGIAdapter1> adapter;
	for (uint32_t i = 0; true; i++) {

		// Get adapter, exit loop if no more adapters
		if (dxgiFactory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) {
			break;
		}

		// Get adapter description
		DXGI_ADAPTER_DESC1 desc;
		CHECK_D3D12 adapter->GetDesc1(&desc);

		// Skip adapter if it is not software renderer
		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) continue;
		foundPixAdapter = true;
		break;
	}
	if (!foundPixAdapter) return ZG_ERROR_NO_SUITABLE_DEVICE;

	// Convert adapter to DXGIAdapter4 and return it
	{
		bool convSuccess = D3D12_SUCC(adapter.As(&adapterOut));
		if (!convSuccess) return ZG_ERROR_NO_SUITABLE_DEVICE;
	}

	// Log adapter description
	DXGI_ADAPTER_DESC1 desc;
	CHECK_D3D12 adapterOut->GetDesc1(&desc);
	ZG_INFO("Using adapter: %S", desc.Description);

	// Create device
	bool deviceSuccess = D3D12_SUCC(D3D12CreateDevice(
		adapterOut.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&deviceOut)));
	if (!deviceSuccess) return ZG_ERROR_NO_SUITABLE_DEVICE;

	return ZG_SUCCESS;
}
