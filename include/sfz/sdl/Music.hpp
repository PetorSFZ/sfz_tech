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

#include "sfz/strings/DynString.hpp"

namespace sfz {

namespace sdl {

// Music class
// ------------------------------------------------------------------------------------------------

/// Class wrapping a Mix_Music from SDL_mixer
class Music final {
public:
	// Constructors functions
	// --------------------------------------------------------------------------------------------

	static Music fromFile(const char* completePath) noexcept;
	static Music fromFileNoLoad(const char* completePath) noexcept;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	Music() noexcept = default;
	Music(const Music&) = delete;
	Music& operator= (const Music&) = delete;

	Music(Music&& other) noexcept;
	Music& operator= (Music&& other) noexcept;
	~Music() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	inline const DynString& filePath() const noexcept { return mFilePath; }
	inline Mix_Music* musicPtr() const noexcept { return mMusicPtr; }

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Loads the music from the specified file. If no path is specified or if the music can't be
	/// loaded an error message will be printed and this function will return false. If this Music
	/// is already loaded then it will first be unloaded and then reloaded.
	bool load() noexcept;

	/// Unloads the music if it is loaded.
	void unload() noexcept;

	inline bool isLoaded() const noexcept { return mMusicPtr != nullptr; }
	inline bool hasPath() const noexcept { return mFilePath.str() != nullptr; }

	/// Plays this music repeated infinitely until it is stopped.
	void play() noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	DynString mFilePath;
	Mix_Music* mMusicPtr = nullptr;
};

// Music functions
// ------------------------------------------------------------------------------------------------

void stopMusic(int fadeOutLengthMs = 0) noexcept;

} // namespace sdl
} // namespace sfz
