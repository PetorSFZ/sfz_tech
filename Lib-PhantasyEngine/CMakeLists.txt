# Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
#               For other contributors see Contributors.txt
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project("PhantasyEngine" LANGUAGES CXX)

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Check if PhantasyEngine is built individually or part of a project
get_directory_property(hasParent PARENT_DIRECTORY)

# Check platforms
# ------------------------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set(iOS true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(macOS true)
endif()

# Compiler flags
# ------------------------------------------------------------------------------------------------

# Set compiler flags only if building sfzCore individually
if(NOT hasParent)
	if(MSVC)
		# MSVC flags
		# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
		# /std:c++17 = Enables C++17 support (and disables newer stuff)
		# /permissive- = This option disables permissive behaviors, and sets the /Zc compiler options for strict conformance
		# /Zc:twoPhase- = Disables two-phase name lookup. Needed because it is not compatible with OpenMP
		# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
		# /Zf = Faster pdb generation
		# /EHsc = TODO: Add explanation
		# /GR- = Disable RTTI
		# /MD(d) = Uses multi-threaded dynamic run-time library
		# /arch:AVX = Enable (require) Intel AVX instructions for code generation
		# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
		# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
		# /Od = "disables optimization, speeding compilation and simplifying debugging"
		# /O2 = Optimize code for fastest speed
		# /Ob3 = "which is a more aggressive version of -Ob2"
		# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
		# /DNDEBUG = defines the "NDEBUG" macro, which disables asserts
		set(CMAKE_CXX_FLAGS "/W4 /std:c++17 /permissive- /Zc:twoPhase- /Zi /Zf /EHsc /GR- /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
		set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Od")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /O2 /Ob3 /fp:fast")
		set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob3 /fp:fast /DNDEBUG")

	elseif(macOS)
		# macOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -fno-strict-aliasing")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	elseif(iOS)
		# iOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -DSFZ_IOS = Defines the "SFZ_IOS" macro
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -fno-strict-aliasing -DSFZ_IOS")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	elseif(EMSCRIPTEN)
		# Emscripten flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -s TOTAL_MEMORY=134217728 = 128MiB heap, TODO: should probably not hard-code this
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -fno-strict-aliasing -s TOTAL_MEMORY=134217728")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	else()
		# GCC / Clang flags
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17 -march=sandybridge -pthread -fno-strict-aliasing")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

	endif()
endif()

# Dependencies
# ------------------------------------------------------------------------------------------------

# Externals directory
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../externals)

# Include dependencies if building individually
if (NOT hasParent)

	# utest.h
	# ${UTESTH_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/utest.h ${CMAKE_BINARY_DIR}/utest.h)

	# SDL2
	# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES}, ${SDL2_RUNTIME_FILES}
	add_subdirectory(${EXTERNALS_DIR}/sdl2 ${CMAKE_BINARY_DIR}/sdl2)

	# stb headers
	# ${STB_FOUND}, ${STB_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/stb ${CMAKE_BINARY_DIR}/stb)

	# cgltf
	# ${CGLTF_FOUND}, ${CGLTF_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/cgltf ${CMAKE_BINARY_DIR}/cgltf)

	# nativefiledialog
	# ${NATIVEFILEDIALOG_FOUND}, ${NATIVEFILEDIALOG_INCLUDE_DIRS}, ${NATIVEFILEDIALOG_LIBRARIES}
	add_subdirectory(${EXTERNALS_DIR}/nativefiledialog ${CMAKE_BINARY_DIR}/nativefiledialog)

	# sajson
	# ${SAJSON_FOUND}, ${SAJSON_INCLUDE_DIRS}
	add_subdirectory(${EXTERNALS_DIR}/sajson ${CMAKE_BINARY_DIR}/sajson)

	# SoLoud
	# ${SOLOUD_FOUND}, ${SOLOUD_INCLUDE_DIRS}, ${SOLOUD_LIBRARIES}
	add_subdirectory(${EXTERNALS_DIR}/soloud ${CMAKE_BINARY_DIR}/soloud)

	# skipifzero Lib-Core
	# ${SFZ_CORE_FOUND}, ${SFZ_CORE_INCLUDE_DIRS}
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Lib-Core ${CMAKE_BINARY_DIR}/Lib-Core)

	# ZeroG
	set(ZEROG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../Lib-ZeroG)
	include(${ZEROG_PATH}/ZeroG.cmake)
	addZeroG()

	# dear-imgui
	# ${IMGUI_FOUND}, ${IMGUI_INCLUDE_DIRS}, ${IMGUI_LIBRARIES}
	add_subdirectory(${EXTERNALS_DIR}/dear-imgui ${CMAKE_BINARY_DIR}/dear-imgui)

	# imgui_plot
	# ${IMGUI_PLOT_FOUND}, ${IMGUI_PLOT_INCLUDE_DIRS}, ${IMGUI_PLOT_LIBRARIES}
	add_subdirectory(${EXTERNALS_DIR}/imgui_plot ${CMAKE_BINARY_DIR}/imgui_plot)

	# ZeroG-ImGui
	# ${ZEROG_IMGUI_FOUND}, ${ZEROG_IMGUI_INCLUDE_DIRS}, ${ZEROG_IMGUI_LIBRARIES}
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Lib-ZeroG-ImGui ${CMAKE_BINARY_DIR}/ZeroG-ImGui)

