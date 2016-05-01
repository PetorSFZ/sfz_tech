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

#include "sfz/sdl/Music.hpp"

#include <algorithm>

#include "sfz/Assert.hpp"

namespace sfz {

namespace sdl {

// Music: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Music::Music(const char* path) noexcept
{
	Mix_Music* tmpPtr = Mix_LoadMUS(path);
	if (tmpPtr == NULL) {
		sfz::printErrorMessage("Mix_LoadMUS() failed for: %s, error: %s", path, Mix_GetError());
	} else {
		this->mPtr = tmpPtr;
	}
}

Music::Music(Music&& other) noexcept
{
	std::swap(this->mPtr, other.mPtr);
}

Music& Music::operator= (Music&& other) noexcept
{
	std::swap(this->mPtr, other.mPtr);
	return *this;
}

Music::~Music() noexcept
{
	if (this->mPtr != nullptr) { // The documentation doesn't say anything about freeing nullptr
		Mix_FreeMusic(mPtr); // Note, if music is fading out this will block until it is complete
	}
}

// Music: Public methods
// ------------------------------------------------------------------------------------------------

void Music::play() noexcept
{
	if (this->mPtr == nullptr) return;
	// TODO: 2nd argument is number of loops. -1 is forever, 0 is 0 times.
	int res = Mix_PlayMusic(this->mPtr, -1);
	if (res == -1) {
		sfz::printErrorMessage("Mix_PlayMusic() failed, error: %s", Mix_GetError());
	}
}

// Music functions
// ------------------------------------------------------------------------------------------------

void stopMusic(int fadeOutLengthMs) noexcept
{
	if (Mix_PlayingMusic()) {
		if (fadeOutLengthMs <= 0) {
			Mix_HaltMusic();
			return;
		}

		if (Mix_FadeOutMusic(fadeOutLengthMs) == 0) {
			sfz::printErrorMessage("Mix_FadeOutMusic() failed, error: %s", Mix_GetError());
		}
	}
}

} // namespace sdl
} // namespace sfz
