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

// Debug assert
// ------------------------------------------------------------------------------------------------

#if !defined(SFZ_NO_DEBUG) && !defined(SFZ_NO_ASSERTIONS)

#ifdef _WIN32
#define sfz_assert_debug_impl(condition) { if (!(condition)) __debugbreak(); }
#else
// Just use standard assert on unknown platforms and hope it is not disabled
#define sfz_assert_debug_impl(condition) assert(condition)
#endif

#else
#define sfz_assert_debug_impl(condition) ((void)0)
#endif

// Release assert
// ------------------------------------------------------------------------------------------------

#if !defined(SFZ_NO_ASSERTIONS)

#ifdef _WIN32
#define sfz_assert_release_impl(condition) \
{ \
	if (!(condition)) { \
		__debugbreak(); \
		sfz::terminateProgram(); \
	} \
}
#else
#define sfz_assert_release_impl(condition) \
{ \
	if (!(condition)) { \
		assert((condition)); \
		sfz::terminateProgram(); \
	} \
}
#endif

#else
#define sfz_assert_release_impl(condition) ((void)0)
#endif
