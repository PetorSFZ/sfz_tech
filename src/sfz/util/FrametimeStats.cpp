// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

#include "sfz/util/FrametimeStats.hpp"

#include <algorithm>

#include <sfz/Assert.hpp>

namespace sfz {

// FrametimeStats: Constructors & destructors
// ------------------------------------------------------------------------------------------------

FrametimeStats::FrametimeStats(uint32_t maxNumSamples) noexcept
{
	mSamples.create(maxNumSamples);
	this->reset();
}

// FrametimeStats: Public methods
// ------------------------------------------------------------------------------------------------

void FrametimeStats::addSample(float sampleInSeconds) noexcept
{
	sfz_assert_debug(mSamples.capacity() > 0);

	if (mSamples.size() == mSamples.capacity()) mSamples.remove(0);
	mSamples.add(sampleInSeconds);

	float sum = 0.0f;
	mMin = 1000000000.0f;
	mMax = -1000000000.0f;

	for (float sample : mSamples) {
		sum += sample;
		mMin = std::min(mMin, sample);
		mMax = std::max(mMax, sample);
	}
	mAvg = sum / float(mSamples.size());

	float varianceSum = 0.0f;
	for (float sample : mSamples) {
		varianceSum += ((sample - mAvg) * (sample - mAvg));
	}
	mSD = std::sqrt(varianceSum / float(mSamples.size()));

	mString.printf("Avg: %.1fms, SD: %.1fms, Min: %.1fms, Max: %.1fms",
	               mAvg * 1000.0f, mSD * 1000.0f, mMin * 1000.0f, mMax * 1000.0f);
}

void FrametimeStats::reset() noexcept
{
	mSamples.clear();
	mString.setCapacity(128);
	mString.clear();
	mMin = -1.0f;
	mMax = -1.0f;
	mAvg = -1.0f;
	mSD = -1.0f;
}

} // namespace sfz