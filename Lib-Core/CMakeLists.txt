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
		# /permissive- = This option disables permissive behaviors, and sets the /Zc compiler options for strict conformance
		# /Zc:twoPhase- = Disables two-phase name lookup. Needed because it is not compatible with OpenMP
		# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
		# /Zf = Faster pdb generation
		# /EHsc = TODO: Add explanation
		# /GR- = Disable RTTI
		# /MD(d) = Uses multi-threaded dynamic run-time library
		# /arch:AVX = Enable (require) Intel AVX instructions for code generation
		# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
		# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
		# /Od = "disables optimization, speeding compilation and simplifying debugging"
		# /O2 = Optimize code for fastest speed
		# /Ob3 = "which is a more aggressive version of -Ob2"
		# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
		# /DNDEBUG = defines the "NDEBUG" macro, which disables asserts
		set(CMAKE_CXX_FLAGS "/W4 /permissive- /Zc:twoPhase- /Zi /Zf /EHsc /GR- /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
		set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Od")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /O2 /Ob3 /fp:fast")
		set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob3 /fp:fast /DNDEBUG")

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
elseif(SFZ_CORE_BUILD_TESTS)
	if (NOT UTESTH_FOUND)
		message(FATAL_ERROR "sfz_core requires utest.h.")
	endif()
endif()

# Lib-Core
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

set(INCLUDE_FILES
	${INCLUDE_DIR}/sfz.h
	${INCLUDE_DIR}/skipifzero.hpp
	${INCLUDE_DIR}/skipifzero_allocators.hpp
	${INCLUDE_DIR}/skipifzero_arrays.hpp
	${INCLUDE_DIR}/skipifzero_defer.hpp
	${INCLUDE_DIR}/skipifzero_geometry.hpp
	${INCLUDE_DIR}/skipifzero_hash_maps.hpp
	${INCLUDE_DIR}/skipifzero_image_view.hpp
	${INCLUDE_DIR}/skipifzero_math.hpp
	${INCLUDE_DIR}/skipifzero_new.hpp
	${INCLUDE_DIR}/skipifzero_pool.hpp
	${INCLUDE_DIR}/skipifzero_ring_buffers.hpp
	${INCLUDE_DIR}/skipifzero_strings.hpp
	
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

# Empty target in order to add headers to IDE
add_custom_target(sfz_core SOURCES ${INCLUDE_FILES})

# sfz_core lib-core tests
# ------------------------------------------------------------------------------------------------

if(NOT hasParent OR SFZ_CORE_BUILD_TESTS)

	set(TEST_FILES
		${TESTS_DIR}/LibCoreHeadersTestsMain.cpp
		${TESTS_DIR}/skipifzero_allocators_tests.cpp
		${TESTS_DIR}/skipifzero_geometry_tests.cpp
		${TESTS_DIR}/skipifzero_arrays_tests.cpp
		${TESTS_DIR}/skipifzero_hash_maps_tests.cpp
		${TESTS_DIR}/skipifzero_new_tests.cpp
		${TESTS_DIR}/skipifzero_pool_tests.cpp
		${TESTS_DIR}/skipifzero_ring_buffers_tests.cpp
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
