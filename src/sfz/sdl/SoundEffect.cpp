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
#include <cstring>

#include "sfz/Assert.hpp"

namespace sfz {

namespace sdl {

// SoundEffect: Constructor functions
// ------------------------------------------------------------------------------------------------

SoundEffect SoundEffect::fromFile(const char* completePath) noexcept
{
	SoundEffect tmp = SoundEffect::fromFileNoLoad(completePath);
	tmp.load();
	return std::move(tmp);
}

SoundEffect SoundEffect::fromFileNoLoad(const char* completePath) noexcept
{
	SoundEffect tmp;
	tmp.mFilePath = DynString(completePath);
	return std::move(tmp);
}

// SoundEffect: Constructors & destructors
// ------------------------------------------------------------------------------------------------

SoundEffect::SoundEffect(SoundEffect&& other) noexcept
{
	std::swap(this->mChunkPtr, other.mChunkPtr);
	this->mFilePath.swap(other.mFilePath);
}

SoundEffect& SoundEffect::operator= (SoundEffect&& other) noexcept
{
	std::swap(this->mChunkPtr, other.mChunkPtr);
	this->mFilePath.swap(other.mFilePath);
	return *this;
}

SoundEffect::~SoundEffect() noexcept
{
	this->unload();
}

// SoundEffect: Public methods
// ------------------------------------------------------------------------------------------------

bool SoundEffect::load() noexcept
{
	// Check if we have a path
	if (mFilePath.str() == nullptr) {
		printErrorMessage("%s", "Attempting to load() sdl::SoundEffect without path.");
		return false;
	}

	// If already loaded we unload first
	if (this->isLoaded()) {
		this->unload();
	}

	Mix_Chunk* tmpPtr = Mix_LoadWAV(mFilePath.str());

	// If load failed we print an error and return false
	if (tmpPtr == NULL) {
		printErrorMessage("Mix_LoadWAV() failed for \"%s\", error: %s",
		                  mFilePath.str(), Mix_GetError());
		return false;
	}
	
	// If load was succesful we return true
	mChunkPtr = tmpPtr;
	return true;
}

void SoundEffect::unload() noexcept
{
	// The documentation doesn't say anything about freeing nullptr
	if (mChunkPtr == nullptr) return;
	// TODO: It's a bad idea to free a chunk that is still being played. Maybe check?
	Mix_FreeChunk(mChunkPtr);
	mChunkPtr = nullptr;
}

void SoundEffect::play() noexcept
{
	if (!this->isLoaded()) return;
	// Channel to play on, or -1 for the first free unreserved channel. 
	Mix_PlayChannel(-1, this->mChunkPtr, 0);
	// Returns channel being played on, ignore for now
}

void SoundEffect::setVolume(float volume) noexcept
{
	sfz_assert_debug(0.0f <= volume);
	sfz_assert_debug(volume <= 1.0f);

	if (!this->isLoaded()) return;

	int volumeInt = int(std::round(volume * MIX_MAX_VOLUME));
	Mix_VolumeChunk(this->mChunkPtr, volumeInt);
}

} // namespace sdl
} // namespace sfz
