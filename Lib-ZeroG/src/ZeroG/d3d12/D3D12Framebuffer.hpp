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

#include "ZeroG/d3d12/D3D12Common.hpp"
#include "ZeroG/BackendInterface.hpp"
#include "ZeroG/ZeroG-CApi.h"

namespace zg {

// D3D12Framebuffer
// ------------------------------------------------------------------------------------------------

class D3D12Framebuffer final : public IFramebuffer {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	D3D12Framebuffer() noexcept = default;
	D3D12Framebuffer(const D3D12Framebuffer&) = delete;
	D3D12Framebuffer& operator= (const D3D12Framebuffer&) = delete;
	D3D12Framebuffer(D3D12Framebuffer&&) = delete;
	D3D12Framebuffer& operator= (D3D12Framebuffer&&) = delete;
	~D3D12Framebuffer() noexcept;

	// Members
	// --------------------------------------------------------------------------------------------

	uint32_t width = 0;
	uint32_t height = 0;
	ComPtr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {};
};

} // namespace zg
