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

#include <skipifzero.hpp>
#include <skipifzero_allocators.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_new.hpp>
#define SFZ_STR_ID_IMPLEMENTATION
#include <skipifzero_strings.hpp>

#include "sfz/Context.hpp"
#include "sfz/Logging.hpp"
#include "sfz/audio/AudioEngine.hpp"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/renderer/ZeroGUtils.hpp"
#include "sfz/rendering/Image.hpp"
#include "sfz/rendering/ImguiSupport.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/sdl/SDLAllocator.hpp"
#include "sfz/shaders/ShaderManager.hpp"
#include "sfz/util/IO.hpp"
#include "sfz/util/TerminalLogger.hpp"

// Request dedicated graphics card over integrated on Windows
// ------------------------------------------------------------------------------------------------

#ifdef _WIN32
extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 1; }
extern "C" { _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1; }
#endif

// Build time macros
// ------------------------------------------------------------------------------------------------

// See https://stackoverflow.com/a/16472369

// Example of __DATE__ string: "Jul 27 2012"
//                              01234567890

#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])

#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')

#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')

#define BUILD_MONTH_CH1 \
    ( \
        (BUILD_MONTH_IS_JAN) ? '1' : \
        (BUILD_MONTH_IS_FEB) ? '2' : \
        (BUILD_MONTH_IS_MAR) ? '3' : \
        (BUILD_MONTH_IS_APR) ? '4' : \
        (BUILD_MONTH_IS_MAY) ? '5' : \
        (BUILD_MONTH_IS_JUN) ? '6' : \
        (BUILD_MONTH_IS_JUL) ? '7' : \
        (BUILD_MONTH_IS_AUG) ? '8' : \
        (BUILD_MONTH_IS_SEP) ? '9' : \
        (BUILD_MONTH_IS_OCT) ? '0' : \
        (BUILD_MONTH_IS_NOV) ? '1' : \
        (BUILD_MONTH_IS_DEC) ? '2' : \
        /* error default */    '?' \
    )

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])

// Example of __TIME__ string: "21:06:19"
//                              01234567

#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])

#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])

#define BUILD_SEC_CH0 (__TIME__[6])
#define BUILD_SEC_CH1 (__TIME__[7])

constexpr char BUILD_TIME[] = {
	BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3, '-',
	BUILD_MONTH_CH0, BUILD_MONTH_CH1, '-',
	BUILD_DAY_CH0, BUILD_DAY_CH1, ' ',
	BUILD_HOUR_CH0, BUILD_HOUR_CH1, ':',
	BUILD_MIN_CH0, BUILD_MIN_CH1, ':',
	BUILD_SEC_CH0, BUILD_SEC_CH1, '\0'
};

// Context
// ------------------------------------------------------------------------------------------------

// This is the global PhantasyEngine context, a pointer to it will be set using setContext().
static SfzAllocator standardAllocator = sfz::createStandardAllocator();
static sfz::Context phantasyEngineContext;
static sfz::TerminalLogger terminalLogger;
static sfz::GlobalConfig globalConfig;
static sfz::ResourceManager resourceManager;
static sfz::ShaderManager shaderManager;
static sfz::Renderer renderer;
static sfz::AudioEngine audioEngine;
static sfz::ProfilingStats profilingStats;

static void setupContext() noexcept
{
	sfz::Context* context = &phantasyEngineContext;

	// Set sfz standard allocator
	standardAllocator = sfz::createStandardAllocator();
	SfzAllocator* allocator = &standardAllocator;
	context->defaultAllocator = allocator;

	// String storage
	sfz::strStorage = sfz_new<sfz::StringStorage>(allocator, sfz_dbg(""), 4096, allocator);

	// Set terminal logger
	terminalLogger.init(256, allocator);
	context->logger = &terminalLogger;

	// Set global config
	context->config = &globalConfig;

	// Set resource manager
	context->resources = &resourceManager;

	// Set shader manager
	context->shaders = &shaderManager;

	// Set renderer
	context->renderer = &renderer;

	// Set audio engine
	context->audioEngine = &audioEngine;

	// Profiling stats
	profilingStats.init(allocator);
	context->profilingStats = &profilingStats;

	// Set Phantasy Engine context
	sfz::setContext(context);
}

