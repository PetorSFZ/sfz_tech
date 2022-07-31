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

#include <skipifzero.hpp>
#include <skipifzero_arrays.hpp>
#include <skipifzero_hash_maps.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_new.hpp>
#include <skipifzero_strings.hpp>

#include "sfz/util/RandomColors.hpp"

// ProfilingStatsState
// ------------------------------------------------------------------------------------------------

struct SfzStatsLabel final {
	f32x4 color = f32x4(1.0f);
	f32 defaultValue = 0.0f;
	sfz::Array<f32> samples;
};

struct SfzStatsCategory final {
	u32 numSamples = 0;
	f32 sampleOutlierMax = F32_MAX;
	sfz::str16 sampleUnit;
	sfz::str16 idxUnit;
	f32 smallestPlotMax = 0.0f;
	SfzStatsVisualizationType visualizationType = SfzStatsVisualizationType::INDIVIDUALLY;

	sfz::HashMapLocal<sfz::str32, SfzStatsLabel, PROFILING_STATS_MAX_NUM_LABELS> labels;
	sfz::ArrayLocal<sfz::str32, PROFILING_STATS_MAX_NUM_LABELS> labelStringBackings;
	sfz::ArrayLocal<const char*, PROFILING_STATS_MAX_NUM_LABELS> labelStrings;

	sfz::Array<u64> indices;
	sfz::Array<f32> indicesAsFloat;
};

struct SfzProfilingStatsState final {
	SfzAllocator* allocator = nullptr;
	sfz::HashMapLocal<sfz::str32, SfzStatsCategory, PROFILING_STATS_MAX_NUM_CATEGORIES> categories;
	sfz::ArrayLocal<sfz::str32, PROFILING_STATS_MAX_NUM_CATEGORIES> categoryStringBackings;
	sfz::ArrayLocal<const char*, PROFILING_STATS_MAX_NUM_CATEGORIES> categoryStrings;
};

// SfzProfilingStats: State methods
// ------------------------------------------------------------------------------------------------

void SfzProfilingStats::init(SfzAllocator* allocator) noexcept
{
	this->destroy();
	mState = sfz_new<SfzProfilingStatsState>(allocator, sfz_dbg("SfzProfilingStatsState"));
	mState->allocator = allocator;
}

void SfzProfilingStats::destroy() noexcept
{
	if (mState == nullptr) return;
	SfzAllocator* allocator = mState->allocator;
	sfz_delete(allocator, mState);
	mState = nullptr;
}

// SfzProfilingStats: Getters
// ------------------------------------------------------------------------------------------------

u32 SfzProfilingStats::numCategories() const noexcept
{
	return mState->categoryStrings.size();
}

const char* const* SfzProfilingStats::categories() const noexcept
{
	return mState->categoryStrings.data();
}

u32 SfzProfilingStats::numLabels(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->labelStrings.size();
}

const char* const* SfzProfilingStats::labels(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->labelStrings.data();
}

bool SfzProfilingStats::categoryExists(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	return cat != nullptr;
}

bool SfzProfilingStats::labelExists(const char* category, const char* label) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	if (cat == nullptr) return false;
	const SfzStatsLabel* lab = cat->labels.get(label);
	return lab != nullptr;
}

u32 SfzProfilingStats::numSamples(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->numSamples;
}

const u64* SfzProfilingStats::sampleIndices(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->indices.data();
}

const f32* SfzProfilingStats::sampleIndicesFloat(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->indicesAsFloat.data();
}

const char* SfzProfilingStats::sampleUnit(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->sampleUnit;
}

const char* SfzProfilingStats::idxUnit(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->idxUnit;
}

f32 SfzProfilingStats::smallestPlotMax(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->smallestPlotMax;
}

SfzStatsVisualizationType SfzProfilingStats::visualizationType(const char* category) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	return cat->visualizationType;
}

const f32* SfzProfilingStats::samples(const char* category, const char* label) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	const SfzStatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);
	return lab->samples.data();
}

f32x4 SfzProfilingStats::color(const char* category, const char* label) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	const SfzStatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);
	return lab->color;
}

