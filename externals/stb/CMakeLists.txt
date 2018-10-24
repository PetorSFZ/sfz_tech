# stb libraries are released as public domain: https://github.com/nothings/stb

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(STB_FILES
	${INCLUDE_DIR}/stb_dxt.h
	${INCLUDE_DIR}/stb_easy_font.h
	${INCLUDE_DIR}/stb_image.h
	${INCLUDE_DIR}/stb_image_write.h
	${INCLUDE_DIR}/stb_perlin.h
	${INCLUDE_DIR}/stb_rect_pack.h
	${INCLUDE_DIR}/stb_sprintf.h
	${INCLUDE_DIR}/stb_textedit.h
	${INCLUDE_DIR}/stb_truetype.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${STB_FILES})

# Empty target in order to add headers to IDE
add_custom_target(stb_libraries SOURCES ${STB_FILES})

set(STB_FOUND true PARENT_SCOPE)
set(STB_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
