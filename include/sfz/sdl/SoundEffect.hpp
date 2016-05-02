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

#include "sfz/containers/DynString.hpp"

namespace sfz {

namespace sdl {

// SoundEffect class
// ------------------------------------------------------------------------------------------------

/// Class wrapping a Mix_Chunk from SDL_mixer.
class SoundEffect final {
public:
	// Constructors functions
	// --------------------------------------------------------------------------------------------
	
	static SoundEffect fromFile(const char* completePath) noexcept;
	static SoundEffect fromFileNoLoad(const char* completePath) noexcept;

	// Constructors and destructors
	// --------------------------------------------------------------------------------------------

	SoundEffect() noexcept = default;
	SoundEffect(const SoundEffect&) = delete;
	SoundEffect& operator= (const SoundEffect&) = delete;

	SoundEffect(SoundEffect&& other) noexcept;
	SoundEffect& operator= (SoundEffect&& other) noexcept;
	~SoundEffect() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	inline const DynString& filePath() const noexcept { return mFilePath; }
	inline Mix_Chunk* chunkPtr() const noexcept { return mChunkPtr; }

	// Public methods
	// --------------------------------------------------------------------------------------------

	/// Loads the sound effect from the specified file. If no path is specified or if the sound
	/// effect can't be loaded an error message will be printed and this function will return
	/// false. If this SoundEffect is already loaded then it will first be unloaded and then
	/// reloaded.
	bool load() noexcept;
	
	/// Unloads the sound effect if it is loaded.
	void unload() noexcept;

	inline bool isLoaded() const noexcept { return mChunkPtr != nullptr; }
	inline bool hasPath() const noexcept { return mFilePath.str() != nullptr; }

	/// Plays sound effect (if loaded) on first free unreserved channel.
	void play() noexcept;

	/// Sets the volume of this sound effect, range [0, 1].
	void setVolume(float volume) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	DynString mFilePath;
	Mix_Chunk* mChunkPtr = nullptr;
};

} // namespace sdl
} // namespace sfz
