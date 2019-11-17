project("nativefiledialog" LANGUAGES CXX C )
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(INCLUDE_FILES
	${INCLUDE_DIR}/nfd.h
)
source_group(TREE ${INCLUDE_DIR} FILES ${INCLUDE_FILES})

set(SRC_FILES
	${SRC_DIR}/common.h
	${SRC_DIR}/nfd_common.h
	${SRC_DIR}/nfd_common.c
)

# Windows
if(MSVC)
	set(SRC_FILES ${SRC_FILES} ${SRC_DIR}/nfd_win.cpp)
	add_library(nativefiledialog
		${INCLUDE_FILES}
		${SRC_FILES}
	)
	target_include_directories(nativefiledialog PUBLIC ${INCLUDE_DIR})

# iOS
elseif(IOS)
	message("[nativefiledialog]: Not available for iOS, adding dummy targets")
	set(nativefiledialog "")

# macOS
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
	set(SRC_FILES ${SRC_FILES} ${SRC_DIR}/nfd_cocoa.m)
	add_library(nativefiledialog
		${INCLUDE_FILES}
		${SRC_FILES}
	)
	target_include_directories(nativefiledialog PUBLIC ${INCLUDE_DIR})

# Emscripten
elseif(EMSCRIPTEN)
	message("[nativefiledialog]: Not available for emscripten, adding dummy targets")
	set(nativefiledialog "")

# Everything else
else()
	message(FATAL_ERROR "[nativefiledialog]: Not yet setup for this platform, exiting")
	# set(SRC_FILES ${SRC_FILES} ${SRC_DIR}/nfd_gtk.c)
endif()

# Add src files to IDE
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

set(NATIVEFILEDIALOG_FOUND true PARENT_SCOPE)
set(NATIVEFILEDIALOG_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(NATIVEFILEDIALOG_LIBRARIES nativefiledialog PARENT_SCOPE)
