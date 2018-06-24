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
project("PhantasyEngine-SharedInterface")

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT SDL2_FOUND)
	message(FATAL_ERROR "PhantasyEngine-SharedInterface requires SDL2. Add Dependency-SDL2.")
endif()

if (NOT IMGUI_FOUND)
	message(FATAL_ERROR "PhantasyEngine-SharedInterface requires dear imgui. Add Dependency-imgui.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "PhantasyEngine-SharedInterface requires sfzCore.")
endif()

# PhantasyEngine-SharedInterface
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PH_SHARED_INTERFACE_HEADER_FILES
	${INCLUDE_DIR}/ph/Bool32.h
	${INCLUDE_DIR}/ph/ConfigInterface.h
	${INCLUDE_DIR}/ph/Context.hpp
	${INCLUDE_DIR}/ph/ExternC.h
	${INCLUDE_DIR}/ph/RendererInterface.h

	${INCLUDE_DIR}/ph/config/GlobalConfig.hpp
	${INCLUDE_DIR}/ph/config/Setting.hpp

	${INCLUDE_DIR}/ph/rendering/CameraData.hpp
	${INCLUDE_DIR}/ph/rendering/ImageView.hpp
	${INCLUDE_DIR}/ph/rendering/ImguiRenderingData.hpp
	${INCLUDE_DIR}/ph/rendering/Material.hpp
	${INCLUDE_DIR}/ph/rendering/MeshView.hpp
	${INCLUDE_DIR}/ph/rendering/RenderEntity.hpp
	${INCLUDE_DIR}/ph/rendering/SphereLight.hpp
	${INCLUDE_DIR}/ph/rendering/Vertex.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${PH_SHARED_INTERFACE_HEADER_FILES})

set(PH_SHARED_INTERFACE_SOURCE_FILES
	${SRC_DIR}/ph/Context.cpp

	${SRC_DIR}/ph/config/GlobalConfig.cpp
	${SRC_DIR}/ph/config/Setting.cpp
)
source_group(TREE ${SRC_DIR} FILES ${PH_SHARED_INTERFACE_SOURCE_FILES})

add_library(
	PhantasyEngine-SharedInterface

	${PH_SHARED_INTERFACE_HEADER_FILES}
	${PH_SHARED_INTERFACE_SOURCE_FILES}
)

target_include_directories(
	PhantasyEngine-SharedInterface
	PUBLIC
	${INCLUDE_DIR}
	${SFZ_CORE_INCLUDE_DIRS}
)

target_link_libraries(
	PhantasyEngine-SharedInterface
	${SFZ_CORE_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PH_SHARED_INTERFACE_FOUND true PARENT_SCOPE)
set(PH_SHARED_INTERFACE_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(PH_SHARED_INTERFACE_LIBRARIES PhantasyEngine-SharedInterface PARENT_SCOPE)
