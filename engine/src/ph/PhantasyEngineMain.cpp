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

#include <sfz/Context.hpp>
#include <sfz/Logging.hpp>
#include <sfz/memory/StandardAllocator.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/strings/StringID.hpp>
#include <sfz/util/IO.hpp>

#include "ph/Context.hpp"
#include "ph/config/GlobalConfig.hpp"
#include "ph/game_loop/GameLoop.hpp"
#include "ph/rendering/Image.hpp"
#include "ph/rendering/ImguiSupport.hpp"
#include "ph/sdl/SDLAllocator.hpp"
#include "ph/util/TerminalLogger.hpp"

namespace ph {

// Request dedicated graphics card over integrated on Windows
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32
extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 1; }
extern "C" { _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1; }
#endif

// Statics
// ------------------------------------------------------------------------------------------------

static void setupContexts() noexcept
{
	// Get sfz standard allocator
	sfz::Allocator* allocator = sfz::getStandardAllocator();

	// Create terminal logger
	ph::TerminalLogger& logger = *ph::getStaticTerminalLoggerForBoot();
	logger.init(256, allocator);

	// Setup context
	phContext* context = ph::getStaticContextBoot();
	context->sfzContext.defaultAllocator = allocator;
	context->sfzContext.logger = &logger;
	context->logger = &logger;
	context->config = ph::getStaticGlobalConfigBoot();
	context->resourceStrings = sfz::sfzNew<StringCollection>(allocator, 4096, allocator);

	// Set Phantasy Engine context
	ph::setContext(context);

	// Set sfzCore context
	sfz::setContext(&context->sfzContext);
}

static const char* basePath() noexcept
{
	static const char* path = []() {
		const char* tmp = SDL_GetBasePath();
		if (tmp == nullptr) {
			SFZ_ERROR_AND_EXIT("PhantasyEngine", "SDL_GetBasePath() failed: %s", SDL_GetError());
		}
		size_t len = std::strlen(tmp);
		char* res = static_cast<char*>(
			sfz::getDefaultAllocator()->allocate(len + 1, 32, "sfz::basePath()"));
		std::strcpy(res, tmp);
		SDL_free((void*)tmp);
		return res;
	}();
	return path;
}

static void ensureAppUserDataDirExists(const char* appName) noexcept
{
	// Create "My Games" directory
	sfz::createDirectory(sfz::gameBaseFolderPath());

	// Create app directory in "My Games"
	sfz::StackString320 tmp;
	tmp.printf("%s%s/", sfz::gameBaseFolderPath(), appName);
	sfz::createDirectory(tmp.str);
}

static void logSDL2Version() noexcept
{
	SDL_version version;

	// Log SDL2 compiled version
	SDL_VERSION(&version);
	SFZ_INFO("SDL2", "Compiled version: %u.%u.%u",
		uint32_t(version.major), uint32_t(version.minor), uint32_t(version.patch));

	// Log SDL2 compiled version
	SDL_GetVersion(&version);
	SFZ_INFO("SDL2", "Linked version: %u.%u.%u",
		uint32_t(version.major), uint32_t(version.minor), uint32_t(version.patch));
}

// Implementation function
// ------------------------------------------------------------------------------------------------

