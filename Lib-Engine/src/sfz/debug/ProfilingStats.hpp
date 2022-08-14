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

#include <sfz.h>

// ProfilingStats
// ------------------------------------------------------------------------------------------------

constexpr u32 PROFILING_STATS_MAX_NUM_CATEGORIES = 8;
constexpr u32 PROFILING_STATS_MAX_NUM_LABELS = 80;

struct SfzLabelStats final {
	f32 avg = 0.0f;
	f32 std = 0.0f;
	f32 min = 0.0f;
	f32 max = 0.0f;
};

enum class SfzStatsVisualizationType : u32 {
	INDIVIDUALLY,
	FIRST_INDIVIDUALLY_REST_ADDED
};

struct SfzProfilingStatsState; // Pimpl pattern

struct SfzProfilingStats final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	SfzProfilingStats() noexcept = default;
	SfzProfilingStats(const SfzProfilingStats&) = delete;
	SfzProfilingStats& operator= (const SfzProfilingStats&) = delete;
	SfzProfilingStats(SfzProfilingStats&&) = delete;
	SfzProfilingStats& operator= (SfzProfilingStats&&) = delete;
	~SfzProfilingStats() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(SfzAllocator* allocator) noexcept;
	void destroy() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------
	
	u32 numCategories() const noexcept;
	const char* const* categories() const noexcept;

	u32 numLabels(const char* category) const noexcept;
	const char* const* labels(const char* category) const noexcept;

	bool categoryExists(const char* category) const noexcept;
	bool labelExists(const char* category, const char* label) const noexcept;

	u32 numSamples(const char* category) const noexcept;
	const u64* sampleIndices(const char* category) const noexcept;
	const f32* sampleIndicesFloat(const char* category) const noexcept;
	const char* sampleUnit(const char* category) const noexcept;
	const char* idxUnit(const char* category) const noexcept;
	f32 smallestPlotMax(const char* category) const noexcept;
	SfzStatsVisualizationType visualizationType(const char* category) const noexcept;

	const f32* samples(const char* category, const char* label) const noexcept;
	f32x4 color(const char* category, const char* label) const noexcept;
	SfzLabelStats stats(const char* category, const char* label) const noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void createCategory(
		const char* category,
		u32 numSamples,
		f32 sampleOutlierMax,
		const char* sampleUnit,
		const char* idxUnit,
		f32 smallestPlotMax = 10.0f,
		SfzStatsVisualizationType visType = SfzStatsVisualizationType::INDIVIDUALLY) noexcept;

	void createLabel(
		const char* category,
		const char* label,
		f32x4 color = f32x4_splat(-1.0f),
		f32 defaultValue = 0.0f) noexcept;

	void addSample(
		const char* category, const char* label, u64 sampleIdx, f32 sample) noexcept;

private:
	SfzProfilingStatsState* mState = nullptr;
};
