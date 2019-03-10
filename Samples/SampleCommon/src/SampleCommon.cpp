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

#include "SampleCommon.hpp"

#include <cstdio>
#include <exception>

#include <SDL_syswm.h>

// Statics
// ------------------------------------------------------------------------------------------------

static HWND getWin32WindowHandle(SDL_Window* window) noexcept
{
	SDL_SysWMinfo info = {};
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info)) return nullptr;
	return info.info.win.window;
}

// Initialization functions
// ------------------------------------------------------------------------------------------------

SDL_Window* initializeSdl2CreateWindow(const char* sampleName) noexcept
{
	// Init SDL2
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init() failed: %s", SDL_GetError());
		fflush(stdout);
		std::terminate();
	}

	// Window
	SDL_Window* window = SDL_CreateWindow(
		sampleName,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 800,
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		printf("SDL_CreateWindow() failed: %s\n", SDL_GetError());
		fflush(stdout);
		SDL_Quit();
		std::terminate();
	}

	return window;
}

void cleanupSdl2(SDL_Window* window) noexcept
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void* getNativeWindowHandle(SDL_Window* window) noexcept
{
#ifdef WIN32
	return getWin32WindowHandle(window);
#else
#error "Not implemented yet"
#endif
}
