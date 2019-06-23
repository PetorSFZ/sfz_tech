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

cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project("Renderer-Null")

# Input options
# ------------------------------------------------------------------------------------------------

# PH_RENDERER_NULL_STATIC: Creates static library if defined

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT PH_INTERFACE_FOUND)
	message(FATAL_ERROR "Renderer-Null requires PhantasyEngine-Interface.")
endif()

# Renderer-NULL
# ------------------------------------------------------------------------------------------------

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(PH_RENDERER_NULL_SOURCE_FILES
	 ${SOURCE_DIR}/ph/Renderer-Null.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${PH_RENDERER_NULL_SOURCE_FILES})

# Library
if(PH_RENDERER_NULL_STATIC)
	add_library(Renderer-Null ${PH_RENDERER_NULL_SOURCE_FILES})
else()
	add_library(Renderer-Null SHARED ${PH_RENDERER_NULL_SOURCE_FILES})
endif()

target_include_directories(Renderer-Null PUBLIC
	${PH_INTERFACE_INCLUDE_DIRS}
)

target_link_libraries(Renderer-Null
	${PH_INTERFACE_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PH_RENDERER_NULL_FOUND true PARENT_SCOPE)
set(PH_RENDERER_NULL_LIBRARIES Renderer-Null PARENT_SCOPE)
