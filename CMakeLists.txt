# Compiler flags and directories
# ------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project("sfzCore")

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
	# MSVC flags
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /arch:AVX = Enable (require) Intel AVX instructions for code generation
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

elseif(EMSCRIPTEN)
	# Emscripten flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++14 = Enable C++14 support
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	# -s USE_SDL=2 = Use SDL2 library
	# -s TOTAL_MEMORY=134217728 = 128MiB heap, TODO: should probably not hard-code this
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-strict-aliasing -s USE_SDL=2 -s TOTAL_MEMORY=134217728 -DSFZ_EMSCRIPTEN")
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

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(INCLUDE_CUDA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include_cuda)
set(INCLUDE_GL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include_gl)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# Check if sfzCore is built individually or part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

# Dependencies
# ------------------------------------------------------------------------------------------------

# Include DownloadProject if building individually
if (NOT hasParent)
	include(cmake/DownloadProject.cmake)
endif()

# SDL2
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES} and ${SDL2_RUNTIME_FILES}
if(hasParent)
	if (NOT ${SDL2_FOUND})
		message(FATAL_ERROR "sfzCore requires SDL2. Add phDependency_SDL2.")
	endif()
else()
	message("Acquiring SDL2")
	download_project(
		PROJ                sdl2
		PREFIX              externals
		GIT_REPOSITORY      https://github.com/PhantasyEngine/phDependency_SDL2.git
		GIT_TAG             22afd827fcedc880e4597a2df8fc8180c8349d8f
		UPDATE_DISCONNECTED 1
		QUIET
	)
	add_subdirectory(${sdl2_SOURCE_DIR})
	message("Finished acquiring SDL2")
endif()

# Catch
add_subdirectory(${EXTERNALS_DIR}/catch)

# OpenGL only libraries
if(SFZ_CORE_OPENGL)

# GLEW (GLEW_INCLUDE_DIRS, GLEW_LIBRARIES and GLEW_DLLS)
if(NOT WIN32)
add_definitions(-DGLEW_STATIC)
endif()
add_subdirectory(${EXTERNALS_DIR}/glew)

# Links OpenGL
find_package(OpenGL REQUIRED) # ${OPENGL_INCLUDE_DIR}, ${OPENGL_LIBRARIES}

endif()

# Accumulated variable with all includes to forward to caller
set(FORWARD_INCLUDE_DIRS
	${INCLUDE_DIR})
if(SFZ_CORE_OPENGL)
set(FORWARD_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	${INCLUDE_GL_DIR}
	${GLEW_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR})
endif()
if(SFZ_CORE_CUDA)
set(FORWARD_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	${INCLUDE_CUDA_DIR})
endif()

# Include directories
include_directories(
	${FORWARD_INCLUDE_DIRS}
	${CATCH_INCLUDE_DIRS}
)

# sfzCore library
# ------------------------------------------------------------------------------------------------

set(SFZ_CORE_INCLUDE_FILES
	${INCLUDE_DIR}/sfz/Assert.hpp
	${INCLUDE_DIR}/sfz/Assert.inl
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
	${INCLUDE_DIR}/sfz/math/MathSupport.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.inl
	${INCLUDE_DIR}/sfz/math/Matrix.hpp
	${INCLUDE_DIR}/sfz/math/Matrix.inl
	${INCLUDE_DIR}/sfz/math/ProjectionMatrices.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.hpp
	${INCLUDE_DIR}/sfz/math/Quaternion.inl
	${INCLUDE_DIR}/sfz/math/Vector.hpp
	${INCLUDE_DIR}/sfz/math/Vector.inl

	${INCLUDE_DIR}/sfz/Memory.hpp
	${INCLUDE_DIR}/sfz/memory/Allocator.hpp
	${INCLUDE_DIR}/sfz/memory/DebugAllocator.hpp
	${INCLUDE_DIR}/sfz/memory/MemoryUtils.hpp
	${INCLUDE_DIR}/sfz/memory/New.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.hpp
	${INCLUDE_DIR}/sfz/memory/SmartPointers.inl
	
	${INCLUDE_DIR}/sfz/Screens.hpp
	${INCLUDE_DIR}/sfz/screens/BaseScreen.hpp
	${INCLUDE_DIR}/sfz/screens/GameLoop.hpp

	${INCLUDE_DIR}/sfz/SDL.hpp
	${INCLUDE_DIR}/sfz/sdl/ButtonState.hpp
	${INCLUDE_DIR}/sfz/sdl/GameController.hpp
	${INCLUDE_DIR}/sfz/sdl/Mouse.hpp
	${INCLUDE_DIR}/sfz/sdl/Session.hpp
	${INCLUDE_DIR}/sfz/sdl/Window.hpp

	${INCLUDE_DIR}/sfz/Strings.hpp
	${INCLUDE_DIR}/sfz/strings/DynString.hpp
	${INCLUDE_DIR}/sfz/strings/StackString.hpp
	${INCLUDE_DIR}/sfz/strings/StringHashers.hpp
	${INCLUDE_DIR}/sfz/strings/StringID.hpp

	${INCLUDE_DIR}/sfz/Util.hpp
	${INCLUDE_DIR}/sfz/util/FrametimeStats.hpp
	${INCLUDE_DIR}/sfz/util/IniParser.hpp
	${INCLUDE_DIR}/sfz/util/IO.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${SFZ_CORE_INCLUDE_FILES})

