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

#include <cctype>
#include <ctime>

#include <imgui.h>
#include <imgui_internal.h>

#include <sfz/Logging.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/util/FrametimeStats.hpp>
#include <sfz/util/IO.hpp>

#include "ph/Context.hpp"
#include "ph/config/GlobalConfig.hpp"
#include "ph/rendering/ImguiSupport.hpp"
#include "ph/util/TerminalLogger.hpp"

namespace ph {

using sfz::FrametimeStats;
using sfz::str32;
using sfz::str96;
using sfz::str128;
using sfz::str256;
using sfz::vec4;
using sfz::vec4_u8;

// Statics
// ------------------------------------------------------------------------------------------------

static void strToLower(char* dst, const char* src) noexcept
{
	size_t srcLen = strlen(src);
	for (size_t i = 0; i <= srcLen; i++) { // <= to catch null-terminator
		dst[i] = char(tolower(src[i]));
	}
}

static void imguiPrintText(const char* str, vec4 color, const char* strEnd = nullptr) noexcept
{
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(str, strEnd);
	ImGui::PopStyleColor();
}

static void renderFilteredText(
	const char* str,
	const char* filter,
	vec4 stringColor,
	vec4 filterColor) noexcept
{
	str128 lowerStackStr;
	strToLower(lowerStackStr.str, str);

	const char* currStr = str;
	const char* currLowerStr = lowerStackStr.str;
	const size_t filterLen = strlen(filter);

	if (filterLen == 0) {
		imguiPrintText(str, stringColor);
		return;
	}

	while (true) {

		const char* nextLower = strstr(currLowerStr, filter);

		// Substring found
		if (nextLower != nullptr) {

			// Render part of string until next filter
			if (nextLower != currLowerStr) {
				size_t len = nextLower - currLowerStr;
				imguiPrintText(currStr, stringColor, currStr + len);
				currStr += len;
				currLowerStr +=len;
			}

			// Render filter
			else {
				imguiPrintText(currStr, filterColor, currStr + filterLen);
				currStr += filterLen;
				currLowerStr += filterLen;
			}

			ImGui::SameLine(0.0f, 2.0f);
		}

		// If no more substrings can be found it is time to render the rest of the string
		else {
			imguiPrintText(currStr, stringColor);
			return;
		}
	}
}

static bool anyContainsFilter(const DynArray<Setting*>& settings, const char* filter) noexcept
{
	for (Setting* setting : settings) {
		if (strstr(setting->key().str, filter) != nullptr) {
			return true;
		}
	}
	return false;
}

static void timeToString(str96& stringOut, time_t timestamp) noexcept
{
	std::tm* tmPtr = std::localtime(&timestamp);
	size_t res = std::strftime(stringOut.str, stringOut.maxSize(), "%Y-%m-%d %H:%M:%S", tmPtr);
	if (res == 0) stringOut.printf("INVALID TIME");
}

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

	UpdateableState mState;
	UniquePtr<GameLogic> mLogic = nullptr;

	// Frametime stats
	FrametimeStats mStats = FrametimeStats(480);
	int mStatsWarmup = 0;

	// Imgui
	DynArray<phImguiVertex> mImguiVertices;
	DynArray<uint32_t> mImguiIndices;
	DynArray<phImguiCommand> mImguiCommands;

	// Global Config
	str32 mConfigFilterString;
	DynArray<str32> mCfgSections;
	DynArray<Setting*> mCfgSectionSettings;

	// Log
	Setting* mLogMinLevelSetting = nullptr;
	str96 mLogTagFilter;

	// Console settings
	bool mImguiFirstRun = false;
	ImGuiID mConsoleDockSpaceId = 0;
	Setting* mConsoleActiveSetting = nullptr;
	bool mConsoleActive = false;
	Setting* mConsoleShowInGamePreview = nullptr;

	// Dynamic material editor
	uint32_t mMaterialEditorCurrentMeshIdx = 0;
	uint32_t mMaterialEditorCurrentMaterialIdx = 0;

	// Overloaded methods from GameLoopUpdateable
	// --------------------------------------------------------------------------------------------
public:

