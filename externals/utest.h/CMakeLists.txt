set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(UTEST_FILES
	${INCLUDE_DIR}/utest.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${UTEST_FILES})

# Empty target in order to add headers to IDE
add_custom_target(utest.h SOURCES ${UTEST_FILES})

set(UTESTH_FOUND true PARENT_SCOPE)
set(UTESTH_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
