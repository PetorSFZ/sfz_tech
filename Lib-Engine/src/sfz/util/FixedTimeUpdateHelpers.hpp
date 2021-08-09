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

#include <sfz.h>

namespace sfz {

// FixedTimeStepper
// ------------------------------------------------------------------------------------------------

// A simple class that can be used in the update function to run a given tick update function every
// tick.
class FixedTimeStepper final {
public:
	// The current accumulated (and unused) time
	f32 accumulatorSecs = 0.0f;

	// The length of a tick in seconds
	f32 tickTimeSecs = 1.0f / 100.0f;

	template<typename F>
	u32 runTickUpdates(f32 deltaTimeSecs, F tickUpdateFunc) noexcept
	{
		u32 numTicksRan = 0;
		accumulatorSecs += deltaTimeSecs;
		while (accumulatorSecs > tickTimeSecs) {
			tickUpdateFunc(tickTimeSecs);
			accumulatorSecs -= tickTimeSecs;
			numTicksRan += 1;
		}
		return numTicksRan;
	}
};

} // namespace sfz
