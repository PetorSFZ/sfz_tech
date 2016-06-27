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

#include <cstdint> // uint8_t, int32_t

#include <SDL.h>

#include "sfz/containers/DynArray.hpp"
#include "sfz/containers/HashMap.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/sdl/ButtonState.hpp"

namespace sfz {

namespace sdl {

using std::int32_t;

/// Struct used for representing the state of a GameController at a given point in time.
struct GameControllerState {
	ButtonState a = ButtonState::NOT_PRESSED;
	ButtonState b = ButtonState::NOT_PRESSED;
	ButtonState x = ButtonState::NOT_PRESSED;
	ButtonState y = ButtonState::NOT_PRESSED;

	float stickDeadzone = 0.15f;
	float triggerDeadzone = 0.05f;

	vec2 leftStick; // Approximate range (length of vector): [0.0f, 1.0f]
	vec2 rightStick; // Approximate range (length of vector): [0.0f, 1.0f]
	ButtonState leftStickButton = ButtonState::NOT_PRESSED;
	ButtonState rightStickButton = ButtonState::NOT_PRESSED;

	ButtonState leftShoulder = ButtonState::NOT_PRESSED;
	ButtonState rightShoulder = ButtonState::NOT_PRESSED;
	float leftTrigger; // Range: (not-pressed) [0.0f, 1.0f] (fully-pressed)
	float rightTrigger; // Range: (not-pressed) [0.0f, 1.0f] (fully-pressed)

	ButtonState padUp = ButtonState::NOT_PRESSED;
	ButtonState padDown = ButtonState::NOT_PRESSED;
	ButtonState padLeft = ButtonState::NOT_PRESSED;
	ButtonState padRight = ButtonState::NOT_PRESSED;

	ButtonState start = ButtonState::NOT_PRESSED;
	ButtonState back = ButtonState::NOT_PRESSED;
	ButtonState guide = ButtonState::NOT_PRESSED;
};

/// Class used for managing an SDL GameController and its state.
class GameController final : public GameControllerState {
public:
	// Getters
	// --------------------------------------------------------------------------------------------

	inline SDL_GameController* gameControllerPtr() const noexcept { return mGameControllerPtr; }
	inline int32_t id() const noexcept { return mID; } // Unique static identifier

	// Public methods
	// --------------------------------------------------------------------------------------------

	GameControllerState state() const noexcept;

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	GameController() noexcept;
	GameController(const GameController&) = delete;
	GameController& operator= (const GameController&) = delete;
	GameController(GameController&& other) noexcept;
	GameController& operator= (GameController&& other) noexcept;

	/// 0 <= deviceIndex < SDL_NumJoysticks()
	GameController(int deviceIndex) noexcept;
	~GameController() noexcept;

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	SDL_GameController* mGameControllerPtr;
	int32_t mID; // The SDL joystick id of this controller, used to identify.
};

// Update functions to update GameController struct
// ------------------------------------------------------------------------------------------------

void update(HashMap<int32_t, GameController>& controllers, const DynArray<SDL_Event>& events) noexcept;

} // namespace sdl
} // namespace sfz
