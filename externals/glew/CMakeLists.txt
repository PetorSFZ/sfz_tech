# GLEW CMake wrapper by Peter Hillerström (https://github.com/PetorSFZ)
# 
# Sets GLEW_INCLUDE_DIRS, GLEW_LIBRARIES and GLEW_DLLS. On 64bit Windows GLEW_LIBRARIES
# will be empty, but GLEW_DLLS will contain the dll needed. On other configurations GLEW
# will be statically built.
# 
# This file is public domain and may be used however you want, but it would be nice

# Check if 32 or 64bit compiler
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITS_64 TRUE)
	set(BITS_32 FALSE)
else()
	set(BITS_64 FALSE)
	set(BITS_32 TRUE)
endif()

set(GLEW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glew-2.0.0/)

if(WIN32 AND BITS_64)

	find_library(GLEW_LIB
	             NAMES
	             glew32
			     HINTS
			     ${GLEW_DIR}/win64)
	set(GLEW_LIBRARIES_TMP ${GLEW_LIB})
	set(GLEW_DLLS_TMP ${GLEW_DIR}/win64/glew32.dll
	                  ${GLEW_DIR}/GLEW-LICENSE.txt)

else()
	add_definitions(-DGLEW_STATIC)

	find_package(OpenGL REQUIRED) # ${OPENGL_INCLUDE_DIR}, ${OPENGL_LIBRARIES}

	# Disable warnings when building GLEW
	if(MSVC)
		set(CMAKE_CXX_FLAGS "/W1")
	else()
		set(CMAKE_CXX_FLAGS "-w")
	endif()

	# Compile glew
	include_directories(${GLEW_DIR}/include)
	add_library(GLEW
		STATIC 
		${GLEW_DIR}/src/glew.c)

	target_link_libraries(GLEW
		${OPENGL_LIBRARIES}
	)

	set(GLEW_LIBRARIES_TMP GLEW)
	set(GLEW_DLLS_TMP ${GLEW_DIR}/GLEW-LICENSE.txt)
endif()

# Variables for linking
set(GLEW_INCLUDE_DIRS ${GLEW_DIR}/include PARENT_SCOPE)
set(GLEW_LIBRARIES ${GLEW_LIBRARIES_TMP} PARENT_SCOPE)
set(GLEW_DLLS ${GLEW_DLLS_TMP} PARENT_SCOPE)
