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

#include "ph/game_loop/GameLoop.hpp"

#include <chrono>
#include <exception> // std::terminate()

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <skipifzero.hpp>

#include <sfz/Logging.hpp>
#include <sfz/containers/DynArray.hpp>

#include "ph/Context.hpp"
#include "ph/config/GlobalConfig.hpp"

namespace ph {

// Typedefs
// ------------------------------------------------------------------------------------------------

using sfz::DynArray;
using time_point = std::chrono::high_resolution_clock::time_point;

// GameLoopState
// ------------------------------------------------------------------------------------------------

struct GameLoopState final {
	UniquePtr<GameLoopUpdateable> updateable;
	UniquePtr<Renderer> renderer;
	SDL_Window* window = nullptr;
	void(*cleanupCallback)(void) = nullptr;
	bool quit = false;

	time_point previousItrTime;

	// Input structs for updateable
	UserInput userInput;
	UpdateInfo updateInfo;

	// Window settings
	Setting* windowWidth = nullptr;
	Setting* windowHeight = nullptr;
	Setting* fullscreen = nullptr;
	bool lastFullscreenValue;
	Setting* maximized = nullptr;
	bool lastMaximizedValue;
};

// Static helper functions
// ------------------------------------------------------------------------------------------------

static void quit(GameLoopState& gameLoopState) noexcept
{
	gameLoopState.quit = true; // Exit infinite while loop (on some platforms)

	SFZ_INFO("PhantasyEngine", "Destroying current updateable");
	gameLoopState.updateable->onQuit();
	gameLoopState.updateable.destroy(); // Destroy the current updateable

	SFZ_INFO("PhantasyEngine", "Destroying renderer");
	gameLoopState.renderer.destroy(); // Destroy the current renderer

	SFZ_INFO("PhantasyEngine", "Closing SDL controllers");
	gameLoopState.userInput.controllers.clear();

	SFZ_INFO("PhantasyEngine", "Destroying SDL Window");
	SDL_DestroyWindow(gameLoopState.window);

	SFZ_INFO("PhantasyEngine", "Calling cleanup callback");
	gameLoopState.cleanupCallback(); // Call the cleanup callback

	// Exit program on Emscripten
#ifdef __EMSCRIPTEN__
	emscripten_cancel_main_loop();
#endif
}

static float calculateDelta(time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();

	using FloatSecond = std::chrono::duration<float>;
	float delta = std::chrono::duration_cast<FloatSecond>(currentTime - previousTime).count();

	previousTime = currentTime;
	return delta;
}

static void initControllers(HashMap<int32_t, GameController>& controllers) noexcept
{
	controllers.clear();

	int numJoysticks = SDL_NumJoysticks();
	for (int i = 0; i < numJoysticks; ++i) {
		if (!SDL_IsGameController(i)) continue;

		GameController c(i);
		if (c.id() == -1) continue;
		if (controllers.get(c.id()) != nullptr) continue;

		controllers[c.id()] = std::move(c);
	}
}

static bool handleUpdateOp(GameLoopState& state, UpdateOp& op) noexcept
{
	switch (op.type) {
	case UpdateOpType::QUIT:
		quit(state);
		return true;
	case UpdateOpType::CHANGE_UPDATEABLE:
		state.updateable = std::move(op.newUpdateable);
		state.updateable->initialize(*state.renderer);
		return true;
	case UpdateOpType::CHANGE_TICK_RATE:
		if (op.ticksPerSecond != 0) {
			state.updateInfo.tickRate = op.ticksPerSecond;
			state.updateInfo.tickTimeSeconds = 1.0f / float(op.ticksPerSecond);
		}
		return true;
	case UpdateOpType::REINIT_CONTROLLERS:
		initControllers(state.userInput.controllers);
		return true;
	case UpdateOpType::NO_OP:
	default:
		// Do nothing
		return false;
	}
}

// gameLoopIteration()
// ------------------------------------------------------------------------------------------------

/// Called for each iteration of the game loop
void gameLoopIteration(void* gameLoopStatePtr) noexcept
{
	GameLoopState& state = *static_cast<GameLoopState*>(gameLoopStatePtr);

	// Calculate delta since previous iteration
	state.updateInfo.iterationDeltaSeconds = calculateDelta(state.previousItrTime);
	//PH_LOG(LogLevel::INFO, "PhantasyEngine", "Frametime = %.3f ms",
	//	state.updateInfo.iterationDeltaSeconds * 100.0f);

	float totalAvailableTime = state.updateInfo.iterationDeltaSeconds + state.updateInfo.lagSeconds;

	// Calculate how many updates should be performed
	state.updateInfo.numUpdateTicks =
		uint32_t(std::floorf(totalAvailableTime / state.updateInfo.tickTimeSeconds));

	// Calculate lag
	float totalUpdateTime =
		float(state.updateInfo.numUpdateTicks) * state.updateInfo.tickTimeSeconds;
	state.updateInfo.lagSeconds = sfzMax(totalAvailableTime - totalUpdateTime, 0.0f);

	// Remove old events
	state.userInput.events.clear();
	state.userInput.controllerEvents.clear();
	state.userInput.mouseEvents.clear();

	// Check window status
	uint32_t currentWindowFlags = SDL_GetWindowFlags(state.window);
	bool isFullscreen = (currentWindowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
	bool isMaximized = (currentWindowFlags & SDL_WINDOW_MAXIMIZED) != 0;
	bool shouldBeFullscreen = state.fullscreen->boolValue();
	bool shouldBeMaximized = state.maximized->boolValue();

	// Process SDL events
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		switch (event.type) {

		// Quitting
		case SDL_QUIT:
			SFZ_INFO("PhantasyEngine", "SDL_QUIT event recevied, quitting.");
			quit(state);
			return;

		// SDL_GameController events
		case SDL_CONTROLLERDEVICEADDED:
		case SDL_CONTROLLERDEVICEREMOVED:
		case SDL_CONTROLLERDEVICEREMAPPED:
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		case SDL_CONTROLLERAXISMOTION:
			state.userInput.controllerEvents.add(event);
			break;

		// Mouse events
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			state.userInput.mouseEvents.add(event);
			break;

		// Window events
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_MAXIMIZED:
				state.maximized->setBool(true);
				isMaximized = true;
				shouldBeMaximized = true;
				break;
			case SDL_WINDOWEVENT_RESIZED:
				if (!isFullscreen && !isMaximized) {
					state.windowWidth->setInt(event.window.data1);
					state.windowHeight->setInt(event.window.data2);
				}
				break;
			case SDL_WINDOWEVENT_RESTORED:
				state.maximized->setBool(false);
				isMaximized = false;
				shouldBeMaximized = false;
				state.fullscreen->setBool(false);
				isFullscreen = false;
				shouldBeFullscreen = false;
				break;
			default:
				// Do nothing.
				break;
			}

			// Still add event to user input
			state.userInput.events.add(event);
			break;

		// All other events
		default:
			state.userInput.events.add(event);
			break;

		}
	}

