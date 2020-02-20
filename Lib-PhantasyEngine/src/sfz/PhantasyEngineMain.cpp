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

#include <sfz/PhantasyEngineMain.hpp>

#include <cstdlib>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <direct.h>
#endif

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <skipifzero.hpp>
#include <skipifzero_allocators.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_smart_pointers.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/rendering/Image.hpp"
#include "sfz/rendering/ImguiSupport.hpp"
#include "sfz/sdl/SDLAllocator.hpp"
#include "sfz/util/IO.hpp"
#include "sfz/util/TerminalLogger.hpp"

// Request dedicated graphics card over integrated on Windows
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32
extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 1; }
extern "C" { _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1; }
#endif

// Context
// ------------------------------------------------------------------------------------------------

// This is the global PhantasyEngine context, a pointer to it will be set using setContext().
static sfz::StandardAllocator standardAllocator;
static sfz::Context phantasyEngineContext;
static sfz::TerminalLogger terminalLogger;
static sfz::GlobalConfig globalConfig;
static sfz::StringCollection stringCollection;
static sfz::ProfilingStats profilingStats;

static void setupContext() noexcept
{
	sfz::Context* context = &phantasyEngineContext;

	// Set sfz standard allocator
	sfz::Allocator* allocator = &standardAllocator;
	context->defaultAllocator = allocator;

	// Set terminal logger
	terminalLogger.init(256, allocator);
	context->logger = &terminalLogger;

	// Set global config
	context->config = &globalConfig;

	// Resource strings
	stringCollection.createStringCollection(4096, allocator);
	context->resourceStrings = &stringCollection;

	// Profiling stats
	profilingStats.init(allocator);
	context->profilingStats = &profilingStats;

	// Set Phantasy Engine context
	sfz::setContext(context);
}

// Statics
// ------------------------------------------------------------------------------------------------

