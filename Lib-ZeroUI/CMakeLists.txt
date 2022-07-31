if (NOT STB_FOUND)
	message(FATAL_ERROR "ZeroUI requires stb libraries.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "ZeroUI requires Lib-Core.")
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.hpp ${SRC_DIR}/*.cpp ${SRC_DIR}/*.h ${SRC_DIR}/*.c)

add_library(ZeroUI ${SRC_FILES})

target_include_directories(ZeroUI PRIVATE
	${SFZ_CORE_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
)

set(ZEROUI_FOUND true PARENT_SCOPE)
set(ZEROUI_INCLUDE_DIRS ${SRC_DIR} PARENT_SCOPE)
set(ZEROUI_LIBRARIES ZeroUI PARENT_SCOPE)
