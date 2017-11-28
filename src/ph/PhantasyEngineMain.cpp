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

#include <sfz/strings/StackString.hpp>
#include <sfz/util/IO.hpp>

#include "ph/config/GlobalConfig.hpp"
#include "ph/game_loop/GameLoop.hpp"
#include "ph/sdl/SDLAllocator.hpp"
#include "ph/utils/Logging.hpp"

namespace ph {

// Statics
// ------------------------------------------------------------------------------------------------

const char* basePath() noexcept
{
	static const char* path = []() {
		const char* tmp = SDL_GetBasePath();
		if (tmp == nullptr) {
			sfz::error("SDL_GetBasePath() failed: %s", SDL_GetError());
		}
		size_t len = std::strlen(tmp);
		char* res = static_cast<char*>(sfz::getDefaultAllocator()->allocate(len + 1, 32, "sfz::basePath()"));
		std::strcpy(res, tmp);
		SDL_free((void*)tmp);
		return res;
	}();
	return path;
}

static void ensureAppUserDataDirExists(const char* appName)
{
	// Create "My Games" directory
	sfz::createDirectory(sfz::gameBaseFolderPath());

	// Create app directory in "My Games"
	sfz::StackString320 tmp;
	tmp.printf("%s%s/", sfz::gameBaseFolderPath(), appName);
	sfz::createDirectory(tmp.str);
}

// Implementation function
// ------------------------------------------------------------------------------------------------

int mainImpl(int, char*[], InitOptions&& options)
{
    // Set SDL allocators
    sdl::setSDLAllocator(sfz::getDefaultAllocator());
    
	// Windwows specific hacks
#ifdef _WIN32
	// Enable hi-dpi awareness
	SetProcessDPIAware();

	// Set current working directory to SDL_GetBasePath()
	_chdir(basePath());
#endif

	// Load global settings
	GlobalConfig& cfg = GlobalConfig::instance();
	{
		// Init config with ini location
		if (options.iniLocation == IniLocation::NEXT_TO_EXECUTABLE) {
			StackString192 iniFileName;
			iniFileName.printf("%s.ini", options.appName);
			cfg.init(basePath(), iniFileName.str);
			PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Ini location set to: %s%s", basePath(), iniFileName.str);
		}
		else if (options.iniLocation == IniLocation::MY_GAMES_DIR) {

			// Create user data directory
			ensureAppUserDataDirExists(options.appName);

			// Initialize ini
			StackString192 iniFileName;
			iniFileName.printf("%s/%s.ini", options.appName, options.appName);
			cfg.init(sfz::gameBaseFolderPath(), iniFileName.str);
			PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Ini location set to: %s%s", sfz::gameBaseFolderPath(), iniFileName.str);
		}
		else {
			sfz_assert_release(false);
		}

		// Load ini file
		cfg.load();
	}

	// Init SDL2
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "SDL_Init() failed: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Load Renderer library (DLL on Windows)
	UniquePtr<Renderer> renderer = sfz::makeUniqueDefault<Renderer>();
	renderer->load(options.rendererName, sfz::getDefaultAllocator());

	// Create SDL_Window
	uint32_t windowFlags = renderer->requiredSDL2WindowFlags() | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	const int width = 1000; // TODO: Arbitrary value not taken from config
	const int height = 500;
	SDL_Window* window = SDL_CreateWindow(options.appName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, windowFlags);
	if (window == NULL) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "SDL_CreateWindow() failed: %s", SDL_GetError());
		renderer.destroy();
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initializing renderer
	renderer->initRenderer(window);

	// Start game loop
	PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Starting game loop");
	runGameLoop(

		// Create initial GameLoopUpdateable
		options.createInitialUpdateable(),

		// Moving renderer
		std::move(renderer),

		// Providing SDL Window handle
		window,

		// Cleanup callback
		[]() {
			// Store global settings
			PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Saving global config to file");
			GlobalConfig& cfg = GlobalConfig::instance();
			if (!cfg.save()) {
				PH_LOG(LOG_LEVEL_WARNING, "PhantasyEngine", "Failed to write ini file");
			}
			cfg.destroy();

			// Cleanup SDL2
			PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Cleaning up SDL2");
			SDL_Quit();
		}
	);

	// DEAD ZONE
	// Don't place any code after the game loop has been initialized, it will never be called on
	// some platforms.

	return EXIT_SUCCESS;
}

} // namespace ph
