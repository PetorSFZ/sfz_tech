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

namespace sfz {

// ProfilingStats
// ------------------------------------------------------------------------------------------------

constexpr uint32_t PROFILING_STATS_MAX_NUM_CATEGORIES = 8;
constexpr uint32_t PROFILING_STATS_MAX_NUM_LABELS = 80;

struct LabelStats final {
	float avg = 0.0f;
	float std = 0.0f;
	float min = 0.0f;
	float max = 0.0f;
};

struct ProfilingStatsState; // Pimpl pattern

class ProfilingStats final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ProfilingStats() noexcept = default;
	ProfilingStats(const ProfilingStats&) = delete;
	ProfilingStats& operator= (const ProfilingStats&) = delete;
	ProfilingStats(ProfilingStats&&) = delete;
	ProfilingStats& operator= (ProfilingStats&&) = delete;
	~ProfilingStats() noexcept { this->destroy(); }

	// State methods
	// --------------------------------------------------------------------------------------------

	void init(Allocator* allocator) noexcept;
	void destroy() noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------
	
	uint32_t numCategories() const noexcept;
	const char* const* categories() const noexcept;

	uint32_t numLabels(const char* category) const noexcept;
	const char* const* labels(const char* category) const noexcept;

	uint32_t numSamples(const char* category) const noexcept;
	const uint64_t* sampleIndices(const char* category) const noexcept;
	const float* sampleIndicesFloat(const char* category) const noexcept;
	const char* sampleUnit(const char* category) const noexcept;
	const char* idxUnit(const char* category) const noexcept;

	const float* samples(const char* category, const char* label) const noexcept;
	vec4 color(const char* category, const char* label) const noexcept;
	LabelStats stats(const char* category, const char* label) const noexcept;

	// Methods
	// --------------------------------------------------------------------------------------------

	void createCategory(
		const char* category,
		uint32_t numSamples,
		float sampleOutlierMax,
		const char* sampleUnit,
		const char* idxUnit) noexcept;

	void createLabel(
		const char* category,
		const char* label,
		vec4 color = vec4(1.0f),
		float defaultValue = 0.0f) noexcept;

	void addSample(
		const char* category, const char* label, uint64_t sampleIdx, float sample) noexcept;

private:
	ProfilingStatsState* mState = nullptr;
};

} // namespace sfz
