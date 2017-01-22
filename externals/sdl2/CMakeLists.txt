# Author: Peter Hillerström
# Originally part of: https://github.com/PetorSFZ/sfzCore
# License: Public Domain (do whatever you want with this file,
#                         but it would be nice if you kept this header)
# SDL2 License: zlib

# Check if 32 or 64bit compiler
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITS_64 TRUE)
	set(BITS_32 FALSE)
else()
	set(BITS_64 FALSE)
	set(BITS_32 TRUE)
endif()

if(WIN32)
	# Use bundled SDL2 on Windows

	set(SDL2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-devel-2.0.5-VC)
	set(SDL2_INCLUDE_DIR ${SDL2_DIR}/include)

	if (BITS_64)
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
		set(DLLPath ${SDL2_DIR}/lib/x64/SDL2.dll
			        ${SDL2_DIR}/README-SDL.txt)
	
	elseif(BITS32)
		find_library(SDL2_LIB
			NAMES
			SDL2
			HINTS
			${SDL2_DIR}/lib/x86)
		find_library(SDL2MAIN_LIB
			NAMES
			SDL2main
			HINTS
			${SDL2_DIR}/lib/x86)
		set(SDL2_LIBRARY ${SDL2_LIB} ${SDL2MAIN_LIB})
		set(DLLPath ${SDL2_DIR}/lib/x86/SDL2.dll
			        ${SDL2_DIR}/README-SDL.txt)	
	endif()

else()
	# Attempt to use bundled FindSDL2.cmake module to find SDL2 installed on users system
	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/find_sdl2_module)
	find_package(SDL2 REQUIRED) # ${SDL2_INCLUDE_DIR}, ${SDL2_LIBRARY}
endif()

# Return variables
set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR} PARENT_SCOPE)
set(SDL2_LIBRARIES ${SDL2_LIBRARY} PARENT_SCOPE)
set(SDL2_DLLS ${DLLPath} PARENT_SCOPE)