// Statics
// ------------------------------------------------------------------------------------------------

static_assert(sfz::MAX_NUM_SCANCODES == SDL_NUM_SCANCODES, "Mismatch");

static const char* basePath() noexcept
{
	static const char* path = []() {
		const char* tmp = SDL_GetBasePath();
		if (tmp == nullptr) {
			SFZ_ERROR_AND_EXIT("PhantasyEngine", "SDL_GetBasePath() failed: %s", SDL_GetError());
		}
		size_t len = std::strlen(tmp);
		char* res = static_cast<char*>(
			sfz::getDefaultAllocator()->alloc(sfz_dbg("sfz::basePath()"), len + 1, 32));
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
		u32(version.major), u32(version.minor), u32(version.patch));

	// Log SDL2 linked version
	SDL_GetVersion(&version);
	SFZ_INFO("SDL2", "Linked version: %u.%u.%u",
		u32(version.major), u32(version.minor), u32(version.patch));
}

// GameLoopState
// ------------------------------------------------------------------------------------------------

struct GameLoopState final {
	SDL_Window* window = nullptr;
	bool quit = false;

	u64 prevPerfCounterTickValue = 0;
	u64 perfCounterTicksPerSec = 0;

	void* userPtr = nullptr;
	sfz::InitFunc* initFunc = nullptr;
	sfz::UpdateFunc* updateFunc = nullptr;
	sfz::QuitFunc* quitFunc = nullptr;

