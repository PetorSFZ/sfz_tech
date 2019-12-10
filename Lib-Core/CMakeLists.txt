cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project("LibCore" LANGUAGES CXX)

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

# Set compiler flags only if building sfzCore individually
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

	elseif(EMSCRIPTEN)
		# Emscripten flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -s TOTAL_MEMORY=134217728 = 128MiB heap, TODO: should probably not hard-code this
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -fno-strict-aliasing -s TOTAL_MEMORY=134217728")
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

# Catch (if building tests, i.e. if no parent)
# ${CATCH_INCLUDE_DIRS}
if(NOT hasParent)
	add_subdirectory(${EXTERNALS_DIR}/catch ${CMAKE_BINARY_DIR}/catch)
endif()

# Lib-Core-Headers
# ${SFZ_CORE_HEADERS_FOUND}, ${SFZ_CORE_HEADERS_INCLUDE_DIRS}
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Lib-Core-Headers ${CMAKE_BINARY_DIR}/Lib-Core-Headers})

# Lib-Core
# ------------------------------------------------------------------------------------------------

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

set(INCLUDE_FILES
	${INCLUDE_DIR}/sfz/Context.hpp
	${INCLUDE_DIR}/sfz/CudaCompatibility.hpp
	${INCLUDE_DIR}/sfz/Logging.hpp
	${INCLUDE_DIR}/sfz/PopWarnings.hpp
	${INCLUDE_DIR}/sfz/PushWarnings.hpp
	${INCLUDE_DIR}/sfz/SimdIntrinsics.hpp

	${INCLUDE_DIR}/sfz/containers/RingBuffer.hpp
	${INCLUDE_DIR}/sfz/containers/RingBuffer.inl

	${INCLUDE_DIR}/sfz/geometry/AABB.hpp
	${INCLUDE_DIR}/sfz/geometry/AABB.inl
	${INCLUDE_DIR}/sfz/geometry/AABB2D.hpp
	${INCLUDE_DIR}/sfz/geometry/Circle.hpp
	${INCLUDE_DIR}/sfz/geometry/Intersection.hpp
	${INCLUDE_DIR}/sfz/geometry/Intersection.inl
	${INCLUDE_DIR}/sfz/geometry/OBB.hpp
	${INCLUDE_DIR}/sfz/geometry/OBB.inl
	${INCLUDE_DIR}/sfz/geometry/Plane.hpp
	${INCLUDE_DIR}/sfz/geometry/Plane.inl
	${INCLUDE_DIR}/sfz/geometry/Sphere.hpp

	${INCLUDE_DIR}/sfz/math/MathPrimitiveToStrings.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.inl
	${INCLUDE_DIR}/sfz/math/Matrix.hpp
	${INCLUDE_DIR}/sfz/math/Matrix.inl
	${INCLUDE_DIR}/sfz/math/ProjectionMatrices.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.inl

	${INCLUDE_DIR}/sfz/memory/ArenaAllocator.hpp
	${INCLUDE_DIR}/sfz/memory/DebugAllocator.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.hpp
	${INCLUDE_DIR}/sfz/memory/StandardAllocator.hpp

	${INCLUDE_DIR}/sfz/strings/DynString.hpp
	${INCLUDE_DIR}/sfz/strings/StringID.hpp

	${INCLUDE_DIR}/sfz/util/Enumerate.hpp
	${INCLUDE_DIR}/sfz/util/FrametimeStats.hpp
	${INCLUDE_DIR}/sfz/util/IniParser.hpp
	${INCLUDE_DIR}/sfz/util/IO.hpp
	${INCLUDE_DIR}/sfz/util/LoggingInterface.hpp
	${INCLUDE_DIR}/sfz/util/StandardLogger.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

set(SRC_FILES
	${SRC_DIR}/sfz/Context.cpp

	${SRC_DIR}/sfz/math/MathPrimitiveToStrings.cpp
	${SRC_DIR}/sfz/math/ProjectionMatrices.cpp

	${SRC_DIR}/sfz/memory/ArenaAllocator.cpp
	${SRC_DIR}/sfz/memory/DebugAllocator.cpp
	${SRC_DIR}/sfz/memory/StandardAllocator.cpp

	${SRC_DIR}/sfz/strings/DynString.cpp
	${SRC_DIR}/sfz/strings/StringID.cpp

	${SRC_DIR}/sfz/util/FrametimeStats.cpp
	${SRC_DIR}/sfz/util/IniParser.cpp
	${SRC_DIR}/sfz/util/IO.cpp
	${SRC_DIR}/sfz/util/StandardLogger.cpp
)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_library(LibCore ${INCLUDE_FILES} ${SRC_FILES})
target_include_directories(LibCore PRIVATE ${INCLUDE_DIR} ${SFZ_CORE_HEADERS_INCLUDE_DIRS})

# sfzCore lib-core tests
# ------------------------------------------------------------------------------------------------

if(NOT hasParent)

	set(TEST_FILES
		${TESTS_DIR}/sfz/Main_Tests.cpp

		${TESTS_DIR}/sfz/containers/RingBuffer_Tests.cpp

		${TESTS_DIR}/sfz/geometry/Intersection_Tests.cpp
		${TESTS_DIR}/sfz/geometry/OBB_Tests.cpp

		${TESTS_DIR}/sfz/math/MathSupport_Tests.cpp
		${TESTS_DIR}/sfz/math/Matrix_Tests.cpp
		${TESTS_DIR}/sfz/math/Quaternion_Tests.cpp

		${TESTS_DIR}/sfz/memory/Allocators_Tests.cpp
		${TESTS_DIR}/sfz/memory/ArenaAllocator_Tests.cpp
		${TESTS_DIR}/sfz/memory/SmartPointers_Tests.cpp

		${TESTS_DIR}/sfz/strings/DynString_Tests.cpp
		${TESTS_DIR}/sfz/strings/StringID_Tests.cpp

		${TESTS_DIR}/sfz/util/Enumerate_Tests.cpp
		${TESTS_DIR}/sfz/util/IniParser_Tests.cpp
		${TESTS_DIR}/sfz/util/IO_Tests.cpp
	)
	source_group(TREE ${TESTS_DIR} FILES ${TEST_FILES})

	add_executable(LibCoreTests ${TEST_FILES})
	target_include_directories(LibCoreTests PRIVATE
		${INCLUDE_DIR}
		${SFZ_CORE_HEADERS_INCLUDE_DIRS}
		${CATCH_INCLUDE_DIRS}
	)
	target_link_libraries(LibCoreTests LibCore)

endif()

# Output variables
# ------------------------------------------------------------------------------------------------

if(hasParent)
	set(SFZ_CORE_FOUND true PARENT_SCOPE)
	set(SFZ_CORE_INCLUDE_DIRS ${INCLUDE_DIR} ${SFZ_CORE_HEADERS_INCLUDE_DIRS} PARENT_SCOPE)
	set(SFZ_CORE_LIBRARIES LibCore PARENT_SCOPE)
endif()
