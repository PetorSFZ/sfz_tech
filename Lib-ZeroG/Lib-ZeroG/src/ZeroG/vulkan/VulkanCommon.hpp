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

#include <vulkan/vulkan.h>

namespace zg {

// Check Vulkan macro
// ------------------------------------------------------------------------------------------------

#define CHECK_VK (zg::CheckVkImpl(__FILE__, __LINE__)) %

struct CheckVkImpl final {
	const char* file = nullptr;
	int line = 0;

	CheckVkImpl() = delete;
	CheckVkImpl(const char* file, int line) noexcept
		: file(file), line(line) {}

	bool operator% (VkResult result) const noexcept;
};


} // namespace zg
