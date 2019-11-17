cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if (NOT STB_FOUND)
	message(FATAL_ERROR "tinygltf requires Dependency-stb.")
endif()

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(INCLUDE_FILES
	${INCLUDE_DIR}/json.hpp
	${INCLUDE_DIR}/tiny_gltf.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

add_custom_target(tinygltf SOURCES ${INCLUDE_FILES})

set(TINYGLTF_FOUND true PARENT_SCOPE)
set(TINYGLTF_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
