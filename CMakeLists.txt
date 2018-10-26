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

	elseif(IOS)
		# iOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -DSFZ_IOS = Defines the "SFZ_IOS" macro
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-strict-aliasing -DSFZ_IOS")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

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
set(CORE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib-core/src)
set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib-core/include)
set(CORE_INCLUDE_CUDA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib-core/include_cuda)
set(CORE_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib-core/tests)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# sfzCore library
# ------------------------------------------------------------------------------------------------

set(SFZ_CORE_INCLUDE_FILES
	${CORE_INCLUDE_DIR}/sfz/Assert.hpp
	${CORE_INCLUDE_DIR}/sfz/Assert.inl
	${CORE_INCLUDE_DIR}/sfz/Context.hpp
	${CORE_INCLUDE_DIR}/sfz/CudaCompatibility.hpp
	${CORE_INCLUDE_DIR}/sfz/Logging.hpp
	${CORE_INCLUDE_DIR}/sfz/PopWarnings.hpp
	${CORE_INCLUDE_DIR}/sfz/PushWarnings.hpp
	${CORE_INCLUDE_DIR}/sfz/SimdIntrinsics.hpp

	${CORE_INCLUDE_DIR}/sfz/containers/DynArray.hpp
	${CORE_INCLUDE_DIR}/sfz/containers/DynArray.inl
	${CORE_INCLUDE_DIR}/sfz/containers/HashMap.hpp
	${CORE_INCLUDE_DIR}/sfz/containers/HashMap.inl
	${CORE_INCLUDE_DIR}/sfz/containers/HashTableKeyDescriptor.hpp
	${CORE_INCLUDE_DIR}/sfz/containers/RingBuffer.hpp
	${CORE_INCLUDE_DIR}/sfz/containers/RingBuffer.inl

	${CORE_INCLUDE_DIR}/sfz/geometry/AABB.hpp
	${CORE_INCLUDE_DIR}/sfz/geometry/AABB.inl
	${CORE_INCLUDE_DIR}/sfz/geometry/AABB2D.hpp
	${CORE_INCLUDE_DIR}/sfz/geometry/Circle.hpp
	${CORE_INCLUDE_DIR}/sfz/geometry/Intersection.hpp
	${CORE_INCLUDE_DIR}/sfz/geometry/Intersection.inl
	${CORE_INCLUDE_DIR}/sfz/geometry/OBB.hpp
	${CORE_INCLUDE_DIR}/sfz/geometry/OBB.inl
	${CORE_INCLUDE_DIR}/sfz/geometry/Plane.hpp
	${CORE_INCLUDE_DIR}/sfz/geometry/Plane.inl
	${CORE_INCLUDE_DIR}/sfz/geometry/Sphere.hpp

	${CORE_INCLUDE_DIR}/sfz/math/MathPrimitiveHashers.hpp
	${CORE_INCLUDE_DIR}/sfz/math/MathPrimitiveHashers.inl
	${CORE_INCLUDE_DIR}/sfz/math/MathPrimitiveToStrings.hpp
	${CORE_INCLUDE_DIR}/sfz/math/MathSupport.hpp
	${CORE_INCLUDE_DIR}/sfz/math/MathSupport.inl
	${CORE_INCLUDE_DIR}/sfz/math/Matrix.hpp
	${CORE_INCLUDE_DIR}/sfz/math/Matrix.inl
	${CORE_INCLUDE_DIR}/sfz/math/ProjectionMatrices.hpp
	${CORE_INCLUDE_DIR}/sfz/math/Quaternion.hpp
	${CORE_INCLUDE_DIR}/sfz/math/Quaternion.inl
	${CORE_INCLUDE_DIR}/sfz/math/Vector.hpp
	${CORE_INCLUDE_DIR}/sfz/math/Vector.inl

	${CORE_INCLUDE_DIR}/sfz/memory/Allocator.hpp
	${CORE_INCLUDE_DIR}/sfz/memory/DebugAllocator.hpp
	${CORE_INCLUDE_DIR}/sfz/memory/MemoryUtils.hpp
	${CORE_INCLUDE_DIR}/sfz/memory/New.hpp
	${CORE_INCLUDE_DIR}/sfz/memory/SmartPointers.hpp
	${CORE_INCLUDE_DIR}/sfz/memory/SmartPointers.inl
	${CORE_INCLUDE_DIR}/sfz/memory/StandardAllocator.hpp

	${CORE_INCLUDE_DIR}/sfz/strings/DynString.hpp
	${CORE_INCLUDE_DIR}/sfz/strings/StackString.hpp
	${CORE_INCLUDE_DIR}/sfz/strings/StringHashers.hpp
	${CORE_INCLUDE_DIR}/sfz/strings/StringID.hpp

	${CORE_INCLUDE_DIR}/sfz/util/FrametimeStats.hpp
	${CORE_INCLUDE_DIR}/sfz/util/IniParser.hpp
	${CORE_INCLUDE_DIR}/sfz/util/IO.hpp
	${CORE_INCLUDE_DIR}/sfz/util/LoggingInterface.hpp
	${CORE_INCLUDE_DIR}/sfz/util/StandardLogger.hpp
)
source_group(TREE ${CORE_INCLUDE_DIR} FILES ${SFZ_CORE_INCLUDE_FILES})

