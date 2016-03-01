cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project("sfzCore")

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Compiler flags
if(MSVC)
	# MSVC flags
	set(CMAKE_CXX_FLAGS "/Wall /Zi /EHsc /wd4201 /wd4350 /wd4514 /wd4571 /wd4668 /wd4710 /wd4711 /wd4820")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG  "/Od /DEBUG")
else()
	# GCC / Clang flags
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-c++11-extensions -std=c++11 -pthread")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
endif()

# Directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)
set(CMAKE_MODULES ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Adds our cmake modules to module path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULES})

# Catch
add_subdirectory(${EXTERNALS_DIR}/catch)

# Include directories
include_directories(
	${INCLUDE_DIR}
	${CATCH_INCLUDE_DIRS}
)

set(SOURCE_UTIL_FILES
	${INCLUDE_DIR}/sfz/Util.hpp
	${INCLUDE_DIR}/sfz/util/Allocators.hpp
	 ${SOURCE_DIR}/sfz/util/Allocators.cpp)
source_group(sfz_util FILES ${SOURCE_UTIL_FILES})

set(SOURCE_ALL_FILES
	${SOURCE_UTIL_FILES})

# Library compile target
add_library(sfzCoreLib ${SOURCE_ALL_FILES})

# Linking libraries
# target_link_libraries(
#	sfzCoreLib
#)

# Variables for linking library
set(SFZ_CORE_INCLUDE_DIRS
	${INCLUDE_DIR}
#	${SDL2_INCLUDE_DIR}
#	${TINYOBJLOADER_INCLUDE_DIRS}
#	${GLEW_INCLUDE_DIRS}
#	${OPENGL_INCLUDE_DIR}
	PARENT_SCOPE)
set(SFZ_CORE_LIBRARIES
	sfzCoreLib
#	${SDL2_LIBRARY}
#	${TINYOBJLOADER_LIBRARIES}
#	${GLEW_LIBRARIES}
#	${OPENGL_LIBRARIES}
	PARENT_SCOPE)

# Tests
if(SFZ_CORE_BUILD_TESTS)

	set(ROOT_TEST_FILES
		${TESTS_DIR}/sfz/Main_Tests.cpp)
	source_group(sfz_root FILES ${ROOT_TEST_FILES})

	set(UTIL_TEST_FILES
		${TESTS_DIR}/sfz/util/Allocators_Tests.cpp)
	source_group(sfz_util FILES ${UTIL_TEST_FILES})

	set(ALL_TEST_FILES
		${ROOT_TEST_FILES}
		${UTIL_TEST_FILES})

	enable_testing(true)
	add_executable(sfzCoreTests ${ALL_TEST_FILES})
	target_link_libraries(
		sfzCoreTests

		sfzCoreLib
	)
	add_test(sfzCoreTestsName sfzCoreTests)
	
endif()