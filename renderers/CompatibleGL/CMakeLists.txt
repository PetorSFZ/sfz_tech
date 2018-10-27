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
project("Renderer-CompatibleGL")

# Input options
# ------------------------------------------------------------------------------------------------

# PH_RENDERER_COMPATIBLE_GL_STATIC: Creates static library if defined

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT SDL2_FOUND)
	message(FATAL_ERROR "Renderer-CompatibleGL requires SDL2. Add Dependency-SDL2.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "Renderer-CompatibleGL requires sfzCore.")
endif()

if (NOT SFZ_CORE_OPENGL_FOUND)
	message(FATAL_ERROR "Renderer-CompatibleGL requires lib-opengl from sfzCore, define SFZ_CORE_OPENGL before including sfzCore.")
endif()

if (NOT PH_INTERFACE_FOUND)
	message(FATAL_ERROR "Renderer-CompatibleGL requires PhantasyEngine-Interface.")
endif()

# Renderer-CompatibleGL
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res_compgl)

# Source files
set(PH_RENDERER_COMPATIBLE_GL_SOURCE_FILES
	${INCLUDE_DIR}/ph/ImguiRendering.hpp
	 ${SOURCE_DIR}/ph/ImguiRendering.cpp
	${INCLUDE_DIR}/ph/Model.hpp
	 ${SOURCE_DIR}/ph/Model.cpp
	 ${SOURCE_DIR}/ph/Renderer-CompatibleGL.cpp
	${INCLUDE_DIR}/ph/Shaders.hpp
	 ${SOURCE_DIR}/ph/Shaders.cpp
	${INCLUDE_DIR}/ph/Texture.hpp
	 ${SOURCE_DIR}/ph/Texture.cpp
)
source_group(TREE ${SOURCE_DIR} FILES ${PH_RENDERER_COMPATIBLE_GL_SOURCE_FILES})

# Library
if(PH_RENDERER_COMPATIBLE_GL_STATIC)
	add_library(Renderer-CompatibleGL ${PH_RENDERER_COMPATIBLE_GL_SOURCE_FILES})
else()
	add_library(Renderer-CompatibleGL SHARED ${PH_RENDERER_COMPATIBLE_GL_SOURCE_FILES})
endif()

target_include_directories(Renderer-CompatibleGL PUBLIC
	${INCLUDE_DIR}
	${SDL2_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${SFZ_CORE_OPENGL_INCLUDE_DIRS}
	${PH_INTERFACE_INCLUDE_DIRS}
)

target_link_libraries(Renderer-CompatibleGL
	${SDL2_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
	${SFZ_CORE_OPENGL_LIBRARIES}
	${PH_INTERFACE_LIBRARIES}
)

# iOS specific stuff
# ------------------------------------------------------------------------------------------------

if (IOS)
	target_link_libraries(Renderer-CompatibleGL "-framework AudioToolbox")
	target_link_libraries(Renderer-CompatibleGL "-framework AVFoundation")
	target_link_libraries(Renderer-CompatibleGL "-framework CoreGraphics")
	target_link_libraries(Renderer-CompatibleGL "-framework CoreMotion")
	target_link_libraries(Renderer-CompatibleGL "-framework Foundation")
	target_link_libraries(Renderer-CompatibleGL "-framework GameController")
	target_link_libraries(Renderer-CompatibleGL "-framework OpenGLES")
	target_link_libraries(Renderer-CompatibleGL "-framework QuartzCore")
	target_link_libraries(Renderer-CompatibleGL "-framework UIKit")
	target_link_libraries(Renderer-CompatibleGL "-framework Metal")
endif()

# Output variables
# ------------------------------------------------------------------------------------------------

set(PH_RENDERER_COMPATIBLE_GL_FOUND true PARENT_SCOPE)
set(PH_RENDERER_COMPATIBLE_GL_LIBRARIES Renderer-CompatibleGL PARENT_SCOPE)
set(PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR ${RESOURCES_DIR} PARENT_SCOPE)