	// Resize window
	if (!isFullscreen && !isMaximized) {
		int prevWidth, prevHeight;
		SDL_GetWindowSize(state.window, &prevWidth, &prevHeight);
		int newWidth = state.windowWidth->intValue();
		int newHeight = state.windowHeight->intValue();
		if (prevWidth != newWidth || prevHeight != newHeight) {
			SDL_SetWindowSize(state.window, newWidth, newHeight);
		}
	}

	// Set maximized
	if (isMaximized != shouldBeMaximized && !isFullscreen && !shouldBeFullscreen) {
		if (shouldBeMaximized) {
			SDL_MaximizeWindow(state.window);
		}
		else {
			SDL_RestoreWindow(state.window);
		}
	}

	// Set fullscreen
	if (isFullscreen != shouldBeFullscreen) {
		uint32_t fullscreenFlags = shouldBeFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
		if (SDL_SetWindowFullscreen(state.window, fullscreenFlags) < 0) {
			SFZ_ERROR("PhantasyEngine", "SDL_SetWindowFullscreen() failed: %s", SDL_GetError());
		}
		if (!shouldBeFullscreen) {
			SDL_SetWindowSize(state.window,
				state.windowWidth->intValue(), state.windowHeight->intValue());
		}
	}

	// Updates controllers
	state.userInput.controllersLastFrameState.clear();
	for (auto pair : state.userInput.controllers) {
		state.userInput.controllersLastFrameState[pair.key] = pair.value.state();
	}
	sdl::update(state.userInput.controllers, state.userInput.controllerEvents);

