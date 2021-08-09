// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

namespace sfz {

// Console
// ------------------------------------------------------------------------------------------------

struct ConsoleState; // Pimpl

class Console final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Console() noexcept = default;
	Console(const Console&) = delete;
	Console& operator= (const Console&) = delete;
	Console(Console&& other) noexcept { this->swap(other); }
	Console& operator= (Console&& other) noexcept { this->swap(other); return *this; }
	~Console() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(SfzAllocator* allocator, u32 numWindowsToDock, const char* const* windowNames) noexcept;
	void swap(Console& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	// If the console is active the game should be paused
	void toggleActive() noexcept;
	bool active() noexcept;

	void render(i32x2 windowRes) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	ConsoleState* mState = nullptr;
};

} // namespace sfz
