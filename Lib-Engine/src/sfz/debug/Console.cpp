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

#include "sfz/debug/Console.hpp"

#include <cstring>
#include <ctime>

#include <skipifzero_arrays.hpp>
#include <skipifzero_new.hpp>
#include <skipifzero_strings.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <imgui_plot.h>

#include "sfz/SfzLogging.h"
#include "sfz/audio/AudioEngine.hpp"
#include "sfz/config/GlobalConfig.hpp"
#include "sfz/config/SfzConfig.h"
#include "sfz/debug/ProfilingStats.hpp"
#include "sfz/renderer/Renderer.hpp"
#include "sfz/resources/ResourceManager.hpp"
#include "sfz/shaders/ShaderManager.hpp"
#include "sfz/util/ImGuiHelpers.hpp"
#include "sfz/util/IO.hpp"

namespace sfz {

// ConsoleState
// ------------------------------------------------------------------------------------------------

struct ConsoleState final {

	SfzAllocator* allocator = nullptr;

	// Console settings
	bool active = false;
	bool imguiFirstRun = false;
	SfzSetting* imguiScale = nullptr;

	// Performance
	SfzSetting* showInGamePerf = nullptr;
	SfzSetting* inGamePerfWidth = nullptr;
	SfzSetting* inGamePerfHeight = nullptr;
	str64 categoryStr = str64("default");
	ArrayLocal<Array<f32>, PROFILING_STATS_MAX_NUM_LABELS> processedValues;

	// Global Config
	str32 configFilterString;
	Array<str32> cfgSections;
	Array<SfzSetting*> cfgSectionSettings;

	// Log
	SfzSetting* showInGameLog = nullptr;
	SfzSetting* inGameLogWidth = nullptr;
	SfzSetting* inGameLogHeight = nullptr;
	SfzSetting* inGameLogMaxAgeSecs = nullptr;
	SfzSetting* logMinLevelSetting = nullptr;
	str96 logFilter;

