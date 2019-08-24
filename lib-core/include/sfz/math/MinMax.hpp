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

// Implementations of min() and max() mathematical operations, similar to std::min() and std::max().
//
// The reason this is necessary is because std::min()/std::max() generate really poor code on
// Visual Studio. The below implementation seem to generate exactly the assembly we want (i.e.
// the minss/maxss x86 instructions for T=float) on Visual Studio 2019. They also seem to generate
// equally good code to std::min()/std::max() on Clang.
//
// See my tweet about it for experiments: https://twitter.com/PetorSFZ/status/1165330439269167104
//
// Pre-emptive FAQ:
// * Why name them sfzMin/sfzMax instead of sfz::min() and sfz::max()?
//    Two reasons:
//      1. Avoiding any ambiguity regarding whether sfzMin() or std::min() is used.
//      2. min/max are annoying names because they are often defined by windows.h, causing
//         trouble when you least expect.
// * Why not place them in namespace sfz like all the other sfzCore code?
//    Since we have prefixed their names with sfz there is literaly no point to also placing them
//    inside the sfz namespace. sfz::sfzMin() just looks stupid.

// sfzMin()
// ------------------------------------------------------------------------------------------------

constexpr float sfzMin(float lhs, float rhs) noexcept
{
	return (lhs < rhs) ? lhs : rhs;
}

constexpr int32_t sfzMin(int32_t lhs, int32_t rhs) noexcept
{
	return (lhs < rhs) ? lhs : rhs;
}

constexpr uint32_t sfzMin(uint32_t lhs, uint32_t rhs) noexcept
{
	return (lhs < rhs) ? lhs : rhs;
}

constexpr int64_t sfzMin(int64_t lhs, int64_t rhs) noexcept
{
	return (lhs < rhs) ? lhs : rhs;
}

constexpr uint64_t sfzMin(uint64_t lhs, uint64_t rhs) noexcept
{
	return (lhs < rhs) ? lhs : rhs;
}

// sfzMax()
// ------------------------------------------------------------------------------------------------

constexpr float sfzMax(float lhs, float rhs) noexcept
{
	return (lhs < rhs) ? rhs : lhs;
}

constexpr int32_t sfzMax(int32_t lhs, int32_t rhs) noexcept
{
	return (lhs < rhs) ? rhs : lhs;
}

constexpr uint32_t sfzMax(uint32_t lhs, uint32_t rhs) noexcept
{
	return (lhs < rhs) ? rhs : lhs;
}

constexpr int64_t sfzMax(int64_t lhs, int64_t rhs) noexcept
{
	return (lhs < rhs) ? rhs : lhs;
}

constexpr uint64_t sfzMax(uint64_t lhs, uint64_t rhs) noexcept
{
	return (lhs < rhs) ? rhs : lhs;
}