static const char* basePath() noexcept
{
	static const char* path = []() {
		const char* tmp = SDL_GetBasePath();
		if (tmp == nullptr) {
			SFZ_ERROR_AND_EXIT("PhantasyEngine", "SDL_GetBasePath() failed: %s", SDL_GetError());
		}
		size_t len = std::strlen(tmp);
		char* res = static_cast<char*>(
			sfz::getDefaultAllocator()->allocate(sfz_dbg("sfz::basePath()"), len + 1, 32));
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
	sfz::str320 tmp("%s%s/", sfz::gameBaseFolderPath(), appName);
	sfz::createDirectory(tmp);
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

// GameLoopState
// ------------------------------------------------------------------------------------------------

struct GameLoopState final {
	sfz::UniquePtr<sfz::Renderer> renderer;
	SDL_Window* window = nullptr;
	void(*cleanupCallback)(void) = nullptr;
	bool quit = false;

	uint64_t prevPerfCounterTickValue = 0;
	uint64_t perfCounterTicksPerSec = 0;

	void* userPtr = nullptr;
	sfz::InitFunc* initFunc = nullptr;
	sfz::UpdateFunc* updateFunc = nullptr;
	sfz::QuitFunc* quitFunc = nullptr;

	// Input structs for updateable
	sfz::UserInput userInput;

	// Window settings
	sfz::Setting* windowWidth = nullptr;
	sfz::Setting* windowHeight = nullptr;
	sfz::Setting* fullscreen = nullptr;
	bool lastFullscreenValue;
	sfz::Setting* maximized = nullptr;
	bool lastMaximizedValue;
};

// Static helper functions
// ------------------------------------------------------------------------------------------------

static void quit(GameLoopState& gameLoopState) noexcept
{
	gameLoopState.quit = true; // Exit infinite while loop (on some platforms)

	SFZ_INFO("PhantasyEngine", "Calling user quit func");
	if (gameLoopState.quitFunc) {
		gameLoopState.quitFunc(gameLoopState.userPtr);
	}

	SFZ_INFO("PhantasyEngine", "Destroying renderer");
	gameLoopState.renderer.destroy(); // Destroy the current renderer

	SFZ_INFO("PhantasyEngine", "Closing SDL controllers");
	gameLoopState.userInput.controllers.clear();

	SFZ_INFO("PhantasyEngine", "Destroying SDL Window");
	SDL_DestroyWindow(gameLoopState.window);

	// Call the cleanup callback
	gameLoopState.cleanupCallback(); 

	// Exit program on Emscripten
#ifdef __EMSCRIPTEN__
	emscripten_cancel_main_loop();
#endif
}

static void initControllers(sfz::HashMap<int32_t, sfz::GameController>& controllers) noexcept
{
	controllers.clear();

	int numJoysticks = SDL_NumJoysticks();
	for (int i = 0; i < numJoysticks; ++i) {
		if (!SDL_IsGameController(i)) continue;

		sfz::GameController c(i);
		if (c.id() == -1) continue;
		if (controllers.get(c.id()) != nullptr) continue;

		controllers[c.id()] = std::move(c);
	}
}

// gameLoopIteration()
// ------------------------------------------------------------------------------------------------

// Called for each iteration of the game loop
void gameLoopIteration(void* gameLoopStatePtr) noexcept
{
	GameLoopState& state = *static_cast<GameLoopState*>(gameLoopStatePtr);

	// Calculate delta since previous iteration
	uint64_t perfCounterTickValue = SDL_GetPerformanceCounter();
	float deltaSecs = float(double(perfCounterTickValue - state.prevPerfCounterTickValue) / double(state.perfCounterTicksPerSec));
	state.prevPerfCounterTickValue = perfCounterTickValue;

	// Remove old events
	state.userInput.events.clear();

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
			SFZ_INFO("PhantasyEngine", "SDL_QUIT event received, quitting.");
			quit(state);
			return;

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
	sfz::sdl::update(state.userInput.controllers, state.userInput.events);

	// Updates mouse
	int windowWidth = -1;
	int windowHeight = -1;
	SDL_GetWindowSize(state.window, &windowWidth, &windowHeight);
	state.userInput.rawMouse.update(windowWidth, windowHeight, state.userInput.events);

	// Add last frame's CPU frametime to the global profiling stats.
	sfz::getProfilingStats().addSample(
		"default", "cpu_frametime", state.renderer->currentFrameIdx(), deltaSecs * 1000.0f);

	// Add last finished GPU frame's frametime to the global profiling stats
	{
		uint64_t frameIdx = ~0u;
		float gpuFrameTimeMs = 0.0f;
		state.renderer->frameTimeMs(frameIdx, gpuFrameTimeMs);
		if (frameIdx != ~0ull) {
			sfz::getProfilingStats().addSample(
				"default", "gpu_frametime", frameIdx, gpuFrameTimeMs);
		}
		else {
			sfz::getProfilingStats().addSample(
				"default", "gpu_frametime", 0, 0.0f);
		}
	}

	// Call user's update func
	sfz::UpdateOp op = state.updateFunc(
		state.renderer.get(),
		deltaSecs,
		&state.userInput,
		state.userPtr);

	// Handle operation returned
	if (op == sfz::UpdateOp::QUIT) quit(state);
	if (op == sfz::UpdateOp::REINIT_CONTROLLERS) initControllers(state.userInput.controllers);
}

// Implementation function
// ------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	// Setup PhantasyEngine context
	setupContext();

	// Set SDL allocators
	if (!sfz::sdl::setSDLAllocator(sfz::getDefaultAllocator())) return EXIT_FAILURE;

	// Set load image allocator
	sfz::setLoadImageAllocator(sfz::getDefaultAllocator());

	// Windwows specific hacks
#ifdef _WIN32
	// Enable hi-dpi awareness
	SetProcessDPIAware();

	// Set current working directory to SDL_GetBasePath()
	_chdir(basePath());
#endif

	// Run user's main function after we have setup the phantasy engine context (allocators and
	// logging).
	//
	// If you are getting linked errors here, it might be because you haven't implemented this
	// function.
	sfz::InitOptions options = PhantasyEngineUserMain(argc, argv);

	// Load global settings
	sfz::GlobalConfig& cfg = sfz::getGlobalConfig();
	{
		// Init config with ini location
		if (options.iniLocation == sfz::IniLocation::NEXT_TO_EXECUTABLE) {
			sfz::str192 iniFileName("%s.ini", options.appName);
			cfg.init(basePath(), iniFileName, sfz::getDefaultAllocator());
			SFZ_INFO("PhantasyEngine", "Ini location set to: %s%s", basePath(), iniFileName.str());
		}
		else if (options.iniLocation == sfz::IniLocation::MY_GAMES_DIR) {

			// Create user data directory
			ensureAppUserDataDirExists(options.appName);

			// Initialize ini
			sfz::str192 iniFileName("%s/%s.ini", options.appName, options.appName);
			cfg.init(sfz::gameBaseFolderPath(), iniFileName, sfz::getDefaultAllocator());
			SFZ_INFO("PhantasyEngine", "Ini location set to: %s%s",
				sfz::gameBaseFolderPath(), iniFileName.str());
		}
		else {
			sfz_assert_hard(false);
		}

		// Load ini file
		cfg.load();
	}

	// Init default category of profiling stats
	sfz::getProfilingStats().createCategory("default", 300, 66.7f, "ms", "frame"); // 300 = 60 fps * 5 seconds
	sfz::getProfilingStats().createLabel("default", "cpu_frametime", sfz:: vec4(1.0f, 0.0f, 0.0f, 1.0f));
	sfz::getProfilingStats().createLabel("default", "gpu_frametime", sfz::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	sfz::getProfilingStats().createLabel("default", "16.67 ms", sfz::vec4(0.5f, 0.5f, 0.7f, 1.0f), 16.67f);

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

	// Window settings
	sfz::Setting* width = cfg.sanitizeInt("Window", "width", false, 1280, 128, 3840, 8);
	sfz::Setting* height = cfg.sanitizeInt("Window", "height", false, 800, 128, 2160, 8);
	sfz::Setting* fullscreen = cfg.sanitizeBool("Window", "fullscreen", false, false);
	sfz::Setting* maximized = cfg.sanitizeBool("Window", "maximized", false, false);
	if (fullscreen->boolValue() && maximized->boolValue()) {
		maximized->setBool(false);
	}

	// Create SDL_Window
	uint32_t windowFlags =
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_ALLOW_HIGHDPI |
		(fullscreen->boolValue() ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
		(maximized->boolValue() ? SDL_WINDOW_MAXIMIZED : 0);
	SDL_Window* window =
		SDL_CreateWindow(options.appName, 0, SDL_WINDOWPOS_UNDEFINED,
		width->intValue(), height->intValue(), windowFlags);
	if (window == NULL) {
		SFZ_ERROR("PhantasyEngine", "SDL_CreateWindow() failed: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize ImGui
	SFZ_INFO("PhantasyEngine", "Initializing Imgui");
	phImageView imguiFontTexView = initializeImgui(sfz::getDefaultAllocator());

	// Initializing renderer
	SFZ_INFO("PhantasyEngine", "Initializing renderer");
	sfz::UniquePtr<sfz::Renderer> renderer = sfz::makeUnique<sfz::Renderer>(sfz::getDefaultAllocator(), sfz_dbg(""));
	bool rendererInitSuccess = renderer->init(window, imguiFontTexView, sfz::getDefaultAllocator());
	if (!rendererInitSuccess) {
		SFZ_ERROR("PhantasyEngine", "Renderer::init() failed");
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Start game loop
	SFZ_INFO("PhantasyEngine", "Starting game loop");
	{
		// Initialize game loop state
		GameLoopState gameLoopState = {};
		gameLoopState.renderer = std::move(renderer);
		gameLoopState.window = window;
		gameLoopState.cleanupCallback = []() {
			// Store global settings
			SFZ_INFO("PhantasyEngine", "Saving global config to file");
			sfz::GlobalConfig& cfg = sfz::getGlobalConfig();
			if (!cfg.save()) {
				SFZ_WARNING("PhantasyEngine", "Failed to write ini file");
			}
			cfg.destroy();

			// Deinitializing Imgui
			SFZ_INFO("PhantasyEngine", "Deinitializing Imgui");
			sfz::deinitializeImgui();

			// Cleanup SDL2
			SFZ_INFO("PhantasyEngine", "Cleaning up SDL2");
			SDL_Quit();
		};

		gameLoopState.userPtr = options.userPtr;
		gameLoopState.initFunc = options.initFunc;
		gameLoopState.updateFunc = options.updateFunc;
		gameLoopState.quitFunc = options.quitFunc;

		gameLoopState.userInput.events.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));
		gameLoopState.userInput.controllers.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));

		gameLoopState.prevPerfCounterTickValue = SDL_GetPerformanceCounter();
		gameLoopState.perfCounterTicksPerSec = SDL_GetPerformanceFrequency();

		// Initialize controllers
		SDL_GameControllerEventState(SDL_ENABLE);
		initControllers(gameLoopState.userInput.controllers);

		// Get settings
		gameLoopState.windowWidth = cfg.getSetting("Window", "width");
		sfz_assert(gameLoopState.windowWidth != nullptr);
		gameLoopState.windowHeight = cfg.getSetting("Window", "height");
		sfz_assert(gameLoopState.windowHeight != nullptr);
		gameLoopState.fullscreen = cfg.getSetting("Window", "fullscreen");
		sfz_assert(gameLoopState.fullscreen != nullptr);
		gameLoopState.maximized = cfg.getSetting("Window", "maximized");

		// Call users init function
		if (gameLoopState.initFunc) {
			gameLoopState.initFunc(gameLoopState.renderer.get(), gameLoopState.userPtr);
		}

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
	}

	// DEAD ZONE
	// Don't place any code after the game loop, it will never be called on some platforms.

	return EXIT_SUCCESS;
}
