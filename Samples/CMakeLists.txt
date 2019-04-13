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
project("ZeroG Samples" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Add the FetchContent module
include(FetchContent)

# The root directory of the ZeroG repository
set(ZEROG_REPO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Place results in the standard output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# SDL2
# ------------------------------------------------------------------------------------------------

# TODO: SDL2 is currently required to run the samples, should be removed sometime in the future

# Adding SDL2
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES}, ${SDL2_RUNTIME_FILES}
if (EXISTS ${ZEROG_REPO_ROOT_DIR}/../Dependency-SDL2)
	message("-- [ZeroG Samples]: Using local Dependency-SDL2")
	add_subdirectory(${ZEROG_REPO_ROOT_DIR}/../Dependency-SDL2 ${CMAKE_BINARY_DIR}/sdl2)

else()
	message("-- [ZeroG Samples]: Downloading Dependency-SDL2 from GitHub")

	FetchContent_Declare(
		sdl2
		GIT_REPOSITORY https://github.com/PetorSFZ/Dependency-SDL2.git
		GIT_TAG 9667334164b7fcaca6a7ec52f38bd79f9839a5dd
	)
	FetchContent_GetProperties(sdl2)
	if(NOT sdl2_POPULATED)
		FetchContent_Populate(sdl2)
		add_subdirectory(${sdl2_SOURCE_DIR} ${CMAKE_BINARY_DIR}/sdl2)
	endif()
endif()

# Copy SDL2.dll on Windows
if (MSVC)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR})
	if(CMAKE_CONFIGURATION_TYPES)
		file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
		file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)
	endif()
endif()

# ZeroG
# ------------------------------------------------------------------------------------------------

# Adding ZeroG
# ${ZEROG_FOUND}, ${ZEROG_INCLUDE_DIRS}, ${ZEROG_LIBRARIES} and ${ZEROG_CPP_LIBRARIES}
add_subdirectory(${ZEROG_REPO_ROOT_DIR}/Lib-ZeroG ${CMAKE_BINARY_DIR}/ZeroG)

# Copy ZeroG runtime files
file(COPY ${ZEROG_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR})
if(CMAKE_CONFIGURATION_TYPES)
	file(COPY ${ZEROG_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${ZEROG_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${ZEROG_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)
endif()

# Compiler flags
# ------------------------------------------------------------------------------------------------

if(MSVC)
	# MSVC flags
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /wd26495 = Disable warning 26495 ("always initialize a member variable")
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


else()
	message(FATAL_ERROR "Not implemented!")
endif()

message("-- [ZeroG Samples]: Compiling with the following compiler flags:")
message("  -- CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message("  -- CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
message("  -- CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message("  -- CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

# Add SampleCommon
# ------------------------------------------------------------------------------------------------

# Helper library with common functions used by multiple samples
# ${ZEROG_SAMPLE_COMMON_INCLUDE_DIRS}, ${ZEROG_SAMPLE_COMMON_LIBRARIES}
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SampleCommon)

# Add samples
# ------------------------------------------------------------------------------------------------

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Sample-1-Experiments)

# Symlink resource ("res") directory into build directory
# ------------------------------------------------------------------------------------------------

if(MSVC)

	set(SYMLINK_FILE "${CMAKE_BINARY_DIR}/create_symlinks.bat")
	set(DEBUG_DIR "${CMAKE_BINARY_DIR}/Debug")
	set(RELWITHDEBINFO_DIR "${CMAKE_BINARY_DIR}/RelWithDebInfo")
	set(RELEASE_DIR "${CMAKE_BINARY_DIR}/Release")

	set(symlinkDirName "res")
	set(symlinkPath "${CMAKE_CURRENT_SOURCE_DIR}/${symlinkDirName}")

	# Append symlink commands to file
	file(APPEND ${SYMLINK_FILE} ": Create symlinks for \"${symlinkDirName}\"\n")
	file(APPEND ${SYMLINK_FILE} "mklink /D \"${CMAKE_BINARY_DIR}/${symlinkDirName}\" \"${symlinkPath}\"\n")
	if(CMAKE_CONFIGURATION_TYPES)
		file(APPEND ${SYMLINK_FILE} "mklink /D \"${DEBUG_DIR}/${symlinkDirName}\" \"${symlinkPath}\"\n")
		file(APPEND ${SYMLINK_FILE} "mklink /D \"${RELWITHDEBINFO_DIR}/${symlinkDirName}\" \"${symlinkPath}\"\n")
		file(APPEND ${SYMLINK_FILE} "mklink /D \"${RELEASE_DIR}/${symlinkDirName}\" \"${symlinkPath}\"\n")
	endif()
	file(APPEND ${SYMLINK_FILE} "\n")

	# Execute file
	message("-- [ZeroG Samples]: Executing create symlink script \"${SYMLINK_FILE}\"")
	execute_process(COMMAND ${SYMLINK_FILE} OUTPUT_QUIET ERROR_QUIET)

else()
	message(FATAL_ERROR "Not implemented")
endif()
