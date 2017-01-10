// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include <initializer_list>

#include <SDL.h>

namespace sfz {

namespace sdl {

using std::initializer_list;

// Enums
// ------------------------------------------------------------------------------------------------

/// SDL2 init flags (https://wiki.libsdl.org/SDL_Init)
enum class SDLInitFlags : Uint32 {
	TIMER = SDL_INIT_TIMER,
	AUDIO = SDL_INIT_AUDIO,
	VIDEO = SDL_INIT_VIDEO,
	JOYSTICK = SDL_INIT_JOYSTICK,
	HAPTIC = SDL_INIT_HAPTIC,
	GAMECONTROLLER = SDL_INIT_GAMECONTROLLER,
	EVENTS = SDL_INIT_EVENTS,
	EVERYTHING = SDL_INIT_EVERYTHING,
	NOPARACHUTE = SDL_INIT_NOPARACHUTE
};

// Session class
// ------------------------------------------------------------------------------------------------

/// Initializes SDL2 upon construction and cleans up upon destruction. This object must be kept
/// alive as long as SDL is used.
///
/// https://wiki.libsdl.org/SDL_Init
/// https://wiki.libsdl.org/SDL_Quit
class Session final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	// Copying not allowed
	Session(const Session&) = delete;
	Session& operator= (const Session&) = delete;

	Session() noexcept = default;
	Session(Session&& other) noexcept;
	Session& operator= (Session&& other) noexcept;

	/// Initializes SDL2 with the specified flags
	Session(initializer_list<SDLInitFlags> sdlInitFlags) noexcept;
	~Session() noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	bool mActive = false;
};

} // namespace sdl
} // namespace sfz
