// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>

// Forward declare SDL_GameController
extern "C" {
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;
}

namespace sfz {

// RawInputState
// ------------------------------------------------------------------------------------------------

constexpr uint32_t MAX_NUM_SCANCODES = 512;

struct KeyboardState final {
	// Array indexed with SDL_SCANCODE's. 1 if key is pressed, 0 otherwise.
	uint8_t scancodes[MAX_NUM_SCANCODES] = {};
};

struct MouseState final {
	vec2_u32 windowDims = vec2_u32(0u); // Position and delta is in range [0, windowDims]
	vec2_u32 pos = vec2_u32(0u); // [0, 0] in bottom left corner
	vec2_i32 delta = vec2_i32(0); // Delta mouse has moved since last frame
	vec2_i32 wheel = vec2_i32(0); // Pos-y "up", neg-y "down", but can vary with touchpads
	uint8_t left = 0;
	uint8_t middle = 0;
	uint8_t right = 0;
};

constexpr uint32_t GPD_NONE = 0;

constexpr uint32_t GPD_A = 1;
constexpr uint32_t GPD_B = 2;
constexpr uint32_t GPD_X = 3;
constexpr uint32_t GPD_Y = 4;

constexpr uint32_t GPD_BACK = 5;
constexpr uint32_t GPD_START = 6;

constexpr uint32_t GPD_LS = 7; // Left stick click
constexpr uint32_t GPD_RS = 8; // Right stick click

constexpr uint32_t GPD_LB = 9; // Left shoulder button
constexpr uint32_t GPD_RB = 10; // Right shoulder button

constexpr uint32_t GPD_LT = 11; // Left trigger button (sort of hack, also available as analog)
constexpr uint32_t GPD_RT = 12; // Right trigger button (sort of hack, also available as analog)

constexpr uint32_t GPD_DPAD_UP = 13;
constexpr uint32_t GPD_DPAD_DOWN = 14;
constexpr uint32_t GPD_DPAD_LEFT = 15;
constexpr uint32_t GPD_DPAD_RIGHT = 16;

constexpr uint32_t GPD_MAX_NUM_BUTTONS = 17;

// The approximate dead zone (as specified by SDL2) for gamepad sticks.
constexpr float GPD_STICK_APPROX_DEADZONE = float(8000) / float(INT16_MAX);

struct GamepadState final {

	// Unique ID for this gamepad. Starts at 0, -1 is invalid. If the gamepad is disconnected
	// and reconnected it will get a new id. Corresponds to SDL_JoystickInstanceID().
	int32_t id = -1;

	// Pointer to the SDL_GameController this state corresponds to. Mainly available for rumble
	// purposes, you are not generally supposed to look at this.
	SDL_GameController* controller = nullptr;

	// Sticks are in range [-1, 1]. Do not however that no deadzone has been applied. Stick's
	// neutral should be somewhere in the range ~[-0.24, 0.24], but this will vary from gamepad to
	// gamepad.
	vec2 leftStick = vec2(0.0f);
	vec2 rightStick = vec2(0.0f);

	float lt = 0.0f;
	float rt = 0.0f;

	// Array indexed with constants above. 1 if button is pressed, 0 otherwise.
	uint8_t buttons[GPD_MAX_NUM_BUTTONS] = {};
};

struct TouchState final {
	int64_t id = -1;
	vec2 pos = vec2(0.0f); // Range [0, 1]
	float pressure = 0.0f; // Range [0, 1]. Haven't found anything that activates it, avoid using?
};

struct RawInputState final {
	vec2_u32 windowDims = vec2_u32(0u);
	KeyboardState kb;
	MouseState mouse;
	Arr6<GamepadState> gamepads;
	Arr8<TouchState> touches;
};

}
