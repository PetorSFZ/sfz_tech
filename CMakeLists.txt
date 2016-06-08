cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project("sfzCore")

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
	# MSVC flags
	set(CMAKE_CXX_FLAGS "/Wall /Zi /EHsc /wd4201 /wd4350 /wd4514 /wd4571 /wd4647 /wd4668 /wd4710 /wd4711 /wd4820 /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")
else()
	# GCC / Clang flags
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-c++11-extensions -std=c++11 -pthread")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
endif()

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)
set(CMAKE_MODULES ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Adds our cmake modules to module path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULES})

# SDL2 & SDL2_mixer (uses 'SDL2' system environment variable)
find_package(SDL2 REQUIRED) # ${SDL2_INCLUDE_DIR}, ${SDL2_LIBRARY}
find_package(SDL2_mixer REQUIRED) # ${SDL2_MIXER_INCLUDE_DIRS}, ${SDL2_MIXER_LIBRARIES}

# Catch
add_subdirectory(${EXTERNALS_DIR}/catch)

# Include directories
include_directories(
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${SDL2_MIXER_INCLUDE_DIRS}
	${CATCH_INCLUDE_DIRS}
)

set(SOURCE_ROOT_FILES
	${INCLUDE_DIR}/sfz/Assert.hpp
	${INCLUDE_DIR}/sfz/Assert.inl
	 ${SOURCE_DIR}/sfz/Assert.cpp
	${INCLUDE_DIR}/sfz/PopWarnings.hpp
	${INCLUDE_DIR}/sfz/PushWarnings.hpp)
source_group(sfz_root FILES ${SOURCE_ROOT_FILES})

set(SOURCE_CONTAINERS_FILES
	${INCLUDE_DIR}/sfz/Containers.hpp
	${INCLUDE_DIR}/sfz/containers/DynArray.hpp
	${INCLUDE_DIR}/sfz/containers/DynArray.inl
	${INCLUDE_DIR}/sfz/containers/DynString.hpp
	${INCLUDE_DIR}/sfz/containers/DynString.inl
	${INCLUDE_DIR}/sfz/containers/HashMap.hpp
	${INCLUDE_DIR}/sfz/containers/HashMap.inl
	${INCLUDE_DIR}/sfz/containers/StackString.hpp
	 ${SOURCE_DIR}/sfz/containers/StackString.cpp)
source_group(sfz_containers FILES ${SOURCE_CONTAINERS_FILES})

set(SOURCE_GEOMETRY_FILES
	${INCLUDE_DIR}/sfz/Geometry.hpp
	${INCLUDE_DIR}/sfz/geometry/AABB.hpp
	${INCLUDE_DIR}/sfz/geometry/AABB.inl
	${INCLUDE_DIR}/sfz/geometry/AABB2D.hpp
	${INCLUDE_DIR}/sfz/geometry/AABB2D.inl
	${INCLUDE_DIR}/sfz/geometry/Circle.hpp
	${INCLUDE_DIR}/sfz/geometry/Circle.inl
	${INCLUDE_DIR}/sfz/geometry/Intersection.hpp
	 ${SOURCE_DIR}/sfz/geometry/Intersection.cpp
	${INCLUDE_DIR}/sfz/geometry/OBB.hpp
	${INCLUDE_DIR}/sfz/geometry/OBB.inl
	${INCLUDE_DIR}/sfz/geometry/Plane.hpp
	${INCLUDE_DIR}/sfz/geometry/Plane.inl
	${INCLUDE_DIR}/sfz/geometry/Sphere.hpp
	${INCLUDE_DIR}/sfz/geometry/Sphere.inl
	${INCLUDE_DIR}/sfz/geometry/ViewFrustum.hpp
	 ${SOURCE_DIR}/sfz/geometry/ViewFrustum.cpp)
source_group(sfz_geometry FILES ${SOURCE_GEOMETRY_FILES})

set(SOURCE_MATH_FILES
	${INCLUDE_DIR}/sfz/Math.hpp
	${INCLUDE_DIR}/sfz/math/MathConstants.hpp
	${INCLUDE_DIR}/sfz/math/MathHelpers.hpp
	${INCLUDE_DIR}/sfz/math/MathHelpers.inl
	${INCLUDE_DIR}/sfz/math/Matrix.hpp
	${INCLUDE_DIR}/sfz/math/Matrix.inl
	${INCLUDE_DIR}/sfz/math/MatrixSupport.hpp
	${INCLUDE_DIR}/sfz/math/MatrixSupport.inl
	${INCLUDE_DIR}/sfz/math/Vector.hpp
	${INCLUDE_DIR}/sfz/math/Vector.inl)
source_group(sfz_math FILES ${SOURCE_MATH_FILES})

set(SOURCE_MEMORY_FILES
	${INCLUDE_DIR}/sfz/Memory.hpp
	${INCLUDE_DIR}/sfz/memory/Allocators.hpp
	 ${SOURCE_DIR}/sfz/memory/Allocators.cpp
	${INCLUDE_DIR}/sfz/memory/MemoryUtils.hpp
	${INCLUDE_DIR}/sfz/memory/New.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.inl)
