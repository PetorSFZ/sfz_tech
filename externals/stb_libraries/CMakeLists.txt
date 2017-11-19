set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(STB_FILES
	${INCLUDE_DIR}/stb_image.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${STB_FILES})

# Empty target in order to add headers to IDE
add_custom_target(stb_libraries SOURCES ${STB_FILES})

set(STB_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