	// Input structs for updateable
	sfz::Array<SDL_Event> events;
	SDL_TouchID touchInputDeviceID = 0;
	sfz::RawInputState rawFrameInput;

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

static bool initController(int deviceIdx, sfz::GamepadState& stateOut)
{
	if (!SDL_IsGameController(deviceIdx)) return false;

	// Open controller
	stateOut.controller = SDL_GameControllerOpen(deviceIdx);
	if (stateOut.controller == nullptr) {
		SFZ_ERROR("PhantasyEngine", "Could not open GameController at device index %i, error: %s",
			deviceIdx, SDL_GetError());
		return false;
	}

	// Get JoystickID
	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(stateOut.controller);
	if (joystick == nullptr) {
		SFZ_ERROR("PhantasyEngine",
			"Could not retrieve SDL_Joystick* from SDL_GameController, error: %s", SDL_GetError());
		SDL_GameControllerClose(stateOut.controller);
		stateOut.controller = nullptr;
		return false;
	}
	SDL_JoystickID id = SDL_JoystickInstanceID(joystick);
	if (id < 0) {
		SFZ_ERROR("PhantasyEngine",
			"Could not retrieve JoystickID from SDL_GameController, error: %s", SDL_GetError());
		SDL_GameControllerClose(stateOut.controller);
		stateOut.controller = nullptr;
		return false;
	}
	stateOut.id = id;

	// Log about gamepad we have connected
	SFZ_INFO("PhantasyEngine", "Connected gamepad with name \"%s\", JoystickID: %i",
		SDL_GameControllerName(stateOut.controller), stateOut.id);

	return true;
}

static void initControllers(sfz::RawInputState& input) noexcept
{
	// Close existing game controllers if any
	for (sfz::GamepadState& state : input.gamepads) {
		if (state.controller != nullptr) {
			SDL_GameControllerClose(state.controller);
			state.controller = nullptr;
		}
	}
	input.gamepads.clear();

	// Open new gamepads
	int numJoysticks = SDL_NumJoysticks();
	for (int deviceIdx = 0; deviceIdx < numJoysticks; deviceIdx++) {
		if (!SDL_IsGameController(deviceIdx)) continue;
		if (input.gamepads.isFull()) {
			SFZ_ERROR("PhantasyEngine", "Too many gamepads attached (>6), skipping this one.");
			continue;
		}

		sfz::GamepadState state;
		if (initController(deviceIdx, state)) {
			input.gamepads.add(state);
		}
	}
}

// gameLoopIteration()
// ------------------------------------------------------------------------------------------------

// Called for each iteration of the game loop
void gameLoopIteration(void* gameLoopStatePtr) noexcept
{
	GameLoopState& state = *static_cast<GameLoopState*>(gameLoopStatePtr);

	// Calculate delta since previous iteration
	u64 perfCounterTickValue = SDL_GetPerformanceCounter();
	f32 deltaSecs = f32(double(perfCounterTickValue - state.prevPerfCounterTickValue) / double(state.perfCounterTicksPerSec));
	state.prevPerfCounterTickValue = perfCounterTickValue;

	// Remove old events
	state.events.clear();

	// Check window status
	u32 currentWindowFlags = SDL_GetWindowFlags(state.window);
	bool isFullscreen = (currentWindowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
	bool isMaximized = (currentWindowFlags & SDL_WINDOW_MAXIMIZED) != 0;
	bool shouldBeFullscreen = state.fullscreen->boolValue();
	bool shouldBeMaximized = state.maximized->boolValue();

	// Process SDL events
	{
		SDL_Event event = {};
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {

				// Quitting
			case SDL_QUIT:
				SFZ_INFO("PhantasyEngine", "SDL_QUIT event received, quitting.");
				state.quit = true;
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
				state.events.add(event);
				break;

				// All other events
			default:
				state.events.add(event);
				break;
			}
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
		u32 fullscreenFlags = shouldBeFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
		if (SDL_SetWindowFullscreen(state.window, fullscreenFlags) < 0) {
			SFZ_ERROR("PhantasyEngine", "SDL_SetWindowFullscreen() failed: %s", SDL_GetError());
		}
		if (!shouldBeFullscreen) {
			SDL_SetWindowSize(state.window,
				state.windowWidth->intValue(), state.windowHeight->intValue());
		}
	}

	// Update frame input
	{
		// Window dimensions
		int windowWidth = -1;
		int windowHeight = -1;
		SDL_GetWindowSize(state.window, &windowWidth, &windowHeight);
		state.rawFrameInput.windowDims = i32x2(windowWidth, windowHeight);

		// Keyboard
		{
			sfz::KeyboardState& kb = state.rawFrameInput.kb;
			memset(kb.scancodes, 0, sfz::MAX_NUM_SCANCODES * sizeof(u8));
			int numKeys = 0;
			const u8* kbState = SDL_GetKeyboardState(&numKeys);
			memcpy(kb.scancodes, kbState, numKeys * sizeof(u8));
		}

		// Mouse
		{
			sfz::MouseState& mouse = state.rawFrameInput.mouse;
			mouse.windowDims = state.rawFrameInput.windowDims;
			
			i32 x = 0;
			i32 y = 0;
			u32 buttonState = SDL_GetMouseState(&x, &y);
			y = sfz::min(y, mouse.windowDims.y - 1);
			mouse.pos = i32x2(x, mouse.windowDims.y - y - 1);
			
			u32 buttonState2 = SDL_GetRelativeMouseState(&mouse.delta.x, &mouse.delta.y);
			sfz_assert(buttonState == buttonState2);
			mouse.delta.y = -mouse.delta.y;
			
			mouse.wheel = i32x2(0);
			for (const SDL_Event& event : state.events) {
				if (event.type == SDL_MOUSEWHEEL) {
					i32x2 delta = i32x2(event.wheel.x, event.wheel.y);
					if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) delta = -delta;
					mouse.wheel += delta;
				}
			}

			mouse.left = ((SDL_BUTTON(SDL_BUTTON_LEFT) & buttonState) == SDL_BUTTON(SDL_BUTTON_LEFT)) ? u8(1) : u8(0);
			mouse.middle = ((SDL_BUTTON(SDL_BUTTON_MIDDLE) & buttonState) == SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? u8(1) : u8(0);
			mouse.right = ((SDL_BUTTON(SDL_BUTTON_RIGHT) & buttonState) == SDL_BUTTON(SDL_BUTTON_RIGHT)) ? u8(1) : u8(0);
		}

		// Gamepads
		{
			// Check if any gamepads got connected/disconnected
			for (const SDL_Event& event : state.events) {
				if (event.type == SDL_CONTROLLERDEVICEADDED) {
					if (state.rawFrameInput.gamepads.isFull()) {
						SFZ_ERROR("PhantasyEngine",
							"Too many gamepads attached (>6), skipping this one.");
						continue;
					}
					sfz::GamepadState gpdState;
					if (initController(event.cdevice.which, gpdState)) {
						state.rawFrameInput.gamepads.add(gpdState);
					}
				}
				else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
					for (u32 gpdIdx = 0; gpdIdx < state.rawFrameInput.gamepads.size(); gpdIdx++) {
						const sfz::GamepadState& gpd = state.rawFrameInput.gamepads[gpdIdx];
						if (gpd.id == event.cdevice.which) {
							SDL_GameControllerClose(gpd.controller);
							state.rawFrameInput.gamepads.remove(gpdIdx);
							break;
						}
					}

				}
			}

			for (sfz::GamepadState& gpd : state.rawFrameInput.gamepads) {

				// We cheat a bit here. The range of a stick axis is [-32768, 32767], with the
				// deadzone somewhere within ~8000 of 0. However, it could also be that the gamepad
				// is not perfectly calibrated and that the actual max is slightly below what SDL2
				// allows for.
				//
				// Thus, we reduce the amount needed to hit max by about ~300 units (slightly less
				// than 1% of total range). This way we should hopefully never end up in the
				// unfortunate scenario where a gamepad is physically incapable of capping out.
				constexpr f32 AXIS_MAX = f32(SDL_JOYSTICK_AXIS_MAX - 300);

				int16_t leftX = SDL_GameControllerGetAxis(gpd.controller, SDL_CONTROLLER_AXIS_LEFTX);
				int16_t leftY = SDL_GameControllerGetAxis(gpd.controller, SDL_CONTROLLER_AXIS_LEFTY);
				gpd.leftStick = f32x2(f32(leftX), -f32(leftY)) / f32x2(AXIS_MAX);
				gpd.leftStick = sfz::clamp(gpd.leftStick, -1.0f, 1.0f);

				int16_t rightX = SDL_GameControllerGetAxis(gpd.controller, SDL_CONTROLLER_AXIS_RIGHTX);
				int16_t rightY = SDL_GameControllerGetAxis(gpd.controller, SDL_CONTROLLER_AXIS_RIGHTY);
				gpd.rightStick = f32x2(f32(rightX), -f32(rightY)) / f32x2(AXIS_MAX);
				gpd.rightStick = sfz::clamp(gpd.rightStick, -1.0f, 1.0f);

				int16_t leftTrigger = SDL_GameControllerGetAxis(gpd.controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
				gpd.lt = sfz::clamp(f32(leftTrigger) / AXIS_MAX, 0.0f, 1.0f);

				int16_t rightTrigger = SDL_GameControllerGetAxis(gpd.controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
				gpd.rt = sfz::clamp(f32(rightTrigger) / AXIS_MAX, 0.0f, 1.0f);

				// Clear previous button states
				for (u32 i = 0; i < sfz::GPD_MAX_NUM_BUTTONS; i++) gpd.buttons[i] = 0;

				gpd.buttons[sfz::GPD_A] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_A);
				gpd.buttons[sfz::GPD_B] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_B);
				gpd.buttons[sfz::GPD_X] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_X);
				gpd.buttons[sfz::GPD_Y] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_Y);
				
				gpd.buttons[sfz::GPD_BACK] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_BACK);
				gpd.buttons[sfz::GPD_START] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_START);