source_group(sfz_memory FILES ${SOURCE_MEMORY_FILES})

set(SOURCE_SDL_FILES
	${INCLUDE_DIR}/sfz/SDL.hpp
	${INCLUDE_DIR}/sfz/sdl/Music.hpp
	 ${SOURCE_DIR}/sfz/sdl/Music.cpp
	${INCLUDE_DIR}/sfz/sdl/Session.hpp
	 ${SOURCE_DIR}/sfz/sdl/Session.cpp
	${INCLUDE_DIR}/sfz/sdl/SoundEffect.hpp
	 ${SOURCE_DIR}/sfz/sdl/SoundEffect.cpp
	${INCLUDE_DIR}/sfz/sdl/Window.hpp
	 ${SOURCE_DIR}/sfz/sdl/Window.cpp)
source_group(sfz_sdl FILES ${SOURCE_SDL_FILES})

set(SOURCE_UTIL_FILES
	${INCLUDE_DIR}/sfz/Util.hpp
	${INCLUDE_DIR}/sfz/util/FrametimeStats.hpp
	 ${SOURCE_DIR}/sfz/util/FrametimeStats.cpp
	${INCLUDE_DIR}/sfz/util/IniParser.hpp
	 ${SOURCE_DIR}/sfz/util/IniParser.cpp
	${INCLUDE_DIR}/sfz/util/IO.hpp
	 ${SOURCE_DIR}/sfz/util/IO.cpp)
source_group(sfz_util FILES ${SOURCE_UTIL_FILES})

set(SOURCE_ALL_FILES
	${SOURCE_ROOT_FILES}
	${SOURCE_CONTAINERS_FILES}
	${SOURCE_GEOMETRY_FILES}
	${SOURCE_MATH_FILES}
	${SOURCE_MEMORY_FILES}
	${SOURCE_SDL_FILES}
	${SOURCE_UTIL_FILES})

# Library compile target
add_library(sfzCoreLib ${SOURCE_ALL_FILES})

# Linking libraries
target_link_libraries(
	sfzCoreLib

	${SDL2_LIBRARY}
	${SDL2_MIXER_LIBRARIES}
)

# Variables for linking library
set(SFZ_CORE_INCLUDE_DIRS
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${SDL2_MIXER_INCLUDE_DIRS}
#	${TINYOBJLOADER_INCLUDE_DIRS}
	PARENT_SCOPE)
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	${SDL2_LIBRARY}
	${SDL2_MIXER_LIBRARIES}
#	${TINYOBJLOADER_LIBRARIES}
	PARENT_SCOPE)

# Tests
if(SFZ_CORE_BUILD_TESTS)

	set(ROOT_TEST_FILES
		${TESTS_DIR}/sfz/Main_Tests.cpp)
	source_group(sfz_root FILES ${ROOT_TEST_FILES})

	set(CONTAINERS_TEST_FILES
		${TESTS_DIR}/sfz/containers/DynArray_Tests.cpp
		${TESTS_DIR}/sfz/containers/DynString_Tests.cpp
		${TESTS_DIR}/sfz/containers/HashMap_Tests.cpp
		${TESTS_DIR}/sfz/containers/StackString_Tests.cpp)
	source_group(sfz_containers FILES ${CONTAINERS_TEST_FILES})

	set(GEOMETRY_TEST_FILES
		${TESTS_DIR}/sfz/geometry/Intersection_Tests.cpp)
	source_group(sfz_geometry FILES ${GEOMETRY_TEST_FILES})

	set(MATH_TEST_FILES
		${TESTS_DIR}/sfz/math/MathConstants_Tests.cpp
		${TESTS_DIR}/sfz/math/Matrix_Tests.cpp
		${TESTS_DIR}/sfz/math/Vector_Tests.cpp)
	source_group(sfz_math FILES ${MATH_TEST_FILES})
	
	set(MEMORY_TEST_FILES
		${TESTS_DIR}/sfz/memory/Allocators_Tests.cpp
		${TESTS_DIR}/sfz/memory/New_Tests.cpp
		${TESTS_DIR}/sfz/memory/SmartPointers_Tests.cpp)
	source_group(sfz_memory FILES ${MEMORY_TEST_FILES})

	set(UTIL_TEST_FILES
		${TESTS_DIR}/sfz/util/IO_Tests.cpp)
	source_group(sfz_util FILES ${UTIL_TEST_FILES})

	set(ALL_TEST_FILES
		${ROOT_TEST_FILES}
		${CONTAINERS_TEST_FILES}
		${GEOMETRY_TEST_FILES}
		${MATH_TEST_FILES}
		${MEMORY_TEST_FILES}
		${UTIL_TEST_FILES})

	enable_testing(true)
	add_executable(sfzCoreTests ${ALL_TEST_FILES})
	target_link_libraries(
		sfzCoreTests

		sfzCoreLib
	)
	add_test(sfzCoreTestsName sfzCoreTests)
	
endif()