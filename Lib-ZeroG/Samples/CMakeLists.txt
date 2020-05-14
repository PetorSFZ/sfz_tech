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

# The root directory of the ZeroG repository
set(ZEROG_REPO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Place results in the standard output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# SDL2
# ------------------------------------------------------------------------------------------------

# SDL2
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES}, ${SDL2_RUNTIME_FILES}
message("-- [ZeroG Samples]: Adding sfz_tech's bundled SDL2")
add_subdirectory(${ZEROG_REPO_ROOT_DIR}/../externals/sdl2 ${CMAKE_BINARY_DIR}/sdl2)

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

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/ZeroG)

if(MSVC)
	if (NOT ZEROG_FOUND)
		message(FATAL_ERROR "-- [ZeroG]: Attempting to msvcCopyZeroGRuntimeFiles(), but ZeroG is not found")
	endif()

	message("-- [ZeroG]: Copying following DLLs to binary directory:")
	foreach(dllPath ${ZEROG_RUNTIME_FILES})
		message("  -- ${dllPath}")
	endforeach()

	foreach(dllPath ${ZEROG_RUNTIME_FILES})

		# Do different things depending on if we are generating .sln or using built-in VS CMake
		if(CMAKE_CONFIGURATION_TYPES)
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/Release)
		else()
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR})
		endif()

	endforeach()
endif()

# Check platforms
# ------------------------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set(iOS true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(macOS true)
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

elseif(macOS)
	# macOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++17 = Enable C++17 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-rtti = Disable RTTI
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-rtti -fno-strict-aliasing -DZG_MACOS")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")

elseif(iOS)
	# iOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++17 = Enable C++17 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-rtti = Disable RTTI
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-rtti -fno-strict-aliasing -DZG_IOS")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")

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

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Sample-1-MinimalClearScreen)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Sample-2-SimpleRendering)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Sample-3-MinimalCompute)

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

elseif(macOS)

	# Directories
	set(DUMMY_DIR "${CMAKE_BINARY_DIR}/dummy")
	set(DUMMY_SYMLINK_FILE "${CMAKE_BINARY_DIR}/dummy/create_symlinks.sh")
	set(SYMLINK_FILE "${CMAKE_BINARY_DIR}/create_symlinks.sh")
	set(DEBUG_DIR "${CMAKE_BINARY_DIR}/Debug")
	set(RELWITHDEBINFO_DIR "${CMAKE_BINARY_DIR}/RelWithDebInfo")
	set(RELEASE_DIR "${CMAKE_BINARY_DIR}/Release")

	set(symlinkDirName "res")
	set(symlinkPath "${CMAKE_CURRENT_SOURCE_DIR}/${symlinkDirName}")

	# Create the dummy directory
	file(MAKE_DIRECTORY ${DUMMY_DIR})

	# Remove old files (if they existed)
	file(REMOVE ${DUMMY_SYMLINK_FILE} ${SYMLINK_FILE})

	# Append create directories commands to file
	file(APPEND ${DUMMY_SYMLINK_FILE} "# Create Debug, Release and RelWithDebInfo directories\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "# (These are used for some IDE's, such as Xcode. Not for makefiles.)\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "mkdir ${DEBUG_DIR}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "mkdir ${RELWITHDEBINFO_DIR}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "mkdir ${RELEASE_DIR}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "\n")

	# Get name of directory to symlink
	get_filename_component(SYMLINK_DIR_NAME ${symlinkPath} NAME)

	# Append symlink commands to file
	file(APPEND ${DUMMY_SYMLINK_FILE} "# Create symlinks for \"${SYMLINK_DIR_NAME}\"\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "ln -sfn ${symlinkPath} ${CMAKE_BINARY_DIR}/${SYMLINK_DIR_NAME}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "ln -sfn ${symlinkPath} ${DEBUG_DIR}/${SYMLINK_DIR_NAME}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "ln -sfn ${symlinkPath} ${RELWITHDEBINFO_DIR}/${SYMLINK_DIR_NAME}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "ln -sfn ${symlinkPath} ${RELEASE_DIR}/${SYMLINK_DIR_NAME}\n")
	file(APPEND ${DUMMY_SYMLINK_FILE} "\n")

	# Copy script from dummy folder to binary directory
	# Small hack, needed because we need the script to be executable. The only way I could find
	# to accomplish this in CMake was to copy the file
	file(COPY ${DUMMY_SYMLINK_FILE} DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS
		OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

	# Execute process
	execute_process(COMMAND ${SYMLINK_FILE} OUTPUT_QUIET ERROR_QUIET)

else()
	message(FATAL_ERROR "Not implemented")
endif()
