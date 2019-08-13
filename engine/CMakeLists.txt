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
project("PhantasyEngine")

# Dependencies
# ------------------------------------------------------------------------------------------------

if (NOT SDL2_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires SDL2. Add Dependency-SDL2.")
endif()

if (NOT STB_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires stb libraries. Add Dependency-stb.")
endif()

if (NOT IMGUI_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires dear imgui. Add Dependency-imgui.")
endif()

if (NOT NATIVEFILEDIALOG_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires nativefiledialog. Add nativefiledialog.")
endif()

if (NOT SAJSON_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires sajson. Add sajson.")
endif()

if (NOT TINYGLTF_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires dear tinygltf.")
endif()

if (NOT ZEROG_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires ZeroG.")
endif()

if (NOT ZEROG_CPP_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires ZeroG-cpp.")
endif()

if (NOT SFZ_CORE_FOUND)
	message(FATAL_ERROR "PhantasyEngine requires sfzCore.")
endif()

# PhantasyEngine
# ------------------------------------------------------------------------------------------------

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res_ph)

set(HEADER_FILES
	${INCLUDE_DIR}/ph/Context.hpp

	${INCLUDE_DIR}/ph/config/GlobalConfig.hpp
	${INCLUDE_DIR}/ph/config/Setting.hpp

	${INCLUDE_DIR}/ph/game_loop/DefaultGameUpdateable.hpp
	${INCLUDE_DIR}/ph/game_loop/GameLoop.hpp
	${INCLUDE_DIR}/ph/game_loop/GameLoopUpdateable.hpp

	${INCLUDE_DIR}/ph/renderer/BuiltInShaderTypes.hpp
	${INCLUDE_DIR}/ph/renderer/Renderer.hpp

	${INCLUDE_DIR}/ph/rendering/ImageView.hpp
	${INCLUDE_DIR}/ph/rendering/Image.hpp
	${INCLUDE_DIR}/ph/rendering/ImguiRenderingData.hpp
	${INCLUDE_DIR}/ph/rendering/ImguiSupport.hpp
	${INCLUDE_DIR}/ph/rendering/Mesh.hpp
	${INCLUDE_DIR}/ph/rendering/SphereLight.hpp
	${INCLUDE_DIR}/ph/rendering/Vertex.hpp

	${INCLUDE_DIR}/ph/sdl/ButtonState.hpp
	${INCLUDE_DIR}/ph/sdl/GameController.hpp
	${INCLUDE_DIR}/ph/sdl/Mouse.hpp
	${INCLUDE_DIR}/ph/sdl/SDLAllocator.hpp

	${INCLUDE_DIR}/ph/state/ArrayHeader.hpp
	${INCLUDE_DIR}/ph/state/ComponentMask.hpp
	${INCLUDE_DIR}/ph/state/Entity.hpp
	${INCLUDE_DIR}/ph/state/GameState.hpp
	${INCLUDE_DIR}/ph/state/GameStateContainer.hpp
	${INCLUDE_DIR}/ph/state/GameStateEditor.hpp

	${INCLUDE_DIR}/ph/util/GltfLoader.hpp
	${INCLUDE_DIR}/ph/util/GltfWriter.hpp
	${INCLUDE_DIR}/ph/util/JsonParser.hpp
	${INCLUDE_DIR}/ph/util/TerminalLogger.hpp

	${INCLUDE_DIR}/ph/PhantasyEngineMain.hpp
)
source_group(TREE ${INCLUDE_DIR} FILES ${HEADER_FILES})

set(SRC_FILES
	${SRC_DIR}/ph/Context.cpp

	${SRC_DIR}/ph/config/GlobalConfig.cpp
	${SRC_DIR}/ph/config/Setting.cpp

	${SRC_DIR}/ph/game_loop/DefaultGameUpdateable.cpp
	${SRC_DIR}/ph/game_loop/GameLoop.cpp

	${SRC_DIR}/ph/renderer/DynamicGpuAllocator.hpp
	${SRC_DIR}/ph/renderer/DynamicGpuAllocator.cpp
	${SRC_DIR}/ph/renderer/GpuMesh.hpp
	${SRC_DIR}/ph/renderer/GpuMesh.cpp
	${SRC_DIR}/ph/renderer/ImGuiRenderer.hpp
	${SRC_DIR}/ph/renderer/ImGuiRenderer.cpp
	${SRC_DIR}/ph/renderer/Renderer.cpp
	${SRC_DIR}/ph/renderer/RendererConfigParser.hpp
	${SRC_DIR}/ph/renderer/RendererConfigParser.cpp
	${SRC_DIR}/ph/renderer/RendererState.hpp
	${SRC_DIR}/ph/renderer/RendererState.cpp
	${SRC_DIR}/ph/renderer/RendererUI.hpp
	${SRC_DIR}/ph/renderer/RendererUI.cpp
	${SRC_DIR}/ph/renderer/ZeroGUtils.hpp
	${SRC_DIR}/ph/renderer/ZeroGUtils.cpp

	${SRC_DIR}/ph/rendering/Image.cpp
	${SRC_DIR}/ph/rendering/ImguiSupport.cpp

	${SRC_DIR}/ph/sdl/GameController.cpp
	${SRC_DIR}/ph/sdl/Mouse.cpp
	${SRC_DIR}/ph/sdl/SDLAllocator.cpp

	${SRC_DIR}/ph/state/ArrayHeader.cpp
	${SRC_DIR}/ph/state/GameState.cpp
	${SRC_DIR}/ph/state/GameStateContainer.cpp
	${SRC_DIR}/ph/state/GameStateEditor.cpp

	${SRC_DIR}/ph/util/GltfLoader.cpp
	${SRC_DIR}/ph/util/GltfWriter.cpp
	${SRC_DIR}/ph/util/JsonParser.cpp
	${SRC_DIR}/ph/util/TerminalLogger.cpp

	${SRC_DIR}/ph/PhantasyEngineMain.cpp
)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_library(PhantasyEngine ${HEADER_FILES} ${SRC_FILES})

target_include_directories(PhantasyEngine PUBLIC
	${INCLUDE_DIR}
	${SRC_DIR}
	${SDL2_INCLUDE_DIRS}
	${ZEROG_INCLUDE_DIRS}
	${ZEROG_CPP_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
	${NATIVEFILEDIALOG_INCLUDE_DIRS}
	${SAJSON_INCLUDE_DIRS}
	${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(PhantasyEngine
	${SDL2_LIBRARIES}
	${ZEROG_LIBRARIES}
	${ZEROG_CPP_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
	${IMGUI_LIBRARIES}
	${NATIVEFILEDIALOG_LIBRARIES}
)

# Output variables
# ------------------------------------------------------------------------------------------------

set(PHANTASY_ENGINE_FOUND true PARENT_SCOPE)
set(PHANTASY_ENGINE_INCLUDE_DIRS ${INCLUDE_DIR} PARENT_SCOPE)
set(PHANTASY_ENGINE_LIBRARIES PhantasyEngine PARENT_SCOPE)
set(PHANTASY_ENGINE_RUNTIME_DIR ${RESOURCES_DIR} PARENT_SCOPE)
