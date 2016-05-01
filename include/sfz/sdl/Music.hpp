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

#include <SDL_mixer.h>

namespace sfz {

namespace sdl {

// Music class
// ------------------------------------------------------------------------------------------------

class Music final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Music() noexcept = default;
	Music(const Music&) = delete;
	Music& operator= (const Music&) = delete;

	Music(const char* path) noexcept;
	Music(Music&& other) noexcept;
	Music& operator= (Music&& other) noexcept;
	~Music() noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	void play() noexcept;

private:
	// Public members
	// --------------------------------------------------------------------------------------------

	Mix_Music* mPtr = nullptr;
};

// Music functions
// ------------------------------------------------------------------------------------------------

void stopMusic(int fadeOutLengthMs = 0) noexcept;

} // namespace sdl
} // namespace sfz