	void initialize(Renderer& renderer) override final
	{
		// Only initialize once
		if (mInitialized) return;
		mInitialized = true;

		// Check if this is first run of imgui or not. I.e., whether imgui.ini existed or not.
		mImguiFirstRun = !sfz::fileExists("imgui.ini");

		// Pick out console settings
		GlobalConfig& cfg = getGlobalConfig();
		mConsoleActiveSetting = cfg.sanitizeBool("Console", "active", false, BoolBounds(false));
		mConsoleActive = mConsoleActiveSetting->boolValue();
		mConsoleShowInGamePreview =
			cfg.sanitizeBool("Console", "showInGamePreview", true, BoolBounds(false));
		mLogMinLevelSetting = cfg.sanitizeInt("Console", "logMinLevel", false, IntBounds(0, 0, 3));

		// Initialize resource manager
		mState.resourceManager = ResourceManager::create(&renderer, sfz::getDefaultAllocator());

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
				event.key.keysym.sym == '~' ||
				event.key.keysym.sym == SDLK_F1) {

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
		// Call the pre-render hook
		RenderSettings settings = mLogic->preRenderHook(mState, updateInfo, renderer);

		// Some assets sanity checks
		sfz_assert_debug(mState.resourceManager.textures().size() == renderer.numTextures());

		// Update performance stats
		if (mStatsWarmup >= 8) mStats.addSample(updateInfo.iterationDeltaSeconds * 1000.0f);
		mStatsWarmup++;

		renderer.beginFrame(settings.clearColor, mState.cam, settings.ambientLight, mState.dynamicSphereLights);

		renderer.renderStaticScene();

		renderer.render(mState.renderEntities.data(), mState.renderEntities.size());

		// Render Imgui
		ImGui::NewFrame();
		renderConsole(renderer);
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

	void renderConsole(Renderer& renderer) noexcept
	{
		// Render in-game console preview
		if(!mConsoleActive && mConsoleShowInGamePreview->boolValue()) {
			this->renderConsoleInGamePreview();
		}

		// Return if console should not be rendered
		if (!mConsoleActive) return;

		// Console dock space
		this->renderConsoleDockSpace();

		// Render console windows
		this->renderPerformanceWindow();
		this->renderLogWindow();
		this->renderConfigWindow();
		this->renderResourceEditorWindow(renderer);

		// Render custom-injected windows
		mLogic->injectConsoleMenu();

		// Initialize dockspace with default docked layout if first run
		if (mImguiFirstRun) this->renderConsoleDockSpaceInitialize();
		mImguiFirstRun = false;
	}

	void renderConsoleInGamePreview() noexcept
	{
		// Calculate and set size of window
		ImGui::SetNextWindowSize(vec2(800, 115), ImGuiCond_Always);
		ImGui::SetNextWindowPos(vec2(0.0f), ImGuiCond_Always);

		// Set window flags
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_NoTitleBar;
		windowFlags |= ImGuiWindowFlags_NoResize;
		windowFlags |= ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoScrollbar;
		windowFlags |= ImGuiWindowFlags_NoCollapse;
		//windowFlags |= ImGuiWindowFlags_NoBackground;
		windowFlags |= ImGuiWindowFlags_NoMouseInputs;
		windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		windowFlags |= ImGuiWindowFlags_NoNav;
		windowFlags |= ImGuiWindowFlags_NoInputs;

		// Begin window
		ImGui::PushStyleColor(ImGuiCol_WindowBg, vec4(0.05f, 0.05f, 0.05f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_Border, vec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Console Preview", nullptr, windowFlags);

		// Render performance numbers
		ImGui::BeginGroup();
		ImGui::Text("Avg: %.1f ms", mStats.avg());
		ImGui::Text("Std: %.1f ms", mStats.sd());
		ImGui::Text("Min: %.1f ms", mStats.min());
		ImGui::Text("Max: %.1f ms", mStats.max());
		//ImGui::Text("%u samples, %.1f s", mStats.currentNumSamples(), mStats.time());
		ImGui::EndGroup();

		// Render performance histogram
		ImGui::SameLine();
		vec2 histogramDims = vec2(ImGui::GetWindowSize()) - vec2(145.0f, 25.0f);
		ImGui::PlotLines("##Frametimes", mStats.samples().data(), mStats.samples().size(), 0, nullptr,
			0.0f, sfz::max(mStats.max(), 0.020f), histogramDims);

		// End window
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}

	void renderConsoleDockSpace() noexcept
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGuiDockNodeFlags dockSpaceFlags = 0;
		dockSpaceFlags |= ImGuiDockNodeFlags_PassthruDockspace;
		mConsoleDockSpaceId = ImGui::DockSpaceOverViewport(viewport, dockSpaceFlags);
	}

	void renderConsoleDockSpaceInitialize() noexcept
	{
		ImGui::DockBuilderRemoveNode(mConsoleDockSpaceId);

		ImGuiDockNodeFlags dockSpaceFlags = 0;
		dockSpaceFlags |= ImGuiDockNodeFlags_PassthruDockspace;
		dockSpaceFlags |= ImGuiDockNodeFlags_Dockspace;
		ImGui::DockBuilderAddNode(mConsoleDockSpaceId, dockSpaceFlags);

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::DockBuilderSetNodeSize(mConsoleDockSpaceId, viewport->Size);

		ImGuiID dockMain = mConsoleDockSpaceId;
		ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.45f, NULL, &dockMain);
		ImGuiID dockUpperLeft = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Up, 0.20f, NULL, &dockLeft);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.5f, NULL, &dockMain);

