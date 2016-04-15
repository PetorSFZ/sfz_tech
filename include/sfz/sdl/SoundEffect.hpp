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

// SoundEffect class
// ------------------------------------------------------------------------------------------------

class SoundEffect final {
public:
	// Constructors and destructors
	// --------------------------------------------------------------------------------------------

	SoundEffect() noexcept = default;
	SoundEffect(const SoundEffect&) = delete;
	SoundEffect& operator= (const SoundEffect&) = delete;

	SoundEffect(const char* path) noexcept;
	SoundEffect(SoundEffect&& other) noexcept;
	SoundEffect& operator= (SoundEffect&& other) noexcept;
	~SoundEffect() noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	void play() noexcept;

	/// Sets the volume of this sound effect, range [0, 1].
	void setVolume(float volume) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	Mix_Chunk* mChunkPtr = nullptr;
};

} // namespace sdl
} // namespace sfz
