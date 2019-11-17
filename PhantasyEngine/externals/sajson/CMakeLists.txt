# sajson available at: https://github.com/chadaustin/sajson

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SAJSON_FILES
	${INCLUDE_DIR}/sajson.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${SAJSON_FILES})

# Empty target in order to add headers to IDE
add_custom_target(sajson SOURCES ${SAJSON_FILES})

set(SAJSON_FOUND true PARENT_SCOPE)
set(SAJSON_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
