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

#include <ph/PhantasyEngineMain.hpp>

#include <cstdlib>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <direct.h>
#endif

#include <ph/game_loop/GameLoop.hpp>
#include <ph/utils/Logging.hpp>

namespace ph {

int mainImpl(int, char*[], UniquePtr<GameLoopUpdateable>(*createInitialUpdateable)(void))
{
	// Windwows specific hacks
#ifdef _WIN32
	// Enable hi-dpi awareness
	SetProcessDPIAware();

	// Set current working directory to SDL_GetBasePath()
	char* basePath = SDL_GetBasePath();
	_chdir(basePath);
	SDL_free(basePath);
#endif

	// Init SDL2
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		PH_LOG(LogLevel::ERROR_LVL, "PhantasyEngine", "SDL_Init() failed: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// TODO: Should init RenderingSystem here

	// Start game loop
	PH_LOG(LogLevel::INFO, "PhantasyEngine", "Starting game loop");
	runGameLoop(
	// Create initial GameLoopUpdateable with user-specified function
	createInitialUpdateable(),
	
	// Cleanup callback
	[]() {
		PH_LOG(LogLevel::INFO, "PhantasyEngine", "Exited game loop");

		// TODO: Should deinit RenderingSystem here

		// Cleanup SDL2
		PH_LOG(LogLevel::INFO, "PhantasyEngine", "Cleaning up SDL2");
		SDL_Quit();
	});

	// DEAD ZONE
	// Don't place any code after the game loop has been initialized, it will never be called on
	// some platforms.

	return EXIT_SUCCESS;
}

} // namespace ph