set(SFZ_CORE_SOURCE_FILES
	
	${SOURCE_DIR}/sfz/Assert.cpp

	${SOURCE_DIR}/sfz/math/MathPrimitiveToStrings.cpp
	${SOURCE_DIR}/sfz/math/ProjectionMatrices.cpp
	
	${SOURCE_DIR}/sfz/memory/Allocator.cpp
	${SOURCE_DIR}/sfz/memory/DebugAllocator.cpp
	
	${SOURCE_DIR}/sfz/screens/GameLoop.cpp

	${SOURCE_DIR}/sfz/sdl/GameController.cpp
	${SOURCE_DIR}/sfz/sdl/Mouse.cpp
	${SOURCE_DIR}/sfz/sdl/Session.cpp
	${SOURCE_DIR}/sfz/sdl/Window.cpp

	${SOURCE_DIR}/sfz/strings/DynString.cpp
	${SOURCE_DIR}/sfz/strings/StackString.cpp
	${SOURCE_DIR}/sfz/strings/StringHashers.cpp
	${SOURCE_DIR}/sfz/strings/StringID.cpp

	${SOURCE_DIR}/sfz/util/FrametimeStats.cpp
	${SOURCE_DIR}/sfz/util/IniParser.cpp
	${SOURCE_DIR}/sfz/util/IO.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${SFZ_CORE_SOURCE_FILES})

if(SFZ_CORE_OPENGL)
set(SFZ_CORE_INCLUDE_GL_FILES
	${INCLUDE_GL_DIR}/sfz/GL.hpp
	${INCLUDE_GL_DIR}/sfz/gl/Context.hpp
	${INCLUDE_GL_DIR}/sfz/gl/Framebuffer.hpp
	${INCLUDE_GL_DIR}/sfz/gl/FullscreenQuad.hpp
	${INCLUDE_GL_DIR}/sfz/gl/GLUtils.hpp
	${INCLUDE_GL_DIR}/sfz/gl/IncludeOpenGL.hpp
	${INCLUDE_GL_DIR}/sfz/gl/Program.hpp
)
source_group(TREE ${INCLUDE_GL_DIR} FILES ${SFZ_CORE_INCLUDE_GL_FILES})
set(SFZ_CORE_SOURCE_GL_FILES
	${SOURCE_DIR}/sfz/gl/Context.cpp
	${SOURCE_DIR}/sfz/gl/Framebuffer.cpp
	${SOURCE_DIR}/sfz/gl/FullscreenQuad.cpp
	${SOURCE_DIR}/sfz/gl/GLUtils.cpp
	${SOURCE_DIR}/sfz/gl/Program.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${SFZ_CORE_SOURCE_GL_FILES})
else()
set(SFZ_CORE_INCLUDE_GL_FILES "")
set(SFZ_CORE_SOURCE_GL_FILES "")
endif()

if(SFZ_CORE_CUDA)
set(SFZ_CORE_INCLUDE_CUDA_FILES
	${INCLUDE_CUDA_DIR}/sfz/Cuda.hpp
	${INCLUDE_CUDA_DIR}/sfz/cuda/Buffer.hpp
	${INCLUDE_CUDA_DIR}/sfz/cuda/CudaUtils.hpp
)
source_group(TREE ${INCLUDE_CUDA_DIR} FILES ${SFZ_CORE_INCLUDE_CUDA_FILES})
else()
set(SFZ_CORE_INCLUDE_CUDA_FILES "")
endif()

set(SFZ_CORE_FILES
	${SFZ_CORE_INCLUDE_FILES}
	${SFZ_CORE_SOURCE_FILES}
	${SFZ_CORE_INCLUDE_GL_FILES}
	${SFZ_CORE_SOURCE_GL_FILES}
	${SFZ_CORE_INCLUDE_CUDA_FILES}
)

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

set(SFZ_CORE_FOUND true PARENT_SCOPE)

set(SFZ_CORE_INCLUDE_DIRS
	${FORWARD_INCLUDE_DIRS}
	PARENT_SCOPE)

if(SFZ_CORE_OPENGL)
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
	PARENT_SCOPE)
else()
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
	PARENT_SCOPE)
endif()

if(SFZ_CORE_OPENGL)
set(SFZ_CORE_DLLS
	${GLEW_DLLS}
	PARENT_SCOPE)
else()
set(SFZ_CORE_DLLS "" PARENT_SCOPE)
endif()


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
		${TESTS_DIR}/sfz/util/IO_Tests.cpp
	)
	source_group(TREE ${TESTS_DIR} FILES ${SFZ_CORE_TEST_FILES})

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
if((NOT hasParent) AND MSVC)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)

	file(COPY ${GLEW_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${GLEW_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${GLEW_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Release)
endif()
