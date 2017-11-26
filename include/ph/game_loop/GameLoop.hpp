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

#include <sfz/memory/SmartPointers.hpp>

#include "ph/game_loop/GameLoopUpdateable.hpp"
#include "ph/rendering/Renderer.hpp"

namespace ph {

using sfz::UniquePtr;

// GameLoop entry function
// ------------------------------------------------------------------------------------------------

/// Entry point for the main game loop of PhantasyEngine.
/// NOTHING should be done in main() after this function has been called. This is due to
/// limitations with Emscripten. Instead, if any cleanup should be performed after the main loop
/// has exited it should be done in the callback function.
/// \param updateable the initial GameLoopUpdateable to receive input
/// \param renderer the renderer to be used
/// \param window the window that is being rendered to be the renderer
/// \param cleanupCallback the callback function called before exiting the game loop
void runGameLoop(
	UniquePtr<GameLoopUpdateable> updateable,
	UniquePtr<Renderer> renderer,
	SDL_Window* window,
	void(*cleanupCallback)(void)) noexcept;

} // namespace ph