set(SFZ_CORE_SOURCE_FILES

	${CORE_SOURCE_DIR}/sfz/Assert.cpp
	${CORE_SOURCE_DIR}/sfz/Context.cpp

	${CORE_SOURCE_DIR}/sfz/math/MathPrimitiveToStrings.cpp
	${CORE_SOURCE_DIR}/sfz/math/ProjectionMatrices.cpp

	${CORE_SOURCE_DIR}/sfz/memory/DebugAllocator.cpp
	${CORE_SOURCE_DIR}/sfz/memory/StandardAllocator.cpp

	${CORE_SOURCE_DIR}/sfz/strings/DynString.cpp
	${CORE_SOURCE_DIR}/sfz/strings/StackString.cpp
	${CORE_SOURCE_DIR}/sfz/strings/StringHashers.cpp
	${CORE_SOURCE_DIR}/sfz/strings/StringID.cpp

	${CORE_SOURCE_DIR}/sfz/util/FrametimeStats.cpp
	${CORE_SOURCE_DIR}/sfz/util/IniParser.cpp
	${CORE_SOURCE_DIR}/sfz/util/IO.cpp
	${CORE_SOURCE_DIR}/sfz/util/StandardLogger.cpp
)
source_group(TREE ${CORE_SOURCE_DIR} FILES ${SFZ_CORE_SOURCE_FILES})

set(SFZ_CORE_INCLUDE_CUDA_FILES
	${CORE_INCLUDE_CUDA_DIR}/sfz/cuda/Buffer.hpp
	${CORE_INCLUDE_CUDA_DIR}/sfz/cuda/CudaUtils.hpp
)
source_group(TREE ${CORE_INCLUDE_CUDA_DIR} FILES ${SFZ_CORE_INCLUDE_CUDA_FILES})

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
	${CORE_INCLUDE_DIR}
	${CORE_INCLUDE_CUDA_DIR}
)

# Output variables
# ------------------------------------------------------------------------------------------------

if(hasParent)
	set(SFZ_CORE_FOUND true PARENT_SCOPE)
	set(SFZ_CORE_INCLUDE_DIRS ${CORE_INCLUDE_DIR} ${CORE_INCLUDE_CUDA_DIR} PARENT_SCOPE)
	set(SFZ_CORE_LIBRARIES sfzCore PARENT_SCOPE)
	set(SFZ_CORE_RUNTIME_FILES "" PARENT_SCOPE)
endif()

# Tests
# ------------------------------------------------------------------------------------------------

if(NOT hasParent)

	# Catch
	add_subdirectory(${EXTERNALS_DIR}/catch)

	set(SFZ_CORE_TEST_FILES
		${CORE_TESTS_DIR}/sfz/Main_Tests.cpp

		${CORE_TESTS_DIR}/sfz/containers/DynArray_Tests.cpp
		${CORE_TESTS_DIR}/sfz/containers/HashMap_Tests.cpp
		${CORE_TESTS_DIR}/sfz/containers/RingBuffer_Tests.cpp

		${CORE_TESTS_DIR}/sfz/geometry/Intersection_Tests.cpp

		${CORE_TESTS_DIR}/sfz/math/MathPrimitiveHashers_Tests.cpp
		${CORE_TESTS_DIR}/sfz/math/MathSupport_Tests.cpp
		${CORE_TESTS_DIR}/sfz/math/Matrix_Tests.cpp
		${CORE_TESTS_DIR}/sfz/math/Vector_Tests.cpp
		${CORE_TESTS_DIR}/sfz/math/Quaternion_Tests.cpp

		${CORE_TESTS_DIR}/sfz/memory/Allocators_Tests.cpp
		${CORE_TESTS_DIR}/sfz/memory/New_Tests.cpp
		${CORE_TESTS_DIR}/sfz/memory/SmartPointers_Tests.cpp

		${CORE_TESTS_DIR}/sfz/strings/DynString_Tests.cpp
		${CORE_TESTS_DIR}/sfz/strings/StackString_Tests.cpp
		${CORE_TESTS_DIR}/sfz/strings/StringHashers_Tests.cpp
		${CORE_TESTS_DIR}/sfz/strings/StringID_Tests.cpp

		${CORE_TESTS_DIR}/sfz/util/IniParser_Tests.cpp
		${CORE_TESTS_DIR}/sfz/util/IO_Tests.cpp
	)
	source_group(TREE ${CORE_TESTS_DIR} FILES ${SFZ_CORE_TEST_FILES})

	enable_testing(true)
	add_executable(sfzCoreTests ${SFZ_CORE_TEST_FILES})
	target_include_directories(
		sfzCoreTests
		PRIVATE
		${CORE_INCLUDE_DIR}
		${CORE_INCLUDE_CUDA_DIR}
		${CATCH_INCLUDE_DIRS}
	)
	target_link_libraries(
		sfzCoreTests

		sfzCore
	)
	add_test(sfzCoreTestsName sfzCoreTests)

endif()
