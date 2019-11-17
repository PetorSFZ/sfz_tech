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

// This header is used to include OpenGL into a file. On some platforms it includes GLEW, on some
// it uses SDL headers, etc.


// Emscripten and iOS verison
#if defined(__EMSCRIPTEN__) || defined(SFZ_IOS)

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES // This seems to enable extension use. Dunno if this is how to thing.
#include <SDL_opengles2.h>


// Desktop version
#else

// windows.h on Windows
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#endif

// GLEW
#include <GL/glew.h>

#endif