# Otherwise check if dependencies are already included
else()

	if (NOT SDL2_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires SDL2.")
	endif()

	if (NOT STB_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires stb libraries.")
	endif()

	if (NOT CGLTF_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires cgltf.")
	endif()

	if (NOT NATIVEFILEDIALOG_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires nativefiledialog.")
	endif()

	if (NOT SAJSON_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires sajson.")
	endif()

	if (NOT SOLOUD_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires SoLoud.")
	endif()

	if (NOT SFZ_CORE_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires sfzCore.")
	endif()

	if (NOT ZEROG_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires ZeroG.")
	endif()

	if (NOT IMGUI_PLOT_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires imgui_plot.")
	endif()

	if (NOT ZEROG_IMGUI_FOUND)
		message(FATAL_ERROR "PhantasyEngine requires ZeroG-ImGui")
	endif()

endif()

# PhantasyEngine
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res_ph)

set(HEADER_FILES
	${INCLUDE_DIR}/sfz/audio/AudioEngine.hpp

	${INCLUDE_DIR}/sfz/config/GlobalConfig.hpp
	${INCLUDE_DIR}/sfz/config/Setting.hpp

	${INCLUDE_DIR}/sfz/debug/Console.hpp
	${INCLUDE_DIR}/sfz/debug/ProfilingStats.hpp

	${INCLUDE_DIR}/sfz/geometry/AABB.hpp
	${INCLUDE_DIR}/sfz/geometry/AABB.inl
	${INCLUDE_DIR}/sfz/geometry/AABB2D.hpp
	${INCLUDE_DIR}/sfz/geometry/Circle.hpp
	${INCLUDE_DIR}/sfz/geometry/Intersection.hpp
	${INCLUDE_DIR}/sfz/geometry/Intersection.inl
	${INCLUDE_DIR}/sfz/geometry/OBB.hpp
	${INCLUDE_DIR}/sfz/geometry/OBB.inl
	${INCLUDE_DIR}/sfz/geometry/Plane.hpp
	${INCLUDE_DIR}/sfz/geometry/Plane.inl
	${INCLUDE_DIR}/sfz/geometry/Sphere.hpp

	${INCLUDE_DIR}/sfz/math/MathPrimitiveToStrings.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.hpp
	${INCLUDE_DIR}/sfz/math/MathSupport.inl
	${INCLUDE_DIR}/sfz/math/Quaternion.hpp

	${INCLUDE_DIR}/sfz/memory/ArenaAllocator.hpp
	${INCLUDE_DIR}/sfz/memory/DebugAllocator.hpp

	${INCLUDE_DIR}/sfz/renderer/BuiltInShaderTypes.hpp
	${INCLUDE_DIR}/sfz/renderer/CascadedShadowMaps.hpp
	${INCLUDE_DIR}/sfz/renderer/GpuMesh.hpp
	${INCLUDE_DIR}/sfz/renderer/GpuTextures.hpp
	${INCLUDE_DIR}/sfz/renderer/Renderer.hpp
	${INCLUDE_DIR}/sfz/renderer/RendererConfigParser.hpp
	${INCLUDE_DIR}/sfz/renderer/RendererState.hpp
	${INCLUDE_DIR}/sfz/renderer/RendererUI.hpp
	${INCLUDE_DIR}/sfz/renderer/ZeroGUtils.hpp

	${INCLUDE_DIR}/sfz/rendering/FullscreenTriangle.hpp
	${INCLUDE_DIR}/sfz/rendering/HSV.hpp
	${INCLUDE_DIR}/sfz/rendering/ImageView.hpp
	${INCLUDE_DIR}/sfz/rendering/Image.hpp
	${INCLUDE_DIR}/sfz/rendering/ImguiSupport.hpp
	${INCLUDE_DIR}/sfz/rendering/Mesh.hpp
	${INCLUDE_DIR}/sfz/rendering/SphereLight.hpp

	${INCLUDE_DIR}/sfz/sdl/ButtonState.hpp
	${INCLUDE_DIR}/sfz/sdl/GameController.hpp
	${INCLUDE_DIR}/sfz/sdl/Mouse.hpp
	${INCLUDE_DIR}/sfz/sdl/SDLAllocator.hpp

	${INCLUDE_DIR}/sfz/state/ArrayHeader.hpp
	${INCLUDE_DIR}/sfz/state/ComponentMask.hpp
	${INCLUDE_DIR}/sfz/state/Entity.hpp
	${INCLUDE_DIR}/sfz/state/GameState.hpp
	${INCLUDE_DIR}/sfz/state/GameStateContainer.hpp
	${INCLUDE_DIR}/sfz/state/GameStateEditor.hpp

	${INCLUDE_DIR}/sfz/strings/DynString.hpp

	${INCLUDE_DIR}/sfz/util/Enumerate.hpp
	${INCLUDE_DIR}/sfz/util/FixedTimeUpdateHelpers.hpp
	${INCLUDE_DIR}/sfz/util/GltfLoader.hpp
	${INCLUDE_DIR}/sfz/util/GltfWriter.hpp
	${INCLUDE_DIR}/sfz/util/IniParser.hpp
	${INCLUDE_DIR}/sfz/util/IO.hpp
	${INCLUDE_DIR}/sfz/util/JsonParser.hpp
	${INCLUDE_DIR}/sfz/util/LoggingInterface.hpp
	${INCLUDE_DIR}/sfz/util/RandomColors.hpp
	${INCLUDE_DIR}/sfz/util/StandardLogger.hpp
	${INCLUDE_DIR}/sfz/util/TerminalLogger.hpp

	${INCLUDE_DIR}/sfz/Context.hpp
	${INCLUDE_DIR}/sfz/CudaCompatibility.hpp
	${INCLUDE_DIR}/sfz/Logging.hpp
	${INCLUDE_DIR}/sfz/PhantasyEngineMain.hpp
	${INCLUDE_DIR}/sfz/PopWarnings.hpp
	${INCLUDE_DIR}/sfz/PushWarnings.hpp
	${INCLUDE_DIR}/sfz/SimdIntrinsics.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${HEADER_FILES})

set(SRC_FILES
	${SRC_DIR}/sfz/audio/AudioEngine.cpp

	${SRC_DIR}/sfz/config/GlobalConfig.cpp
	${SRC_DIR}/sfz/config/Setting.cpp

	${SRC_DIR}/sfz/debug/Console.cpp
	${SRC_DIR}/sfz/debug/ProfilingStats.cpp

	${SRC_DIR}/sfz/math/MathPrimitiveToStrings.cpp

	${SRC_DIR}/sfz/memory/ArenaAllocator.cpp
	${SRC_DIR}/sfz/memory/DebugAllocator.cpp

	${SRC_DIR}/sfz/renderer/CascadedShadowMaps.cpp
	${SRC_DIR}/sfz/renderer/GpuMesh.cpp
	${SRC_DIR}/sfz/renderer/GpuTextures.cpp
	${SRC_DIR}/sfz/renderer/Renderer.cpp
	${SRC_DIR}/sfz/renderer/RendererConfigParser.cpp
	${SRC_DIR}/sfz/renderer/RendererState.cpp
	${SRC_DIR}/sfz/renderer/RendererUI.cpp
	${SRC_DIR}/sfz/renderer/ZeroGUtils.cpp

	${SRC_DIR}/sfz/rendering/FullscreenTriangle.cpp
	${SRC_DIR}/sfz/rendering/Image.cpp
	${SRC_DIR}/sfz/rendering/ImguiSupport.cpp

	${SRC_DIR}/sfz/sdl/GameController.cpp
	${SRC_DIR}/sfz/sdl/Mouse.cpp
	${SRC_DIR}/sfz/sdl/SDLAllocator.cpp

	${SRC_DIR}/sfz/state/ArrayHeader.cpp
	${SRC_DIR}/sfz/state/GameState.cpp
	${SRC_DIR}/sfz/state/GameStateContainer.cpp
	${SRC_DIR}/sfz/state/GameStateEditor.cpp

	${SRC_DIR}/sfz/strings/DynString.cpp

	${SRC_DIR}/sfz/util/GltfLoader.cpp
	${SRC_DIR}/sfz/util/GltfWriter.cpp
	${SRC_DIR}/sfz/util/IniParser.cpp
	${SRC_DIR}/sfz/util/IO.cpp
	${SRC_DIR}/sfz/util/JsonParser.cpp
	${SRC_DIR}/sfz/util/RandomColors.cpp
	${SRC_DIR}/sfz/util/StandardLogger.cpp
	${SRC_DIR}/sfz/util/TerminalLogger.cpp

	${SRC_DIR}/sfz/Context.cpp
	${SRC_DIR}/sfz/PhantasyEngineMain.cpp
)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_library(PhantasyEngine ${HEADER_FILES} ${SRC_FILES})

target_include_directories(PhantasyEngine PUBLIC
	${INCLUDE_DIR}
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
	${ZEROG_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${CGLTF_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
	${IMGUI_PLOT_INCLUDE_DIRS}
	${ZEROG_IMGUI_INCLUDE_DIRS}
	${NATIVEFILEDIALOG_INCLUDE_DIRS}
	${SAJSON_INCLUDE_DIRS}
	${SOLOUD_INCLUDE_DIRS}
)

target_link_libraries(PhantasyEngine
	${SDL2_LIBRARIES}
	${ZEROG_D3D12_LIBRARIES}
	${IMGUI_LIBRARIES}
	${IMGUI_PLOT_LIBRARIES}
	${ZEROG_IMGUI_LIBRARIES}
	${NATIVEFILEDIALOG_LIBRARIES}
	${SOLOUD_LIBRARIES}
)

# PhantasyEngine tests
# ------------------------------------------------------------------------------------------------

if(NOT hasParent OR PHANTASY_ENGINE_BUILD_TESTS)

	set(TEST_FILES
		${TESTS_DIR}/sfz/Main_Tests.cpp

		${TESTS_DIR}/sfz/geometry/Intersection_Tests.cpp
		${TESTS_DIR}/sfz/geometry/OBB_Tests.cpp

		${TESTS_DIR}/sfz/math/MathSupport_Tests.cpp
		${TESTS_DIR}/sfz/math/Matrix_Tests.cpp
		${TESTS_DIR}/sfz/math/Quaternion_Tests.cpp

		${TESTS_DIR}/sfz/rendering/HSV_Tests.cpp

		${TESTS_DIR}/sfz/memory/ArenaAllocator_Tests.cpp

		${TESTS_DIR}/sfz/strings/DynString_Tests.cpp

		${TESTS_DIR}/sfz/util/Enumerate_Tests.cpp
		${TESTS_DIR}/sfz/util/IniParser_Tests.cpp
		${TESTS_DIR}/sfz/util/IO_Tests.cpp
	)
	source_group(TREE ${TESTS_DIR} FILES ${TEST_FILES})

	add_executable(PhantasyEngineTests ${TEST_FILES})
	target_include_directories(PhantasyEngineTests PRIVATE
		${INCLUDE_DIR}
		${SFZ_CORE_HEADERS_INCLUDE_DIRS}
		${UTESTH_INCLUDE_DIRS}
	)
	target_link_libraries(PhantasyEngineTests PhantasyEngine)

endif()

# Output variables
# ------------------------------------------------------------------------------------------------

if (hasParent)
	set(PHANTASY_ENGINE_FOUND true PARENT_SCOPE)
	set(PHANTASY_ENGINE_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
	set(PHANTASY_ENGINE_LIBRARIES PhantasyEngine PARENT_SCOPE)
	set(PHANTASY_ENGINE_RUNTIME_DIR ${RESOURCES_DIR} PARENT_SCOPE)
endif()
