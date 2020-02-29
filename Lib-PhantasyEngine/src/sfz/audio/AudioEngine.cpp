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

#include "sfz/audio/AudioEngine.hpp"

namespace sfz {

// AudioEngineState
// ------------------------------------------------------------------------------------------------

struct AudioEngineState final {

	Allocator* allocator = nullptr;
};

// AudioEngine:: State methods
// ------------------------------------------------------------------------------------------------

bool AudioEngine::init(Allocator* allocator) noexcept
{
	this->destroy();
	mState = allocator->newObject<AudioEngineState>(sfz_dbg(""));
	mState->allocator = allocator;
	return true;
}

void AudioEngine::swap(AudioEngine& other) noexcept
{
	std::swap(this->mState, other.mState);
}

void AudioEngine::destroy() noexcept
{
	if (mState == nullptr) return;
	Allocator* allocator = mState->allocator;
	allocator->deleteObject(mState);
	mState = nullptr;
}

} // namespace sfz
