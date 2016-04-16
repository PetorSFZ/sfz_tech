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

#include "sfz/sdl/Window.hpp"

#include <cstdio>
#include <algorithm>

#include "sfz/Assert.hpp"

namespace sfz {

namespace sdl {

namespace {

Uint32 processFlags(const initializer_list<WindowFlags>& flags) noexcept
{
	Uint32 flag = 0;
	for (WindowFlags tempFlag : flags) {
		flag = flag | static_cast<Uint32>(tempFlag);
	}
	return flag;
}

SDL_Window* createWindow(const char* title, int width, int height, Uint32 flags) noexcept
{
	SDL_Window* window = NULL;
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                          width, height, flags);
	if (window == NULL) {
		sfz::error("SDL_CreateWindow() failed: %s", SDL_GetError());
	}
	return window;
}

} // namespace

// Window: Constructors and destructors
// ------------------------------------------------------------------------------------------------

Window::Window(Window&& other) noexcept
{
	std::swap(this->mPtr, other.mPtr);
}

Window& Window::operator= (Window&& other) noexcept
{
	std::swap(this->mPtr, other.mPtr);
	return *this;
}

Window::Window(const char* title, int width, int height,
               initializer_list<WindowFlags> flags) noexcept
:
	mPtr{createWindow(title, width, height, processFlags(flags))}
{ }

Window::~Window() noexcept
{
	if (mPtr != nullptr) {
		SDL_DestroyWindow(mPtr);
	}
}

// Window: Getters
// ------------------------------------------------------------------------------------------------

SDL_Surface* Window::surfacePtr() const noexcept
{
	return SDL_GetWindowSurface(mPtr);
}

int Window::width() const noexcept
{
	int width;
	SDL_GetWindowSize(mPtr, &width, nullptr);
	return width;
}

int Window::height() const noexcept
{
	int height;
	SDL_GetWindowSize(mPtr, nullptr, &height);
	return height;
}

vec2i Window::dimensions() const noexcept
{
	int width, height;
	SDL_GetWindowSize(mPtr, &width, &height);
	return vec2i{width, height};
}

vec2 Window::dimensionsFloat() const noexcept
{
	int width, height;
	SDL_GetWindowSize(mPtr, &width, &height);
	return vec2{static_cast<float>(width), static_cast<float>(height)};
}

int Window::drawableWidth() const noexcept
{
	int width;
	SDL_GL_GetDrawableSize(mPtr, &width, nullptr);
	return width;
}

int Window::drawableHeight() const noexcept
{
	int height;
	SDL_GL_GetDrawableSize(mPtr, nullptr, &height);
	return height;
}

vec2i Window::drawableDimensions() const noexcept
{
	int width, height;
	SDL_GL_GetDrawableSize(mPtr, &width, &height);
	return vec2i{width, height};
}

vec2 Window::drawableDimensionsFloat() const noexcept
{
	int width, height;
	SDL_GL_GetDrawableSize(mPtr, &width, &height);
	return vec2{static_cast<float>(width), static_cast<float>(height)};
}

// Window: Setters
// ------------------------------------------------------------------------------------------------

void Window::setSize(int width, int height) noexcept
{
	sfz_assert_debug(width > 0);
	sfz_assert_debug(height > 0);
	SDL_SetWindowSize(mPtr, width, height);
}

void Window::setSize(vec2i dimensions) noexcept
{
	this->setSize(dimensions.x, dimensions.y);
}

void Window::setVSync(VSync mode) noexcept
{
	int vsyncInterval = 0;
	switch (mode) {
	case VSync::OFF:
		vsyncInterval = 0;
		break;
	case VSync::ON:
		vsyncInterval = 1;
		break;
	case VSync::SWAP_CONTROL_TEAR:
		vsyncInterval = -1;
		break;
	default:
		sfz_assert_release(false);
	}
	if (SDL_GL_SetSwapInterval(vsyncInterval) < 0) {
		sfz::printErrorMessage("SDL_GL_SetSwapInterval() failed: %s", SDL_GetError());
	}
}

void Window::setFullscreen(Fullscreen mode, int displayIndex) noexcept
{
	Uint32 fullscreenFlags = 0;
	switch (mode) {
	case Fullscreen::OFF:
		fullscreenFlags = 0;
		break;
	case Fullscreen::WINDOWED:
		fullscreenFlags = SDL_WINDOW_FULLSCREEN_DESKTOP;
		break;
	case Fullscreen::EXCLUSIVE:
		{
			// Acquiring the display index to use
			const int numDisplays = SDL_GetNumVideoDisplays();
			if (numDisplays < 0) sfz::printErrorMessage("SDL_GetNumVideoDisplays() failed: %s", SDL_GetError());
			if (displayIndex >= numDisplays) {
				sfz::printErrorMessage("Invalid display index (%i), using 0 instead.", displayIndex);
				displayIndex = 0;
			}
			// -1 Means that the user wants the currently used screen
			if (displayIndex == -1) {
				displayIndex = SDL_GetWindowDisplayIndex(mPtr);
				if (displayIndex < 0) {
					sfz::printErrorMessage("SDL_GetWindowDisplayIndex() failed: %s\n, using 0 instead.", SDL_GetError());
					displayIndex = 0;
				}
			}

			// Gets and sets the display mode to that of the wanted screen
			SDL_DisplayMode desktopDisplayMode;
			if (SDL_GetDesktopDisplayMode(displayIndex, &desktopDisplayMode) < 0) {
				sfz::error("SDL_GetDesktopDisplayMode() failed: %s", SDL_GetError());
			}
			if (SDL_SetWindowDisplayMode(mPtr, &desktopDisplayMode) < 0) {
				sfz::error("SDL_SetWindowDisplayMode() failed: %s", SDL_GetError());
			}
		}
		fullscreenFlags = SDL_WINDOW_FULLSCREEN;
		break;
	case Fullscreen::EXCLUSIVE_KEEP_CURRENT_DISPLAY_MODE:
		fullscreenFlags = SDL_WINDOW_FULLSCREEN;
		break;
	default:
		sfz_assert_release(false);
	}
	if (SDL_SetWindowFullscreen(mPtr, fullscreenFlags) < 0) {
		sfz::error("SDL_SetWindowFullscreen() failed: %s", SDL_GetError());
	}
}

// Functions
// ------------------------------------------------------------------------------------------------

DynArray<vec2i> getAvailableResolutions() noexcept
{
	// Get number of displays
	const int numDisplays = SDL_GetNumVideoDisplays();
	if (numDisplays < 0) {
		sfz::error("SDL_GetNumVideoDisplays() failed: %s", SDL_GetError());
	}

	// Get all resolutions
	DynArray<vec2i> resolutions;
	SDL_DisplayMode mode;
	for (int index = 0; index < numDisplays; ++index) {
		int numDisplayModes = SDL_GetNumDisplayModes(index);
		if (numDisplayModes < 0) {
			sfz::error("SDL_GetNumDisplayModes() failed: %s", SDL_GetError());
		}
		for (int i = 0; i < numDisplayModes; ++i) {
			mode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};
			if (SDL_GetDisplayMode(index, i, &mode) != 0) {
				sfz::error("SDL_GetDisplayMode() failed: %s\n", SDL_GetError());
			}
			resolutions.add(vec2i{mode.w, mode.h});
		}
	}

	// Sort by vertical resolution
	std::sort(resolutions.begin(), resolutions.end(), [](vec2i lhs, vec2i rhs) {
		return lhs.y < rhs.y;
	});

	// Remove duplicates
	for (uint32_t i = 1; i < resolutions.size(); ++i) {
		if (resolutions[i] == resolutions[i-1]) {
			resolutions.remove(i);
			i -= 1;
		}
	}

	return std::move(resolutions);
}

} // namespace sdl
} // namespace sfz
