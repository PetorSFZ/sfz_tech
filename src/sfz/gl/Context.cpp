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

#include "sfz/gl/Context.hpp"

#include <algorithm>

#include "sfz/Assert.hpp"

namespace sfz {

namespace gl {

Context::Context(Context&& other) noexcept
{
	std::swap(this->mActive, other.mActive);
}

Context& Context::operator= (Context&& other) noexcept
{
	std::swap(this->mActive, other.mActive);
	return *this;
}

Context::Context(SDL_Window* window, int major, int minor, GLContextProfile profile, bool debug) noexcept
:
	mActive(true)
{
	// Set context attributes
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major) < 0) {
		error("Failed to set gl context major version: %s", SDL_GetError());
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor) < 0) {
		error("Failed to set gl context minor version: %s", SDL_GetError());
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, int(Uint32(profile))) < 0) {
		error("Failed to set gl context profile: %s", SDL_GetError());
	}

	// Set debug context if requested
	if (debug) {
		if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) < 0) {
			printErrorMessage("Failed to request debug context: %s", SDL_GetError());
		}
	}

	handle = SDL_GL_CreateContext(window);
	if (handle == NULL) {
		error("Failed to create GL context: %s", + SDL_GetError());
	}
}

Context::~Context() noexcept
{
	if (mActive) SDL_GL_DeleteContext(handle);
	mActive = false;
}

} // namespace gl
} // namespace sfz
