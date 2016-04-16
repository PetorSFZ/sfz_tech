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

#define sfz_assert_debug_impl(condition) assert(condition)

#define sfz_assert_debug_m_impl(condition, message) \
{ \
	if (!condition) { \
		sfz::printErrorMessage("%s", message); \
		assert(condition); \
	} \
}

#else
#define sfz_assert_debug_impl(condition) ((void)0)
#define sfz_assert_debug_m_impl(condition, message) ((void)0)
#endif

// Release assert
// ------------------------------------------------------------------------------------------------

#if !defined(SFZ_NO_ASSERTIONS)

#define sfz_assert_release_impl(condition) \
{ \
	if (!condition) { \
		assert(condition); \
		sfz::terminateProgram(); \
	} \
}

#define sfz_assert_release_m_impl(condition, message) \
{ \
	if (!condition) { \
		sfz::printErrorMessage("%s", message); \
		assert(condition); \
		sfz::terminateProgram(); \
	} \
}

#else
#define sfz_assert_release_impl(condition) ((void)0)
#define sfz_assert_release_m_impl(condition, message) ((void)0)
#endif

// Errors
// ------------------------------------------------------------------------------------------------

#if !defined(SFZ_DISABLE_ERRORS)

#define sfz_error_impl(message) \
{ \
	sfz::printErrorMessage("%s", message); \
	sfz::terminateProgram(); \
}

#else
#define sfz_error_impl(message) ((void)0)
#endif
