set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SRC_FILES
	${INCLUDE_DIR}/mtlpp.hpp
	${SRC_DIR}/mtlpp.mm
)

add_library(mtlpp ${SRC_FILES})

target_include_directories(mtlpp PUBLIC
	${INCLUDE_DIR}
)

set(MTLPP_FOUND true PARENT_SCOPE)
set(MTLPP_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(MTLPP_LIBRARIES mtlpp PARENT_SCOPE)