				gpd.buttons[sfz::GPD_LS_CLICK] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_LEFTSTICK);
				if (sfz::length(gpd.leftStick) > 0.75f) {
					if (abs(leftY) >= abs(leftX)) {
						gpd.buttons[sfz::GPD_LS_UP] = -leftY > 0 ? 1 : 0;
						gpd.buttons[sfz::GPD_LS_DOWN] = -leftY < 0 ? 1 : 0;
					}
					else {
						gpd.buttons[sfz::GPD_LS_LEFT] = leftX < 0 ? 1 : 0;
						gpd.buttons[sfz::GPD_LS_RIGHT] = leftX > 0 ? 1 : 0;
					}
				}

				gpd.buttons[sfz::GPD_RS_CLICK] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
				if (sfz::length(gpd.rightStick) > 0.75f) {
					if (abs(rightY) >= abs(rightX)) {
						gpd.buttons[sfz::GPD_RS_UP] = -rightY < 0 ? 1 : 0;
						gpd.buttons[sfz::GPD_RS_DOWN] = -rightY > 0 ? 1 : 0;
					}
					else {
						gpd.buttons[sfz::GPD_RS_LEFT] = rightX < 0 ? 1 : 0;
						gpd.buttons[sfz::GPD_RS_RIGHT] = rightX > 0 ? 1 : 0;
					}
				}

				gpd.buttons[sfz::GPD_LB] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
				gpd.buttons[sfz::GPD_RB] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
				
				gpd.buttons[sfz::GPD_LT] = gpd.lt >= 0.75f ? u8(1) : u8(0);
				gpd.buttons[sfz::GPD_RT] = gpd.rt >= 0.75f ? u8(1) : u8(0);

				gpd.buttons[sfz::GPD_DPAD_UP] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
				gpd.buttons[sfz::GPD_DPAD_DOWN] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
				gpd.buttons[sfz::GPD_DPAD_LEFT] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
				gpd.buttons[sfz::GPD_DPAD_RIGHT] = SDL_GameControllerGetButton(gpd.controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
			}
		}

		// Touch inputs
		{
			sfz::Arr8<sfz::TouchState>& touches = state.rawFrameInput.touches;

			// Find touch input device we are using
			const int numTouchDevices = SDL_GetNumTouchDevices();
			for (int touchDeviceIdx = 0; touchDeviceIdx < numTouchDevices; touchDeviceIdx++) {
				SDL_TouchID touchID = SDL_GetTouchDevice(touchDeviceIdx);
				sfz_assert(touchID != 0); // 0 if invalid

				// We don't care about emulated touch inputs by mouse
				if (touchID == SDL_MOUSE_TOUCHID) continue;

				// We only support "direct" touches (abs position relative to window) for now
				SDL_TouchDeviceType type = SDL_GetTouchDeviceType(touchID);
				if (type != SDL_TOUCH_DEVICE_DIRECT) continue;

				state.touchInputDeviceID = touchID;
				break;
			}

			// Get current touch inputs from touch device
			touches.clear();
			if (state.touchInputDeviceID != 0) {
				const int numFingers = SDL_GetNumTouchFingers(state.touchInputDeviceID);
				for (int fingerIdx = 0; fingerIdx < numFingers; fingerIdx++) {
					SDL_Finger* finger = SDL_GetTouchFinger(state.touchInputDeviceID, fingerIdx);
					if (touches.isFull()) break;
					sfz::TouchState& touch = touches.add();
					touch.id = finger->id;
					touch.pos = sfz::clamp(f32x2(finger->x, 1.0f - finger->y), 0.0f, 1.0f);
					touch.pressure = finger->pressure;
				}
			}
		}
	}

	// Add last frame's CPU frametime to the global profiling stats.
	sfz::getProfilingStats().addSample(
		"default", "cpu_frametime", sfz::getRenderer().currentFrameIdx(), deltaSecs * 1000.0f);

	// Add last finished GPU frame's frametime to the global profiling stats
	{
		u64 frameIdx = ~0u;
		f32 gpuFrameTimeMs = 0.0f;
		sfz::getRenderer().frameTimeMs(frameIdx, gpuFrameTimeMs);
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
		deltaSecs,
		state.window,
		state.events.data(),
		state.events.size(),
		&state.rawFrameInput,
		state.userPtr);

	// Handle operation returned
	if (op == sfz::UpdateOp::QUIT) {
		state.quit = true;
		return;
	}
	if (op == sfz::UpdateOp::REINIT_CONTROLLERS) initControllers(state.rawFrameInput);
}

