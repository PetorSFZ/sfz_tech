# Copyright (c) Peter Hillerström 2022 (skipifzero.com, peter@hstroem.se)
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
	message(FATAL_ERROR "Please include gpu_lib using add_subdirectory")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "gpu_lib requires Lib-Core.")
endif()

# Bundled externals
# ------------------------------------------------------------------------------------------------

set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# D3D12 Agility SDK
# ${D3D12_AGILITY_SDK_FOUND}, ${D3D12_AGILITY_SDK_INCLUDE_DIRS}, ${D3D12_AGILITY_SDK_RUNTIME_FILES}
add_subdirectory(${EXTERNALS_DIR}/D3D12AgilitySDK)

# dxc
# ${DXC_FOUND}, ${DXC_INCLUDE_DIRS}, ${DXC_LIBRARIES}, ${DXC_RUNTIME_FILES}
add_subdirectory(${EXTERNALS_DIR}/dxc)

# gpu_lib library
# ------------------------------------------------------------------------------------------------

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.hpp ${SRC_DIR}/*.cpp ${SRC_DIR}/*.h ${SRC_DIR}/*.c)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_library(gpu_lib ${SRC_FILES})

target_include_directories(gpu_lib PUBLIC
	${SRC_DIR}
	${D3D12_AGILITY_SDK_INCLUDE_DIRS}
	${DXC_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
)

target_link_libraries(gpu_lib
	${DXC_LIBRARIES}
)

# Output variables (Parent scope)
# ------------------------------------------------------------------------------------------------

set(GPU_LIB_FOUND true PARENT_SCOPE)
set(GPU_LIB_INCLUDE_DIRS ${SRC_DIR} PARENT_SCOPE)
set(GPU_LIB_LIBRARIES gpu_lib PARENT_SCOPE)
set(GPU_LIB_RUNTIME_FILES ${DXC_RUNTIME_FILES} PARENT_SCOPE)
set(GPU_LIB_D3D12_AGILITY_SDK_FILES ${D3D12_AGILITY_SDK_RUNTIME_FILES} PARENT_SCOPE)
