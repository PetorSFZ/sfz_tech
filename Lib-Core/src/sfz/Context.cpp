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

#include "sfz/Context.hpp"

#include <skipifzero.hpp>

#include "sfz/memory/StandardAllocator.hpp"
#include "sfz/util/StandardLogger.hpp"

namespace sfz {

// Context getters/setters
// ------------------------------------------------------------------------------------------------

static Context* globalContextPtr = nullptr;

Context* getContext() noexcept
{
	sfz_assert(globalContextPtr != nullptr);
	return globalContextPtr;
}

bool setContext(Context* context) noexcept
{
	sfz_assert_hard(context != nullptr);
	if (globalContextPtr != nullptr) return false;
	globalContextPtr = context;
	return true;
}

// Standard context
// ------------------------------------------------------------------------------------------------

Context* getStandardContext() noexcept
{
	static Context context = []() {
		Context tmp;
		tmp.defaultAllocator = getStandardAllocator();
		tmp.logger = getStandardLogger();
		return tmp;
	}();
	return &context;
}

} // namespace sfz
