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

#ifndef SFZ_TIME_H
#define SFZ_TIME_H
#pragma once

#include "sfz.h"

// Platform specific nonsense
// ------------------------------------------------------------------------------------------------

#if defined(_MSC_VER)

// Forward declare QueryPerformanceCounter() and QueryPerformanceFrequency() from windows.h
union _LARGE_INTEGER;
sfz_extern_c __declspec(dllimport) i32 __stdcall QueryPerformanceCounter(_LARGE_INTEGER * lpPerformanceCount);
sfz_extern_c __declspec(dllimport) i32 __stdcall QueryPerformanceFrequency(_LARGE_INTEGER * lpFrequency);

// Helper functions to call above
inline i64 sfzQueryPerfCounter(void)
{
	i64 time = 0;
	const i32 res = QueryPerformanceCounter((_LARGE_INTEGER*)&time);
	sfz_assert(res);
	return time;
}
inline i64 sfzQueryPerfFreq(void)
{
	i64 freq = 0;
	const i32 res = QueryPerformanceFrequency((_LARGE_INTEGER*)&freq);
	sfz_assert(res);
	return freq;
}

#else
#error "Not implemented for this compiler"
#endif

// Time API
// ------------------------------------------------------------------------------------------------

sfz_struct(SfzTime) {
	i64 us; // Microseconds
#ifdef __cplusplus
	f32 ms() const { return f32(f64(us) / 1000.0); }
	f32 s() const { return f32(f64(us) / 1000000.0); }
	i32 wholeSecs() const { return i32(ms() / 1000.0f); }
	f32 subSecMillis() const { return (s() - f32(wholeSecs())) * 1000.0f; }
	constexpr bool operator== (SfzTime o) const { return this->us == o.us; }
	constexpr bool operator!= (SfzTime o) const { return this->us != o.us; }
#endif
};

inline SfzTime sfzTimeNow(void)
{
	const i64 freq = sfzQueryPerfFreq();
	const i64 timestamp = sfzQueryPerfCounter();
	SfzTime t = {};
	t.us = (timestamp * i64(1000000)) / freq;
	return t;
}

inline SfzTime sfzTimeDiff(SfzTime before, SfzTime after)
{
	SfzTime diff = {};
	diff.us = after.us - before.us;
	return diff;
}

inline SfzTime sfzTimeSinceLastCall(SfzTime* lastTime)
{
	const SfzTime currTime = sfzTimeNow();
	const SfzTime diff = sfzTimeDiff(*lastTime, currTime);
	*lastTime = currTime;
	return diff;
}

#endif // SFZ_TIME_H