// Implementation function
// ------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	// Setup PhantasyEngine context
	setupContext();

	// Log time
	{
		const time_t currentTime = std::time(nullptr);
		std::tm* tmPtr = std::localtime(&currentTime);
		sfz::str64 timeStr;
		size_t res = std::strftime(timeStr.mRawStr, timeStr.capacity(), "%Y-%m-%d %H:%M:%S", tmPtr);
		if (res == 0) {
			timeStr.clear();
			timeStr.appendf("INVALID TIME");
		}
		SFZ_INFO("PhantasyEngine", "Build time: %s\nCurrent time: %s", BUILD_TIME, timeStr.str());
	}

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
	sfz::getProfilingStats().createCategory("default", 300, 66.7f, "ms", "frame", 25.0f); // 300 = 60 fps * 5 seconds
	sfz::getProfilingStats().createLabel("default", "cpu_frametime", f32x4(1.0f, 0.0f, 0.0f, 1.0f));
	sfz::getProfilingStats().createLabel("default", "gpu_frametime", f32x4(0.0f, 1.0f, 0.0f, 1.0f));

	// Init SDL2
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
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
	SDL_Window* window = nullptr;
	{
		u32 windowFlags =
			SDL_WINDOW_RESIZABLE |
			SDL_WINDOW_ALLOW_HIGHDPI |
			(fullscreen->boolValue() ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
			(maximized->boolValue() ? SDL_WINDOW_MAXIMIZED : 0);
		const char* windowName =
			options.windowNameOverride != nullptr ? options.windowNameOverride : options.appName;
		sfz::str256 windowTitle = sfz::str256("%s", windowName);
		if (options.appendBuildTimeToWindowTitle) {
			windowTitle.appendf("   (Build: %s)", BUILD_TIME);
		}
		window = SDL_CreateWindow(windowTitle.str(), 0, SDL_WINDOWPOS_UNDEFINED,
			width->intValue(), height->intValue(), windowFlags);
		if (window == NULL) {
			SFZ_ERROR("PhantasyEngine", "SDL_CreateWindow() failed: %s", SDL_GetError());
			SDL_Quit();
			return EXIT_FAILURE;
		}
	}

	// Initialize ZeroG
	SFZ_INFO("PhantasyEngine", "Initializing ZeroG");
	bool zgInitSuccess = sfz::initializeZeroG(
		window,
		sfz::getDefaultAllocator(),
		cfg.sanitizeBool("Renderer", "vsync", true, true));
	if (!zgInitSuccess) {
		SFZ_ERROR("PhantasyEngine", "Failed to initialize ZeroG");
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize resource manager
	SFZ_INFO("PhantasyEngine", "Initializing resource manager");
	sfz::getResourceManager().init(options.maxNumResources, sfz::getDefaultAllocator());

	// Initialize shader manager
	SFZ_INFO("PhantasyEngine", "Initializing shader manager");
	sfz::getShaderManager().init(options.maxNumShaders, sfz::getDefaultAllocator());

	// Initialize ImGui
	SFZ_INFO("PhantasyEngine", "Initializing Imgui");
	sfz::ImageView imguiFontTexView = sfz::initializeImgui(sfz::getDefaultAllocator());

	// Initialize renderer
	SFZ_INFO("PhantasyEngine", "Initializing renderer");
	bool rendererInitSuccess = sfz::getRenderer().init(window, imguiFontTexView, sfz::getDefaultAllocator());
	if (!rendererInitSuccess) {
		SFZ_ERROR("PhantasyEngine", "Renderer::init() failed");
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize audio engine
	SFZ_INFO("PhantasyEngine", "Initializing audio engine");
	bool audioInitSuccess = sfz::getAudioEngine().init(sfz::getDefaultAllocator());
	if (!audioInitSuccess) {
		SFZ_ERROR("PhantasyEngine", "AudioEngine::init() failed");
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize game loop state
	GameLoopState gameLoopState = {};
	gameLoopState.window = window;

	gameLoopState.userPtr = options.userPtr;
	gameLoopState.initFunc = options.initFunc;
	gameLoopState.updateFunc = options.updateFunc;
	gameLoopState.quitFunc = options.quitFunc;

	gameLoopState.events.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));
		
	gameLoopState.prevPerfCounterTickValue = SDL_GetPerformanceCounter();
	gameLoopState.perfCounterTicksPerSec = SDL_GetPerformanceFrequency();

	// Initialize controllers
	SDL_GameControllerEventState(SDL_ENABLE);
	initControllers(gameLoopState.rawFrameInput);

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
		gameLoopState.initFunc(window, gameLoopState.userPtr);
	}
	sfz::getProfilingStats().createLabel("default", "16.67 ms", f32x4(0.5f, 0.5f, 0.7f, 1.0f), 16.67f);

	// Start the game loop
	SFZ_INFO("PhantasyEngine", "Starting game loop");
	while (!gameLoopState.quit) {
		gameLoopIteration(&gameLoopState);
	}

	// Store global settings
	SFZ_INFO("PhantasyEngine", "Saving global config to file");
	if (!cfg.save()) {
		SFZ_WARNING("PhantasyEngine", "Failed to write ini file");
	}
	cfg.destroy();

	SFZ_INFO("PhantasyEngine", "Deinitializing Imgui");
	sfz::deinitializeImgui();

	SFZ_INFO("PhantasyEngine", "Destroying renderer");
	sfz::getRenderer().destroy();

	SFZ_INFO("PhantasyEngine", "Destroying resource manager");
	sfz::getResourceManager().destroy();

	SFZ_INFO("PhantasyEngine", "Destroying shader manager");
	sfz::getShaderManager().destroy();

	SFZ_INFO("PhantasyEngine", "Deinitializing ZeroG");
	CHECK_ZG zgContextDeinit();

	SFZ_INFO("PhantasyEngine", "Destroying audio engine");
	sfz::getAudioEngine().destroy();

	SFZ_INFO("PhantasyEngine", "Closing SDL controllers");
	for (sfz::GamepadState& gpd : gameLoopState.rawFrameInput.gamepads) {
		SDL_GameControllerClose(gpd.controller);
		gpd.controller = nullptr;
	}
	gameLoopState.rawFrameInput.gamepads.clear();

	SFZ_INFO("PhantasyEngine", "Destroying SDL Window");
	SDL_DestroyWindow(gameLoopState.window);

	SFZ_INFO("PhantasyEngine", "Cleaning up SDL2");
	SDL_Quit();

	SFZ_INFO("PhantasyEngine", "Destroying string ID storage");
	sfz_delete(sfz::getDefaultAllocator(), sfz::strStorage);

	return EXIT_SUCCESS;
}
