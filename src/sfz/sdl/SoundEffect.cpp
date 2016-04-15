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

#include "sfz/sdl/SoundEffect.hpp"

#include <algorithm>
#include <cmath>

#include "sfz/Assert.hpp"
#include "sfz/containers/StackString.hpp"

namespace sfz {

namespace sdl {

// SoundEffect: Constructors & destructors
// ------------------------------------------------------------------------------------------------

SoundEffect::SoundEffect(const char* path) noexcept
{
	// SDL_OpenAudio() must have been called before this

	Mix_Chunk* tmpPtr = Mix_LoadWAV(path);
	if (tmpPtr == NULL) {
		StackString512 tmp;
		tmp.printf("Mix_LoadWAV() failed for \"%s\", error: ", path, Mix_GetError());
		sfz_error(tmp.str);
	} else {
		mChunkPtr = tmpPtr;
	}
}

SoundEffect::SoundEffect(SoundEffect&& other) noexcept
{
	std::swap(this->mChunkPtr, other.mChunkPtr);
}

SoundEffect& SoundEffect::operator= (SoundEffect&& other) noexcept
{
	std::swap(this->mChunkPtr, other.mChunkPtr);
	return *this;
}

SoundEffect::~SoundEffect() noexcept
{
	if (this->mChunkPtr != nullptr) { // The documentation doesn't say anything about freeing nullptr
		Mix_FreeChunk(this->mChunkPtr);
		// Do not use chunk after this without loading a new sample to it.
		// Note: It's a bad idea to free a chunk that is still being played... 
	}
}

// SoundEffect: Public methods
// ------------------------------------------------------------------------------------------------

void SoundEffect::play() noexcept
{
	if (this->mChunkPtr == nullptr) return;
	// Channel to play on, or -1 for the first free unreserved channel. 
	Mix_PlayChannel(-1, this->mChunkPtr, 0);
	// Returns channel being played on, ignore for now
}

void SoundEffect::setVolume(float volume) noexcept
{
	sfz_assert_debug(0.0f <= volume);
	sfz_assert_debug(volume <= 1.0f);

	int volumeInt = int(std::round(volume * MIX_MAX_VOLUME));
	Mix_VolumeChunk(this->mChunkPtr, volumeInt);
}

} // namespace sdl
} // namespace sfz
