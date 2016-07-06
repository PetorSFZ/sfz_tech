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

#include <initializer_list>

#include <SDL.h>

#include <sfz/containers/DynArray.hpp>
#include <sfz/math/Vector.hpp>

namespace sfz {

namespace sdl {

using std::initializer_list;

// Enums
// ------------------------------------------------------------------------------------------------

/// Enum wrapper for SDL_WindowFlags (https://wiki.libsdl.org/SDL_WindowFlags)
enum class WindowFlags : Uint32 {
	FULLSCREEN = SDL_WINDOW_FULLSCREEN,
	FULLSCREEN_DESKTOP = SDL_WINDOW_FULLSCREEN_DESKTOP,
	OPENGL = SDL_WINDOW_OPENGL,
	SHOWN = SDL_WINDOW_SHOWN,
	HIDDEN = SDL_WINDOW_HIDDEN,
	BORDERLESS = SDL_WINDOW_BORDERLESS,
	RESIZABLE = SDL_WINDOW_RESIZABLE,
	MINIMIZED = SDL_WINDOW_MINIMIZED,
	MAXIMIZED = SDL_WINDOW_MAXIMIZED,
	INPUT_GRABBED = SDL_WINDOW_INPUT_GRABBED,
	INPUT_FOCUS = SDL_WINDOW_INPUT_FOCUS,
	MOUSE_FOCUS = SDL_WINDOW_MOUSE_FOCUS,
	FOREIGN = SDL_WINDOW_FOREIGN,
	ALLOW_HIGHDPI = SDL_WINDOW_ALLOW_HIGHDPI,
	MOUSE_CAPTURE = SDL_WINDOW_MOUSE_CAPTURE
};

enum class VSync : uint8_t {
	OFF = 0,
	ON = 1,
	SWAP_CONTROL_TEAR = 2 // See https://www.opengl.org/registry/specs/EXT/wgl_swap_control_tear.txt
};

enum class Fullscreen : uint8_t {
	OFF = 0,
	WINDOWED = 1,
	EXCLUSIVE = 2,
	EXCLUSIVE_KEEP_CURRENT_DISPLAY_MODE = 3
};

// Window class
// ------------------------------------------------------------------------------------------------

/// Class responsible for creating, holding and destroying an SDL window
/// https://wiki.libsdl.org/SDL_CreateWindow
/// https://wiki.libsdl.org/SDL_DestroyWindow
class Window final {
public:
	// Constructors and destructors
	// --------------------------------------------------------------------------------------------

	// Copying not allowed
	Window(const Window&) = delete;
	Window& operator= (const Window&) = delete;

	Window() noexcept = default;
	Window(Window&& other) noexcept;
	Window& operator= (Window&& other) noexcept;

	Window(const char* title, int width, int height,
	       initializer_list<WindowFlags> flags) noexcept;
	~Window() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	inline SDL_Window* ptr() const noexcept { return mPtr; }
	SDL_Surface* surfacePtr() const noexcept;

	int width() const noexcept;
	int height() const noexcept;
	vec2i dimensions() const noexcept;
	vec2 dimensionsFloat() const noexcept;

	int drawableWidth() const noexcept;
	int drawableHeight() const noexcept;
	vec2i drawableDimensions() const noexcept;
	vec2 drawableDimensionsFloat() const noexcept;

	// Setters
	// --------------------------------------------------------------------------------------------

	void setSize(int width, int height) noexcept;
	void setSize(vec2i dimensions) noexcept;
	void setVSync(VSync mode) noexcept;

	/// Sets fullscreen mode
	/// Display index is only used for Fullscreen:EXCLUSIVE. Default (-1) means that the current
	/// display of the window will be used.
	void setFullscreen(Fullscreen mode, int displayIndex = -1) noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------
	
	SDL_Window* mPtr = nullptr;
};

// Functions
// ------------------------------------------------------------------------------------------------

DynArray<vec2i> getAvailableResolutions() noexcept;

} // namespace sdl
} // namespace sfz
