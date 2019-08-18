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

// D3D12 headers
// ------------------------------------------------------------------------------------------------

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
#include <d3d12shader.h>

// DXGI Debug
//#include <dxgidebug.h>
//#pragma comment (lib, "dxguid.lib")

// DXC compiler
#include <dxcapi.h>
#pragma comment (lib, "dxcompiler.lib")

// D3DX12 library
#include "d3dx12.h"

// D3DX12Residency library
#ifdef NDEBUG
// Use single threaded D3DX12Residency in debug builds, workaround for:
// "The Visual Studio Graphics Debugging (VSGD) tools crash when capturing an app that uses this
// library"
#define RESIDENCY_SINGLE_THREADED 1
#else
#define RESIDENCY_SINGLE_THREADED 0
#endif
#pragma warning(push, 0)
#include "d3dx12Residency.h"
#pragma warning(pop)

#include "ZeroG/util/Logging.hpp"

namespace zg { using Microsoft::WRL::ComPtr; }

namespace zg {

// PendingState struct
// ------------------------------------------------------------------------------------------------

class D3D12Buffer; // Forward declare
class D3D12Texture2D; // Forward declare

// Struct representing the pending state for a buffer in a command list
struct PendingBufferState final {

	// The associated D3D12Buffer
	D3D12Buffer* buffer = nullptr;

	// The state the resource need to be in before the command list is executed
	D3D12_RESOURCE_STATES neededInitialState = D3D12_RESOURCE_STATE_COMMON;

	// The state the resource is in after the command list is executed
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

struct PendingTextureState final {

	// The associated D3D12Texture
	D3D12Texture2D* texture = nullptr;

	// The mip level of the associated texture
	uint32_t mipLevel = ~0u;

	// The state the resource need to be in before the command list is executed
	D3D12_RESOURCE_STATES neededInitialState = D3D12_RESOURCE_STATE_COMMON;

	// The state the resource is in after the command list is executed
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

// Helper functions
// ------------------------------------------------------------------------------------------------

bool utf8ToWide(WCHAR* wideOut, uint32_t numWideChars, const char* utf8In) noexcept;

// Checks result (HRESULT) from D3D call and log if not success, returns result unmodified
#define CHECK_D3D12 (CheckD3D12Impl(__FILE__, __LINE__)) %

// Checks result (HRESULT) from D3D call and log if not success, true on success
#define D3D12_SUCC(result) \
	(CheckD3D12Impl(__FILE__, __LINE__).succeeded((result)))

// Checks result (HRESULT) from D3D call and log if not success, true on failure
#define D3D12_FAIL(result) \
	(!CheckD3D12Impl(__FILE__, __LINE__).succeeded((result)))

struct CheckD3D12Impl final {
	const char* file = nullptr;
	int line = 0;

	CheckD3D12Impl() = delete;
	CheckD3D12Impl(const char* file, int line) noexcept
		: file(file), line(line) {}

	HRESULT operator% (HRESULT result) noexcept;
	bool succeeded(HRESULT result) noexcept;
};

} // namespace zg