SfzLabelStats SfzProfilingStats::stats(const char* category, const char* label) const noexcept
{
	const SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	const SfzStatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);

	// Calculate number of valid samples, total, min, max and average of the valid samples
	SfzLabelStats stats;
	u32 numValidSamples = 0;
	f32 total = 0.0f;
	stats.min = F32_MAX;
	stats.max = -F32_MAX;
	for (u32 i = cat->numSamples; i > 0; i--) {
		
		// Get index and check that it is valid (i.e. not 0, which is default)
		// TODO: Maybe bitset that marks each sample that has been set?
		u64 idx = cat->indices[i - 1];
		if (idx == 0) break;
		numValidSamples += 1;

		// Update total, min and max
		f32 sample = lab->samples[i - 1];
		total += sample;
		stats.min = sfz::min(stats.min, sample);
		stats.max = sfz::max(stats.max, sample);
	}
	stats.avg = numValidSamples != 0 ? total / f32(numValidSamples) : 0.0f;
	
	// Fix min and max if not set
	if (stats.min == F32_MAX) stats.min = lab->defaultValue;
	if (stats.max == -F32_MAX) stats.max = lab->defaultValue;

	// Calculate standard deviation
	f32 varianceSum = 0.0f;
	for (u32 i = cat->numSamples; i > 0; i--) {
		u64 idx = cat->indices[i - 1];
		if (idx == 0) break;
		f32 sample = lab->samples[i - 1];
		varianceSum += ((sample - stats.avg) * (sample - stats.avg));
	}
	stats.std = numValidSamples != 0 ? ::sqrtf(varianceSum / f32(numValidSamples)) : 0.0f;

	return stats;
}

// SfzProfilingStats: Methods
// ------------------------------------------------------------------------------------------------

void SfzProfilingStats::createCategory(
	const char* category,
	u32 numSamples,
	f32 sampleOutlierMax,
	const char* sampleUnit,
	const char* idxUnit,
	f32 smallestPlotMax,
	SfzStatsVisualizationType visualizationType) noexcept
{
	sfz_assert(mState->categories.get(category) == nullptr);
	sfz_assert(strnlen(category, 33) < 32);
	sfz_assert(strnlen(sampleUnit, 9) < 8);
	sfz_assert(strnlen(idxUnit, 9) < 8);

	// Add category
	SfzStatsCategory& cat = mState->categories.put(category, {});
	cat.numSamples = numSamples;
	cat.sampleOutlierMax = sampleOutlierMax;
	cat.sampleUnit.appendf("%s", sampleUnit);
	cat.idxUnit.appendf("%s", idxUnit);
	cat.smallestPlotMax = smallestPlotMax;
	cat.visualizationType = visualizationType;

	// Add category string
	mState->categoryStringBackings.add(sfz::str32("%s", category));
	mState->categoryStrings.add(mState->categoryStringBackings.last());

	// Add indices (0), fudge f32 variant so it has negative values until last one
	cat.indices.init(cat.numSamples, mState->allocator, sfz_dbg(""));
	cat.indices.add(0ull, cat.numSamples);
	cat.indicesAsFloat.init(cat.numSamples, mState->allocator, sfz_dbg(""));
	for (u32 i = 0; i < cat.numSamples; i++) {
		f32 val = -(f32(cat.numSamples) - f32(i) - 1.0f);
		cat.indicesAsFloat.add(val);
	}
	sfz_assert(cat.indices.capacity() == cat.numSamples);
	sfz_assert(cat.indicesAsFloat.capacity() == cat.numSamples);
}

void SfzProfilingStats::createLabel(
	const char* category,
	const char* label,
	f32x4 color,
	f32 defaultValue) noexcept
{
	sfz_assert(strnlen(label, 33) < 32);

	SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	sfz_assert(cat->labels.get(label) == nullptr);

	// Add label and fill with default values
	SfzStatsLabel& lab = cat->labels.put(label, {});
	lab.defaultValue = defaultValue;
	lab.samples.init(cat->numSamples, mState->allocator, sfz_dbg(""));
	lab.samples.add(defaultValue, cat->numSamples);

	// If no color specified, get random color
	if (sfz::elemMax(color) < 0.0f) {
		lab.color = f32x4(sfz::getRandomColor(cat->labels.size() - 1), 1.0f);
	}
	else {
		lab.color = color;
	}

	// Add label string
	cat->labelStringBackings.add(sfz::str32("%s", label));
	cat->labelStrings.add(cat->labelStringBackings.last());
}

void SfzProfilingStats::addSample(
	const char* category, const char* label, u64 sampleIdx, f32 sample) noexcept
{
	SfzStatsCategory* cat = mState->categories.get(category);
	sfz_assert(cat != nullptr);
	SfzStatsLabel* lab = cat->labels.get(label);
	sfz_assert(lab != nullptr);

	// Clamp sample against ceiling for this category
	sample = sfz::min(sample, cat->sampleOutlierMax);

	// Find latest matching idx
	sfz_assert(sampleIdx >= cat->indices.first());
	u32 insertLoc = ~0u;
	for (u32 i = cat->numSamples; i > 0; i--) {
		u64 idx = cat->indices[i - 1];
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
		cat->indicesAsFloat.add(f32(sampleIdx));
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
