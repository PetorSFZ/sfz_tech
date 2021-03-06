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
project("ZeroG Sample 2 - Simple Rendering" LANGUAGES CXX)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_FILES
	${SRC_DIR}/Sample2.cpp
)

add_executable(ZeroG-Sample2-SimpleRendering ${SRC_FILES})

target_include_directories(ZeroG-Sample2-SimpleRendering PRIVATE
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
	${ZEROG_INCLUDE_DIRS}
	${ZEROG_SAMPLE_COMMON_INCLUDE_DIRS}
)

target_link_libraries(ZeroG-Sample2-SimpleRendering
	${SDL2_LIBRARIES}
	${ZEROG_SAMPLE_COMMON_LIBRARIES}
)

if (MSVC)
	target_link_libraries(ZeroG-Sample2-SimpleRendering ${ZEROG_D3D12_LIBRARIES})
else()
	target_link_libraries(ZeroG-Sample2-SimpleRendering ${ZEROG_VULKAN_LIBRARIES})
endif()
