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

# Ensure this CMake file is included as part of a larger CMake project
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	message(FATAL_ERROR "Please include ZeroG-ImGui using add_subdirectory")
endif()

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "ZeroG-ImGui requires sfzCore.")
endif()

if (NOT ZEROG_FOUND)
	message(FATAL_ERROR "ZeroG-ImGui requires ZeroG.")
endif()

if (NOT IMGUI_FOUND)
	message(FATAL_ERROR "ZeroG-ImGui requires dear-imgui.")
endif()

# ZeroG-ImGui
# ------------------------------------------------------------------------------------------------

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(ZeroG-ImGui ${INCLUDE_DIR}/ZeroG-ImGui.hpp ${SRC_DIR}/ZeroG-ImGui.cpp)

target_include_directories(ZeroG-ImGui PUBLIC
	${INCLUDE_DIR}
	${SFZ_CORE_INCLUDE_DIRS}
	${ZEROG_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
)

target_link_libraries(ZeroG-ImGui
	${ZEROG_D3D12_LIBRARIES}
	${IMGUI_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(ZEROG_IMGUI_FOUND true PARENT_SCOPE)
set(ZEROG_IMGUI_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(ZEROG_IMGUI_LIBRARIES ZeroG-ImGui PARENT_SCOPE)
