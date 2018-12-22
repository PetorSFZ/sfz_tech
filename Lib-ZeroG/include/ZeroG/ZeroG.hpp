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

#include <cstdint>

namespace zg {

// Version information
// ------------------------------------------------------------------------------------------------

// Returns the API version that was used when compiling. If this version needs to match the version
// in the linked DLL.
uint32_t apiVersionCompiled() noexcept;

// Returns the API version of the linked DLL. This needs to match the version that was used when
// compiling.
uint32_t apiVersionLinked() noexcept;

} // namespace zg
