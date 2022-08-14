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

# Ensure this CMake file is included as part of a larger CMake project
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	message(FATAL_ERROR "Please include PhantasyEngine using add_subdirectory")
endif()

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT SDL2_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires SDL2.")
endif()

if (NOT STB_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires stb libraries.")
endif()

if (NOT SAJSON_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires sajson.")
endif()

if (NOT SOLOUD_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires SoLoud.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires sfzCore.")
endif()

if (NOT ZEROG_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires ZeroG.")
endif()

if (NOT IMGUI_PLOT_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires imgui_plot.")
endif()

if (NOT ZEROG_IMGUI_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires ZeroG-ImGui")
endif()

# PhantasyEngine
# ------------------------------------------------------------------------------------------------

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res_ph)

# Find all source files
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.hpp ${SRC_DIR}/*.cpp ${SRC_DIR}/*.h ${SRC_DIR}/*.c)

add_library(PhantasyEngine ${SRC_FILES})

target_include_directories(PhantasyEngine PUBLIC
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
	${ZEROG_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
	${IMGUI_PLOT_INCLUDE_DIRS}
	${ZEROG_IMGUI_INCLUDE_DIRS}
	${OGT_INCLUDE_DIRS}
	${SAJSON_INCLUDE_DIRS}
	${SOLOUD_INCLUDE_DIRS}
)

target_link_libraries(PhantasyEngine
	${SDL2_LIBRARIES}
	${ZEROG_D3D12_LIBRARIES}
	${IMGUI_LIBRARIES}
	${IMGUI_PLOT_LIBRARIES}
	${ZEROG_IMGUI_LIBRARIES}
	${SOLOUD_LIBRARIES}
)

# PhantasyEngine tests
# ------------------------------------------------------------------------------------------------

# Find all test files
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS ${TESTS_DIR}/*.hpp ${TESTS_DIR}/*.cpp ${TESTS_DIR}/*.h ${TESTS_DIR}/*.c)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PHANTASY_ENGINE_FOUND true PARENT_SCOPE)
set(PHANTASY_ENGINE_INCLUDE_DIRS ${SRC_DIR} PARENT_SCOPE)
set(PHANTASY_ENGINE_LIBRARIES PhantasyEngine PARENT_SCOPE)
set(PHANTASY_ENGINE_RUNTIME_DIR ${RESOURCES_DIR} PARENT_SCOPE)
set(PHANTASY_ENGINE_TEST_FILES ${TEST_FILES} PARENT_SCOPE)
