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

#include <sfz/memory/SmartPointers.hpp>

#include "ph/game_loop/GameLoopUpdateable.hpp"

namespace ph {

using sfz::UniquePtr;


// Phantasy Engine main macro
// ------------------------------------------------------------------------------------------------

/// This is used to initialize PhantasyEngine.
/// The "Main.cpp" file for your project should essentially only include this header and call this
/// macro.
/// \param createInitialUpdateable a function pointer to a function that returns a sfz::UniquePtr
/// holding a GameLoopUpdateable. This function is only called once right before the game loop is
/// started.
#define PHANTASY_ENGINE_MAIN(createInitialUpdateable) \
	int main(int argc, char* argv[]) \
	{ \
		return ph::mainImpl(argc, argv, (createInitialUpdateable)); \
	}

// Implementation function
// ------------------------------------------------------------------------------------------------

int mainImpl(int argc, char* argv[], UniquePtr<GameLoopUpdateable>(*createInitialUpdateable)(void));

} // namespace ph
