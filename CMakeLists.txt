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

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT ${SDL2_FOUND})
	message(FATAL_ERROR "PhantasyEngine requires SDL2. Add Dependency-SDL2.")
endif()

if (NOT ${SFZ_CORE_FOUND})
	message(FATAL_ERROR "PhantasyEngine requires sfzCore.")
endif()

if (NOT ${PH_SHARED_INTERFACE_FOUND})
	message(FATAL_ERROR "PhantasyEngine requires PhantasyEngine-SharedInterface.")
endif()

# Bundled dependencies
# ------------------------------------------------------------------------------------------------

set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# stb libraries
# ${STB_INCLUDE_DIRS}
add_subdirectory(${EXTERNALS_DIR}/stb_libraries)

# PhantasyEngine
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PHANTASY_ENGINE_HEADER_FILES
	${INCLUDE_DIR}/ph/PhantasyEngineMain.hpp

	${INCLUDE_DIR}/ph/config/GlobalConfig.hpp
	${INCLUDE_DIR}/ph/config/Setting.hpp

	${INCLUDE_DIR}/ph/game_loop/GameLoop.hpp
	${INCLUDE_DIR}/ph/game_loop/GameLoopUpdateable.hpp

	${INCLUDE_DIR}/ph/rendering/Image.hpp
	${INCLUDE_DIR}/ph/rendering/Renderer.hpp

	${INCLUDE_DIR}/ph/sdl/ButtonState.hpp
	${INCLUDE_DIR}/ph/sdl/GameController.hpp
	${INCLUDE_DIR}/ph/sdl/Mouse.hpp

	${INCLUDE_DIR}/ph/utils/Logging.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${PHANTASY_ENGINE_HEADER_FILES})

set(PHANTASY_ENGINE_SOURCE_FILES
	${SOURCE_DIR}/ph/PhantasyEngineMain.cpp

	${SOURCE_DIR}/ph/config/GlobalConfig.cpp
	${SOURCE_DIR}/ph/config/Setting.cpp

	${SOURCE_DIR}/ph/game_loop/GameLoop.cpp

	${SOURCE_DIR}/ph/rendering/Image.cpp
	${SOURCE_DIR}/ph/rendering/Renderer.cpp

	${SOURCE_DIR}/ph/sdl/GameController.cpp
	${SOURCE_DIR}/ph/sdl/Mouse.cpp

	${SOURCE_DIR}/ph/utils/Logging.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${PHANTASY_ENGINE_SOURCE_FILES})

add_library(phCore ${PHANTASY_ENGINE_HEADER_FILES} ${PHANTASY_ENGINE_SOURCE_FILES})

target_include_directories(phCore PUBLIC
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${PH_SHARED_INTERFACE_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
)

target_link_libraries(phCore
	${SDL2_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PHANTASY_ENGINE_FOUND true PARENT_SCOPE)
set(PHANTASY_ENGINE_INCLUDE_DIRS ${INCLUDE_DIR} ${STB_INCLUDE_DIRS} PARENT_SCOPE)
set(PHANTASY_ENGINE_LIBRARIES phCore PARENT_SCOPE)
