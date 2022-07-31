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

#include <imgui.h>

#include <SDL.h>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

#include "sfz/input/RawInputState.hpp"
#include "sfz/renderer/Renderer.hpp"

struct SfzConfig;

namespace sfz {

// Initializes imgui, returns font image view to be sent to renderers initImgui() function.
SfzImageView initializeImgui(SfzAllocator* allocator) noexcept;

void deinitializeImgui() noexcept;

void updateImgui(
	i32x2 windowResolution,
	const RawInputState& rawInputState,
	const SDL_Event* keyboardEvents,
	u32 numKeyboardEvents,
	SfzConfig* cfg) noexcept;

// The fonts initialized with Imgui
ImFont* imguiFontDefault() noexcept;
ImFont* imguiFontMonospace() noexcept;

} // namespace sfz
