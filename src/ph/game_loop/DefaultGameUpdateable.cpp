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

#include "ph/game_loop/DefaultGameUpdateable.hpp"

#include <imgui.h>

#include <sfz/math/MathSupport.hpp>
#include <sfz/util/FrametimeStats.hpp>

#include <ph/config/GlobalConfig.hpp>
#include <ph/rendering/ImguiSupport.hpp>

namespace ph {

using sfz::FrametimeStats;
using sfz::StackString32;
using sfz::StackString256;

// DefaultGameUpdateable class
// ------------------------------------------------------------------------------------------------

class DefaultGameUpdateable final : public GameLoopUpdateable {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	DefaultGameUpdateable() noexcept = default;
	DefaultGameUpdateable(const DefaultGameUpdateable&) = delete;
	DefaultGameUpdateable& operator= (const DefaultGameUpdateable&) = delete;

	// Members
	// --------------------------------------------------------------------------------------------

	bool mInitialized = false;

	UpdateableState mState;
	UniquePtr<GameLogic> mLogic = nullptr;

	// Frametime stats
	FrametimeStats mStats = FrametimeStats(480);
	int mStatsWarmup = 0;

	// Imgui
	DynArray<ph::ImguiVertex> mImguiVertices;
	DynArray<uint32_t> mImguiIndices;
	DynArray<ph::ImguiCommand> mImguiCommands;

	// Global Config
	DynArray<sfz::StackString32> mCfgSections;
	DynArray<ph::Setting*> mCfgSectionSettings;

	// Console settings
	Setting* mConsoleActiveSetting = nullptr;
	bool mConsoleActive = false;
	Setting* mConsoleAlwaysShowPerformance = nullptr;

	// Overloaded methods from GameLoopUpdateable
	// --------------------------------------------------------------------------------------------

	void initialize(Renderer& renderer) override final
	{
		// Only initialize once
		if (mInitialized) return;
		mInitialized = true;

		// Pick out console settings
		GlobalConfig& cfg = GlobalConfig::instance();
		mConsoleActiveSetting = cfg.sanitizeBool("Console", "active", false, BoolBounds(false));
		mConsoleActive = mConsoleActiveSetting->boolValue();
		mConsoleAlwaysShowPerformance =
			cfg.sanitizeBool("Console", "alwaysShowPerformance", true, BoolBounds(false));

		// Initialize logic
		mLogic->initialize(mState, renderer);
	}

	UpdateOp processInput(
		const UserInput& input,
		const UpdateInfo& updateInfo,
		Renderer& renderer) override final
	{
		// Check if console key is pressed
		for (const SDL_Event& event : input.events) {
			if (event.type != SDL_KEYUP) continue;
			if (event.key.keysym.sym == '`' ||
				event.key.keysym.sym == '~') {

				mConsoleActive = mConsoleActiveSetting->boolValue();
				mConsoleActiveSetting->setBool(!mConsoleActive);
			}
		}

		// Call console activated/deactivated function if console active state changed
		if (mConsoleActive != mConsoleActiveSetting->boolValue()) {
			mConsoleActive = mConsoleActiveSetting->boolValue();
			if (mConsoleActive) mLogic->onConsoleActivated();
			else mLogic->onConsoleDeactivated();
		}

		// Retrieve what inputs should be passed to imgui according to the logic
		ImguiControllers imguiControllers = mLogic->imguiController(input);

		const sdl::Mouse* imguiMousePtr = nullptr;
		if (imguiControllers.useMouse) {
			imguiMousePtr = &input.rawMouse;
		}

		const DynArray<SDL_Event>* imguiEventsPtr = nullptr;
		if (imguiControllers.useKeyboard) {
			imguiEventsPtr = &input.events;
		}

		const sdl::GameControllerState* imguiControllerPtr = nullptr;
		if (imguiControllers.controllerIndex != -1) {
			imguiControllerPtr = input.controllers.get(imguiControllers.controllerIndex);
		}

		// Update imgui
		updateImgui(renderer, imguiMousePtr, imguiEventsPtr, imguiControllerPtr);

		// Forward input to logic
		if (!mConsoleActive) return mLogic->processInput(mState, input, updateInfo, renderer);

		// If console is active, just return NO OP
		return UpdateOp::NO_OP();
	}

