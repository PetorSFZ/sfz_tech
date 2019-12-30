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

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project("ZeroG" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Check platforms
# ------------------------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set(iOS true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(macOS true)
endif()

# Vulkan
# ------------------------------------------------------------------------------------------------

# Attempt to find Vulkan, done now because we might need to set compiler flags

# Vulkan
# ${VULKAN_FOUND}, ${VULKAN_INCLUDE_DIRS} and ${VULKAN_LIBRARIES}
#if(iOS)
#	set(MoltenVK_Path $ENV{VULKAN_SDK}/..)
#	set(VULKAN_FOUND true)
#	set(VULKAN_INCLUDE_DIRS ${MoltenVK_Path}/MoltenVK/include)
#
#	find_library(MoltenVK_LIB
#		NAMES
#		MoltenVK
#		HINTS
#		${MoltenVK_Path}/MoltenVK/iOS)
#	set(VULKAN_LIBRARIES ${MoltenVK_LIB})
#else()
	find_package(Vulkan)
	set(VULKAN_FOUND ${Vulkan_FOUND})
	set(VULKAN_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
	set(VULKAN_LIBRARIES ${Vulkan_LIBRARIES})
#endif()

if(VULKAN_FOUND)
	message("-- [ZeroG]: Vulkan SDK found")
	message("  -- VULKAN_INCLUDE_DIRS: ${VULKAN_INCLUDE_DIRS}")
	message("  -- VULKAN_LIBRARIES: ${VULKAN_LIBRARIES}")
else()
	message(WARNING "-- [ZeroG]: Vulkan SDK not found, compiling without Vulkan support")
endif()

# Compiler flags
# ------------------------------------------------------------------------------------------------

if(MSVC)
	# MSVC flags
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /wd26451 = Disable warning C26451 ("arithmetic overflow")
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /arch:AVX = Enable (require) Intel AVX instructions for code generation
	# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
	# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
	# /Od = "disables optimization, speeding compilation and simplifying debugging"
	# /DEBUG = "creates debugging information for the .exe file or DLL"
	# /O2 = Optimize code for fastest speed
	# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
	set(CMAKE_CXX_FLAGS "/W4 /wd4201 /wd26495 /wd26451 /std:c++17 /permissive- /Zc:twoPhase- /Zi /Zf /EHsc /GR- /arch:AVX /D_CRT_SECURE_NO_WARNINGS /DWIN32 /D_WINDOWS /JMC /utf-8 /openmp:experimental")
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Od /DEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /O2 /Ob3 /fp:fast /DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob3 /fp:fast /DNDEBUG")
	if (VULKAN_FOUND)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DZG_VULKAN")
	endif()

elseif(macOS)
	# macOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++17 = Enable C++17 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-rtti = Disable RTTI
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-rtti -fno-strict-aliasing -DZG_MACOS")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")

elseif(iOS)
	# iOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++17 = Enable C++17 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-rtti = Disable RTTI
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-rtti -fno-strict-aliasing -DZG_IOS")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")

else()
	message(FATAL_ERROR "Not implemented!")
endif()

message("-- [ZeroG]: Compiling with the following compiler flags:")
message("  -- CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message("  -- CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
message("  -- CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message("  -- CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

# sfz_core
# ------------------------------------------------------------------------------------------------

# Note: sfz_core is currently handled a bit wobbly. We essentially require that "Lib-Core" is placed
#       in the same relative location to "Lib-ZeroG" as it is in the "sfz_tech" repo.

# ${SFZ_CORE_FOUND}, ${SFZ_CORE_INCLUDE_DIRS}
if (NOT SFZ_CORE_FOUND)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../Lib-Core ${CMAKE_BINARY_DIR}/sfz_core)
endif()

# Bundled externals
# ------------------------------------------------------------------------------------------------

set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

if (MSVC)

	# D3DX12
	# ${D3DX12_FOUND}, ${D3DX12_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/D3DX12)

	# D3DX12Residency
	# ${D3DX12_RESIDENCY_FOUND}, ${D3DX12_RESIDENCY_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/D3DX12Residency)
endif()

# SPIRV-Cross
# ${SPIRV_CROSS_FOUND}, ${SPIRV_CROSS_INCLUDE_DIRS}, ${SPIRV_CROSS_LIBRARIES}
add_subdirectory(${EXTERNALS_DIR}/SPIRV-Cross)

# ZeroG C-API
# ------------------------------------------------------------------------------------------------

set(ZEROG_CAPI_INCLUDE_FILES
	${INCLUDE_DIR}/ZeroG.h
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ZEROG_CAPI_INCLUDE_FILES})

