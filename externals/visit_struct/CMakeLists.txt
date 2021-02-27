cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(INCLUDE_FILES
	${INCLUDE_DIR}/visit_struct/visit_struct.hpp
	${INCLUDE_DIR}/visit_struct/visit_struct_boost_fusion.hpp
	${INCLUDE_DIR}/visit_struct/visit_struct_boost_hana.hpp
	${INCLUDE_DIR}/visit_struct/visit_struct_intrusive.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

set(VISIT_STRUCT_FOUND true PARENT_SCOPE)
set(VISIT_STRUCT_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
