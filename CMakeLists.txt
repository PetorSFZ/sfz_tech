# Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
#               For other contributors see Contributors.txt
# 
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
# 
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project("PhantasyEngine")

# Compiler flags and directories
# ------------------------------------------------------------------------------------------------

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
	# Visual Studio flags
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /EHsc = TODO: Add explanation
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
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-strict-aliasing -s USE_SDL=2 -DSFZ_EMSCRIPTEN")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0")

else()
	# GCC / Clang flags
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -pthread -fno-strict-aliasing")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

endif()

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT ${SDL2_FOUND})
	message(FATAL_ERROR "PhantasyEngine requires SDL2. Add phDependency_SDL2.")
endif()

# PhantasyEngine
# ------------------------------------------------------------------------------------------------

set(PHANTASY_ENGINE_HEADER_FILES
	${INCLUDE_DIR}/ph/PhantasyEngineMain.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${PHANTASY_ENGINE_HEADER_FILES})

set(PHANTASY_ENGINE_SOURCE_FILES
	${SOURCE_DIR}/ph/PhantasyEngineMain.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${PHANTASY_ENGINE_SOURCE_FILES})

add_library(PhantasyEngineLib ${PHANTASY_ENGINE_HEADER_FILES} ${PHANTASY_ENGINE_SOURCE_FILES})

target_include_directories(PhantasyEngineLib PUBLIC
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIRS}
)

target_link_libraries(PhantasyEngineLib
	${SDL2_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PHANTASY_ENGINE_FOUND true PARENT_SCOPE)
set(PHANTASY_ENGINE_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(PHANTASY_ENGINE_LIBRARIES PhantasyEngineLib PARENT_SCOPE)
