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

#include "sfz/debug/ProfilingStats.hpp"

#include <cfloat>

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_new.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/util/RandomColors.hpp"

namespace sfz {

// ProfilingStatsState
// ------------------------------------------------------------------------------------------------

struct StatsLabel final {
	vec4 color = vec4(1.0f);
	float defaultValue = 0.0f;
	Array<float> samples;
};

struct StatsCategory final {
	uint32_t numSamples = 0;
	float sampleOutlierMax = FLT_MAX;
	str16 sampleUnit;
	str16 idxUnit;
	float smallestPlotMax = 0.0f;
	StatsVisualizationType visualizationType = StatsVisualizationType::INDIVIDUALLY;

	HashMapLocal<str32, StatsLabel, PROFILING_STATS_MAX_NUM_LABELS> labels;
	ArrayLocal<str32, PROFILING_STATS_MAX_NUM_LABELS> labelStringBackings;
	ArrayLocal<const char*, PROFILING_STATS_MAX_NUM_LABELS> labelStrings;

	Array<uint64_t> indices;
	Array<float> indicesAsFloat;
};

struct ProfilingStatsState final {
	Allocator* allocator = nullptr;
	HashMapLocal<str32, StatsCategory, PROFILING_STATS_MAX_NUM_CATEGORIES> categories;
	ArrayLocal<str32, PROFILING_STATS_MAX_NUM_CATEGORIES> categoryStringBackings;
	ArrayLocal<const char*, PROFILING_STATS_MAX_NUM_CATEGORIES> categoryStrings;
};

// ProfilingStats: State methods
// ------------------------------------------------------------------------------------------------

void ProfilingStats::init(Allocator* allocator) noexcept
{
	this->destroy();
	mState = sfz_new<ProfilingStatsState>(allocator, sfz_dbg("ProfilingStatsState"));
	mState->allocator = allocator;
}

void ProfilingStats::destroy() noexcept
{
	if (mState == nullptr) return;
	Allocator* allocator = mState->allocator;
	sfz_delete(allocator, mState);
	mState = nullptr;
}

// ProfilingStats: Getters
// ------------------------------------------------------------------------------------------------

uint32_t ProfilingStats::numCategories() const noexcept
{
	return mState->categoryStrings.size();
}

const char* const* ProfilingStats::categories() const noexcept
{
	return mState->categoryStrings.data();
}

uint32_t ProfilingStats::numLabels(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->labelStrings.size();
}

const char* const* ProfilingStats::labels(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->labelStrings.data();
}

bool ProfilingStats::categoryExists(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	return cat != nullptr;
}

bool ProfilingStats::labelExists(const char* category, const char* label) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	if (cat == nullptr) return false;
	const StatsLabel* lab = cat->labels.get(label);
	return lab != nullptr;
}

uint32_t ProfilingStats::numSamples(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->numSamples;
}

const uint64_t* ProfilingStats::sampleIndices(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->indices.data();
}

const float* ProfilingStats::sampleIndicesFloat(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->indicesAsFloat.data();
}

const char* ProfilingStats::sampleUnit(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->sampleUnit;
}

const char* ProfilingStats::idxUnit(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->idxUnit;
}

float ProfilingStats::smallestPlotMax(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->smallestPlotMax;
}

StatsVisualizationType ProfilingStats::visualizationType(const char* category) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->visualizationType;
}

const float* ProfilingStats::samples(const char* category, const char* label) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	const StatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);
	return lab->samples.data();
}

vec4 ProfilingStats::color(const char* category, const char* label) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	const StatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);
	return lab->color;
}

LabelStats ProfilingStats::stats(const char* category, const char* label) const noexcept
{
	const StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	const StatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);

	// Calculate number of valid samples, total, min, max and average of the valid samples
	LabelStats stats;
	uint32_t numValidSamples = 0;
	float total = 0.0f;
	stats.min = FLT_MAX;
	stats.max = -FLT_MAX;
	for (uint32_t i = cat->numSamples; i > 0; i--) {
		
		// Get index and check that it is valid (i.e. not 0, which is default)
		// TODO: Maybe bitset that marks each sample that has been set?
		uint64_t idx = cat->indices[i - 1];
		if (idx == 0) break;
		numValidSamples += 1;

		// Update total, min and max
		float sample = lab->samples[i - 1];
		total += sample;
		stats.min = sfz::min(stats.min, sample);
		stats.max = sfz::max(stats.max, sample);
	}
	stats.avg = numValidSamples != 0 ? total / float(numValidSamples) : 0.0f;
	
	// Fix min and max if not set
	if (stats.min == FLT_MAX) stats.min = lab->defaultValue;
	if (stats.max == -FLT_MAX) stats.max = lab->defaultValue;

	// Calculate standard deviation
	float varianceSum = 0.0f;
	for (uint32_t i = cat->numSamples; i > 0; i--) {
		uint64_t idx = cat->indices[i - 1];
		if (idx == 0) break;
		float sample = lab->samples[i - 1];
		varianceSum += ((sample - stats.avg) * (sample - stats.avg));
	}
	stats.std = numValidSamples != 0 ? std::sqrtf(varianceSum / float(numValidSamples)) : 0.0f;

	return stats;
}

