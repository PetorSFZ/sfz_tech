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

#include <SDL.h>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include <sfz/geometry/AABB2D.hpp>

#include "ph/sdl/ButtonState.hpp"

namespace ph {

namespace sdl {

using sfz::Array;
using sfz::AABB2D;
using sfz::vec2;

// Mouse structs
// ------------------------------------------------------------------------------------------------

struct Mouse final {

	// Public members
	// --------------------------------------------------------------------------------------------

	ButtonState leftButton = ButtonState::NOT_PRESSED;
	ButtonState rightButton = ButtonState::NOT_PRESSED;
	ButtonState middleButton = ButtonState::NOT_PRESSED;

	/// A raw position should be in the range [0, 1] where (0,0) is the bottom left corner.
	/// In a scaled mouse from "scaleMouse()" the position should be in the specified coordinate
	/// system.
	vec2 position;
	vec2 motion; // Positive-x: right, Positive-y: up
	vec2 wheel;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Mouse() noexcept = default;
	Mouse(const Mouse&) noexcept = default;
	Mouse& operator= (const Mouse&) noexcept = default;

	// Public methods
	// --------------------------------------------------------------------------------------------

	void update(int windowWidth, int windowHeight, const Array<SDL_Event>& events) noexcept;
	Mouse scaleMouse(vec2 camPos, vec2 camDim) const noexcept;
	Mouse scaleMouse(const AABB2D& camera) const noexcept;
};

} // namespace sdl
} // namespace ph
