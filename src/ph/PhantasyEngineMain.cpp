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
#include "ph/utils/Logging.hpp"

namespace ph {

// Statics
// ------------------------------------------------------------------------------------------------

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
	// Windwows specific hacks
#ifdef _WIN32
	// Enable hi-dpi awareness
	SetProcessDPIAware();

	// Set current working directory to SDL_GetBasePath()
	_chdir(sfz::basePath());
#endif

	// Load global settings
	GlobalConfig& cfg = GlobalConfig::instance();
	{
		// Init config with ini location
		if (options.iniLocation == IniLocation::NEXT_TO_EXECUTABLE) {
			StackString192 iniFileName;
			iniFileName.printf("%s.ini", options.appName);
			cfg.init(sfz::basePath(), iniFileName.str);
			PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Ini location set to: %s%s", sfz::basePath(), iniFileName.str);
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
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		PH_LOG(LOG_LEVEL_ERROR, "PhantasyEngine", "SDL_Init() failed: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// TODO: Should init RenderingSystem here

	// Start game loop
	PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Starting game loop");
	runGameLoop(
	
	// Create initial GameLoopUpdateable
	options.createInitialUpdateable(),
	
	// Cleanup callback
	[]() {
		PH_LOG(LOG_LEVEL_INFO, "PhantasyEngine", "Exited game loop");

		// TODO: Should deinit RenderingSystem here

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
	});

	// DEAD ZONE
	// Don't place any code after the game loop has been initialized, it will never be called on
	// some platforms.

	return EXIT_SUCCESS;
}

} // namespace ph
