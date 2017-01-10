add_definitions(
	-DGLEW_STATIC
)

find_package(OpenGL REQUIRED) # ${OPENGL_INCLUDE_DIR}, ${OPENGL_LIBRARIES}

# Disable warnings when building GLEW
if(MSVC)
	set(CMAKE_CXX_FLAGS "/W1")
else()
	set(CMAKE_CXX_FLAGS "-w")
endif()

# Compile glew
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(GLEW
	STATIC 
	src/glew.c)

target_link_libraries(GLEW
	${OPENGL_LIBRARIES}
)

# Variables for linking
set(GLEW_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
set(GLEW_LIBRARIES GLEW PARENT_SCOPE)