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
project("ZeroG Sample 3 - Minimal Compute" LANGUAGES CXX)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_FILES
	${SRC_DIR}/Sample3.cpp
)

add_executable(ZeroG-Sample3-MinimalCompute ${SRC_FILES})

target_include_directories(ZeroG-Sample3-MinimalCompute PRIVATE
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
	${ZEROG_INCLUDE_DIRS}
	${ZEROG_SAMPLE_COMMON_INCLUDE_DIRS}
)

target_link_libraries(ZeroG-Sample3-MinimalCompute
	${SDL2_LIBRARIES}
	${ZEROG_LIBRARIES}
	${ZEROG_SAMPLE_COMMON_LIBRARIES}
)
