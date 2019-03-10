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

// Initialization functions
// ------------------------------------------------------------------------------------------------

// Helper methods for initializing/deinitializing SDL2 and creating a window. The samples do not
// aim to teach SDL2, and the user of ZeroG might not be using SDL2 in the first place.
SDL_Window* initializeSdl2CreateWindow(const char* sampleName) noexcept;
void cleanupSdl2(SDL_Window* window) noexcept;

// A function that given an SDL2 window returns the platform specific native window handle, in
// the form of a void pointer which can be passed to ZeroG.
void* getNativeWindowHandle(SDL_Window* window) noexcept;


