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

# Add samples
# ------------------------------------------------------------------------------------------------

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Sample-1-Experiments)
