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

# Usage
# ------------------------------------------------------------------------------------------------

# This is an optional CMake file with helper functions to make it easier to bundle ZeroG directly
# in your project. Essentially, you are meant to copy the entire ZeroG repo (with this file in the
# root) somewhere and then include this file.
#
# I.e., if you have placed the ZeroG repo in a directory called "externals", you could include
# this file with the following command:
#
#     include(${CMAKE_CURRENT_SOURCE_DIR}/externals/ZeroG/ZeroG.cmake)
#
# Then you can add ZeroG with:
#
#     addZeroG()
#
# And link ZeroG to a CMake target with:
#
#     linkZeroG(<target>)

# Misc initialization
# ------------------------------------------------------------------------------------------------

# Set the root of the ZeroG repo
set(ZEROG_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR})

# ZeroG
# ------------------------------------------------------------------------------------------------

# Adds the main ZeroG targets
function(addZeroG)
	message("-- [ZeroG]: Adding ZeroG targets")

	add_subdirectory(
		${ZEROG_REPO_ROOT}/Lib-ZeroG
		${CMAKE_BINARY_DIR}/Lib-ZeroG
	)

	set(ZEROG_FOUND ${ZEROG_FOUND} PARENT_SCOPE)
	set(ZEROG_INCLUDE_DIRS ${ZEROG_INCLUDE_DIRS} PARENT_SCOPE)
	set(ZEROG_LIBRARIES ${ZEROG_LIBRARIES} PARENT_SCOPE)
	set(ZEROG_RUNTIME_FILES ${ZEROG_RUNTIME_FILES} PARENT_SCOPE)
endfunction()

# Links ZeroG with the specified target
function(linkZeroG linkTarget)
	if (NOT ZEROG_FOUND)
		message(FATAL_ERROR "-- [ZeroG]: Attempting to linkZeroG(), but ZeroG is not found")
	endif()
	message("-- [ZeroG]: Linking ZeroG to target: ${linkTarget}")

	target_include_directories(${linkTarget} PUBLIC ${ZEROG_INCLUDE_DIRS})
	target_link_libraries(${linkTarget} ${ZEROG_LIBRARIES})
endfunction()

# Misc helper functions
# ------------------------------------------------------------------------------------------------

function(msvcCopyZeroGRuntimeFiles)
	if(MSVC)
		if (NOT ZEROG_FOUND)
			message(FATAL_ERROR "-- [ZeroG]: Attempting to msvcCopyZeroGRuntimeFiles(), but ZeroG is not found")
		endif()

		message("-- [ZeroG]: Copying following DLLs to binary directory:")
		foreach(dllPath ${ZEROG_RUNTIME_FILES})
			message("  -- ${dllPath}")
		endforeach()

		foreach(dllPath ${ZEROG_RUNTIME_FILES})

			# Do different things depending on if we are generating .sln or using built-in VS CMake
			if(CMAKE_CONFIGURATION_TYPES)
				file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
				file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
				file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/Release)
			else()
				file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR})
			endif()

		endforeach()

	endif()
endfunction()
