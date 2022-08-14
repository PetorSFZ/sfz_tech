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

# Lib-Core
# ------------------------------------------------------------------------------------------------

# Find all source files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.hpp ${SRC_DIR}/*.cpp ${SRC_DIR}/*.h ${SRC_DIR}/*.c)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

# Empty target in order to add headers to IDE
add_custom_target(sfz_core SOURCES ${SRC_DIR})

# Output variables
# ------------------------------------------------------------------------------------------------

get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
	set(SFZ_CORE_FOUND true PARENT_SCOPE)
	set(SFZ_CORE_INCLUDE_DIRS ${SRC_DIR} PARENT_SCOPE)
endif()

