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
#include <skipifzero_math.hpp>

// Forward declare SDL_GameController
extern "C" {
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;
}

namespace sfz {

// RawInputState
// ------------------------------------------------------------------------------------------------

constexpr u32 MAX_NUM_SCANCODES = 512;

struct KeyboardState final {
	// Array indexed with SDL_SCANCODE's. 1 if key is pressed, 0 otherwise.
	u8 scancodes[MAX_NUM_SCANCODES] = {};
};

struct MouseState final {
	i32x2 windowDims = i32x2(0); // Position and delta is in range [0, windowDims]
	i32x2 pos = i32x2(0); // [0, 0] in bottom left corner
	i32x2 delta = i32x2(0); // Delta mouse has moved since last frame
	i32x2 wheel = i32x2(0); // Pos-y "up", neg-y "down", but can vary with touchpads
	u8 left = 0;
	u8 middle = 0;
	u8 right = 0;
};

constexpr u32 GPD_NONE = 0;

constexpr u32 GPD_A = 1;
constexpr u32 GPD_B = 2;
constexpr u32 GPD_X = 3;
constexpr u32 GPD_Y = 4;

constexpr u32 GPD_BACK = 5;
constexpr u32 GPD_START = 6;

constexpr u32 GPD_LS_CLICK = 7; // Left stick click
constexpr u32 GPD_LS_UP = 8; // Left stick up (sort of hack, also available as analog)
constexpr u32 GPD_LS_DOWN = 9; // Left stick down (sort of hack, also available as analog)
constexpr u32 GPD_LS_LEFT = 10; // Left stick left (sort of hack, also available as analog)
constexpr u32 GPD_LS_RIGHT = 11; // Left stick right (sort of hack, also available as analog)

constexpr u32 GPD_RS_CLICK = 12; // Right stick click
constexpr u32 GPD_RS_UP = 13; // Right stick up (sort of hack, also available as analog)
constexpr u32 GPD_RS_DOWN = 14; // Right stick down (sort of hack, also available as analog)
constexpr u32 GPD_RS_LEFT = 15; // Right stick left (sort of hack, also available as analog)
constexpr u32 GPD_RS_RIGHT = 16; // Right stick right (sort of hack, also available as analog)

constexpr u32 GPD_LB = 17; // Left shoulder button
constexpr u32 GPD_RB = 18; // Right shoulder button

constexpr u32 GPD_LT = 19; // Left trigger button (sort of hack, also available as analog)
constexpr u32 GPD_RT = 20; // Right trigger button (sort of hack, also available as analog)

constexpr u32 GPD_DPAD_UP = 21;
constexpr u32 GPD_DPAD_DOWN = 22;
constexpr u32 GPD_DPAD_LEFT = 23;
constexpr u32 GPD_DPAD_RIGHT = 24;

constexpr u32 GPD_MAX_NUM_BUTTONS = 25;

// The approximate dead zone (as specified by SDL2) for gamepad sticks.
constexpr f32 GPD_STICK_APPROX_DEADZONE = f32(8000) / f32(INT16_MAX);

struct GamepadState final {

	// Unique ID for this gamepad. Starts at 0, -1 is invalid. If the gamepad is disconnected
	// and reconnected it will get a new id. Corresponds to SDL_JoystickInstanceID().
	i32 id = -1;

	// Pointer to the SDL_GameController this state corresponds to. Mainly available for rumble
	// purposes, you are not generally supposed to look at this.
	SDL_GameController* controller = nullptr;

	// Sticks are in range [-1, 1]. Do not however that no deadzone has been applied. Stick's
	// neutral should be somewhere in the range ~[-0.24, 0.24], but this will vary from gamepad to
	// gamepad.
	f32x2 leftStick = f32x2(0.0f);
	f32x2 rightStick = f32x2(0.0f);

	f32 lt = 0.0f;
	f32 rt = 0.0f;

	// Array indexed with constants above. 1 if button is pressed, 0 otherwise.
	u8 buttons[GPD_MAX_NUM_BUTTONS] = {};
};

inline f32x2 applyDeadzone(f32x2 stick, f32 deadzone)
{
	if (deadzone <= 0.0f) return stick;
	sfz_assert(deadzone < 1.0f);
	const f32 stickLen = sfz::length(stick);
	if (stickLen >= deadzone) {
		const f32 scale = 1.0f / (1.0f - deadzone);
		const f32 adjustedLen = sfz::min(sfz::max(0.0f, stickLen - deadzone) * scale, 1.0f);
		const f32x2 dir = stick * (1.0f / stickLen);
		const f32x2 adjustedStick = dir * adjustedLen;
		return adjustedStick;
	}
	return f32x2(0.0f);
}

struct TouchState final {
	int64_t id = -1;
	f32x2 pos = f32x2(0.0f); // Range [0, 1]
	f32 pressure = 0.0f; // Range [0, 1]. Haven't found anything that activates it, avoid using?
};

struct RawInputState final {
	i32x2 windowDims = i32x2(0);
	KeyboardState kb;
	MouseState mouse;
	Arr6<GamepadState> gamepads;
	Arr8<TouchState> touches;
};

}
