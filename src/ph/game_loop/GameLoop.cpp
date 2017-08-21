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

#include <exception> // std::terminate()

#include <SDL.h>

#ifdef SFZ_EMSCRIPTEN
#include <emscripten.h>
#endif

#include "ph/utils/Logging.hpp"

namespace ph {

struct GameLoopState final {
	void(*cleanupCallback)(void) = nullptr;
	bool quit = false;
};

void gameLoopIteration(void* gameLoopStatePtr) noexcept
{
	GameLoopState& gameLoopState = *static_cast<GameLoopState*>(gameLoopStatePtr);

	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE) {
			PH_LOG(LogLevel::INFO, "PhantasyEngine", "Exiting game loop");
#ifdef SFZ_EMSCRIPTEN
			gameLoopState.cleanupCallback();
			emscripten_cancel_main_loop();
#else
			gameLoopState.quit = true;
#endif
		}
		if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_UP) {
			PH_LOG(LogLevel::INFO, "PhantasyEngine", "up pressed");
		}
	}
}

// GameLoop entry function
// ------------------------------------------------------------------------------------------------

void runGameLoop(UniquePtr<GameLoopUpdateable> updateable, void(*cleanupCallback)(void)) noexcept
{
	// // TODO: Should be done in RenderingSystem
	const char* title = "Temp Window Title";
	const int width = 512;
	const int height = 512;
	SDL_Window* window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (window == NULL) {
		PH_LOG(LogLevel::ERROR_LVL, "PhantasyEngine", "SDL_CreateWindow() failed: %s", SDL_GetError());
		std::terminate();
	}


	GameLoopState gameLoopState;
	gameLoopState.cleanupCallback = cleanupCallback;
#ifdef SFZ_EMSCRIPTEN
	// https://kripken.github.io/emscripten-site/docs/api_reference/emscripten.h.html#browser-execution-environment
	// Setting 0 or a negative value as the fps will instead use the browser’s requestAnimationFrame mechanism to
	// call the main loop function. This is HIGHLY recommended if you are doing rendering, as the browser’s
	// requestAnimationFrame will make sure you render at a proper smooth rate that lines up properly with the
	// browser and monitor.
	emscripten_set_main_loop_arg(gameLoopIteration, &gameLoopState, 0, true);
#else
	while (!gameLoopState.quit) {
		gameLoopIteration(&gameLoopState);
	}
	gameLoopState.cleanupCallback();
#endif

	// TODO: Should be done in RenderingSystem
	SDL_DestroyWindow(window);
}

} // namespace ph