	UpdateOp updateTick(const UpdateInfo& updateInfo) override final
	{
		// Forward update to logic
		if (!mConsoleActive) return mLogic->updateTick(mState, updateInfo);
		return UpdateOp::NO_OP();
	}

	void render(const UpdateInfo& updateInfo, Renderer& renderer) override final
	{
		// Update performance stats
		if (mStatsWarmup >= 8) mStats.addSample(updateInfo.iterationDeltaSeconds);
		mStatsWarmup++;

		renderer.beginFrame(mState.cam, mState.dynamicSphereLights);

		renderer.render(mState.renderEntities.data(), mState.renderEntities.size());

		// Render Imgui
		ImGui::NewFrame();
		renderConsole();
		if (!mConsoleActive) mLogic->renderCustomImgui();
		ImGui::Render();
		convertImguiDrawData(mImguiVertices, mImguiIndices, mImguiCommands);
		renderer.renderImgui(mImguiVertices, mImguiIndices, mImguiCommands);

		// Finish rendering frame
		renderer.finishFrame();
	}

	void onQuit() override final
	{
		mLogic->onQuit(mState);
	}

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void renderConsole() noexcept
	{
		// Render performance window
		if (mConsoleActive || mConsoleAlwaysShowPerformance->boolValue()) {
			vec2 histogramDims = vec2(mStats.maxNumSamples() * 1.25f, 80.0f);
			ImGui::SetNextWindowSize(histogramDims + vec2(17.0f, 50.0f));
			ImGuiWindowFlags performanceWindowFlags = 0;
			//performanceWindowFlags |= ImGuiWindowFlags_NoTitleBar;
			performanceWindowFlags |= ImGuiWindowFlags_NoScrollbar;
			//performanceWindowFlags |= ImGuiWindowFlags_NoMove;
			performanceWindowFlags |= ImGuiWindowFlags_NoResize;
			performanceWindowFlags |= ImGuiWindowFlags_NoCollapse;
			performanceWindowFlags |= ImGuiWindowFlags_NoNav;
			ImGui::Begin("Performance", nullptr, performanceWindowFlags);
			ImGui::Text("%s", mStats.toString());
			ImGui::PlotLines("Frametimes", mStats.samples().data(), mStats.samples().size(), 0, nullptr,
				sfz::min(mStats.min(), 0.012f), sfz::max(mStats.max(), 0.020f), histogramDims);
			ImGui::End();
		}

		// Render global config window
		if (mConsoleActive) {
			// Get Global Config sections
			GlobalConfig& cfg = GlobalConfig::instance();
			mCfgSections.clear();
			cfg.getSections(mCfgSections);

			// Global Config Window
			ImGuiWindowFlags configWindowFlags = 0;
			//configWindowFlags |= ImGuiWindowFlags_NoMove;
			//configWindowFlags |= ImGuiWindowFlags_NoResize;
			//configWindowFlags |= ImGuiWindowFlags_NoCollapse;
			ImGui::SetNextWindowSize(vec2(550.0f, 0.0f));
			ImGui::Begin("Config", nullptr, configWindowFlags);
			for (auto& sectionKey : mCfgSections) {

				// Get settings from Global Config
				mCfgSectionSettings.clear();
				cfg.getSectionSettings(sectionKey.str, mCfgSectionSettings);

				// Skip if header is closed
				if (!ImGui::CollapsingHeader(sectionKey.str)) continue;

				// Start columns
				StackString256 tmpStr;
				tmpStr.printf("%s___column___", sectionKey.str);
				ImGui::Columns(4, tmpStr.str);
				ImGui::Separator();

				// Set columns widths
				ImGui::SetColumnWidth(0, 40.0f);
				ImGui::SetColumnWidth(1, 210.0f);
				ImGui::SetColumnWidth(2, 150.0f);
				ImGui::SetColumnWidth(3, 150.0f);

				// Column headers
				ImGui::Text("Save"); ImGui::NextColumn();
				ImGui::Text("Setting Key"); ImGui::NextColumn();
				ImGui::Text("Value Input"); ImGui::NextColumn();
				ImGui::Text("Alternate Input"); ImGui::NextColumn();

				ImGui::Separator();

				for (Setting* setting : mCfgSectionSettings) {

					// Write to file checkbox
					tmpStr.printf("          %s___writeToFile___", setting->key().str);
					bool writeToFile = setting->value().writeToFile;
					if (ImGui::Checkbox(tmpStr.str, &writeToFile)) {
						setting->setWriteToFile(writeToFile);
					}
					ImGui::NextColumn();

					// Key text
					ImGui::Text("%s", setting->key().str);
					ImGui::NextColumn();

					// Value input field
					tmpStr.printf("                         %s___valueInput___", setting->key().str);
					switch (setting->type()) {
					case ValueType::INT:
						{
							int32_t i = setting->intValue();
							if (ImGui::InputInt(tmpStr.str, &i, setting->value().i.bounds.step)) {
								setting->setInt(i);
							}
						}
						break;
					case ValueType::FLOAT:
						{
							float f = setting->floatValue();
							if (ImGui::InputFloat(tmpStr.str, &f, 0.25f)) {
								setting->setFloat(f);
							}
						}
						break;
					case ValueType::BOOL:
						{
							bool b = setting->boolValue();
							if (ImGui::Checkbox(tmpStr.str, &b)) {
								setting->setBool(b);
							}
						}
						break;
					}
					ImGui::NextColumn();

					// Alternate value input field
					tmpStr.printf("                         %s___altValueInput___", setting->key().str);
					switch (setting->type()) {
					case ValueType::INT:
						{
							const IntBounds& bounds = setting->value().i.bounds;
							if (bounds.minValue != INT32_MIN || bounds.maxValue != INT32_MAX) {
								int32_t i = setting->intValue();
								if (ImGui::SliderInt(tmpStr.str, &i, bounds.minValue, bounds.maxValue)) {
									setting->setInt(i);
								}
							}
						}
						break;
					case ValueType::FLOAT:
						{
							const FloatBounds& bounds = setting->value().f.bounds;
							if (bounds.minValue != FLT_MIN || bounds.maxValue != FLT_MAX) {
								float f = setting->floatValue();
								if (ImGui::SliderFloat(tmpStr.str, &f, bounds.minValue, bounds.maxValue)) {
									setting->setFloat(f);
								}
							}
						}
						break;
					default:
						// Do nothing
						break;
					}
					ImGui::NextColumn();

					ImGui::Separator();
				}

				// Return to 1 column
				ImGui::Columns(1);
			}
			ImGui::End();
		}
	}
};

// DefaultGameUpdateable creation function
// ------------------------------------------------------------------------------------------------

UniquePtr<GameLoopUpdateable> createDefaultGameUpdateable(
	Allocator* allocator,
	UniquePtr<GameLogic> logic) noexcept
{
	// Create updateable and set members
	UniquePtr<DefaultGameUpdateable> updateable = sfz::makeUnique<DefaultGameUpdateable>(allocator);
	updateable->mLogic = std::move(logic);

	// Imgui
	updateable->mImguiVertices.create(1024, allocator);
	updateable->mImguiIndices.create(1024, allocator);
	updateable->mImguiCommands.create(1024, allocator);

	// Global Config
	updateable->mCfgSections.create(32, allocator);
	updateable->mCfgSectionSettings.create(64, allocator);

	return updateable;
}

} // namespace ph