// ProfilingStats: Methods
// ------------------------------------------------------------------------------------------------

void ProfilingStats::createCategory(
	const char* category,
	uint32_t numSamples,
	float sampleOutlierMax,
	const char* sampleUnit,
	const char* idxUnit,
	float smallestPlotMax,
	StatsVisualizationType visualizationType) noexcept
{
	sfz_assert(mState->categories.get(category) == nullptr);
	sfz_assert(strnlen(category, 33) < 32);
	sfz_assert(strnlen(sampleUnit, 9) < 8);
	sfz_assert(strnlen(idxUnit, 9) < 8);

	// Add category
	StatsCategory& cat = mState->categories.put(category, {});
	cat.numSamples = numSamples;
	cat.sampleOutlierMax = sampleOutlierMax;
	cat.sampleUnit.appendf("%s", sampleUnit);
	cat.idxUnit.appendf("%s", idxUnit);
	cat.smallestPlotMax = smallestPlotMax;
	cat.visualizationType = visualizationType;

	// Add category string
	mState->categoryStringBackings.add(str32("%s", category));
	mState->categoryStrings.add(mState->categoryStringBackings.last());

	// Add indices (0), fudge float variant so it has negative values until last one
	cat.indices.init(cat.numSamples, mState->allocator, sfz_dbg(""));
	cat.indices.add(0ull, cat.numSamples);
	cat.indicesAsFloat.init(cat.numSamples, mState->allocator, sfz_dbg(""));
	for (uint32_t i = 0; i < cat.numSamples; i++) {
		float val = -(float(cat.numSamples) - float(i) - 1.0f);
		cat.indicesAsFloat.add(val);
	}
	sfz_assert(cat.indices.capacity() == cat.numSamples);
	sfz_assert(cat.indicesAsFloat.capacity() == cat.numSamples);
}

void ProfilingStats::createLabel(
	const char* category,
	const char* label,
	vec4 color,
	float defaultValue) noexcept
{
	sfz_assert(strnlen(label, 33) < 32);

	StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	sfz_assert(cat->labels.get(label) == nullptr);

	// Add label and fill with default values
	StatsLabel& lab = cat->labels.put(label, {});
	lab.defaultValue = defaultValue;
	lab.samples.init(cat->numSamples, mState->allocator, sfz_dbg(""));
	lab.samples.add(defaultValue, cat->numSamples);

	// If no color specified, get random color
	if (elemMax(color) < 0.0f) {
		lab.color = vec4(getRandomColor(cat->labels.size() - 1), 1.0f);
	}
	else {
		lab.color = color;
	}

	// Add label string
	cat->labelStringBackings.add(str32("%s", label));
	cat->labelStrings.add(cat->labelStringBackings.last());
}

void ProfilingStats::addSample(
	const char* category, const char* label, uint64_t sampleIdx, float sample) noexcept
{
	StatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	StatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);

	// Clamp sample against ceiling for this category
	sample = sfz::min(sample, cat->sampleOutlierMax);

	// Find latest matching idx
	sfz_assert(sampleIdx >= cat->indices.first());
	uint32_t insertLoc = ~0u;
	for (uint32_t i = cat->numSamples; i > 0; i--) {
		uint64_t idx = cat->indices[i - 1];
		if (idx == sampleIdx) {
			insertLoc = (i - 1);
			break;
		}
		if (sampleIdx >= idx) break;
	}

	// Insertion location found, just insert sample
	if (insertLoc != ~0u) {
		lab->samples[insertLoc] = sample;
	}

	// If no insertion location found, insert sample last
	else {
		sfz_assert(cat->indices.last() <= sampleIdx);

		// Remove first sample index and insert new one last
		cat->indices.remove(0, 1);
		cat->indices.add(sampleIdx);
		sfz_assert(cat->indices.size() == cat->numSamples);

		cat->indicesAsFloat.remove(0, 1);
		cat->indicesAsFloat.add(float(sampleIdx));
		sfz_assert(cat->indicesAsFloat.size() == cat->numSamples);
		
		// Remove first sample and add default one last for all labels
		for (auto pair : cat->labels) {
			pair.value.samples.remove(0, 1);
			pair.value.samples.add(pair.value.defaultValue);
		}
		
		// Set last sample to the new sample for the given label
		lab->samples.last() = sample;
	}
}

} // namespace sfz
