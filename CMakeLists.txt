# Compiler flags and directories
# ------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project("sfzCore" CXX)

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Check if sfzCore is built individually or part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

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
		set(CMAKE_CXX_FLAGS "/W4 /wd4201 /Zi /EHsc /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
		# /O2 = Optimize code for fastest speed
		# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
		# /DSFZ_NO_DEBUG = defines the "SFZ_NO_DEBUG" macro, which disables sfz_assert_debug()
		set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")
		# /DEBUG = "creates debugging information for the .exe file or DLL"
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
		# /Od = "disables optimization, speeding compilation and simplifying debugging"
		set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")

	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
		# macOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -fno-strict-aliasing")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	elseif(EMSCRIPTEN)
		# Emscripten flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -s TOTAL_MEMORY=134217728 = 128MiB heap, TODO: should probably not hard-code this
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-strict-aliasing -s TOTAL_MEMORY=134217728")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	else()
		# GCC / Clang flags
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -pthread -fno-strict-aliasing")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	endif()
endif()

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(INCLUDE_CUDA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include_cuda)
set(INCLUDE_GL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include_gl)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# sfzCore library
# ------------------------------------------------------------------------------------------------

set(SFZ_CORE_INCLUDE_FILES
	${INCLUDE_DIR}/sfz/Assert.hpp
	${INCLUDE_DIR}/sfz/Assert.inl
	${INCLUDE_DIR}/sfz/Context.hpp
	${INCLUDE_DIR}/sfz/CudaCompatibility.hpp
	${INCLUDE_DIR}/sfz/Logging.hpp
	${INCLUDE_DIR}/sfz/PopWarnings.hpp
	${INCLUDE_DIR}/sfz/PushWarnings.hpp
	${INCLUDE_DIR}/sfz/SimdIntrinsics.hpp

	${INCLUDE_DIR}/sfz/containers/DynArray.hpp
	${INCLUDE_DIR}/sfz/containers/DynArray.inl
	${INCLUDE_DIR}/sfz/containers/HashMap.hpp
	${INCLUDE_DIR}/sfz/containers/HashMap.inl
	${INCLUDE_DIR}/sfz/containers/HashTableKeyDescriptor.hpp
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

	${INCLUDE_DIR}/sfz/math/MathPrimitiveHashers.hpp
	${INCLUDE_DIR}/sfz/math/MathPrimitiveHashers.inl
	${INCLUDE_DIR}/sfz/math/MathPrimitiveToStrings.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.inl
	${INCLUDE_DIR}/sfz/math/Matrix.hpp
	${INCLUDE_DIR}/sfz/math/Matrix.inl
	${INCLUDE_DIR}/sfz/math/ProjectionMatrices.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.inl
	${INCLUDE_DIR}/sfz/math/Vector.hpp
	${INCLUDE_DIR}/sfz/math/Vector.inl

	${INCLUDE_DIR}/sfz/memory/Allocator.hpp
	${INCLUDE_DIR}/sfz/memory/DebugAllocator.hpp
	${INCLUDE_DIR}/sfz/memory/MemoryUtils.hpp
	${INCLUDE_DIR}/sfz/memory/New.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.inl
	${INCLUDE_DIR}/sfz/memory/StandardAllocator.hpp

	${INCLUDE_DIR}/sfz/strings/DynString.hpp
	${INCLUDE_DIR}/sfz/strings/StackString.hpp
	${INCLUDE_DIR}/sfz/strings/StringHashers.hpp
	${INCLUDE_DIR}/sfz/strings/StringID.hpp

	${INCLUDE_DIR}/sfz/util/FrametimeStats.hpp
	${INCLUDE_DIR}/sfz/util/IniParser.hpp
	${INCLUDE_DIR}/sfz/util/IO.hpp
	${INCLUDE_DIR}/sfz/util/LoggingInterface.hpp
	${INCLUDE_DIR}/sfz/util/StandardLogger.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${SFZ_CORE_INCLUDE_FILES})

set(SFZ_CORE_SOURCE_FILES

	${SOURCE_DIR}/sfz/Assert.cpp
	${SOURCE_DIR}/sfz/Context.cpp

	${SOURCE_DIR}/sfz/math/MathPrimitiveToStrings.cpp
	${SOURCE_DIR}/sfz/math/ProjectionMatrices.cpp

	${SOURCE_DIR}/sfz/memory/DebugAllocator.cpp
	${SOURCE_DIR}/sfz/memory/StandardAllocator.cpp

	${SOURCE_DIR}/sfz/strings/DynString.cpp
	${SOURCE_DIR}/sfz/strings/StackString.cpp
	${SOURCE_DIR}/sfz/strings/StringHashers.cpp
	${SOURCE_DIR}/sfz/strings/StringID.cpp

	${SOURCE_DIR}/sfz/util/FrametimeStats.cpp
	${SOURCE_DIR}/sfz/util/IniParser.cpp
	${SOURCE_DIR}/sfz/util/IO.cpp
	${SOURCE_DIR}/sfz/util/StandardLogger.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${SFZ_CORE_SOURCE_FILES})

set(SFZ_CORE_INCLUDE_CUDA_FILES
	${INCLUDE_CUDA_DIR}/sfz/cuda/Buffer.hpp
	${INCLUDE_CUDA_DIR}/sfz/cuda/CudaUtils.hpp
)
source_group(TREE ${INCLUDE_CUDA_DIR} FILES ${SFZ_CORE_INCLUDE_CUDA_FILES})

set(SFZ_CORE_FILES
	${SFZ_CORE_INCLUDE_FILES}
	${SFZ_CORE_SOURCE_FILES}
	${SFZ_CORE_INCLUDE_CUDA_FILES}
)

# Library compile target
add_library(
	sfzCore

	${SFZ_CORE_INCLUDE_FILES}
	${SFZ_CORE_SOURCE_FILES}
	${SFZ_CORE_INCLUDE_CUDA_FILES}
)

# Include directories
target_include_directories(
	sfzCore
	PRIVATE
	${INCLUDE_DIR}
	${INCLUDE_CUDA_DIR}
)

# Output variables
# ------------------------------------------------------------------------------------------------

if(hasParent)
	set(SFZ_CORE_FOUND true PARENT_SCOPE)
	set(SFZ_CORE_INCLUDE_DIRS ${INCLUDE_DIR} ${INCLUDE_CUDA_DIR} PARENT_SCOPE)
	set(SFZ_CORE_LIBRARIES sfzCore PARENT_SCOPE)
	set(SFZ_CORE_RUNTIME_FILES "" PARENT_SCOPE)
endif()

# Tests
# ------------------------------------------------------------------------------------------------

if(NOT hasParent)

	# Catch
	add_subdirectory(${EXTERNALS_DIR}/catch)

	set(SFZ_CORE_TEST_FILES
		${TESTS_DIR}/sfz/Main_Tests.cpp

		${TESTS_DIR}/sfz/containers/DynArray_Tests.cpp
		${TESTS_DIR}/sfz/containers/HashMap_Tests.cpp
		${TESTS_DIR}/sfz/containers/RingBuffer_Tests.cpp

		${TESTS_DIR}/sfz/geometry/Intersection_Tests.cpp

		${TESTS_DIR}/sfz/math/MathPrimitiveHashers_Tests.cpp
		${TESTS_DIR}/sfz/math/MathSupport_Tests.cpp
		${TESTS_DIR}/sfz/math/Matrix_Tests.cpp
		${TESTS_DIR}/sfz/math/Vector_Tests.cpp
		${TESTS_DIR}/sfz/math/Quaternion_Tests.cpp

		${TESTS_DIR}/sfz/memory/Allocators_Tests.cpp
		${TESTS_DIR}/sfz/memory/New_Tests.cpp
		${TESTS_DIR}/sfz/memory/SmartPointers_Tests.cpp

		${TESTS_DIR}/sfz/strings/DynString_Tests.cpp
		${TESTS_DIR}/sfz/strings/StackString_Tests.cpp
		${TESTS_DIR}/sfz/strings/StringHashers_Tests.cpp
		${TESTS_DIR}/sfz/strings/StringID_Tests.cpp

		${TESTS_DIR}/sfz/util/IniParser_Tests.cpp
		${TESTS_DIR}/sfz/util/IO_Tests.cpp
	)
	source_group(TREE ${TESTS_DIR} FILES ${SFZ_CORE_TEST_FILES})

	enable_testing(true)
	add_executable(sfzCoreTests ${SFZ_CORE_TEST_FILES})
	target_include_directories(
		sfzCoreTests
		PRIVATE
		${INCLUDE_DIR}
		${INCLUDE_CUDA_DIR}
		${SDL2_INCLUDE_DIRS}
		${CATCH_INCLUDE_DIRS}
	)
	target_link_libraries(
		sfzCoreTests

		sfzCore
	)
	add_test(sfzCoreTestsName sfzCoreTests)

endif()
