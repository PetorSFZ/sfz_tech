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

#pragma once

#include <cstdint>

#include <skipifzero_arrays.hpp>

#include "sfz/strings/DynString.hpp"

namespace sfz {

using std::uint32_t;

/// Class used to calculate useful frametime statistics. All frametimes entered and received are
/// in milliseconds.
class FrametimeStats final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	FrametimeStats() = delete;
	FrametimeStats(const FrametimeStats&) = delete;
	FrametimeStats& operator= (const FrametimeStats&) = delete;
	FrametimeStats(FrametimeStats&&) noexcept = default;
	FrametimeStats& operator= (FrametimeStats&&) noexcept = default;

	FrametimeStats(uint32_t maxNumSamples) noexcept;

	// Public methods
	// --------------------------------------------------------------------------------------------

	const ArrayDynamic<float>& samples() const noexcept { return mSamples; }

	void addSample(float sampleInMs) noexcept;
	void reset() noexcept;

	inline uint32_t maxNumSamples() const noexcept { return mSamples.capacity(); }
	inline uint32_t currentNumSamples() const noexcept { return mSamples.size(); }
	inline float min() const noexcept { return mMin; }
	inline float max() const noexcept { return mMax; }
	inline float avg() const noexcept { return mAvg; }
	inline float sd() const noexcept { return mSD; }
	inline float time() const noexcept { return mTotalTime; }
	inline const char* toString() const noexcept { return mString.str(); }

private:
	// Private members
	// --------------------------------------------------------------------------------------------

	ArrayDynamic<float> mSamples;
	DynString mString;
	float mMin, mMax, mAvg, mSD, mTotalTime;
};

} // namespace sfz
