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

#include <SDL.h>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>

#include "sfz/sdl/GameController.hpp"
#include "sfz/sdl/Mouse.hpp"

namespace sfz {

using sdl::GameController;
using sdl::GameControllerState;
using sdl::Mouse;

// Structs
// ------------------------------------------------------------------------------------------------

enum class UpdateOp : uint32_t {
	NO_OP = 0,
	QUIT,
	REINIT_CONTROLLERS
};

struct UserInput final {

	// SDL events
	Array<SDL_Event> events;

	// Processed controller and mouse input
	HashMap<int32_t, GameController> controllers;
	Mouse rawMouse;
};

enum class IniLocation {
	
	// The ini file is placed next to the exe file.
	NEXT_TO_EXECUTABLE,

	// "C:\Users\<username>\Documents\My Games" on Windows, i.e. where many games store their
	// save files and config files. On macOS (and Linux) this is instead "~/My Games".
	MY_GAMES_DIR
};

using InitFunc = void(void* userPtr);
using UpdateFunc = UpdateOp(
	float deltaSecs,
	const UserInput* input,
	void* userPtr);
using QuitFunc = void(void* userPtr);

struct InitOptions final {

	// Name of application. Is used for, among other things, window title, name of ini file, etc.
	const char* appName = "NO_APP_NAME";

	// Location of Ini file
	IniLocation iniLocation = IniLocation::NEXT_TO_EXECUTABLE;

	// User specified pointer which will be passed as an argument to the specified functions.
	void* userPtr = nullptr;

	// Init function, called right before gameloop starts.
	InitFunc* initFunc = nullptr;

	// Called each iteration of the gameloop.
	UpdateFunc* updateFunc = nullptr;

	// Called when program is exiting.
	QuitFunc* quitFunc = nullptr;
};

} // namespace sfz

// User's main signature
// ------------------------------------------------------------------------------------------------

// The signature of the user's main function called when PhantasyEngine is initialized.
//
// The "Main.cpp" file for your project should implement this function. It will be called fairly
// early on in the actual "main" function that is owned by PhantasyEngine, mainly the allocator
// and logging parts of PhantasyEngine's context will be setup before this is called.
//
// You should not perform too much work in this function, mainly setting some options and callbacks
// for the game loop.
sfz::InitOptions PhantasyEngineUserMain(int argc, char* argv[]);
