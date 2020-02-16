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

#include "sfz/console/Console.hpp"

#include <cctype>
#include <ctime>

#include <skipifzero_arrays.hpp>
#include <skipifzero_strings.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include "sfz/config/GlobalConfig.hpp"
#include "sfz/util/FrametimeStats.hpp"
#include "sfz/util/IO.hpp"
#include "sfz/util/TerminalLogger.hpp"

namespace sfz {

// ConsoleState
// ------------------------------------------------------------------------------------------------

struct ConsoleState final {

	Allocator* allocator = nullptr;

	// Console settings
	bool active = false;
	bool imguiFirstRun = false;
	ImGuiID dockSpaceId = 0;
	Setting* showInGamePreview = nullptr;

	// Frametime stats
	FrametimeStats stats = FrametimeStats(384);
	int statsWarmup = 0;

	// Global Config
	str32 configFilterString;
	Array<str32> cfgSections;
	Array<Setting*> cfgSectionSettings;

	// Log
	Setting* logMinLevelSetting = nullptr;
	str96 logTagFilter;

	// Injected windows
	ArrayLocal<str96, 32> injectedWindowNames;
};

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
	strToLower(lowerStackStr.mRawStr, str);

	const char* currStr = str;
	const char* currLowerStr = lowerStackStr.str();
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

static bool anyContainsFilter(const Array<Setting*>& settings, const char* filter) noexcept
{
	for (Setting* setting : settings) {
		if (strstr(setting->key(), filter) != nullptr) {
			return true;
		}
	}
	return false;
}

static void timeToString(str96& stringOut, time_t timestamp) noexcept
{
	std::tm* tmPtr = std::localtime(&timestamp);
	size_t res = std::strftime(stringOut.mRawStr, stringOut.capacity(), "%Y-%m-%d %H:%M:%S", tmPtr);
	if (res == 0) {
		stringOut.clear();
		stringOut.appendf("INVALID TIME");
	}
}

static void renderConsoleInGamePreview(ConsoleState& state) noexcept
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
	ImGui::Text("Avg: %.1f ms", state.stats.avg());
	ImGui::Text("Std: %.1f ms", state.stats.sd());
	ImGui::Text("Min: %.1f ms", state.stats.min());
	ImGui::Text("Max: %.1f ms", state.stats.max());
	//ImGui::Text("%u samples, %.1f s", mStats.currentNumSamples(), mStats.time());
	ImGui::EndGroup();

	// Render performance histogram
	ImGui::SameLine();
	vec2 histogramDims = vec2(ImGui::GetWindowSize()) - vec2(145.0f, 25.0f);
	ImGui::PlotLines("##Frametimes", state.stats.samples().data(), state.stats.samples().size(), 0, nullptr,
		0.0f, sfz::max(state.stats.max(), 0.020f), histogramDims);

	// End window
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

static void renderConsoleDockSpace(ConsoleState& state) noexcept
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGuiDockNodeFlags dockSpaceFlags = 0;
	dockSpaceFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
	state.dockSpaceId = ImGui::DockSpaceOverViewport(viewport, dockSpaceFlags);
}

static void renderConsoleDockSpaceInitialize(ConsoleState& state) noexcept
{
	ImGui::DockBuilderRemoveNode(state.dockSpaceId);

	ImGuiDockNodeFlags dockSpaceFlags = 0;
	dockSpaceFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
	dockSpaceFlags |= ImGuiDockNodeFlags_DockSpace;
	ImGui::DockBuilderAddNode(state.dockSpaceId, dockSpaceFlags);

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(state.dockSpaceId, viewport->Size);

	ImGuiID dockMain = state.dockSpaceId;
	ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.45f, NULL, &dockMain);
	ImGuiID dockUpperLeft = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Up, 0.20f, NULL, &dockLeft);
	ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.5f, NULL, &dockMain);

	ImGui::DockBuilderDockWindow("Performance", dockUpperLeft);
	ImGui::DockBuilderDockWindow("Log", dockBottom);
	ImGui::DockBuilderDockWindow("Config", dockLeft);
	ImGui::DockBuilderDockWindow("Renderer", dockLeft);

	for (uint32_t i = 0; i < state.injectedWindowNames.size(); i++) {
		const char* windowName = state.injectedWindowNames[i].str();
		ImGui::DockBuilderDockWindow(windowName, dockLeft);
	}

	ImGui::DockBuilderFinish(state.dockSpaceId);
}

