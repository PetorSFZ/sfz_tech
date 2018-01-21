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

#include <cstdint>

#include <SDL.h>

#include <sfz/containers/DynArray.hpp>
#include <sfz/containers/HashMap.hpp>
#include <sfz/math/Vector.hpp>
#include <sfz/memory/SmartPointers.hpp>

#include "ph/rendering/Renderer.hpp"
#include "ph/sdl/GameController.hpp"
#include "ph/sdl/Mouse.hpp"

namespace ph {

using std::int32_t;
using sfz::DynArray;
using sfz::HashMap;
using sfz::UniquePtr;
using sfz::vec2i;
using sdl::GameController;
using sdl::GameControllerState;
using sdl::Mouse;
class GameLoopUpdateable; // Forward declaration

// UpdateOp
// ------------------------------------------------------------------------------------------------

enum class UpdateOpType {
	NO_OP = 0,
	QUIT,
	CHANGE_UPDATEABLE,
	CHANGE_TICK_RATE,
	REINIT_CONTROLLERS
};

struct UpdateOp final {
	UpdateOpType type = UpdateOpType::NO_OP;
	UniquePtr<GameLoopUpdateable> newUpdateable;
	uint32_t ticksPerSecond = 0;

	UpdateOp(UpdateOpType type, UniquePtr<GameLoopUpdateable>&& updateable, uint32_t ticksPerSecond) noexcept
	:
		type(type),
		newUpdateable(std::move(updateable)),
		ticksPerSecond(ticksPerSecond)
	{ }

	/// Normal return value, does nothing.
	static inline UpdateOp NO_OP() noexcept
	{
		return UpdateOp(UpdateOpType::NO_OP, nullptr, 0);
	}

	/// Quits the application.
	static inline UpdateOp QUIT() noexcept
	{
		return UpdateOp(UpdateOpType::QUIT, nullptr, 0);
	}

	/// Tells the game loop to change what updateable receives updates. Will cause the old
	/// Updateable to be destroyed.
	static inline UpdateOp CHANGE_UPDATEABLE(UniquePtr<GameLoopUpdateable> updateable) noexcept
	{
		return UpdateOp(UpdateOpType::CHANGE_UPDATEABLE, std::move(updateable), 0);
	}

	/// Changes the current tick rate.
	static inline UpdateOp CHANGE_TICK_RATE(uint32_t ticksPerSecond) noexcept
	{
		return UpdateOp(UpdateOpType::QUIT, nullptr, ticksPerSecond);
	}

	/// Re-initializes controllers.
	static inline UpdateOp REINIT_CONTROLLERS() noexcept
	{
		return UpdateOp(UpdateOpType::REINIT_CONTROLLERS, nullptr, 0);
	}
};

// Input structs
// ------------------------------------------------------------------------------------------------

struct UserInput final {
	// SDL events, the events array does not contain controller or mouse events
	DynArray<SDL_Event> events;
	DynArray<SDL_Event> controllerEvents;
	DynArray<SDL_Event> mouseEvents;

	// Processed controller and mouse input
	HashMap<int32_t, GameController> controllers;
	HashMap<int32_t, GameControllerState> controllersLastFrameState;
	Mouse rawMouse;
};

struct UpdateInfo final {
	/// The time since the last game loop iteration. Should NOT be used for most things simulated.
	/// Main use is for performance statistics, such as current frametime and fps.
	float iterationDeltaSeconds;

	/// The number of updates to be performed this iteration
	uint32_t numUpdateTicks;

	/// The current tick rate
	uint32_t tickRate;

	/// The current time slice per tick. Should be used to update simulation in updateTick()
	float tickTimeSeconds;

	/// The amount of lag left after the updates have been performed. Should be used to interpolate
	/// objects positions in render().
	float lagSeconds;
};

// GameLoopUpdateable
// ------------------------------------------------------------------------------------------------

class GameLoopUpdateable {
public:
	virtual ~GameLoopUpdateable() = default;

	/// Initializes this instance. Initialization should preferably be done in this method instead
	/// of the constructor. Will be called by the gameloop. If you are reusing updateables you
	/// should be careful to check if the updateable is already in an initialized state before
	/// initializing.
	virtual void initialize(Renderer& renderer) = 0;

	/// Called once every iteration of the game loop, all the user input since the previous
	/// should be handled here.
	virtual UpdateOp processInput(
		const UserInput& input,
		const UpdateInfo& updateInfo,
		Renderer& renderer) = 0;

	/// Potentially called multiple times (or non at all) each iteration of the game loop.
	/// Corresponds to updating the simulation a single tick, i.e. updateInfo.tickTimeSeconds
	/// seconds.
	virtual UpdateOp updateTick(const UpdateInfo& updateInfo) = 0;

	/// Called last each iteration of the game loop. Responsible for rendering everything. Of note
	/// is updateInfo.lagSeconds, which contains the amount of time since the last tick update.
	/// A good renderer should extrapolate objects positions before rendering them using this value.
	virtual void render(const UpdateInfo& updateInfo, Renderer& renderer) = 0;

	/// Called if the application is being shutdown. Either because a SDL_QUIT even was received or
	/// because an UpdateOp::QUIT() operation was returned. Not called when changing updateable.
	virtual void onQuit();
};

inline void GameLoopUpdateable::onQuit() { /* Default empty implementation. */ }

} // namespace ph
