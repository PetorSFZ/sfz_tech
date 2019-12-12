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

#include <skipifzero.hpp>
#include <skipifzero_smart_pointers.hpp>

#include "ph/game_loop/GameLoopUpdateable.hpp"

namespace sfz {

using sfz::Allocator;
using sfz::UniquePtr;

using sfz::vec3;
using sfz::vec4;

// DefaultGameUpdateable logic
// ------------------------------------------------------------------------------------------------

struct ImguiControllers final {
	bool useMouse = true;
	bool useKeyboard = true;
	int32_t controllerIndex = -1;
};

class GameLogic {
public:
	virtual ~GameLogic() {}

	virtual void initialize(Renderer& renderer) = 0;

	// Returns the index of the controller to be used for Imgui. If -1 is returned no controller
	// input will be provided to Imgui.
	virtual ImguiControllers imguiController(const UserInput&) { return ImguiControllers(); }

	virtual UpdateOp processInput(
		const UserInput& input,
		const UpdateInfo& updateInfo,
		Renderer& renderer) = 0;

	virtual UpdateOp updateTick(const UpdateInfo& updateInfo, Renderer& renderer) = 0;

	virtual void render(const UpdateInfo& updateInfo, Renderer& renderer) = 0;

	// Small hook that is called last in a frame, after rendering, regardless of whether the
	// console is active or not.
	//
	// This is useful if you need to do some operations each frame when the renderer is not busy
	// preparing commands to render a new frame (i.e., not between beginFrame() and finishFrame()).
	//
	// This is also the last thing that happens each frame, so it can also be a good place to put
	// some per frame book keeping you are doing.
	virtual void postRenderHook(sfz::Renderer& renderer, bool consoleActive) {};

	// Renders custom Imgui commands.
	//
	// This function and injectConsoleMenu() are the only places where Imgui commands can safely
	// be called. BeginFrame() and EndFrame() are called before and after this function. Other
	// Imgui commands from the DefaultGameUpdateable console itself may be sent within this same
	// frame if they are set to be always shown. This function will not be called if the console
	// is currently active.
	virtual void renderCustomImgui() {}

	// Call when console is active after all the built-in menus have been drawn. Can be used to
	// inject game-specific custom menus into the console.
	virtual void injectConsoleMenu() {}

	// These two functions are used to dock injected console windows. The first one should return
	// the number of windows you want to dock, the second one the name of each window given the
	// index in the range you provided with the first function. These are typically only called
	// during the first boot of the engine/game. You don't need to provide them even if you are
	// injecting console windows;
	virtual uint32_t injectConsoleMenuNumWindowsToDockInitially() { return 0; }
	virtual const char* injectConsoleMenuNameOfWindowToDockInitially(uint32_t idx) { (void)idx; return nullptr; }

	// Called when console is activated. The logic instance will not receive any additional calls
	// until the console is closed, at which point onConsoleDeactivated() will be called. onQuit()
	// may be called before the console is deactivated.
	virtual void onConsoleActivated() {}

	// Called when the console is deactivated.
	virtual void onConsoleDeactivated() {}

	virtual void onQuit() { }
};

// DefaultGameUpdateable creation function
// ------------------------------------------------------------------------------------------------

UniquePtr<GameLoopUpdateable> createDefaultGameUpdateable(
	Allocator* allocator,
	UniquePtr<GameLogic> logic) noexcept;

} // namespace sfz