	// Injected windows
	Arr32<str96> injectedWindowNames;
};

// Statics
// ------------------------------------------------------------------------------------------------

static bool anyContainsFilter(const Array<SfzSetting*>& settings, const char* filter) noexcept
{
	for (SfzSetting* setting : settings) {
		if (strstr(setting->key(), filter) != nullptr) {
			return true;
		}
	}
	return false;
}

static void timeToString(str96& stringOut, time_t timestamp) noexcept
{
	std::tm* tmPtr = std::localtime(&timestamp);
	//size_t res = std::strftime(stringOut.mRawStr, stringOut.capacity(), "%Y-%m-%d %H:%M:%S", tmPtr);
	size_t res = std::strftime(stringOut.mRawStr, stringOut.capacity(), "%H:%M:%S", tmPtr);
	if (res == 0) {
		stringOut.clear();
		stringOut.appendf("INVALID TIME");
	}
}

// Returns whether window is docked or not, small hack to create initial docked layout
static void renderPerformanceWindow(
	ConsoleState& state, bool isPreview, const SfzProfilingStats* profStats) noexcept
{
	// Get information about the selected category from the stats
	const u32 numLabels = profStats->numLabels(state.categoryStr);
	const u32 numSamples = profStats->numSamples(state.categoryStr);
	const char* const* labelStrs = profStats->labels(state.categoryStr);
	const char* idxUnit = profStats->idxUnit(state.categoryStr);
	const char* sampleUnit = profStats->sampleUnit(state.categoryStr);
	SfzStatsVisualizationType visType = profStats->visualizationType(state.categoryStr);

	// Retrieve sample arrays, colors and stats for each label in the selected category
	// Also get the worst max
	ArrayLocal<const f32*, PROFILING_STATS_MAX_NUM_LABELS> valuesList;
	ArrayLocal<ImU32, PROFILING_STATS_MAX_NUM_LABELS> colorsList;
	ArrayLocal<SfzLabelStats, PROFILING_STATS_MAX_NUM_LABELS> labelStats;
	f32 worstMax = -F32_MAX;
	u32 longestLabelStr = 0;
	for (u32 i = 0; i < numLabels; i++) {

		// Get samples
		const char* label = labelStrs[i];
		const f32* samples = profStats->samples(state.categoryStr, label);

		// Process samples and copy them to temp array, add temp array to valuesList
		Array<f32>& processed = state.processedValues[i];
		processed.ensureCapacity(numSamples);
		processed.clear();
		processed.add(samples, numSamples);
		valuesList.add(processed.data());

		// Get color and stats
		f32x4 color = profStats->color(state.categoryStr, label);
		colorsList.add(ImGui::GetColorU32(color));
		SfzLabelStats labelStat = profStats->stats(state.categoryStr, label);
		labelStats.add(labelStat);

		// Calculate worst max from stats and longest label string
		worstMax = sfz::max(worstMax, labelStat.max);
		longestLabelStr = sfz::max(longestLabelStr, u32(strnlen(label, 33)));
	}

	// Combine samples if requested
	if (visType == SfzStatsVisualizationType::FIRST_INDIVIDUALLY_REST_ADDED) {
		for (u32 i = 1; i < numLabels; i++) {
			Array<f32>& targetVals = state.processedValues[i];
			for (u32 j = i + 1; j < numLabels; j++) {
				const Array<f32>& readVals = state.processedValues[j];
				for (u32 k = 0; k < numSamples; k++) {
					targetVals[k] += readVals[k];
				}
			}
		}
	}

	// Create (most of) plot config
	ImGui::PlotConfig conf;
	conf.values.xs = profStats->sampleIndicesFloat(state.categoryStr);
	conf.values.count = int(numSamples);
	conf.values.ys_count = int(numLabels);
	conf.values.ys_list = valuesList.data();
	conf.values.colors = colorsList.data();

	conf.scale.min = 0.0f;
	const f32 smallestPlotMax = profStats->smallestPlotMax(state.categoryStr);
	conf.scale.max = sfz::max(worstMax, smallestPlotMax);

	conf.tooltip.show = true;
	str64 tooltipFormat("%s %%.0f: %%.2f %s", idxUnit, sampleUnit);
	conf.tooltip.format = tooltipFormat.str();

	conf.grid_x.show = true;
	conf.grid_x.size = 60;
	conf.grid_x.subticks = 1;


	// Preview version
	if (isPreview) {

		// Calculate window size
		f32x2 windowSize =
			f32x2(f32(state.inGamePerfWidth->intValue()), f32(state.inGamePerfHeight->intValue()));
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(f32x2(0.0f), ImGuiCond_Always);

		// Begin window
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
		ImGui::PushStyleColor(ImGuiCol_WindowBg, f32x4(0.05f, 0.05f, 0.05f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_Border, f32x4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Console Preview", nullptr, windowFlags);

		// Render performance numbers
		ImGui::SetNextItemWidth(800.0f);
		ImGui::BeginGroup();
		for (u32 i = 0; i < numLabels; i++) {
			ImGui::PushStyleColor(ImGuiCol_Text, colorsList[i]);
			str64 format("%%-%us  avg %%5.1f %%s   max %%5.1f %%s", longestLabelStr);
			ImGui::Text(format.str(),
				labelStrs[i], labelStats[i].avg, sampleUnit, labelStats[i].max, sampleUnit);
			ImGui::PopStyleColor();
		}
		ImGui::EndGroup();

		// Render plot
		const f32x2 infoDims = ImGui::GetItemRectMax();
		ImGui::SameLine();
		f32x2 plotDims = f32x2(ImGui::GetWindowSize()) - f32x2(infoDims.x + 10.0f, 20.0f);
		conf.frame_size = plotDims;
		conf.line_thickness = 1.0f;
		ImGui::Plot("##PerformanceGraph", conf);

		// End window
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		return;
	}

	// If not preview
	else {

		// Begin window
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_NoScrollbar;
		windowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
		windowFlags |= ImGuiWindowFlags_NoNav;
		ImGui::Begin("Perf", nullptr, windowFlags);

		// Tab bar
		if (ImGui::BeginTabBar("PerformanceTabBar")) {
			const u32 numCategories = profStats->numCategories();
			const char* const* categories = profStats->categories();
			for (u32 i = 0; i < numCategories; i++) {
				if (ImGui::BeginTabItem(str96("%s##PerfBar", categories[i]))) {
					state.categoryStr.clear();
					state.categoryStr.appendf("%s", categories[i]);
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		// Render plot
		f32x2 plotDims = f32x2(ImGui::GetWindowSize().y, 360);
		conf.frame_size = plotDims;
		conf.line_thickness = 1.0f;
		ImGui::Plot("##PerformanceGraph", conf);

		// Render performance numbers
		for (u32 i = 0; i < numLabels; i++) {
			ImGui::PushStyleColor(ImGuiCol_Text, colorsList[i]);
			str64 format("%%-%us  avg %%5.1f %%s   max %%5.1f %%s", longestLabelStr);
			ImGui::Text(format.str(),
				labelStrs[i], labelStats[i].avg, sampleUnit, labelStats[i].max, sampleUnit);
			ImGui::PopStyleColor();
		}

		// End window
		ImGui::End();
		return;
	}
}

static void renderLogWindow(ConsoleState& state, f32x2 imguiWindowRes, bool isPreview, f32 maxAgeSecs = 6.0f) noexcept
{
	constexpr f32x4 filterTextColor = f32x4(1.0f, 0.0f, 0.0f, 1.0f);
	str96 timeStr;

	auto getMessageColor = [](SfzLogLevel level) -> f32x4 {
		switch (level) {
		case SFZ_LOG_LEVEL_NOISE: return f32x4(0.6f, 0.6f, 0.8f, 1.0f);
		case SFZ_LOG_LEVEL_INFO: return f32x4(0.8f, 0.8f, 0.8f, 1.0f);
		case SFZ_LOG_LEVEL_WARNING: return f32x4(1.0f, 1.0f, 0.0f, 1.0f);
		case SFZ_LOG_LEVEL_ERROR: return f32x4(1.0f, 0.0f, 0.0f, 1.0f);
		}
		sfz_assert(false);
		return f32x4(1.0f);
	};

	if (isPreview) {

		// Find how many active messages there are
		const u32 numMessages = sfzLoggingCurrentNumMessages();
		const u32 numActiveMessages = sfzLoggingGetNumMessagesWithAgeLessThan(maxAgeSecs);

		// Exit if no active messages
		if (numActiveMessages == 0) return;

		// Calculate window size
		const f32x2 windowSize =
			f32x2(f32(state.inGameLogWidth->intValue()), f32(state.inGameLogHeight->intValue()));
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(imguiWindowRes - windowSize - f32x2(5.0f), ImGuiCond_Always);

		// Begin window
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
		ImGui::PushStyleColor(ImGuiCol_WindowBg, f32x4(0.05f, 0.05f, 0.05f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_Border, f32x4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Log Preview", nullptr, windowFlags);

		for (u32 i = 0; i < numActiveMessages; i++) {
			// Reverse order, newest first
			const u32 msgIdx = numMessages - i - 1;
			const char* msgFile = sfzLoggingGetMessageFile(msgIdx);
			const i32 msgLine = sfzLoggingGetMessageLine(msgIdx);
			const SfzLogLevel msgLevel = sfzLoggingGetMessageLevel(msgIdx);
			const time_t msgTimestamp = time_t(sfzLoggingGetMessageTimestamp(msgIdx));
			const char* msg = sfzLoggingGetMessageMessage(msgIdx);

			// Skip if log level is too low
			if (i32(msgLevel) < state.logMinLevelSetting->intValue()) continue;

			// Print message header
			const f32x4 messageColor = getMessageColor(msgLevel);
			timeToString(timeStr, msgTimestamp);
			imguiPrintText(str64("[%s] - [%s:%i]", timeStr.str(), msgFile, msgLine), messageColor);

			ImGui::Spacing();

			// Print message
			ImGui::PushStyleColor(ImGuiCol_Text, messageColor);
			ImGui::TextWrapped("%s", msg);
			ImGui::PopStyleColor();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		return;
	}

	ImGui::SetNextWindowPos(f32x2(0.0f, 130.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(f32x2(800, 800), ImGuiCond_FirstUseEver);

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
	ImGui::InputText("##Tag filter", state.logFilter.mRawStr, state.logFilter.capacity());
	ImGui::PopItemWidth();
	ImGui::SameLine();
	state.logFilter.toLower();
	const bool logFilterMode = state.logFilter != "";

	int logMinLevelVal = state.logMinLevelSetting->intValue();
	ImGui::PushItemWidth(160.0f);
	constexpr const char* LOG_LEVEL_STRINGS[] = {
		"NOISE",
		"INFO",
		"WARNING",
		"ERROR"
	};
	ImGui::Combo("##Minimum log level", &logMinLevelVal, LOG_LEVEL_STRINGS,
		IM_ARRAYSIZE(LOG_LEVEL_STRINGS));
	ImGui::PopItemWidth();
	state.logMinLevelSetting->setInt(logMinLevelVal);

	ImGui::PopStyleColor();

	ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
	if (ImGui::Button("Clear messages")) {
		sfzLoggingClearMessages();
	}

	// Print all messages
	ImGui::BeginChild("LogItems");
	const u32 numLogMessages = sfzLoggingCurrentNumMessages();
	for (u32 i = 0; i < numLogMessages; i++) {
		// Reverse order, newest first
		const u32 msgIdx = numLogMessages - i - 1;
		const char* msgFile = sfzLoggingGetMessageFile(msgIdx);
		const i32 msgLine = sfzLoggingGetMessageLine(msgIdx);
		const SfzLogLevel msgLevel = sfzLoggingGetMessageLevel(msgIdx);
		const time_t msgTimestamp = time_t(sfzLoggingGetMessageTimestamp(msgIdx));
		const char* msg = sfzLoggingGetMessageMessage(msgIdx);

		// Skip if log level is too low
		if (i32(msgLevel) < state.logMinLevelSetting->intValue()) continue;

		// Skip message if nothing matches when filtering
		if (logFilterMode) {
			str128 fileLowerStr("%s", msgFile);
			fileLowerStr.toLower();
			bool tagFilter = strstr(fileLowerStr.str(), state.logFilter.str()) != nullptr;
			if (!tagFilter) continue;
		}

		// Print message header
		const f32x4 messageColor = getMessageColor(msgLevel);
		timeToString(timeStr, msgTimestamp);
		imguiPrintText(str64("[%s] - [", timeStr.str()), messageColor);
		ImGui::SameLine();
		imguiRenderFilteredText(msgFile, state.logFilter, messageColor, filterTextColor);
		ImGui::SameLine();
		imguiPrintText(str96(": %i ]", msgLine), messageColor);

		ImGui::Spacing();

		// Print message
		ImGui::PushStyleColor(ImGuiCol_Text, messageColor);
		ImGui::TextWrapped("%s", msg);
		ImGui::PopStyleColor();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}

	// Show last message by default
	ImGui::EndChild();

	// Return to 1 column
	ImGui::Columns(1);

	// End window
	ImGui::End();
}

static void renderConfigWindow(ConsoleState& state, SfzGlobalConfig& cfg) noexcept
{
	const f32x4 filterTextColor = f32x4(1.0f, 0.0f, 0.0f, 1.0f);
	str256 tmpStr;

	// Get Global Config sections
	state.cfgSections.clear();
	cfg.getSections(state.cfgSections);

	// Set window size
	ImGui::SetNextWindowPos(f32x2(300.0f * 1.25f + 17.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(f32x2(400.0f, 0.0f), ImGuiCond_FirstUseEver);

	// Set window flags
	ImGuiWindowFlags configWindowFlags = 0;
	//configWindowFlags |= ImGuiWindowFlags_NoMove;
	//configWindowFlags |= ImGuiWindowFlags_NoResize;
	//configWindowFlags |= ImGuiWindowFlags_NoCollapse;
	configWindowFlags |= ImGuiWindowFlags_NoFocusOnAppearing;

	// Begin window
	ImGui::Begin("Cfg", nullptr, configWindowFlags);

	// Config filter string
	//ImGui::PushItemWidth(-1.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, filterTextColor);
	ImGui::InputText("Filter", state.configFilterString.mRawStr, state.configFilterString.capacity());
	ImGui::PopStyleColor();
	state.configFilterString.toLower();
	bool filterMode = state.configFilterString != "";

	// Add spacing and separator between filter and configs
	ImGui::Spacing();

	// Start columns
	ImGui::Columns(3);
	f32 windowWidth = ImGui::GetWindowSize().x;
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
			str32 sectionLowerStr("%s", sectionKey.str());
			sectionLowerStr.toLower();
			bool sectionFilter = strstr(sectionLowerStr, state.configFilterString) != nullptr;
			bool settingsFilter = anyContainsFilter(state.cfgSectionSettings, state.configFilterString);
			if (!sectionFilter && !settingsFilter) continue;
		}

		// Write header
		ImGui::Columns(1);
		if (filterMode) {
			ImGui::Separator();
			imguiRenderFilteredText(
				sectionKey, state.configFilterString, f32x4(1.0f), filterTextColor);
		}
		else {
			if (ImGui::CollapsingHeader(sectionKey)) continue;
		}
		ImGui::Columns(3);
		ImGui::SetColumnWidth(0, 55.0f);
		ImGui::SetColumnWidth(1, windowWidth - 275.0f);
		ImGui::SetColumnWidth(2, 200.0f);

		for (SfzSetting* setting : state.cfgSectionSettings) {

			// Combine key strings
			str128 combinedKeyStr("%s%s", sectionKey.str(), setting->key().str());
			str128 combinedKeyLowerStr("%s", combinedKeyStr.str());
			combinedKeyLowerStr.toLower();

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
				imguiRenderFilteredText(
					setting->key(), state.configFilterString, f32x4(1.0f), filterTextColor);
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
			case SfzValueType::INT:
				{
					i32 i = setting->intValue();
					if (ImGui::InputInt(tmpStr, &i, setting->value().i.bounds.step)) {
						setting->setInt(i);
					}
				}
				break;
			case SfzValueType::FLOAT:
				{
					f32 f = setting->floatValue();
					if (ImGui::InputFloat(tmpStr, &f, 0.25f, 0.0f, "%.4f")) {
						setting->setFloat(f);
					}
				}
				break;
			case SfzValueType::BOOL:
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

void Console::init(SfzAllocator* allocator, SfzGlobalConfig* cfg, u32 numWindowsToDock, const char* const* windowNames) noexcept
{
	// Allocate ConsoleState and set allocator
	this->destroy();
	mState = sfz_new<ConsoleState>(allocator, sfz_dbg("ConsoleState"));
	mState->allocator = allocator;

	// Check if this is first run of imgui or not. I.e., whether imgui.ini existed or not.
	mState->imguiFirstRun = !fileExists("imgui.ini");

	// Initialize some temp arrays with allocators
	for (u32 i = 0; i < mState->processedValues.capacity(); i++) {
		mState->processedValues.add(Array<f32>(0, allocator, sfz_dbg("")));
	}

	// Pick out console settings
	mState->imguiScale = cfg->sanitizeFloat("Imgui", "scale", true, 1.25f, 1.0f, 3.0f);
	mState->showInGamePerf =
		cfg->sanitizeBool("Console", "showInPerfPreview", true, SfzBoolBounds(false));
	mState->inGamePerfWidth = cfg->sanitizeInt("Console", "inGamePerfWidth", true, SfzIntBounds(700, 500, 1500, 50));
	mState->inGamePerfHeight = cfg->sanitizeInt("Console", "inGamePerfHeight", true, SfzIntBounds(100, 50, 500, 25));
	mState->showInGameLog = cfg->sanitizeBool("Console", "showInGameLog", true, true);
	mState->inGameLogWidth = cfg->sanitizeInt("Console", "inGameLogWidth", true, SfzIntBounds(1000, 700, 1500, 50));
	mState->inGameLogHeight = cfg->sanitizeInt("Console", "inGameLogHeight", true, SfzIntBounds(600, 400, 2000, 50));
	mState->inGameLogMaxAgeSecs = cfg->sanitizeFloat("Console", "inGameLogMaxAgeSecs", false, 2.0f, 0.1f, 10.0f);
	mState->logMinLevelSetting = cfg->sanitizeInt("Console", "logMinLevel", false, SfzIntBounds(0, 0, 3));

	// Global Config
	mState->cfgSections.init(32, allocator, sfz_dbg("ConsoleState member"));
	mState->cfgSectionSettings.init(64, allocator, sfz_dbg("ConsoleState member"));

	// Injected window names
	for (u32 i = 0; i < numWindowsToDock; i++) {
		mState->injectedWindowNames.add(str96("%s", windowNames[i]));
	}
}

void Console::swap(Console& other) noexcept
{
	sfz::swap(this->mState, other.mState);
}

void Console::destroy() noexcept
{
	if (mState == nullptr) return;
	SfzAllocator* allocator = mState->allocator;
	sfz_delete(allocator, mState);
	mState = nullptr;
}

// Console: Methods
// ------------------------------------------------------------------------------------------------

void Console::toggleActive() noexcept
{
	mState->active = !mState->active;
}

bool Console::active() const noexcept
{
	return mState->active;
}

void Console::render(
	i32x2 windowRes,
	SfzStrIDs* ids,
	SfzConfig* cfg,
	SfzRenderer* renderer,
	SfzShaderManager* shaderMan,
	SfzResourceManager* resMan,
	const SfzProfilingStats* profStats) noexcept
{
	const f32x2 imguiWindowRes = f32x2(windowRes) / mState->imguiScale->floatValue();

	// Render in-game previews
	if (!mState->active && mState->showInGamePerf->boolValue()) {
		renderPerformanceWindow(*mState, true, profStats);
	}
	if (!mState->active && mState->showInGameLog->boolValue()) {
		renderLogWindow(*mState, imguiWindowRes, true, mState->inGameLogMaxAgeSecs->floatValue());
	}

	// Return if console should not be rendered
	if (!mState->active) return;

	// Console dock space
	ImGuiID dockSpaceId = 0;
	{
		ImGuiDockNodeFlags dockSpaceFlags = 0;
		dockSpaceFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
		dockSpaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dockSpaceFlags);
	}

	// Render console windows
	renderLogWindow(*mState, imguiWindowRes, false);
	renderConfigWindow(*mState, *sfzCfgGetLegacyConfig(cfg));
	renderPerformanceWindow(*mState, false, profStats);
	resMan->renderDebugUI(ids);
	shaderMan->renderDebugUI(ids);
	renderer->renderImguiUI();
	//getAudioEngine().renderDebugUI();

	// Initialize dockspace with default docked layout if first run
	if (mState->imguiFirstRun) {
		ImGui::DockBuilderRemoveNode(dockSpaceId);

		ImGuiDockNodeFlags dockSpaceFlags = 0;
		dockSpaceFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
		dockSpaceFlags |= ImGuiDockNodeFlags_DockSpace;
		ImGui::DockBuilderAddNode(dockSpaceId, dockSpaceFlags);
		ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->Size);

		ImGuiID dockMain = dockSpaceId;
		ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.35f, NULL, &dockMain);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.35f, NULL, &dockMain);

		ImGui::DockBuilderDockWindow("Log", dockBottom);
		ImGui::DockBuilderDockWindow("Cfg", dockLeft);
		ImGui::DockBuilderDockWindow("Perf", dockLeft);
		ImGui::DockBuilderDockWindow("Res", dockLeft);
		ImGui::DockBuilderDockWindow("Shaders", dockLeft);
		ImGui::DockBuilderDockWindow("Renderer", dockLeft);
		//ImGui::DockBuilderDockWindow("Audio", dockLeft);

		for (u32 i = 0; i < mState->injectedWindowNames.size(); i++) {
			const char* windowName = mState->injectedWindowNames[i].str();
			ImGui::DockBuilderDockWindow(windowName, dockLeft);
		}

		ImGui::DockBuilderFinish(dockSpaceId);
		mState->imguiFirstRun = false;
	}
}

} // namespace sfz
