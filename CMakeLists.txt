cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project("sfzCore")

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Require 64bit compiler
if(NOT (CMAKE_SIZEOF_VOID_P EQUAL 8))
	message(FATAL_ERROR "Can only compile in 64bit mode")
endif()

# Compiler flags
if(MSVC)
	# MSVC flags
	set(CMAKE_CXX_FLAGS "/Wall /Zi /EHsc /arch:AVX /wd4201 /wd4350 /wd4514 /wd4571 /wd4647 /wd4668 /wd4710 /wd4711 /wd4820 /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")
else()
	# GCC / Clang flags
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -pthread -fno-strict-aliasing")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
endif()

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(INCLUDE_GL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include_gl)
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

# OpenGL only libraries
if(SFZ_CORE_OPENGL)

# GLEW (GLEW_INCLUDE_DIRS and GLEW_LIBRARIES)
add_definitions(-DGLEW_STATIC)
add_subdirectory(${EXTERNALS_DIR}/glew)

# Links OpenGL
find_package(OpenGL REQUIRED) # ${OPENGL_INCLUDE_DIR}, ${OPENGL_LIBRARIES}

endif()

# Accumulated variable with all includes to forward to caller
set(FORWARD_INCLUDE_DIRS
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${SDL2_MIXER_INCLUDE_DIRS})
if(SFZ_CORE_OPENGL)
set(FORWARD_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	${INCLUDE_GL_DIR}
	${GLEW_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR})
endif()

# Include variable returned to parent

# Include directories
include_directories(
	${FORWARD_INCLUDE_DIRS}
	${CATCH_INCLUDE_DIRS}
)

set(SOURCE_ROOT_FILES
	${INCLUDE_DIR}/sfz/Assert.hpp
	${INCLUDE_DIR}/sfz/Assert.inl
	 ${SOURCE_DIR}/sfz/Assert.cpp
	${INCLUDE_DIR}/sfz/CudaCompatibility.hpp
	${INCLUDE_DIR}/sfz/PopWarnings.hpp
	${INCLUDE_DIR}/sfz/PushWarnings.hpp
	${INCLUDE_DIR}/sfz/SimdIntrinsics.hpp)
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
	${INCLUDE_DIR}/sfz/geometry/Circle.hpp
	${INCLUDE_DIR}/sfz/geometry/Intersection.hpp
	${INCLUDE_DIR}/sfz/geometry/Intersection.inl
	${INCLUDE_DIR}/sfz/geometry/OBB.hpp
	${INCLUDE_DIR}/sfz/geometry/OBB.inl
	${INCLUDE_DIR}/sfz/geometry/Plane.hpp
	${INCLUDE_DIR}/sfz/geometry/Plane.inl
	${INCLUDE_DIR}/sfz/geometry/Sphere.hpp)
source_group(sfz_geometry FILES ${SOURCE_GEOMETRY_FILES})

if(SFZ_CORE_OPENGL)
set(SOURCE_GL_FILES
	${INCLUDE_GL_DIR}/sfz/GL.hpp
	${INCLUDE_GL_DIR}/sfz/gl/Context.hpp
	    ${SOURCE_DIR}/sfz/gl/Context.cpp
	${INCLUDE_GL_DIR}/sfz/gl/Framebuffer.hpp
	    ${SOURCE_DIR}/sfz/gl/Framebuffer.cpp
	${INCLUDE_GL_DIR}/sfz/gl/FullscreenQuad.hpp
	    ${SOURCE_DIR}/sfz/gl/FullscreenQuad.cpp
	${INCLUDE_GL_DIR}/sfz/gl/GLUtils.hpp
	    ${SOURCE_DIR}/sfz/gl/GLUtils.cpp
	${INCLUDE_GL_DIR}/sfz/gl/IncludeOpenGL.hpp
	${INCLUDE_GL_DIR}/sfz/gl/Program.hpp
	    ${SOURCE_DIR}/sfz/gl/Program.cpp)
source_group(sfz_gl FILES ${SOURCE_GL_FILES})
endif()

set(SOURCE_MATH_FILES
	${INCLUDE_DIR}/sfz/Math.hpp
	${INCLUDE_DIR}/sfz/math/MathPrimitiveHashers.hpp
	${INCLUDE_DIR}/sfz/math/MathPrimitiveHashers.inl
	${INCLUDE_DIR}/sfz/math/MathPrimitiveToStrings.hpp
	 ${SOURCE_DIR}/sfz/math/MathPrimitiveToStrings.cpp
	${INCLUDE_DIR}/sfz/math/MathSupport.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.inl
	${INCLUDE_DIR}/sfz/math/Matrix.hpp
	${INCLUDE_DIR}/sfz/math/Matrix.inl
	${INCLUDE_DIR}/sfz/math/ProjectionMatrices.hpp
	 ${SOURCE_DIR}/sfz/math/ProjectionMatrices.cpp
	${INCLUDE_DIR}/sfz/math/Quaternion.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.inl
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

set(SOURCE_SCREENS_FILES
	${INCLUDE_DIR}/sfz/Screens.hpp
	${INCLUDE_DIR}/sfz/screens/BaseScreen.hpp
	${INCLUDE_DIR}/sfz/screens/GameLoop.hpp
	 ${SOURCE_DIR}/sfz/screens/GameLoop.cpp)
source_group(sfz_screens FILES ${SOURCE_SCREENS_FILES})

set(SOURCE_SDL_FILES
	${INCLUDE_DIR}/sfz/SDL.hpp
	${INCLUDE_DIR}/sfz/sdl/ButtonState.hpp
	${INCLUDE_DIR}/sfz/sdl/GameController.hpp
	 ${SOURCE_DIR}/sfz/sdl/GameController.cpp
	${INCLUDE_DIR}/sfz/sdl/Mouse.hpp
	 ${SOURCE_DIR}/sfz/sdl/Mouse.cpp
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
	${SOURCE_GL_FILES}
	${SOURCE_MATH_FILES}
	${SOURCE_MEMORY_FILES}
	${SOURCE_SCREENS_FILES}
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

# Linking OpenGL only libraries
if(SFZ_CORE_OPENGL)
target_link_libraries(
	sfzCoreLib

	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
)
endif()

# Variables returned to parent
set(SFZ_CORE_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	PARENT_SCOPE)
if(SFZ_CORE_OPENGL)
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	${SDL2_LIBRARY}
	${SDL2_MIXER_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
	PARENT_SCOPE)
else()
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	${SDL2_LIBRARY}
	${SDL2_MIXER_LIBRARIES}
	PARENT_SCOPE)
endif()

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
		${TESTS_DIR}/sfz/math/MathPrimitiveHashers_Tests.cpp
		${TESTS_DIR}/sfz/math/MathSupport_Tests.cpp
		${TESTS_DIR}/sfz/math/Matrix_Tests.cpp
		${TESTS_DIR}/sfz/math/Vector_Tests.cpp
		${TESTS_DIR}/sfz/math/Quaternion_Tests.cpp)
	source_group(sfz_math FILES ${MATH_TEST_FILES})
	
	set(MEMORY_TEST_FILES
		${TESTS_DIR}/sfz/memory/Allocators_Tests.cpp
		${TESTS_DIR}/sfz/memory/New_Tests.cpp
		${TESTS_DIR}/sfz/memory/SmartPointers_Tests.cpp)
	source_group(sfz_memory FILES ${MEMORY_TEST_FILES})

	set(UTIL_TEST_FILES
		${TESTS_DIR}/sfz/util/IniParser_Tests.cpp
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