// Returns whether window is docked or not, small hack to create initial docked layout
static void renderPerformanceWindow(ConsoleState& state) noexcept
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
	ImGui::Text("Avg: %.1f ms", state.stats.avg());
	ImGui::Text("Std: %.1f ms", state.stats.sd());
	ImGui::Text("Min: %.1f ms", state.stats.min());
	ImGui::Text("Max: %.1f ms", state.stats.max());
	//ImGui::Text("%u samples, %.1f s", mStats.currentNumSamples(), mStats.time());
	ImGui::EndGroup();

	// Render performance histogram
	ImGui::SameLine();
	vec2 histogramDims = vec2(ImGui::GetWindowSize()) - vec2(140.0f, 50.0f);
	ImGui::PlotLines("##Frametimes", state.stats.samples().data(), state.stats.samples().size(), 0, nullptr,
		0.0f, sfz::max(state.stats.max(), 0.020f), histogramDims);

	// End window
	ImGui::End();
}

static void renderLogWindow(ConsoleState& state) noexcept
{
	const vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	TerminalLogger& logger = *getPhContext()->logger;
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
	ImGui::InputText("##Tag filter", state.logTagFilter.mRawStr, state.logTagFilter.capacity());
	ImGui::PopItemWidth();
	ImGui::SameLine();
	strToLower(state.logTagFilter.mRawStr, state.logTagFilter);
	bool tagFilterMode = state.logTagFilter != "";

	int logMinLevelVal = state.logMinLevelSetting->intValue();
	ImGui::PushItemWidth(160.0f);
	ImGui::Combo("##Minimum log level", &logMinLevelVal, sfz::LOG_LEVEL_STRINGS,
		IM_ARRAYSIZE(sfz::LOG_LEVEL_STRINGS));
	ImGui::PopItemWidth();
	state.logMinLevelSetting->setInt(logMinLevelVal);

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
		if (int32_t(message.level) < state.logMinLevelSetting->intValue()) continue;

		// Skip section if nothing matches when filtering
		if (tagFilterMode) {
			str32 tagLowerStr;
			strToLower(tagLowerStr.mRawStr, message.tag);
			bool tagFilter = strstr(tagLowerStr.str(), state.logTagFilter.str()) != nullptr;
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
		renderFilteredText(message.tag, state.logTagFilter, messageColor, filterTextColor);
		ImGui::NextColumn();
		ImGui::PushStyleColor(ImGuiCol_Text, messageColor);
		ImGui::TextWrapped("%s", message.message.str()); ImGui::NextColumn();
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
				toString(message.level), timeStr.str(), message.file.str(), message.lineNumber);
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

static void renderConfigWindow(ConsoleState& state) noexcept
{
	const vec4 filterTextColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	str256 tmpStr;

	// Get Global Config sections
	GlobalConfig& cfg = getGlobalConfig();
	state.cfgSections.clear();
	cfg.getSections(state.cfgSections);

	// Set window size
	ImGui::SetNextWindowPos(vec2(state.stats.maxNumSamples() * 1.25f + 17.0f, 0.0f), ImGuiCond_FirstUseEver);
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
	ImGui::InputText("Filter", state.configFilterString.mRawStr, state.configFilterString.capacity());
	ImGui::PopStyleColor();
	strToLower(state.configFilterString.mRawStr, state.configFilterString);
	bool filterMode = state.configFilterString != "";

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

	for (auto& sectionKey : state.cfgSections) {

		// Get settings from Global Config
		state.cfgSectionSettings.clear();
		cfg.getSectionSettings(sectionKey, state.cfgSectionSettings);

		// Skip section if nothing matches when filtering
		if (filterMode) {
			str32 sectionLowerStr;
			strToLower(sectionLowerStr.mRawStr, sectionKey);
			bool sectionFilter = strstr(sectionLowerStr, state.configFilterString) != nullptr;
			bool settingsFilter = anyContainsFilter(state.cfgSectionSettings, state.configFilterString);
			if (!sectionFilter && !settingsFilter) continue;
		}

		// Write header
		ImGui::Columns(1);
		if (filterMode) {
			ImGui::Separator();
			renderFilteredText(sectionKey, state.configFilterString,
				vec4(1.0f), filterTextColor);
		}
		else {
			if (ImGui::CollapsingHeader(sectionKey)) continue;
		}
		ImGui::Columns(3);
		ImGui::SetColumnWidth(0, 55.0f);
		ImGui::SetColumnWidth(1, windowWidth - 275.0f);
		ImGui::SetColumnWidth(2, 200.0f);

		for (Setting* setting : state.cfgSectionSettings) {

			// Combine key strings
			str128 combinedKeyStr("%s%s", sectionKey.str(), setting->key().str());
			str128 combinedKeyLowerStr;
			strToLower(combinedKeyLowerStr.mRawStr, combinedKeyStr);

			// Check if setting contains filter
			bool containsFilter = strstr(combinedKeyLowerStr, state.configFilterString) != nullptr;
			if (!containsFilter) continue;

			// Write to file checkbox
			tmpStr.clear();
			tmpStr.appendf("##%s___writeToFile___", setting->key().str());
			bool writeToFile = setting->value().writeToFile;
			if (ImGui::Checkbox(tmpStr, &writeToFile)) {
				setting->setWriteToFile(writeToFile);
			}
			ImGui::NextColumn();

			// Render setting key
			if (filterMode) {
				renderFilteredText(setting->key(), state.configFilterString,
					vec4(1.0f), filterTextColor);
			}
			else {
				ImGui::TextUnformatted(setting->key().str());
			}
			ImGui::NextColumn();

			// Value input field
			ImGui::PushItemWidth(-1.0f);
			tmpStr.clear();
			tmpStr.appendf("##%s_%s___valueInput___", setting->section().str(), setting->key().str());
			switch (setting->type()) {
			case ValueType::INT:
				{
					int32_t i = setting->intValue();
					if (ImGui::InputInt(tmpStr, &i, setting->value().i.bounds.step)) {
						setting->setInt(i);
					}
				}
				break;
			case ValueType::FLOAT:
				{
					float f = setting->floatValue();
					if (ImGui::InputFloat(tmpStr, &f, 0.25f, 0.0f, "%.4f")) {
						setting->setFloat(f);
					}
				}
				break;
			case ValueType::BOOL:
				{
					bool b = setting->boolValue();
					if (ImGui::Checkbox(tmpStr, &b)) {
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

// Console: State methods
// ------------------------------------------------------------------------------------------------

void Console::init(Allocator* allocator, uint32_t numWindowsToDock, const char* const* windowNames) noexcept
{
	// Allocate ConsoleState and set allocator
	this->destroy();
	mState = allocator->newObject<ConsoleState>(sfz_dbg("ConsoleState"));
	mState->allocator = allocator;

	// Check if this is first run of imgui or not. I.e., whether imgui.ini existed or not.
	mState->imguiFirstRun = !fileExists("imgui.ini");

	// Pick out console settings
	GlobalConfig& cfg = getGlobalConfig();
	mState->showInGamePreview =
		cfg.sanitizeBool("Console", "showInGamePreview", true, BoolBounds(false));
	mState->logMinLevelSetting = cfg.sanitizeInt("Console", "logMinLevel", false, IntBounds(0, 0, 3));

	// Global Config
	mState->cfgSections.init(32, allocator, sfz_dbg("ConsoleState member"));
	mState->cfgSectionSettings.init(64, allocator, sfz_dbg("ConsoleState member"));

	// Injected window names
	for (uint32_t i = 0; i < numWindowsToDock; i++) {
		mState->injectedWindowNames.add(str96("%s", windowNames[i]));
	}
}

void Console::swap(Console& other) noexcept
{
	std::swap(this->mState, other.mState);
}

void Console::destroy() noexcept
{
	if (mState == nullptr) return;
	sfz::Allocator* allocator = mState->allocator;
	allocator->deleteObject(mState);
	mState = nullptr;
}

// Console: Methods
// ------------------------------------------------------------------------------------------------

void Console::toggleActive() noexcept
{
	mState->active = !mState->active;
}

bool Console::active() noexcept
{
	return mState->active;
}

void Console::render(float deltaSampleMs) noexcept
{
	// Update performance stats
	if (mState->statsWarmup >= 8) mState->stats.addSample(deltaSampleMs);
	mState->statsWarmup++;

	// Render in-game console preview
	if (!mState->active && mState->showInGamePreview->boolValue()) {
		renderConsoleInGamePreview(*mState);
	}

	// Return if console should not be rendered
	if (!mState->active) return;

	// Console dock space
	renderConsoleDockSpace(*mState);

	// Render console windows
	renderPerformanceWindow(*mState);
	renderLogWindow(*mState);
	renderConfigWindow(*mState);

	// Render custom-injected windows
	//mLogic->injectConsoleMenu();

	// Initialize dockspace with default docked layout if first run
	if (mState->imguiFirstRun) renderConsoleDockSpaceInitialize(*mState);
	mState->imguiFirstRun = false;
}

} // namespace sfz
