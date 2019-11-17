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

#include <sfz/containers/DynArray.hpp>
#include <ph/rendering/ImguiRenderingData.hpp>

#include "ph/renderer/Renderer.hpp"
#include "ph/sdl/Mouse.hpp"
#include "ph/sdl/GameController.hpp"

namespace ph {

using sfz::Allocator;
using sfz::DynArray;

// Initializes imgui, returns font image view to be sent to renderers initImgui() function.
phImageView initializeImgui(Allocator* allocator) noexcept;

void deinitializeImgui() noexcept;

void updateImgui(
	Renderer& renderer,
	const sdl::Mouse* rawMouse,
	const DynArray<SDL_Event>* keyboardEvents,
	const sdl::GameControllerState* controller) noexcept;

void convertImguiDrawData(
	DynArray<phImguiVertex>& vertices,
	DynArray<uint32_t>& indices,
	DynArray<phImguiCommand>& commands) noexcept;

// The fonts initialized with Imgui
ImFont* imguiFontDefault() noexcept;
ImFont* imguiFontMonospace() noexcept;

} // namespace ph
