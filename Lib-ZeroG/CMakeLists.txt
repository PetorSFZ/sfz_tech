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

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project("ZeroG" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

else()
	message(FATAL_ERROR "Not implemented!")
endif()

message("-- [ZeroG]: Compiling with the following compiler flags:")
message("  -- CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message("  -- CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
message("  -- CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message("  -- CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

# Bundled externals
# ------------------------------------------------------------------------------------------------

set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# D3DX12
# ${D3DX12_FOUND}, ${D3DX12_INCLUDE_DIRS}
add_subdirectory(${EXTERNALS_DIR}/D3DX12)

# D3DX12Residency
# ${D3DX12_RESIDENCY_FOUND}, ${D3DX12_RESIDENCY_INCLUDE_DIRS}
add_subdirectory(${EXTERNALS_DIR}/D3DX12Residency)

# SPIRV-Cross
# ${SPIRV_CROSS_FOUND}, ${SPIRV_CROSS_INCLUDE_DIRS}, ${SPIRV_CROSS_LIBRARIES}
add_subdirectory(${EXTERNALS_DIR}/SPIRV-Cross)

# ZeroG C-API
# ------------------------------------------------------------------------------------------------

set(ZEROG_CAPI_INCLUDE_FILES
	${INCLUDE_DIR}/ZeroG.h
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ZEROG_CAPI_INCLUDE_FILES})

set(ZEROG_CAPI_SRC_FILES
	${SRC_DIR}/ZeroG/d3d12/D3D12Backend.hpp
	${SRC_DIR}/ZeroG/d3d12/D3D12Backend.cpp
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
	${SRC_DIR}/ZeroG/d3d12/D3D12Memory.hpp
	${SRC_DIR}/ZeroG/d3d12/D3D12Memory.cpp
	${SRC_DIR}/ZeroG/d3d12/D3D12PipelineRendering.hpp
	${SRC_DIR}/ZeroG/d3d12/D3D12PipelineRendering.cpp
	${SRC_DIR}/ZeroG/d3d12/D3D12Textures.hpp
	${SRC_DIR}/ZeroG/d3d12/D3D12Textures.cpp
	${SRC_DIR}/ZeroG/util/Assert.hpp
	${SRC_DIR}/ZeroG/util/CpuAllocation.hpp
	${SRC_DIR}/ZeroG/util/CpuAllocation.cpp
	${SRC_DIR}/ZeroG/util/Logging.hpp
	${SRC_DIR}/ZeroG/util/Logging.cpp
	${SRC_DIR}/ZeroG/util/RingBuffer.hpp
	${SRC_DIR}/ZeroG/util/Vector.hpp
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
	${D3DX12_INCLUDE_DIRS}
	${D3DX12_RESIDENCY_INCLUDE_DIRS}
	${SPIRV_CROSS_INCLUDE_DIRS}
)

target_link_libraries(ZeroG
	${SPIRV_CROSS_LIBRARIES}
)

# ZeroG C++ Wrapper
# ------------------------------------------------------------------------------------------------

set(ZEROG_CPP_INCLUDE_FILES
	${INCLUDE_DIR}/ZeroG-cpp.hpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ZEROG_CPP_INCLUDE_FILES})

set(ZEROG_CPP_SRC_FILES
	${SRC_DIR}/ZeroG/ZeroG-cpp.cpp
)
source_group(TREE ${SRC_DIR} FILES ${ZEROG_CPP_SRC_FILES})

add_library(ZeroG-cpp ${ZEROG_CPP_INCLUDE_FILES} ${ZEROG_CPP_SRC_FILES})

target_include_directories(ZeroG-cpp PRIVATE
	${INCLUDE_DIR}
	${SRC_DIR}
)

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
	set(ZEROG_CPP_LIBRARIES ZeroG-cpp ZeroG PARENT_SCOPE)
	set(ZEROG_RUNTIME_FILES ${RUNTIME_FILES} PARENT_SCOPE)
endif()
