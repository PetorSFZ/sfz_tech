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

#include <SDL.h>

namespace sfz {

namespace gl {

/// Enum describing the different types of OpenGL profiles that can be requested.
enum class GLContextProfile : Uint32 { // TODO: Check if actually Uint32
	CORE = SDL_GL_CONTEXT_PROFILE_CORE,
	COMPATIBILITY = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY,
	ES = SDL_GL_CONTEXT_PROFILE_ES
};

/// Wrapper class responsible for creating and destroying a OpenGL context.
class Context final {
public:

	// Public members
	// --------------------------------------------------------------------------------------------

	SDL_GLContext handle;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	// Copying not allowed
	Context(const Context&) = delete;
	Context& operator= (const Context&) = delete;

	Context() noexcept = default;
	Context(Context&& other) noexcept;
	Context& operator= (Context&& other) noexcept;

	Context(SDL_Window* window, int major, int minor, GLContextProfile profile, bool debug = false) noexcept;
	~Context() noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	bool mActive = false;
};

} // namespace gl
} // namespace sfz
