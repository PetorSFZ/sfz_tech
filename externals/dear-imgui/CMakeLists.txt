# dear imgui is licensed under MIT, see: https://github.com/ocornut/imgui/

cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if (NOT STB_FOUND)
	message(FATAL_ERROR "Dependency-imgui requires Dependency-stb.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "Dependency-imgui requires sfzCore.")
endif()

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(INCLUDE_FILES
	${INCLUDE_DIR}/imconfig.h
	${INCLUDE_DIR}/imgui.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

set(SRC_FILES
	${SRC_DIR}/imgui_demo.cpp
	${SRC_DIR}/imgui_draw.cpp
	${SRC_DIR}/imgui_internal.h
	${SRC_DIR}/imgui_widgets.cpp
	${SRC_DIR}/imgui.cpp
	${SRC_DIR}/imstb_rectpack.h
	${SRC_DIR}/imstb_textedit.h
	${SRC_DIR}/imstb_truetype.h
)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_library(dear-imgui
	${INCLUDE_FILES}
	${SRC_FILES}
)

target_include_directories(
	dear-imgui
	PUBLIC
	${INCLUDE_DIR}
	${SRC_DIR}
	${STB_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
)

target_link_libraries(dear-imgui
	${SFZ_CORE_LIBRARIES}
)

set(IMGUI_FOUND true PARENT_SCOPE)
set(IMGUI_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(IMGUI_LIBRARIES dear-imgui PARENT_SCOPE)
