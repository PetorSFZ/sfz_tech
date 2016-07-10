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

#include "sfz/screens/GameLoop.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "sfz/math/Vector.hpp"
#include "sfz/sdl/GameController.hpp"

namespace sfz {

// Typedefs
// ------------------------------------------------------------------------------------------------

using std::int32_t;
using time_point = std::chrono::high_resolution_clock::time_point;

// Static helper functions
// ------------------------------------------------------------------------------------------------

static float calculateDelta(time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();

	using FloatSecond = std::chrono::duration<float>;
	float delta = std::chrono::duration_cast<FloatSecond>(currentTime - previousTime).count();

	previousTime = currentTime;
	return delta;
}

static void initControllers(HashMap<int32_t, sdl::GameController>& controllers) noexcept
{
	controllers.clear();

	int numJoysticks = SDL_NumJoysticks();
	for (int i = 0; i < numJoysticks; ++i) {
		if (!SDL_IsGameController(i)) continue;
		
		sdl::GameController c(i);
		if (c.id() == -1) continue;
		if (controllers.get(c.id()) != nullptr) continue;

		controllers[c.id()] = std::move(c);
	}
}

// GameLoop function
// ------------------------------------------------------------------------------------------------

void runGameLoop(sdl::Window& window, SharedPtr<BaseScreen> currentScreen)
{
	UpdateState state(window);

	// Initialize controllers
	initControllers(state.controllers);

	// Initialize time delta
	time_point previousTime;
	state.delta = calculateDelta(previousTime);

	// Initialize SDL events
	SDL_GameControllerEventState(SDL_ENABLE);
	SDL_Event event;

	while (true) {
		// Calculate delta
		state.delta = std::min(calculateDelta(previousTime), 0.2f);

		// Process events
		state.events.clear();
		state.controllerEvents.clear();
		state.mouseEvents.clear();
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {

			// Quitting and resizing window
			case SDL_QUIT:
				currentScreen->onQuit();
				return;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					currentScreen->onResize(window.dimensionsFloat(), window.drawableDimensionsFloat());
					break;
				default:
					state.events.add(event);
					break;
				}
				break;

			// SDL_GameController events
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
			case SDL_CONTROLLERDEVICEREMAPPED:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERAXISMOTION:
				state.controllerEvents.add(event);
				break;

			// Mouse events
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEWHEEL:
				state.mouseEvents.add(event);
				break;

			default:
				state.events.add(event);
				break;
			}
		}

		// Updates controllers
		state.controllersLastFrameState.clear();
		for (auto pair : state.controllers) {
			state.controllersLastFrameState[pair.key] = pair.value.state();
		}
		update(state.controllers, state.controllerEvents);

		// Updates mouse
		state.rawMouse.update(window, state.mouseEvents);

		// Update current screen
		UpdateOp op = currentScreen->update(state);

		// Perform eventual operations requested by screen update
		switch (op.type) {
		case UpdateOpType::SWITCH_SCREEN:
			currentScreen = op.newScreen;
			continue;
		case UpdateOpType::QUIT:
			currentScreen->onQuit();
			return;
		case UpdateOpType::REINIT_CONTROLLERS:
			initControllers(state.controllers);
			continue;

		case UpdateOpType::NO_OP:
		default:
			// Do nothing
			break;
		}

		// Render current screen
		currentScreen->render(state);
	}
}

} // namespace sfz
