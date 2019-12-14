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

#include <skipifzero_smart_pointers.hpp>

#include "sfz/game_loop/GameLoopUpdateable.hpp"

namespace sfz {

// InitOptions struct
// ------------------------------------------------------------------------------------------------

enum class IniLocation {
	/// The ini file is placed next to the exe file.
	NEXT_TO_EXECUTABLE,

	/// "C:\Users\<username>\Documents\My Games" on Windows, i.e. where many games store their
	/// save files and config files. On macOS (and Linux) this is instead "~/My Games".
	MY_GAMES_DIR
};

struct InitOptions final {
	/// Name of application. Is used for, among other things, window title, name of ini file, etc.
	const char* appName = "NO_APP_NAME";

	/// Location of Ini file
	IniLocation iniLocation = IniLocation::NEXT_TO_EXECUTABLE;

	/// Function that creates the initial GameLoopUpdateable, will only be called once. It's okay
	/// (and necessary) to use sfz::Allocator in this function, but nowhere else in the ini code.
	UniquePtr<GameLoopUpdateable> (*createInitialUpdateable)(void);
};

// Phantasy Engine main macro
// ------------------------------------------------------------------------------------------------

/// This is used to initialize PhantasyEngine.
/// The "Main.cpp" file for your project should essentially only include this header and call this
/// macro. It is very important that you don't allocate any heap memory (especially using
/// sfz::Allocator) before this function has executed. PhantasyEngine may replace the default
/// allocator with a custom one.
/// \param createInitOptions a function that creates an InitOptions struct
#define PHANTASY_ENGINE_MAIN(createInitOptions) \
	int main(int argc, char* argv[]) \
	{ \
		return sfz::mainImpl(argc, argv, (createInitOptions)()); \
	}

// Implementation function
// ------------------------------------------------------------------------------------------------

int mainImpl(int argc, char* argv[], InitOptions&& options);

} // namespace sfz
