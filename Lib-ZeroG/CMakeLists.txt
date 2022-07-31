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
	message(FATAL_ERROR "Please include ZeroG using add_subdirectory")
endif()

# sfz_core
# ------------------------------------------------------------------------------------------------

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "ZeroG requires Lib-Core.")
endif()

# Bundled externals
# ------------------------------------------------------------------------------------------------

set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

if (MSVC)

	# D3D12 Agility SDK
	# ${D3D12_AGILITY_SDK_FOUND}, ${D3D12_AGILITY_SDK_INCLUDE_DIRS}, ${D3D12_AGILITY_SDK_RUNTIME_FILES}
	add_subdirectory(${EXTERNALS_DIR}/D3D12AgilitySDK)

	# D3D12 Memory Allocator
	# ${D3D12_MEMORY_ALLOCATOR_FOUND}, ${D3D12_MEMORY_ALLOCATOR_INCLUDE_DIRS}, ${D3D12_MEMORY_ALLOCATOR_LIBRARIES}
	add_subdirectory(${EXTERNALS_DIR}/D3D12MemoryAllocator)

	# D3DX12
	# ${D3DX12_FOUND}, ${D3DX12_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/D3DX12)

	# dxc
	# ${DXC_FOUND}, ${DXC_INCLUDE_DIRS}, ${DXC_LIBRARIES}, ${DXC_RUNTIME_FILES}
	add_subdirectory(${EXTERNALS_DIR}/dxc)
endif()

# ZeroG
# ------------------------------------------------------------------------------------------------

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(ZEROG_INCLUDE_FILES
	${INCLUDE_DIR}/ZeroG.h
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ZEROG_INCLUDE_FILES})

# Common files
file(GLOB_RECURSE ZEROG_COMMON_FILES CONFIGURE_DEPENDS ${SRC_DIR}/common/*.hpp ${SRC_DIR}/common/*.cpp ${SRC_DIR}/common/*.h ${SRC_DIR}/common/*.c)
source_group(TREE ${SRC_DIR} FILES ${ZEROG_COMMON_FILES})

# D3D12 source files
if (MSVC)
	file(GLOB_RECURSE ZEROG_D3D12_SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/d3d12/*.hpp ${SRC_DIR}/d3d12/*.cpp ${SRC_DIR}/d3d12/*.h ${SRC_DIR}/d3d12/*.c)
	source_group(TREE ${SRC_DIR} FILES ${ZEROG_D3D12_SRC_FILES})

	add_library(ZeroG-D3D12 SHARED
		${ZEROG_INCLUDE_FILES}
		${ZEROG_COMMON_FILES}
		${ZEROG_D3D12_SRC_FILES})

	target_include_directories(ZeroG-D3D12 PRIVATE
		${INCLUDE_DIR}
		${SRC_DIR}
		${SFZ_CORE_INCLUDE_DIRS}
		${D3D12_AGILITY_SDK_INCLUDE_DIRS}
		${D3D12_MEMORY_ALLOCATOR_INCLUDE_DIRS}
		${D3DX12_INCLUDE_DIRS}
		${DXC_INCLUDE_DIRS}
	)

	target_link_libraries(ZeroG-D3D12
		${D3D12_MEMORY_ALLOCATOR_LIBRARIES}
		${DXC_LIBRARIES}
	)
endif()

# Runtime files (DLLs)
# ------------------------------------------------------------------------------------------------

if (MSVC)
	set(RUNTIME_FILES ${DXC_RUNTIME_FILES})
endif()

# Output variables (Parent scope)
# ------------------------------------------------------------------------------------------------

set(ZEROG_FOUND true PARENT_SCOPE)
set(ZEROG_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(ZEROG_D3D12_LIBRARIES ZeroG-D3D12 PARENT_SCOPE)
set(ZEROG_VULKAN_LIBRARIES ZeroG-Vulkan PARENT_SCOPE)
set(ZEROG_RUNTIME_FILES ${RUNTIME_FILES} PARENT_SCOPE)
set(ZEROG_D3D12_AGILITY_SDK_FILES ${D3D12_AGILITY_SDK_RUNTIME_FILES} PARENT_SCOPE)
