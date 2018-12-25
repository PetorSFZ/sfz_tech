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
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /arch:AVX = Enable (require) Intel AVX instructions for code generation
	# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
	# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
	# /Od = "disables optimization, speeding compilation and simplifying debugging"
	# /DEBUG = "creates debugging information for the .exe file or DLL"
	# /O2 = Optimize code for fastest speed
	# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
	set(CMAKE_CXX_FLAGS "/W4 /wd4201 /Zi /EHsc /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast")

else()
	message(FATAL_ERROR "Not implemented!")
endif()

message("-- [ZeroG]: Compiling with the following compiler flags:")
message("  -- CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message("  -- CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
message("  -- CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message("  -- CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

# Dependencies
# ------------------------------------------------------------------------------------------------

# TODO: ZeroG should not depend on SDL2
if (NOT SDL2_FOUND)
	message(FATAL_ERROR "ZeroG currently requires SDL2. Add Dependency-SDL2.")
endif()

# ZeroG C-API
# ------------------------------------------------------------------------------------------------

set(ZEROG_CAPI_INCLUDE_FILES
	${INCLUDE_DIR}/ZeroG/ZeroG-CApi.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${ZEROG_CAPI_INCLUDE_FILES})

set(ZEROG_CAPI_SRC_FILES
	${SRC_DIR}/ZeroG/d3d12/D3D12Api.hpp
	${SRC_DIR}/ZeroG/d3d12/D3D12Api.cpp
	${SRC_DIR}/ZeroG/Api.hpp
	${SRC_DIR}/ZeroG/ZeroG-CApi.cpp
)
source_group(TREE ${SRC_DIR} FILES ${ZEROG_CAPI_SRC_FILES})

add_library(ZeroG SHARED ${ZEROG_CAPI_INCLUDE_FILES} ${ZEROG_CAPI_SRC_FILES})

target_include_directories(ZeroG PRIVATE
	${INCLUDE_DIR}
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
)

target_link_libraries(ZeroG ${SDL2_LIBRARIES})

# ZeroG C++ Wrapper
# ------------------------------------------------------------------------------------------------

set(ZEROG_CPP_INCLUDE_FILES
	${INCLUDE_DIR}/ZeroG/ZeroG.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${ZEROG_CPP_INCLUDE_FILES})

set(ZEROG_CPP_SRC_FILES
	${SRC_DIR}/ZeroG/ZeroG-CppWrapper.cpp
)
source_group(TREE ${SRC_DIR} FILES ${ZEROG_CPP_SRC_FILES})

add_library(ZeroG-cpp ${ZEROG_CPP_INCLUDE_FILES} ${ZEROG_CPP_SRC_FILES})

target_include_directories(ZeroG-cpp PRIVATE
	${INCLUDE_DIR}
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
)

target_link_libraries(ZeroG-cpp ZeroG ${SDL2_LIBRARIES})

# Output variables (Parent scope)
# ------------------------------------------------------------------------------------------------

# Check if ZeroG is built individually or as part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

if(hasParent)
	set(ZEROG_FOUND true PARENT_SCOPE)
	set(ZEROG_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
	set(ZEROG_LIBRARIES ZeroG PARENT_SCOPE)
	set(ZEROG_CPP_LIBRARIES ZeroG-cpp ZeroG PARENT_SCOPE)
endif()
