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

#include "sfz/sdl/Mouse.hpp"

#include <cstdint>

namespace sfz {

namespace sdl {

using std::uint8_t;

// Static functions
// ------------------------------------------------------------------------------------------------

static ButtonState* buttonStatePtr(Mouse& mouse, uint8_t button) noexcept
{
	switch (button) {
	case SDL_BUTTON_LEFT: return &mouse.leftButton;
	case SDL_BUTTON_RIGHT: return &mouse.rightButton;
	case SDL_BUTTON_MIDDLE: return &mouse.middleButton;
	
	case SDL_BUTTON_X1:
	case SDL_BUTTON_X2:
	default:
		return nullptr;
	}
}

// Mouse: Public methods
// ------------------------------------------------------------------------------------------------

void Mouse::update(const Window& window, const DynArray<SDL_Event>& events) noexcept
{
	// Pre-processing
	// Changes previous DOWN state to HELD state.
	if (leftButton == ButtonState::DOWN) leftButton = ButtonState::HELD;
	if (rightButton == ButtonState::DOWN) rightButton = ButtonState::HELD;
	if (middleButton == ButtonState::DOWN) middleButton = ButtonState::HELD;
	// Changes previous UP state to NOT_PRESSED state.
	if (leftButton == ButtonState::UP) leftButton = ButtonState::NOT_PRESSED;
	if (rightButton == ButtonState::UP) rightButton = ButtonState::NOT_PRESSED;
	if (middleButton == ButtonState::UP) middleButton = ButtonState::NOT_PRESSED;
	// Resetting motion and wheel
	motion = vec2{0.0f, 0.0f};
	wheel = vec2{0.0f, 0.0f};

	const float scale = 1.0f / (float)window.width();

	// Process events
	for (const SDL_Event& event : events) {
		switch (event.type) {
		case SDL_MOUSEMOTION:
			motion = vec2{(float)event.motion.xrel, (float)-event.motion.yrel} * scale;
			break;
		case SDL_MOUSEBUTTONDOWN:
			{
				ButtonState* buttonPtr = buttonStatePtr(*this, event.button.button);
				if (buttonPtr != nullptr) *buttonPtr = ButtonState::DOWN;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			{
				ButtonState* buttonPtr = buttonStatePtr(*this, event.button.button);
				if (buttonPtr != nullptr) *buttonPtr = ButtonState::UP;
			}
			break;
		case SDL_MOUSEWHEEL:
			wheel = vec2{(float)event.wheel.x, (float)event.wheel.y};
			if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) wheel.y = -wheel.y;
			break;

		default:
			// Do nothing
			break;
		}
	}

	// Get raw position
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	position = vec2{(float)mouseX, (float)(window.height() - mouseY)} * scale;
}

Mouse Mouse::scaleMouse(vec2 camPos, vec2 camDim) const noexcept
{
	return this->scaleMouse(AABB2D{camPos, camDim});
}

Mouse Mouse::scaleMouse(const AABB2D& cam) const noexcept
{
	Mouse temp = *this;
	float camWidth = cam.width();
	temp.position = cam.min + (temp.position * camWidth);
	temp.motion = temp.motion * camWidth;
	return temp;
}

} // namespace sdl
} // namespace sfz
