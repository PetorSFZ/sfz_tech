# Author: Peter Hillerström (https://github.com/PetorSFZ)
# https://github.com/PhantasyEngine/Dependency-SDL2
# License: Public Domain (do whatever you want with this file,
#                         but it would be nice if you kept this header)
# SDL2 License: zlib

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# Check if 32 or 64bit compiler
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITS_64 TRUE)
	set(BITS_32 FALSE)
else()
	set(BITS_64 FALSE)
	set(BITS_32 TRUE)
endif()

if(WIN32)

	# Don't support 32 bit Windows build
	if (BITS32)
		message(FATAL_ERROR "No support for 32bit build")
	endif()

	# Use bundled SDL2 on Windows
	set(SDL2_FOUND true)
	set(SDL2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Windows/SDL2-2.0.12)
	set(SDL2_INCLUDE_DIR ${SDL2_DIR}/include)

	find_library(SDL2_LIB
		NAMES
		SDL2
		HINTS
		${SDL2_DIR}/lib/x64)
	find_library(SDL2MAIN_LIB
		NAMES
		SDL2main
		HINTS
		${SDL2_DIR}/lib/x64)
	set(SDL2_LIBRARY ${SDL2_LIB} ${SDL2MAIN_LIB})
	set(DLLPath
		${SDL2_DIR}/lib/x64/SDL2.dll
		${SDL2_DIR}/README-SDL.txt)

	# Grab all headers from include dir and add to empty custom target for IDE
	file(GLOB_RECURSE SDL2_HEADER_FILES
		"Windows/*.h"
	)
	source_group(TREE ${SDL2_DIR} FILES ${SDL2_HEADER_FILES})
	add_custom_target(SDL2 SOURCES ${SDL2_HEADER_FILES})

elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")

	# Don't support 32 bit iOS build
	if (BITS32)
		message(FATAL_ERROR "No support for 32bit build")
	endif()

	# Use bundled SDL2
	set(SDL2_FOUND true)
	set(SDL2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iOS)
	set(SDL2_INCLUDE_DIR ${SDL2_DIR}/include)
	set(SDL2_LIBRARY ${SDL2_DIR}/lib/libSDL2.a)

	# Grab all headers from include dir and add to empty custom target for IDE
	file(GLOB_RECURSE SDL2_HEADER_FILES
		"iOS/*.h"
	)
	source_group(TREE ${SDL2_DIR} FILES ${SDL2_HEADER_FILES})
	add_custom_target(SDL2 SOURCES ${SDL2_HEADER_FILES})

elseif(EMSCRIPTEN)
	# Do nothing. Emscripten has SDL2 built in.
	set(SDL2_FOUND true)
	set(SDL2_INCLUDE_DIR "")
	set(SDL2_LIBRARY "")
	set(DLLPath "")

else()
	# Attempt to use bundled FindSDL2.cmake module to find SDL2 installed on users system
	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/find_sdl2_module)
	find_package(SDL2 REQUIRED) # ${SDL2_INCLUDE_DIR}, ${SDL2_LIBRARY}
endif()

# Return variables
set(SDL2_FOUND ${SDL2_FOUND} PARENT_SCOPE)
set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR} PARENT_SCOPE)
set(SDL2_LIBRARIES ${SDL2_LIBRARY} PARENT_SCOPE)
set(SDL2_RUNTIME_FILES ${DLLPath} PARENT_SCOPE)
