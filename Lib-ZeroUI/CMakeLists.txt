if (NOT STB_FOUND)
	message(FATAL_ERROR "ZeroUI requires stb libraries.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "ZeroUI requires Lib-Core.")
endif()

if (NOT FONTSTASH_FOUND)
	message(FATAL_ERROR "ZeroUI requires fontstash.")
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SRC_FILES
	${SRC_DIR}/ZeroUI.hpp
	${SRC_DIR}/ZeroUI.cpp
	${SRC_DIR}/ZeroUI_CoreWidgets.hpp
	${SRC_DIR}/ZeroUI_CoreWidgets.cpp
	${SRC_DIR}/ZeroUI_Drawing.hpp
	${SRC_DIR}/ZeroUI_Drawing.cpp
	${SRC_DIR}/ZeroUI_Internal.hpp
	${SRC_DIR}/ZeroUI_Storage.hpp
)

add_library(ZeroUI ${SRC_FILES})

target_include_directories(ZeroUI PRIVATE
	${SFZ_CORE_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
	${FONTSTASH_INCLUDE_DIRS}
)
target_link_libraries(ZeroUI ${FONTSTASH_LIBRARIES})

set(ZEROUI_FOUND true PARENT_SCOPE)
set(ZEROUI_INCLUDE_DIRS ${SRC_DIR} PARENT_SCOPE)
set(ZEROUI_LIBRARIES ZeroUI PARENT_SCOPE)
