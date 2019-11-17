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

#include "ZeroG/d3d12/D3D12Common.hpp"

#include "ZeroG/Context.hpp"
#include "ZeroG/util/Assert.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

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

// TextureFormats conversion
// ------------------------------------------------------------------------------------------------

DXGI_FORMAT zgToDxgiTextureFormat(ZgTextureFormat format) noexcept
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

	ZG_ASSERT(false);
	return DXGI_FORMAT_UNKNOWN;
}

// Helper functions
// ------------------------------------------------------------------------------------------------

bool utf8ToWide(WCHAR* wideOut, uint32_t numWideChars, const char* utf8In) noexcept
{
	// TODO: Unsure if 6th paramter should be num chars or num bytes =/
	int res = MultiByteToWideChar(CP_UTF8, 0, utf8In, -1, wideOut, numWideChars);
	return res != 0;
}

HRESULT CheckD3D12Impl::operator% (HRESULT result) noexcept
{
	if (SUCCEEDED(result)) return result;
	logWrapper(file, line, ZG_LOG_LEVEL_ERROR,
		"D3D12 error: %s\n", resultToString(result));
	return result;
}

bool CheckD3D12Impl::succeeded(HRESULT result) noexcept
{
	if (SUCCEEDED(result)) return true;
	logWrapper(file, line, ZG_LOG_LEVEL_ERROR,
		"D3D12 error: %s\n", resultToString(result));
	return false;
}

} // namespace zg
