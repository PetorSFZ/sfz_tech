# Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
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

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project("ZeroG-cpp" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Check that ZeroG was included first
# ------------------------------------------------------------------------------------------------

if (NOT ZEROG_FOUND)
	message(FATAL_ERROR "ZeroG-cpp requires ZeroG. Add ZeroG.")
endif()

# Compiler flags
# ------------------------------------------------------------------------------------------------

if(MSVC)
	# MSVC flags
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /wd26451 = Disable warning C26451 ("arithmetic overflow")
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /arch:AVX = Enable (require) Intel AVX instructions for code generation
	# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
	# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
	# /Od = "disables optimization, speeding compilation and simplifying debugging"
	# /DEBUG = "creates debugging information for the .exe file or DLL"
	# /O2 = Optimize code for fastest speed
	# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
	set(CMAKE_CXX_FLAGS "/W4 /wd4201 /wd26495 /wd26451 /std:c++17 /permissive- /Zc:twoPhase- /Zi /Zf /EHsc /GR- /arch:AVX /D_CRT_SECURE_NO_WARNINGS /DWIN32 /D_WINDOWS /JMC /utf-8 /openmp:experimental")
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Od /DEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /O2 /Ob3 /fp:fast /DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob3 /fp:fast /DNDEBUG")

elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
	# macOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++14 = Enable C++14 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-rtti = Disable RTTI
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-rtti -fno-strict-aliasing")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")

else()
	message(FATAL_ERROR "Not implemented!")
endif()

message("-- [ZeroG-cpp]: Compiling with the following compiler flags:")
message("  -- CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message("  -- CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
message("  -- CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message("  -- CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

# ZeroG C++ Wrapper
# ------------------------------------------------------------------------------------------------

# Output variables (Parent scope)
# ------------------------------------------------------------------------------------------------

# Check if ZeroG-cpp is built individually or as part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

if(hasParent)
	set(ZEROG_CPP_FOUND true PARENT_SCOPE)
	set(ZEROG_CPP_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
	set(ZEROG_CPP_LIBRARIES "" ZeroG PARENT_SCOPE)
endif()
