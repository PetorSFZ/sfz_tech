if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DSPIRV_CROSS_C_API_GLSL /DSPIRV_CROSS_C_API_HLSL /wd4245 /wd4702")
endif()

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SRC_FILES
	${INCLUDE_DIR}/spirv_cross_c.h
	${INCLUDE_DIR}/spirv.h

	${SRC_DIR}/GLSL.std.450.h
	${SRC_DIR}/spirv.hpp
	${SRC_DIR}/spirv_cfg.hpp
	${SRC_DIR}/spirv_cfg.cpp
	${SRC_DIR}/spirv_common.hpp
	${SRC_DIR}/spirv_cpp.hpp
	${SRC_DIR}/spirv_cpp.cpp
	${SRC_DIR}/spirv_cross.hpp
	${SRC_DIR}/spirv_cross.cpp
	${SRC_DIR}/spirv_cross_c.cpp
	${SRC_DIR}/spirv_cross_containers.hpp
	${SRC_DIR}/spirv_cross_error_handling.hpp
	${SRC_DIR}/spirv_cross_parsed_ir.hpp
	${SRC_DIR}/spirv_cross_parsed_ir.cpp
	${SRC_DIR}/spirv_cross_util.hpp
	${SRC_DIR}/spirv_cross_util.cpp
	${SRC_DIR}/spirv_glsl.hpp
	${SRC_DIR}/spirv_glsl.cpp
	${SRC_DIR}/spirv_hlsl.hpp
	${SRC_DIR}/spirv_hlsl.cpp
	${SRC_DIR}/spirv_msl.hpp
	${SRC_DIR}/spirv_msl.cpp
	${SRC_DIR}/spirv_parser.hpp
	${SRC_DIR}/spirv_parser.cpp
	${SRC_DIR}/spirv_reflect.hpp
	${SRC_DIR}/spirv_reflect.cpp
)

add_library(SPIRV-Cross ${SRC_FILES})

target_include_directories(SPIRV-Cross PUBLIC
	${INCLUDE_DIR}
	${SRC_DIR}
)

set(SPIRV_CROSS_FOUND true PARENT_SCOPE)
set(SPIRV_CROSS_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(SPIRV_CROSS_LIBRARIES SPIRV-Cross PARENT_SCOPE)