# D3D12 source files
if (MSVC)
	set(ZEROG_D3D12_SRC_FILES
		${SRC_DIR}/ZeroG/d3d12/D3D12Backend.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Backend.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Buffer.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Buffer.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12CommandList.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12CommandList.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12CommandQueue.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12CommandQueue.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Common.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Common.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12DescriptorRingBuffer.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12DescriptorRingBuffer.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Framebuffer.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Framebuffer.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12MemoryHeap.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12MemoryHeap.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Pipelines.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Pipelines.cpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Textures.hpp
		${SRC_DIR}/ZeroG/d3d12/D3D12Textures.cpp
	)
endif()

# Vulkan source files
if (VULKAN_FOUND)
	set(ZEROG_VULKAN_SRC_FILES
		${SRC_DIR}/ZeroG/vulkan/VulkanBackend.hpp
		${SRC_DIR}/ZeroG/vulkan/VulkanBackend.cpp
		${SRC_DIR}/ZeroG/vulkan/VulkanCommandQueue.hpp
		${SRC_DIR}/ZeroG/vulkan/VulkanCommandQueue.cpp
		${SRC_DIR}/ZeroG/vulkan/VulkanCommon.hpp
		${SRC_DIR}/ZeroG/vulkan/VulkanCommon.cpp
		${SRC_DIR}/ZeroG/vulkan/VulkanDebug.hpp
		${SRC_DIR}/ZeroG/vulkan/VulkanDebug.cpp
	)
endif()

set(ZEROG_CAPI_SRC_FILES ${ZEROG_D3D12_SRC_FILES} ${ZEROG_METAL_SRC_FILES} ${ZEROG_VULKAN_SRC_FILES})

# Common source files
set(ZEROG_CAPI_SRC_FILES ${ZEROG_CAPI_SRC_FILES}
	${SRC_DIR}/ZeroG/util/ErrorReporting.hpp
	${SRC_DIR}/ZeroG/util/Logging.hpp
	${SRC_DIR}/ZeroG/util/Logging.cpp
	${SRC_DIR}/ZeroG/util/Mutex.hpp
	${SRC_DIR}/ZeroG/util/Strings.hpp
	${SRC_DIR}/ZeroG/BackendInterface.hpp
	${SRC_DIR}/ZeroG/Context.hpp
	${SRC_DIR}/ZeroG/Context.cpp
	${SRC_DIR}/ZeroG/NewOverloads.cpp

	${SRC_DIR}/ZeroG/ZeroG.cpp
)
source_group(TREE ${SRC_DIR} FILES ${ZEROG_CAPI_SRC_FILES})

add_library(ZeroG SHARED ${ZEROG_CAPI_INCLUDE_FILES} ${ZEROG_CAPI_SRC_FILES})

target_include_directories(ZeroG PRIVATE
	${INCLUDE_DIR}
	${SRC_DIR}
	${SFZ_CORE_INCLUDE_DIRS}
	${D3DX12_INCLUDE_DIRS}
	${D3DX12_RESIDENCY_INCLUDE_DIRS}
	${MTLPP_INCLUDE_DIRS}
	${SPIRV_CROSS_INCLUDE_DIRS}
)

target_link_libraries(ZeroG
	${SPIRV_CROSS_LIBRARIES}
	${MTLPP_LIBRARIES}
)

if(VULKAN_FOUND)
	target_include_directories(ZeroG PRIVATE ${VULKAN_INCLUDE_DIRS})
	target_link_libraries(ZeroG ${VULKAN_LIBRARIES})
endif()

# Runtime files (DLLs)
# ------------------------------------------------------------------------------------------------

if (MSVC)
	set(RUNTIME_FILES
		"C:\\Program\ Files\ (x86)\\Windows\ Kits\\10\\Redist\\D3D\\x64\\dxcompiler.dll"
		"C:\\Program\ Files\ (x86)\\Windows\ Kits\\10\\Redist\\D3D\\x64\\dxil.dll")
endif()

# Output variables (Parent scope)
# ------------------------------------------------------------------------------------------------

# Check if ZeroG is built individually or as part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

if(hasParent)
	set(ZEROG_FOUND true PARENT_SCOPE)
	set(ZEROG_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
	set(ZEROG_LIBRARIES ZeroG PARENT_SCOPE)
	set(ZEROG_RUNTIME_FILES ${RUNTIME_FILES} PARENT_SCOPE)
endif()
