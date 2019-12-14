cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project("sfz_core" LANGUAGES CXX)

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Check if sfzCore is built individually or part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

# Check platforms
# ------------------------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set(iOS true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(macOS true)
endif()

# Compiler flags
# ------------------------------------------------------------------------------------------------

# Set compiler flags only if building sfz_core individually
if(NOT hasParent)
	if(MSVC)
		# MSVC flags
		# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
		# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
		# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
		# /arch:AVX = Enable (require) Intel AVX instructions for code generation
		# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
		# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
		set(CMAKE_CXX_FLAGS "/W4 /std:c++17 /wd4201 /Zi /EHsc /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
		# /O2 = Optimize code for fastest speed
		# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
		# /DNDEBUG = defines the "NDEBUG" macro, which disables asserts
		set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DNDEBUG")
		# /DEBUG = "creates debugging information for the .exe file or DLL"
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DNDEBUG")
		# /Od = "disables optimization, speeding compilation and simplifying debugging"
		set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")

	elseif(macOS)
		# macOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-strict-aliasing")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	elseif(iOS)
		# iOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -DSFZ_IOS = Defines the "SFZ_IOS" macro
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -fno-strict-aliasing -DSFZ_IOS")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	else()
		# GCC / Clang flags
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -pthread -fno-strict-aliasing")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	endif()
endif()

# Dependencies
# ------------------------------------------------------------------------------------------------

# Externals directory
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../externals)

# utest.h (if building tests, i.e. if no parent)
# ${UTESTH_INCLUDE_DIRS}
if(NOT hasParent)
	add_subdirectory(${EXTERNALS_DIR}/utest.h ${CMAKE_BINARY_DIR}/utest.h)
endif()

# Lib-Core
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

set(INCLUDE_FILES
	${INCLUDE_DIR}/skipifzero.hpp
	${INCLUDE_DIR}/skipifzero_allocators.hpp
	${INCLUDE_DIR}/skipifzero_hash_maps.hpp
	${INCLUDE_DIR}/skipifzero_smart_pointers.hpp
	${INCLUDE_DIR}/skipifzero_strings.hpp
	${INCLUDE_DIR}/skipifzero_arrays.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

# Empty target in order to add headers to IDE
add_custom_target(sfz_core SOURCES ${INCLUDE_FILES})

# sfz_core lib-core tests
# ------------------------------------------------------------------------------------------------

if(NOT hasParent)

	set(TEST_FILES
		${TESTS_DIR}/LibCoreHeadersTestsMain.cpp
		${TESTS_DIR}/skipifzero_arrays_tests.cpp
		${TESTS_DIR}/skipifzero_hash_maps_tests.cpp
		${TESTS_DIR}/skipifzero_smart_pointers_tests.cpp
		${TESTS_DIR}/skipifzero_strings_tests.cpp
		${TESTS_DIR}/skipifzero_tests.cpp
	)
	source_group(TREE ${TESTS_DIR} FILES ${TEST_FILES})

	add_executable(sfz_core_tests ${TEST_FILES})
	target_include_directories(sfz_core_tests PRIVATE
		${INCLUDE_DIR}
		${UTESTH_INCLUDE_DIRS}
	)

endif()

# Output variables
# ------------------------------------------------------------------------------------------------

if (hasParent)
	set(SFZ_CORE_FOUND true PARENT_SCOPE)
	set(SFZ_CORE_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
endif()
