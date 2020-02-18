project("imgui_plot" LANGUAGES CXX C )
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if (MSVC)
	string(REPLACE "/W4" "" TMP1 ${CMAKE_CXX_FLAGS})
	set(CMAKE_CXX_FLAGS "${TMP1} /W0")
endif()

if (NOT IMGUI_FOUND)
	message(FATAL_ERROR "imgui_plot requires dear-imgui.")
endif()

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(INCLUDE_FILES
	${INCLUDE_DIR}/imgui_plot.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

set(SRC_FILES
	${SRC_DIR}/imgui_plot.cpp
)

add_library(imgui_plot ${INCLUDE_FILES} ${SRC_FILES})
target_include_directories(imgui_plot PUBLIC
	${INCLUDE_DIR}
	${IMGUI_INCLUDE_DIRS}
)
target_link_libraries(imgui_plot
	${IMGUI_LIBRARIES}
)

set(IMGUI_PLOT_FOUND true PARENT_SCOPE)
set(IMGUI_PLOT_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(IMGUI_PLOT_LIBRARIES imgui_plot PARENT_SCOPE)
