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

// Music: Constructor functions
// ------------------------------------------------------------------------------------------------

Music Music::fromFile(const char* completePath) noexcept
{
	Music tmp = Music::fromFileNoLoad(completePath);
	tmp.load();
	return std::move(tmp);
}

Music Music::fromFileNoLoad(const char* completePath) noexcept
{
	Music tmp;
	tmp.mFilePath = DynString(completePath);
	return std::move(tmp);
}

// Music: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Music::Music(Music&& other) noexcept
{
	std::swap(this->mMusicPtr, other.mMusicPtr);
	this->mFilePath.swap(other.mFilePath);
}

Music& Music::operator= (Music&& other) noexcept
{
	std::swap(this->mMusicPtr, other.mMusicPtr);
	this->mFilePath.swap(other.mFilePath);
	return *this;
}

Music::~Music() noexcept
{
	this->unload();
}

// Music: Public methods
// ------------------------------------------------------------------------------------------------

bool Music::load() noexcept
{
	// Check if we have a path
	if (mFilePath.str() == nullptr) {
		printErrorMessage("%s", "Attempting to load sdl::Music without path.");
		return false;
	}

	// If already loaded we unload first
	if (this->isLoaded()) {
		this->unload();
	}

	Mix_Music* tmpPtr = Mix_LoadMUS(mFilePath.str());

	// If load failed we print an error and return false
	if (tmpPtr == NULL) {
		printErrorMessage("Mix_LoadMUS() failed for: %s, error: %s",
		                   mFilePath.str(), Mix_GetError());
		return false;
	}
	
	// If load was succesful we return true
	mMusicPtr = tmpPtr;
	return true;
}

void Music::unload() noexcept
{
	// The documentation doesn't say anything about freeing nullptr
	if (mMusicPtr == nullptr) return;
	// Note, if music is fading out this will block until it is complete
	Mix_FreeMusic(mMusicPtr);
	mMusicPtr = nullptr;
}

void Music::play() noexcept
{
	if (this->mMusicPtr == nullptr) return;
	// TODO: 2nd argument is number of loops. -1 is forever, 0 is 0 times.
	int res = Mix_PlayMusic(this->mMusicPtr, -1);
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
