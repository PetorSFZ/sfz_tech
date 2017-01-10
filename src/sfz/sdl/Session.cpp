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

#include "sfz/sdl/Session.hpp"

#include <algorithm>

#include "sfz/Assert.hpp"

namespace sfz {

namespace sdl {

// Session: Constructors & destructors
// ------------------------------------------------------------------------------------------------

Session::Session(Session&& other) noexcept
{
	std::swap(this->mActive, other.mActive);
}

Session& Session::operator= (Session&& other) noexcept
{
	std::swap(this->mActive, other.mActive);
	return *this;
}

Session::Session(initializer_list<SDLInitFlags> sdlInitFlags) noexcept
:
	mActive{true}
{
	// Initialize SDL2
	Uint32 sdlInitFlag = 0;
	for (SDLInitFlags tempFlag : sdlInitFlags) {
		sdlInitFlag = sdlInitFlag | static_cast<Uint32>(tempFlag);
	}
	if (SDL_Init(sdlInitFlag) < 0) {
		sfz::error("SDL_Init() failed: %s", SDL_GetError());
	}
}
	
Session::~Session() noexcept
{
	if (mActive) {
		// Cleanup SDL2
		SDL_Quit();
	}
	mActive = false;
}

} // namespace sdl
} // namespace sfz
