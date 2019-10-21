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

#include "ZeroG/d3d12/D3D12Textures.hpp"

namespace zg {


// D3D12Texture2D: Constructors & destructors
// ------------------------------------------------------------------------------------------------

D3D12Texture2D::~D3D12Texture2D() noexcept
{

}

// D3D12Texture2D: Methods
// ------------------------------------------------------------------------------------------------

ZgResult D3D12Texture2D::setDebugName(const char* name) noexcept
{
	// Small hack to fix D3D12 bug with debug name shorter than 4 chars
	char tmpBuffer[256] = {};
	snprintf(tmpBuffer, 256, "zg__%s", name); 

	// Convert to wide
	WCHAR tmpBufferWide[256] = {};
	utf8ToWide(tmpBufferWide, 256, tmpBuffer);

	// Set debug name
	CHECK_D3D12 this->resource->SetName(tmpBufferWide);

	return ZG_SUCCESS;
}

} // namespace zg