		ImGui::DockBuilderDockWindow("Performance", dockUpperLeft);
		ImGui::DockBuilderDockWindow("Log", dockBottom);
		ImGui::DockBuilderDockWindow("Config", dockLeft);
		ImGui::DockBuilderDockWindow("Resources", dockLeft);
		ImGui::DockBuilderDockWindow("Dynamic Materials", dockLeft);

		uint32_t numInjectedWindowsToDock = mLogic->injectConsoleMenuNumWindowsToDockInitially();
		for (uint32_t i = 0; i < numInjectedWindowsToDock; i++) {
			const char* windowName = mLogic->injectConsoleMenuNameOfWindowToDockInitially(i);
			ImGui::DockBuilderDockWindow(windowName, dockLeft);
		}

		ImGui::DockBuilderFinish(mConsoleDockSpaceId);
	}

	// Returns whether window is docked or not, small hack to create initial docked layout
	void renderPerformanceWindow() noexcept
	{
		// Calculate and set size of window
		ImGui::SetNextWindowSize(vec2(800, 135), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(vec2(0.0f), ImGuiCond_FirstUseEver);

		// Set window flags
		ImGuiWindowFlags performanceWindowFlags = 0;
		//performanceWindowFlags |= ImGuiWindowFlags_NoTitleBar;
		performanceWindowFlags |= ImGuiWindowFlags_NoScrollbar;
		//performanceWindowFlags |= ImGuiWindowFlags_NoMove;
		//performanceWindowFlags |= ImGuiWindowFlags_NoResize;
		//performanceWindowFlags |= ImGuiWindowFlags_NoCollapse;
		performanceWindowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
		performanceWindowFlags |= ImGuiWindowFlags_NoNav;

		// Begin window
		ImGui::Begin("Performance", nullptr, performanceWindowFlags);

		// Render performance numbers
		ImGui::BeginGroup();
		ImGui::Text("Avg: %.1f ms", mStats.avg());
		ImGui::Text("Std: %.1f ms", mStats.sd());
		ImGui::Text("Min: %.1f ms", mStats.min());
		ImGui::Text("Max: %.1f ms", mStats.max());
		//ImGui::Text("%u samples, %.1f s", mStats.currentNumSamples(), mStats.time());
		ImGui::EndGroup();

		// Render performance histogram
		ImGui::SameLine();
		vec2 histogramDims = vec2(ImGui::GetWindowSize()) - vec2(140.0f, 50.0f);
		ImGui::PlotLines("##Frametimes", mStats.samples().data(), mStats.samples().size(), 0, nullptr,
			0.0f, sfz::max(mStats.max(), 0.020f), histogramDims);

		// End window
		ImGui::End();
	}

	void renderLogWindow() noexcept
	{
		const vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		TerminalLogger& logger = *getContext()->logger;
		str96 timeStr;

		ImGui::SetNextWindowPos(vec2(0.0f, 130.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(vec2(800, 800), ImGuiCond_FirstUseEver);

		// Set window flags
		ImGuiWindowFlags logWindowFlags = 0;
		//logWindowFlags |= ImGuiWindowFlags_NoMove;
		//logWindowFlags |= ImGuiWindowFlags_NoResize;
		//logWindowFlags |= ImGuiWindowFlags_NoCollapse;
		logWindowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;

		// Begin window
		ImGui::Begin("Log", nullptr, logWindowFlags);

		// Options
		ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);

		ImGui::PushItemWidth(ImGui::GetWindowWidth() - 160.0f - 160.0f - 40.0f);
		ImGui::InputText("##Tag filter", mLogTagFilter.str, mLogTagFilter.maxSize());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		strToLower(mLogTagFilter.str, mLogTagFilter.str);
		bool tagFilterMode = mLogTagFilter != "";

		int logMinLevelVal = mLogMinLevelSetting->intValue();
		ImGui::PushItemWidth(160.0f);
		ImGui::Combo("##Minimum log level", &logMinLevelVal, sfz::LOG_LEVEL_STRINGS,
			IM_ARRAYSIZE(sfz::LOG_LEVEL_STRINGS));
		ImGui::PopItemWidth();
		mLogMinLevelSetting->setInt(logMinLevelVal);

		ImGui::PopStyleColor();

		ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
		if (ImGui::Button("Clear messages")) {
			logger.clearMessages();
		}

		// Print all messages
		ImGui::BeginChild("LogItems");
		uint32_t numLogMessages = logger.numMessages();
		for (uint32_t i = 0; i < numLogMessages; i++) {
			// Reverse order, newest first
			const TerminalMessageItem& message = logger.getMessage(numLogMessages - i - 1);

			// Skip if log level is too low
			if (int32_t(message.level) < mLogMinLevelSetting->intValue()) continue;

			// Skip section if nothing matches when filtering
			if (tagFilterMode) {
				str32 tagLowerStr;
				strToLower(tagLowerStr.str, message.tag.str);
				bool tagFilter = strstr(tagLowerStr.str, mLogTagFilter.str) != nullptr;
				if (!tagFilter) continue;
			}

			// Get color of message
			vec4 messageColor = vec4(0.0f);
			switch (message.level) {
			case LogLevel::INFO_NOISY: messageColor = vec4(0.6f, 0.6f, 0.8f, 1.0f); break;
			case LogLevel::INFO: messageColor = vec4(0.8f, 0.8f, 0.8f, 1.0f); break;
			case LogLevel::WARNING: messageColor = vec4(1.0f, 1.0f, 0.0f, 1.0f); break;
			case LogLevel::ERROR_LVL: messageColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); break;
			}

			// Create columns
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 220.0f);

			// Print tag and messagess
			ImGui::Separator();
			renderFilteredText(message.tag.str, mLogTagFilter.str, messageColor, filterTextColor);
			ImGui::NextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, messageColor);
			ImGui::TextWrapped("%s", message.message.str); ImGui::NextColumn();
			ImGui::PopStyleColor();

			// Restore to 1 column
			ImGui::Columns(1);

			// Tooltip with timestamp, file and explicit warning level
			if (ImGui::IsItemHovered()) {

				// Get time string
				timeToString(timeStr, message.timestamp);

				// Print tooltip
				ImGui::BeginTooltip();
				ImGui::Text("%s -- %s -- %s:%i",
					toString(message.level), timeStr.str, message.file.str, message.lineNumber);
				ImGui::EndTooltip();
			}
		}

		// Show last message by default
		ImGui::EndChild();

		// Return to 1 column
		ImGui::Columns(1);

		// End window
		ImGui::End();
	}

	void renderConfigWindow() noexcept
	{
		const vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		str256 tmpStr;

		// Get Global Config sections
		GlobalConfig& cfg = getGlobalConfig();
		mCfgSections.clear();
		cfg.getSections(mCfgSections);

		// Set window size
		ImGui::SetNextWindowPos(vec2(mStats.maxNumSamples() * 1.25f + 17.0f, 0.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(vec2(400.0f, 0.0f), ImGuiCond_FirstUseEver);

		// Set window flags
		ImGuiWindowFlags configWindowFlags = 0;
		//configWindowFlags |= ImGuiWindowFlags_NoMove;
		//configWindowFlags |= ImGuiWindowFlags_NoResize;
		//configWindowFlags |= ImGuiWindowFlags_NoCollapse;
		configWindowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;

		// Begin window
		ImGui::Begin("Config", nullptr, configWindowFlags);

		// Config filter string
		//ImGui::PushItemWidth(-1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
		ImGui::InputText("Filter", mConfigFilterString.str, mConfigFilterString.maxSize());
		ImGui::PopStyleColor();
		strToLower(mConfigFilterString.str, mConfigFilterString.str);
		bool filterMode = mConfigFilterString != "";

		// Add spacing and separator between filter and configs
		ImGui::Spacing();

		// Start columns
		ImGui::Columns(3);
		float windowWidth = ImGui::GetWindowSize().x;
		ImGui::SetColumnWidth(0, 55.0f);
		ImGui::SetColumnWidth(1, windowWidth - 275.0f);
		ImGui::SetColumnWidth(2, 200.0f);

		// Column headers
		ImGui::Text("Save"); ImGui::NextColumn();
		ImGui::Text("Setting"); ImGui::NextColumn();
		ImGui::Text("Value"); ImGui::NextColumn();

		for (auto& sectionKey : mCfgSections) {

			// Get settings from Global Config
			mCfgSectionSettings.clear();
			cfg.getSectionSettings(sectionKey.str, mCfgSectionSettings);

			// Skip section if nothing matches when filtering
			if (filterMode) {
				str32 sectionLowerStr;
				strToLower(sectionLowerStr.str, sectionKey.str);
				bool sectionFilter = strstr(sectionLowerStr.str, mConfigFilterString.str) != nullptr;
				bool settingsFilter = anyContainsFilter(mCfgSectionSettings, mConfigFilterString.str);
				if (!sectionFilter && !settingsFilter) continue;
			}

			// Write header
			ImGui::Columns(1);
			if (filterMode) {
				ImGui::Separator();
				renderFilteredText(sectionKey.str, mConfigFilterString.str,
					vec4(1.0f), filterTextColor);
			}
			else {
				if (ImGui::CollapsingHeader(sectionKey.str)) continue;
			}
			ImGui::Columns(3);
			ImGui::SetColumnWidth(0, 55.0f);
			ImGui::SetColumnWidth(1, windowWidth - 275.0f);
			ImGui::SetColumnWidth(2, 200.0f);

			for (Setting* setting : mCfgSectionSettings) {

				// Combine key strings
				str128 combinedKeyStr;
				combinedKeyStr.printf("%s%s", sectionKey.str, setting->key().str);
				str128 combinedKeyLowerStr;
				strToLower(combinedKeyLowerStr.str, combinedKeyStr.str);

				// Check if setting contains filter
				bool containsFilter = strstr(combinedKeyLowerStr.str, mConfigFilterString.str) != nullptr;
				if (!containsFilter) continue;

				// Write to file checkbox
				tmpStr.printf("##%s___writeToFile___", setting->key().str);
				bool writeToFile = setting->value().writeToFile;
				if (ImGui::Checkbox(tmpStr.str, &writeToFile)) {
					setting->setWriteToFile(writeToFile);
				}
				ImGui::NextColumn();

				// Render setting key
				if (filterMode) {
					renderFilteredText(setting->key().str, mConfigFilterString.str,
						vec4(1.0f), filterTextColor);
				}
				else {
					ImGui::TextUnformatted(setting->key().str);
				}
				ImGui::NextColumn();

				// Value input field
				ImGui::PushItemWidth(-1.0f);
				tmpStr.printf("##%s___valueInput___", setting->key().str);
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
				ImGui::PopItemWidth();
				ImGui::NextColumn();
			}
		}

		// Return to 1 column
		ImGui::Columns(1);

		// End window
		ImGui::End();
	}

	void renderResourceEditorWindow(Renderer& renderer) noexcept
	{
		// Get resource manager and resource strings
		const StringCollection& resStrings = getResourceStrings();

		// Set window flags
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;

		ImGui::SetNextWindowPos(vec2(500.0f, 500.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowContentSize(vec2(630.0f, 0.0f));
		ImGui::Begin("Resources", nullptr, windowFlags);

		// Tabs
		ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("ResourcesTabBar", tabBarFlags)) {

			// Meshes
			if (ImGui::BeginTabItem("Meshes")) {
				ImGui::Spacing();

				for (const MeshDescriptor& descr : mState.resourceManager.meshDescriptors()) {
					const char* globalPath = resStrings.getString(descr.globalPathId);
					uint32_t globalIdx = descr.globalIdx;

					str256 meshName("%u -- \"%s\" -- %u components",
						globalIdx, globalPath, descr.componentDescriptors.size());
					if (ImGui::CollapsingHeader(meshName.str)) {
						ImGui::Indent(30.0f);
						for (uint32_t i = 0; i < descr.componentDescriptors.size(); i++) {
							const auto& compDescr = descr.componentDescriptors[i];
							ImGui::Text("Component %u -- Material: %u", i, compDescr.materialIdx);
						}
						ImGui::Unindent(30.0f);
					}
				}

				ImGui::EndTabItem();
			}

			// Textures
			if (ImGui::BeginTabItem("Textures")) {
				ImGui::Spacing();

				for (const ResourceMapping& texMapping : mState.resourceManager.textures()) {
					const char* globalPath = resStrings.getString(texMapping.globalPathId);
					uint32_t globalIdx = texMapping.globalIdx;

					ImGui::Text("%u -- \"%s\"", globalIdx, globalPath);
				}

				ImGui::EndTabItem();
			}

			// Materials
			if (ImGui::BeginTabItem("Materials")) {
				ImGui::Spacing();

				// Check that mesh index is in range
				DynArray<MeshDescriptor>& meshes = mState.resourceManager.meshDescriptors();
				if (mMaterialEditorCurrentMeshIdx >= meshes.size()) {
					mMaterialEditorCurrentMeshIdx = 0;
				}
				sfz_assert_debug(meshes.size() != 0);

				// Mesh index selection combo box
				MeshDescriptor* currentMesh = &meshes[mMaterialEditorCurrentMeshIdx];
				str256 meshStr("%u -- \"%s\"",
					mMaterialEditorCurrentMeshIdx, resStrings.getString(currentMesh->globalPathId));
				if (ImGui::BeginCombo("Mesh", meshStr.str)) {
					for (uint32_t i = 0; i < meshes.size(); i++) {

						// Convert index to string and check if it is selected
						meshStr = str256("%u -- \"%s\"",
							i, resStrings.getString(meshes[i].globalPathId));
						bool isSelected = mMaterialEditorCurrentMeshIdx == i;

						// Report index to ImGui combo button and update current if it has changed
						if (ImGui::Selectable(meshStr.str, isSelected)) {
							mMaterialEditorCurrentMeshIdx = i;
							mMaterialEditorCurrentMaterialIdx = 0;
							currentMesh = &meshes[i];
						}
					}
					ImGui::EndCombo();
				}

				// Check that material is in range
				DynArray<phMaterial>& materials = currentMesh->materials;
				if (mMaterialEditorCurrentMaterialIdx >= materials.size()) {
					mMaterialEditorCurrentMaterialIdx = 0;
				}
				sfz_assert_debug(materials.size() != 0);

				// Material index selection combo box
				phMaterial* material = &materials[mMaterialEditorCurrentMaterialIdx];
				if (ImGui::BeginCombo(
					"Material", str32("Material %u", mMaterialEditorCurrentMaterialIdx).str)) {
					for (uint32_t i = 0; i < materials.size(); i++) {

						// Convert index to string and check if it is selected
						str32 materialStr("Material %u", i);
						bool isSelected = mMaterialEditorCurrentMaterialIdx == i;

						// Report index to ImGui combo button and update current if it has changed
						if (ImGui::Selectable(materialStr.str, isSelected)) {
							mMaterialEditorCurrentMaterialIdx = i;
							material = &materials[i];
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				bool sendUpdatedMaterialToRenderer = false;

				// Lambdas for converting vec4_u8 to vec4_f32 and back
				auto u8ToF32 = [](vec4_u8 v) { return vec4(v) * (1.0f / 255.0f); };
				auto f32ToU8 = [](vec4 v) { return vec4_u8(v * 255.0f); };

				// Lambda for converting texture index to combo string label
				auto textureToComboStr = [&](uint16_t idx) {
					const char* globalPathStr = mState.resourceManager.debugTextureIndexToGlobalPath(idx);
					return str128("%u - %s", uint32_t(idx), globalPathStr);
				};

				// Lambda for creating a combo box to select texture
				auto textureComboBox = [&](const char* comboName, uint16_t& texIndex) {
					str128 selectedMaterialStr = textureToComboStr(texIndex);
					if (ImGui::BeginCombo(comboName, selectedMaterialStr.str)) {

						// Special case for no texture (~0)
						{
							bool isSelected = texIndex == uint16_t(~0);
							if (ImGui::Selectable("~0 - NO TEXTURE", isSelected)) {
								texIndex = uint16_t(~0);;
								sendUpdatedMaterialToRenderer = true;
							}
						}

						// Existing textures
						uint32_t numTextures = mState.resourceManager.textures().size();
						for (uint32_t i = 0; i < numTextures; i++) {

							// Convert index to string and check if it is selected
							str128 materialStr = textureToComboStr(uint16_t(i));
							bool isSelected = texIndex == i;

							// Report index to ImGui combo button and update current if it has changed
							if (ImGui::Selectable(materialStr.str, isSelected)) {
								texIndex = uint16_t(i);
								sendUpdatedMaterialToRenderer = true;
							}
						}
						ImGui::EndCombo();
					}
				};

				// Albedo
				vec4 colorFloat = u8ToF32(material->albedo);
				if (ImGui::ColorEdit4("Albedo Factor", colorFloat.data(),
					ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float)) {
					material->albedo = f32ToU8(colorFloat);
					sendUpdatedMaterialToRenderer = true;
				}
				textureComboBox("Albedo Texture", material->albedoTexIndex);

				// Emissive
				colorFloat = u8ToF32(vec4_u8(material->emissive, 0));
				if (ImGui::ColorEdit3("Emissive Factor", colorFloat.data(), ImGuiColorEditFlags_Float)) {
					material->emissive = f32ToU8(colorFloat).xyz;
					sendUpdatedMaterialToRenderer = true;
				}
				textureComboBox("Emissive Texture", material->emissiveTexIndex);

				// Metallic & roughness
				vec4_u8 metallicRoughnessU8(material->metallic, material->roughness, 0, 0);
				vec4 metallicRoughness = u8ToF32(metallicRoughnessU8);
				if (ImGui::SliderFloat2("Metallic Roughness Factors", metallicRoughness.data(), 0.f, 1.f)) {
					metallicRoughnessU8 = f32ToU8(metallicRoughness);
					material->metallic = metallicRoughnessU8.x;
					material->roughness = metallicRoughnessU8.y;
					sendUpdatedMaterialToRenderer = true;
				}
				textureComboBox("Metallic Roughness Texture", material->metallicRoughnessTexIndex);

				// Normal and Occlusion textures
				textureComboBox("Normal Texture", material->normalTexIndex);
				textureComboBox("Occlusion Texture", material->occlusionTexIndex);

				// Send updated material to renderer
				if (sendUpdatedMaterialToRenderer) {
					renderer.updateMeshMaterials(mMaterialEditorCurrentMeshIdx, currentMesh->materials);
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
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
