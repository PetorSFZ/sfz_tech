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

#include "sfz/game_loop/DefaultGameUpdateable.hpp"

#include <imgui.h>

#include <skipifzero_strings.hpp>

#include "sfz/Context.hpp"
#include "sfz/console/Console.hpp"
#include "sfz/rendering/ImguiSupport.hpp"

namespace sfz {

// DefaultGameUpdateable class
// ------------------------------------------------------------------------------------------------

class DefaultGameUpdateable final : public GameLoopUpdateable {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	DefaultGameUpdateable() noexcept = default;
	DefaultGameUpdateable(const DefaultGameUpdateable&) = delete;
	DefaultGameUpdateable& operator= (const DefaultGameUpdateable&) = delete;

	// Public members
	// --------------------------------------------------------------------------------------------

	bool mInitialized = false;

	sfz::Allocator* allocator = nullptr;
	Console mConsole;

	UniquePtr<GameLogic> mLogic = nullptr;

	// Overloaded methods from GameLoopUpdateable
	// --------------------------------------------------------------------------------------------
public:

	void initialize(Renderer& renderer) override final
	{
		// Only initialize once
		if (mInitialized) return;
		mInitialized = true;

		// Initialize console
		ArrayLocal<str96, 32> windowNames;
		ArrayLocal<const char*, 32> windowNamesPtrs;
		uint32_t numWindowNames = mLogic->injectConsoleMenuNumWindowsToDockInitially();
		for (uint32_t i = 0; i < numWindowNames; i++) {
			windowNames.add(str96("%s", mLogic->injectConsoleMenuNameOfWindowToDockInitially(i)));
			windowNamesPtrs.add(windowNames.last().str());
		}
		mConsole.init(allocator, windowNamesPtrs.size(), windowNamesPtrs.data());

		// Initialize logic
		mLogic->initialize(renderer);
	}

	UpdateOp processInput(
		const UserInput& input,
		const UpdateInfo& updateInfo,
		Renderer& renderer) override final
	{
		// Check if console key is pressed
		bool wasConsoleActive = mConsole.active();
		for (const SDL_Event& event : input.events) {
			if (event.type != SDL_KEYUP) continue;
			if (event.key.keysym.sym == '`' ||
				event.key.keysym.sym == '~' ||
				event.key.keysym.sym == SDLK_F1) {

				mConsole.toggleActive();
			}
		}

		// Call console activated/deactivated function if console active state changed
		if (wasConsoleActive != mConsole.active()) {
			if (mConsole.active()) mLogic->onConsoleActivated();
			else mLogic->onConsoleDeactivated();
		}

		// Retrieve what inputs should be passed to imgui according to the logic
		ImguiControllers imguiControllers = mLogic->imguiController(input);

		const sdl::Mouse* imguiMousePtr = nullptr;
		if (imguiControllers.useMouse) {
			imguiMousePtr = &input.rawMouse;
		}

		const Array<SDL_Event>* imguiEventsPtr = nullptr;
		if (imguiControllers.useKeyboard) {
			imguiEventsPtr = &input.events;
		}

		const sdl::GameControllerState* imguiControllerPtr = nullptr;
		if (imguiControllers.controllerIndex != -1) {
			imguiControllerPtr = input.controllers.get(imguiControllers.controllerIndex);
		}

		// Update imgui
		updateImgui(renderer.windowResolution(), imguiMousePtr, imguiEventsPtr, imguiControllerPtr);

		// Forward input to logic
		if (!mConsole.active()) return mLogic->processInput(input, updateInfo, renderer);

		// If console is active, just return NO OP
		return UpdateOp::NO_OP();
	}

	UpdateOp updateTick(const UpdateInfo& updateInfo, Renderer& renderer) override final
	{
		// Forward update to logic
		if (!mConsole.active()) return mLogic->updateTick(updateInfo, renderer);
		return UpdateOp::NO_OP();
	}

	void render(const UpdateInfo& updateInfo, Renderer& renderer) override final
	{
		// Begin ImGui frame
		ImGui::NewFrame();

		// Begin renderer frame
		renderer.frameBegin();

		// Render
		mLogic->render(updateInfo, renderer);

		// Render Imgui
		mConsole.render(updateInfo.iterationDeltaSeconds * 1000.0f);
		if (mConsole.active()) {
			mLogic->injectConsoleMenu();
			renderer.renderImguiUI();
		}
		else {
			mLogic->renderCustomImgui();
		}

		// Finish rendering frame
		renderer.frameFinish();

		// Post render hook
		mLogic->postRenderHook(renderer, mConsole.active());
	}

	void onQuit() override final
	{
		mLogic->onQuit();
	}
};

// DefaultGameUpdateable creation function
// ------------------------------------------------------------------------------------------------

UniquePtr<GameLoopUpdateable> createDefaultGameUpdateable(
	Allocator* allocator,
	UniquePtr<GameLogic> logic) noexcept
{
	// Create updateable and set members
	UniquePtr<DefaultGameUpdateable> updateable = sfz::makeUnique<DefaultGameUpdateable>(allocator, sfz_dbg(""));
	updateable->allocator = allocator;
	updateable->mLogic = std::move(logic);

	return updateable;
}

} // namespace sfz
