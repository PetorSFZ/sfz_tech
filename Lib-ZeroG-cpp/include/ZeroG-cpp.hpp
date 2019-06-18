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

#include "ZeroG.h"

namespace zg {

// Context
// ------------------------------------------------------------------------------------------------

class Context final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Context() noexcept = default;
	Context(const Context&) = delete;
	Context& operator= (const Context&) = delete;
	Context(Context&& other) noexcept { this->swap(other); }
	Context& operator= (Context&& other) noexcept { this->swap(other); return *this; }
	~Context() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	// Creates and initializes a context, see zgCreateContext().
	ZgErrorCode init(const ZgContextInitSettings& settings);

	void swap(Context& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------


	// Private members
	// --------------------------------------------------------------------------------------------
private:

	bool mInitialized = false;
};

} // namespace zg
