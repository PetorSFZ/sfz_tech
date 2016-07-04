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

#include <cstdint>

#include "sfz/containers/DynArray.hpp"
#include "sfz/containers/HashMap.hpp"
#include "sfz/math/Vector.hpp"
#include "sfz/memory/SmartPointers.hpp"
#include "sfz/sdl/GameController.hpp"
#include "sfz/sdl/Mouse.hpp"
#include "sfz/sdl/Window.hpp"

namespace sfz {

using std::int32_t;
class BaseScreen; // Forward declaration for ScreenUpdateOp

// UpdateOp
// ------------------------------------------------------------------------------------------------

enum class UpdateOpType {
	NO_OP,
	SWITCH_SCREEN,
	QUIT,
	REINIT_CONTROLLERS
};

struct UpdateOp final {
	UpdateOp() noexcept = default;
	UpdateOp(const UpdateOp&) noexcept = default;
	UpdateOp& operator= (const UpdateOp&) noexcept = default;
	inline UpdateOp(UpdateOpType type, SharedPtr<BaseScreen> screen = nullptr) noexcept
	:
		type(type),
		newScreen(screen)
	{ }

	UpdateOpType type;
	SharedPtr<BaseScreen> newScreen;
};

const UpdateOp SCREEN_NO_OP(UpdateOpType::NO_OP);
const UpdateOp SCREEN_QUIT(UpdateOpType::QUIT);
const UpdateOp SCREEN_REINIT_CONTROLLERS(UpdateOpType::REINIT_CONTROLLERS);

// UpdateState
// ------------------------------------------------------------------------------------------------

struct UpdateState final {
	UpdateState() = delete;
	UpdateState(const UpdateState&) = delete;
	UpdateState& operator= (const UpdateState&) = delete;
	inline UpdateState(sdl::Window& window) noexcept : window{window} { }

	sdl::Window& window;
	DynArray<SDL_Event> events;
	DynArray<SDL_Event> controllerEvents;
	DynArray<SDL_Event> mouseEvents;
	HashMap<int32_t, sdl::GameController> controllers;
	HashMap<int32_t, sdl::GameControllerState> controllersLastFrameState;
	sdl::Mouse rawMouse;
	float delta;
};

// BaseScreen
// ------------------------------------------------------------------------------------------------

class BaseScreen {
public:
	virtual ~BaseScreen() = default;

	virtual UpdateOp update(UpdateState& state) = 0;
	virtual void render(UpdateState& state) = 0;

	virtual void onQuit();
	virtual void onResize(vec2 windowDimensions, vec2 drawableDimensions);
};

inline void BaseScreen::onQuit() { /* Default empty implementation. */ }
inline void BaseScreen::onResize(vec2, vec2) { /* Default empty implementation. */ }

} // namespace sfz
