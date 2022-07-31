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

if(SFZ_CORE_BUILD_TESTS)
	if (NOT UTESTH_FOUND)
		message(FATAL_ERROR "sfz_core requires utest.h for tests.")
	endif()
endif()

# Lib-Core
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Find all source files
file(GLOB_RECURSE INCLUDE_FILES CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.cpp ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.c)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

# Empty target in order to add headers to IDE
add_custom_target(sfz_core SOURCES ${INCLUDE_FILES})

# sfz_core lib-core tests
# ------------------------------------------------------------------------------------------------

if(SFZ_CORE_BUILD_TESTS)

	file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS ${TESTS_DIR}/*.hpp ${TESTS_DIR}/*.cpp ${TESTS_DIR}/*.h ${TESTS_DIR}/*.c)
	source_group(TREE ${TESTS_DIR} FILES ${TEST_FILES})

	add_executable(sfz_core_tests ${TEST_FILES})
	target_include_directories(sfz_core_tests PRIVATE
		${INCLUDE_DIR}
		${UTESTH_INCLUDE_DIRS}
	)

endif()

# Output variables
# ------------------------------------------------------------------------------------------------

set(SFZ_CORE_FOUND true PARENT_SCOPE)
set(SFZ_CORE_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