int mainImpl(int, char*[], InitOptions&& options)
{
	// Setup sfzCore and PhantasyEngine contexts
	setupContexts();

	// Set SDL allocators
	if (!sdl::setSDLAllocator(sfz::getDefaultAllocator())) return EXIT_FAILURE;

	// Set load image allocator
	ph::setLoadImageAllocator(sfz::getDefaultAllocator());

	// Windwows specific hacks
#ifdef _WIN32
	// Enable hi-dpi awareness
	SetProcessDPIAware();

	// Set current working directory to SDL_GetBasePath()
	_chdir(basePath());
#endif

	// Load global settings
	GlobalConfig& cfg = ph::getGlobalConfig();
	{
		// Init config with ini location
		if (options.iniLocation == IniLocation::NEXT_TO_EXECUTABLE) {
			sfz::StackString192 iniFileName;
			iniFileName.printf("%s.ini", options.appName);
			cfg.init(basePath(), iniFileName.str, sfz::getDefaultAllocator());
			SFZ_INFO("PhantasyEngine", "Ini location set to: %s%s", basePath(), iniFileName.str);
		}
		else if (options.iniLocation == IniLocation::MY_GAMES_DIR) {

			// Create user data directory
			ensureAppUserDataDirExists(options.appName);

			// Initialize ini
			sfz::StackString192 iniFileName;
			iniFileName.printf("%s/%s.ini", options.appName, options.appName);
			cfg.init(sfz::gameBaseFolderPath(), iniFileName.str, sfz::getDefaultAllocator());
			SFZ_INFO("PhantasyEngine", "Ini location set to: %s%s",
				sfz::gameBaseFolderPath(), iniFileName.str);
		}
		else {
			sfz_assert_release(false);
		}

		// Load ini file
		cfg.load();
	}

	// Init SDL2
	uint32_t sdlInitFlags =
#ifdef __EMSCRIPTEN__
	SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO;
#else
	SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER;
#endif
	if (SDL_Init(sdlInitFlags) < 0) {
		SFZ_ERROR("PhantasyEngine", "SDL_Init() failed: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Log SDL2 version
	logSDL2Version();

	// Load Renderer library (DLL on Windows)
	UniquePtr<Renderer> renderer = sfz::makeUniqueDefault<Renderer>();
	renderer->load(options.rendererName, sfz::getDefaultAllocator());

	// Window settings
	Setting* width = cfg.sanitizeInt("Window", "width", false, 1280, 128, 3840, 32);
	Setting* height = cfg.sanitizeInt("Window", "height", false, 800, 128, 2160, 32);
	Setting* fullscreen = cfg.sanitizeBool("Window", "fullscreen", false, false);
	Setting* maximized = cfg.sanitizeBool("Window", "maximized", false, false);
	if (fullscreen->boolValue() && maximized->boolValue()) {
		maximized->setBool(false);
	}

	// Create SDL_Window
	uint32_t windowFlags =
		renderer->requiredSDL2WindowFlags() |
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_ALLOW_HIGHDPI |
		(fullscreen->boolValue() ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
		(maximized->boolValue() ? SDL_WINDOW_MAXIMIZED : 0);
	SDL_Window* window =
		SDL_CreateWindow(options.appName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width->intValue(), height->intValue(), windowFlags);
	if (window == NULL) {
		SFZ_ERROR("PhantasyEngine", "SDL_CreateWindow() failed: %s", SDL_GetError());
		renderer.destroy();
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initializing Imgui
	SFZ_INFO("PhantasyEngine", "Initializing Imgui");
	phImageView imguiFontTexView = initializeImgui(sfz::getDefaultAllocator());

	// Initializing renderer
	SFZ_INFO("PhantasyEngine", "Initializing renderer");
	renderer->initRenderer(window);
	renderer->initImgui(imguiFontTexView);

	// Start game loop
	SFZ_INFO("PhantasyEngine", "Starting game loop");
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
			SFZ_INFO("PhantasyEngine", "Saving global config to file");
			GlobalConfig& cfg = ph::getGlobalConfig();
			if (!cfg.save()) {
				SFZ_WARNING("PhantasyEngine", "Failed to write ini file");
			}
			cfg.destroy();

			// Deinitializing Imgui
			SFZ_INFO("PhantasyEngine", "Deinitializing Imgui");
			deinitializeImgui();

			// Cleanup SDL2
			SFZ_INFO("PhantasyEngine", "Cleaning up SDL2");
			SDL_Quit();
		}
	);

	// DEAD ZONE
	// Don't place any code after the game loop has been initialized, it will never be called on
	// some platforms.

	return EXIT_SUCCESS;
}

} // namespace ph