	// Updates mouse
	int windowWidth = -1;
	int windowHeight = -1;
	SDL_GetWindowSize(state.window, &windowWidth, &windowHeight);
	state.userInput.rawMouse.update(windowWidth, windowHeight, state.userInput.mouseEvents);

	// Process input
	UpdateOp op = state.updateable->processInput(
		state.userInput, state.updateInfo, *state.renderer);
	if (handleUpdateOp(state, op)) return;

	// Update
	for (uint32_t i = 0; i < state.updateInfo.numUpdateTicks; i++) {
		op = state.updateable->updateTick(state.updateInfo, *state.renderer);
		if (handleUpdateOp(state, op)) return;
	}

	// Render
	state.updateable->render(state.updateInfo, *state.renderer);
}

// GameLoop entry function
// ------------------------------------------------------------------------------------------------

void runGameLoop(
	UniquePtr<GameLoopUpdateable> updateable,
	UniquePtr<Renderer> renderer,
	SDL_Window* window,
	void(*cleanupCallback)(void)) noexcept
{
	// Initialize game loop state
	GameLoopState gameLoopState = {};
	gameLoopState.updateable = std::move(updateable);
	gameLoopState.renderer = std::move(renderer);
	gameLoopState.window = window;
	gameLoopState.cleanupCallback = cleanupCallback;
	gameLoopState.userInput.events.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));
	gameLoopState.userInput.controllerEvents.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));
	gameLoopState.userInput.mouseEvents.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));

	calculateDelta(gameLoopState.previousItrTime); // Sets previousItrTime to current time

	// Set initial tickrate to 100
	gameLoopState.updateInfo.tickRate = 100;
	gameLoopState.updateInfo.tickTimeSeconds = 0.01f;

	// Initialize controllers
	SDL_GameControllerEventState(SDL_ENABLE);
	initControllers(gameLoopState.userInput.controllers);

	// Initialize GameLoopUpdateable
	gameLoopState.updateable->initialize(*gameLoopState.renderer);

	// Get settings
	GlobalConfig& cfg = getGlobalConfig();
	gameLoopState.windowWidth = cfg.getSetting("Window", "width");
	sfz_assert(gameLoopState.windowWidth != nullptr);
	gameLoopState.windowHeight = cfg.getSetting("Window", "height");
	sfz_assert(gameLoopState.windowHeight != nullptr);
	gameLoopState.fullscreen = cfg.getSetting("Window", "fullscreen");
	sfz_assert(gameLoopState.fullscreen != nullptr);
	gameLoopState.maximized = cfg.getSetting("Window", "maximized");

	// Start the game loop
#ifdef __EMSCRIPTEN__
	// https://kripken.github.io/emscripten-site/docs/api_reference/emscripten.h.html#browser-execution-environment
	// Setting 0 or a negative value as the fps will instead use the browser’s requestAnimationFrame
	// mechanism to call the main loop function. This is HIGHLY recommended if you are doing
	// rendering, as the browser’s requestAnimationFrame will make sure you render at a proper
	// smooth rate that lines up properly with the browser and monitor.
	emscripten_set_main_loop_arg(gameLoopIteration, &gameLoopState, 0, true);
#else
	while (!gameLoopState.quit) {
		gameLoopIteration(&gameLoopState);
	}
#endif

	// DEAD ZONE
	// Don't place any code after the above #ifdef, it will never be called on some platforms.
}

} // namespace ph
