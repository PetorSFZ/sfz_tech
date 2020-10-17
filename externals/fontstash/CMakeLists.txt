cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(INCLUDE_FILES
	${INCLUDE_DIR}/fontstash.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

# Empty target in order to add headers to IDE
add_custom_target(fontstash SOURCES ${STB_FILES})

set(FONTSTASH_FOUND true PARENT_SCOPE)
set(FONTSTASH_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
