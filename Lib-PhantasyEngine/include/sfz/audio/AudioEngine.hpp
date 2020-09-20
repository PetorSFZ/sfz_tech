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

#include <skipifzero.hpp>
#include <skipifzero_strings.hpp>

namespace sfz {

// AudioEngine
// ------------------------------------------------------------------------------------------------

struct AudioEngineState; // Pimpl pattern

class AudioEngine final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	AudioEngine() noexcept = default;
	AudioEngine(const AudioEngine&) = delete;
	AudioEngine& operator= (const AudioEngine&) = delete;
	AudioEngine(AudioEngine&& o) noexcept { this->swap(o); }
	AudioEngine& operator= (AudioEngine&& o) noexcept { this->swap(o); return *this; }
	~AudioEngine() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	bool active() const noexcept { return mState != nullptr; }
	bool init(Allocator* allocator) noexcept;
	void swap(AudioEngine& other) noexcept;
	void destroy() noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void renderDebugUI() noexcept;




	// Private members
	// --------------------------------------------------------------------------------------------
private:

	AudioEngineState* mState = nullptr;
};

} // namespace sfz
