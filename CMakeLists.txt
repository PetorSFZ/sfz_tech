# Functions
# ------------------------------------------------------------------------------------------------

# Automatically set source group to correspond to directory structure.
# From Stack Overflow post by Florian:
# http://stackoverflow.com/a/31423421
function(set_directory_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if (IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(source_rel "${_source}")
		endif()
		get_filename_component(_source_path "${_source_rel}" PATH)
		string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
		source_group("${_source_path_msvc}" FILES "${_source}")
	endforeach()
endfunction(set_directory_source_group)

# Compiler flags and directories
# ------------------------------------------------------------------------------------------------

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
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /arch:AVX = Enable (require) Intel AVX instruction for code generation
	# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
	set(CMAKE_CXX_FLAGS "/W4 /wd4201 /Zi /EHsc /arch:AVX /D_CRT_SECURE_NO_WARNINGS")
	# /O2 = Optimize code for fastest speed
	# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
	# /DSFZ_NO_DEBUG = defines the "SFZ_NO_DEBUG" macro, which disables sfz_assert_debug()
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")
	# /DEBUG = "creates debugging information for the .exe file or DLL"
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
	# /Od = "disables optimization, speeding compilation and simplifying debugging"
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

# Dependencies
# ------------------------------------------------------------------------------------------------

# SDL2
add_subdirectory(${EXTERNALS_DIR}/sdl2) # ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES}, ${SDL2_DLLS}

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
	${SDL2_INCLUDE_DIRS})
if(SFZ_CORE_OPENGL)
set(FORWARD_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	${INCLUDE_GL_DIR}
	${GLEW_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR})
endif()

# Include directories
include_directories(
	${FORWARD_INCLUDE_DIRS}
	${CATCH_INCLUDE_DIRS}
)

# sfzCore library
# ------------------------------------------------------------------------------------------------

set(SFZ_CORE_FILES
	${INCLUDE_DIR}/sfz/Assert.hpp
	${INCLUDE_DIR}/sfz/Assert.inl
	 ${SOURCE_DIR}/sfz/Assert.cpp
	${INCLUDE_DIR}/sfz/CudaCompatibility.hpp
	${INCLUDE_DIR}/sfz/PopWarnings.hpp
	${INCLUDE_DIR}/sfz/PushWarnings.hpp
	${INCLUDE_DIR}/sfz/SimdIntrinsics.hpp

	${INCLUDE_DIR}/sfz/Containers.hpp
	${INCLUDE_DIR}/sfz/containers/DynArray.hpp
	${INCLUDE_DIR}/sfz/containers/DynArray.inl
	${INCLUDE_DIR}/sfz/containers/HashMap.hpp
	${INCLUDE_DIR}/sfz/containers/HashMap.inl
	${INCLUDE_DIR}/sfz/containers/HashTableKeyDescriptor.hpp

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
	${INCLUDE_DIR}/sfz/geometry/Sphere.hpp

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
	${INCLUDE_DIR}/sfz/math/Vector.inl

	${INCLUDE_DIR}/sfz/Memory.hpp
	${INCLUDE_DIR}/sfz/memory/Allocator.hpp
	 ${SOURCE_DIR}/sfz/memory/Allocator.cpp
	${INCLUDE_DIR}/sfz/memory/DebugAllocator.hpp
	 ${SOURCE_DIR}/sfz/memory/DebugAllocator.cpp
	${INCLUDE_DIR}/sfz/memory/MemoryUtils.hpp
	${INCLUDE_DIR}/sfz/memory/New.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.inl

	${INCLUDE_DIR}/sfz/Screens.hpp
	${INCLUDE_DIR}/sfz/screens/BaseScreen.hpp
	${INCLUDE_DIR}/sfz/screens/GameLoop.hpp
	 ${SOURCE_DIR}/sfz/screens/GameLoop.cpp

	${INCLUDE_DIR}/sfz/SDL.hpp
	${INCLUDE_DIR}/sfz/sdl/ButtonState.hpp
	${INCLUDE_DIR}/sfz/sdl/GameController.hpp
	 ${SOURCE_DIR}/sfz/sdl/GameController.cpp
	${INCLUDE_DIR}/sfz/sdl/Mouse.hpp
	 ${SOURCE_DIR}/sfz/sdl/Mouse.cpp
	${INCLUDE_DIR}/sfz/sdl/Session.hpp
	 ${SOURCE_DIR}/sfz/sdl/Session.cpp
	${INCLUDE_DIR}/sfz/sdl/Window.hpp
	 ${SOURCE_DIR}/sfz/sdl/Window.cpp

	${INCLUDE_DIR}/sfz/Strings.hpp
	${INCLUDE_DIR}/sfz/strings/DynString.hpp
	 ${SOURCE_DIR}/sfz/strings/DynString.cpp
	${INCLUDE_DIR}/sfz/strings/StackString.hpp
	 ${SOURCE_DIR}/sfz/strings/StackString.cpp
	${INCLUDE_DIR}/sfz/strings/StringHashers.hpp
	 ${SOURCE_DIR}/sfz/strings/StringHashers.cpp
	${INCLUDE_DIR}/sfz/strings/StringID.hpp
	 ${SOURCE_DIR}/sfz/strings/StringID.cpp

	${INCLUDE_DIR}/sfz/Util.hpp
	${INCLUDE_DIR}/sfz/util/FrametimeStats.hpp
	 ${SOURCE_DIR}/sfz/util/FrametimeStats.cpp
	${INCLUDE_DIR}/sfz/util/IniParser.hpp
	 ${SOURCE_DIR}/sfz/util/IniParser.cpp
	${INCLUDE_DIR}/sfz/util/IO.hpp
	 ${SOURCE_DIR}/sfz/util/IO.cpp)

if(SFZ_CORE_OPENGL)
set(SFZ_CORE_FILES ${SFZ_CORE_FILES}
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
endif()

set_directory_source_group(${SFZ_CORE_FILES})

# Library compile target
add_library(sfzCoreLib ${SFZ_CORE_FILES})

# Linking libraries
target_link_libraries(
	sfzCoreLib

	${SDL2_LIBRARIES}
)

# Linking OpenGL only libraries
if(SFZ_CORE_OPENGL)
target_link_libraries(
	sfzCoreLib

	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
)
endif()

# Output variables
# ------------------------------------------------------------------------------------------------

set(SFZ_CORE_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	PARENT_SCOPE)
if(SFZ_CORE_OPENGL)
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	${SDL2_LIBRARIES}
	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
	PARENT_SCOPE)
else()
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	${SDL2_LIBRARIES}
	PARENT_SCOPE)
endif()
set(SFZ_CORE_DLLS
	${SDL2_DLLS}
	PARENT_SCOPE)

# Tests
# ------------------------------------------------------------------------------------------------

if(SFZ_CORE_BUILD_TESTS)

	set(SFZ_CORE_TEST_FILES
		${TESTS_DIR}/sfz/Main_Tests.cpp

		${TESTS_DIR}/sfz/containers/DynArray_Tests.cpp
		${TESTS_DIR}/sfz/containers/HashMap_Tests.cpp
		
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
		${TESTS_DIR}/sfz/util/IO_Tests.cpp)
	set_directory_source_group(${SFZ_CORE_TEST_FILES})

	enable_testing(true)
	add_executable(sfzCoreTests ${SFZ_CORE_TEST_FILES})
	target_link_libraries(
		sfzCoreTests

		sfzCoreLib
	)
	add_test(sfzCoreTestsName sfzCoreTests)
	
endif()

# Copy DLLs
# ------------------------------------------------------------------------------------------------

# Only copy DLLs if sfzCore is built individually, not if it is built as subproject
get_directory_property(hasParent PARENT_DIRECTORY)
if((NOT hasParent) AND MSVC)
	file(COPY ${SDL2_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SDL2_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SDL2_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Release)
endif()
