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
project("Renderer-WebGL")

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT ${SDL2_FOUND})
	message(FATAL_ERROR "Renderer-WebGL requires SDL2. Add Dependency-SDL2.")
endif()

if (NOT ${SFZ_CORE_FOUND})
	message(FATAL_ERROR "Renderer-WebGL requires sfzCore.")
endif()

if (NOT ${SFZ_GL_FOUND})
	message(FATAL_ERROR "Renderer-WebGL requires sfzGL.")
endif()

if (NOT ${PH_SHARED_INTERFACE_FOUND})
	message(FATAL_ERROR "Renderer-WebGL requires PhantasyEngine-SharedInterface.")
endif()

# Renderer-WebGL
# ------------------------------------------------------------------------------------------------

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(PH_RENDERER_WEB_GL_SOURCE_FILES
	${SOURCE_DIR}/ph/Renderer-WebGL.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${PH_RENDERER_WEB_GL_SOURCE_FILES})

# Library
add_library(Renderer-WebGL ${PH_RENDERER_WEB_GL_SOURCE_FILES})

target_include_directories(Renderer-WebGL PUBLIC
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${SFZ_GL_INCLUDE_DIRS}
	${PH_SHARED_INTERFACE_INCLUDE_DIRS}
)

target_link_libraries(Renderer-WebGL
	${SDL2_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
	${SFZ_GL_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PH_RENDERER_WEB_GL_FOUND true PARENT_SCOPE)
set(PH_RENDERER_WEB_GL_LIBRARIES Renderer-WebGL PARENT_SCOPE)
