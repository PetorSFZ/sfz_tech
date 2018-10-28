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

# Input options
# ------------------------------------------------------------------------------------------------

# PH_CUDA_SUPPORT: Will build with CUDA support if defined

# PH_SDL2_ROOT: Optional path to the root of the Dependency-SDL2 directory, if you don't want to
#               download from GitHub.

# PH_SFZ_CORE_ROOT: Optional path to the root of the sfzCore directory, if you don't want to
#                   download from GitHub.

# Miscallenous initialization operations
# ------------------------------------------------------------------------------------------------

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Add the FetchContent module
include(FetchContent)

# Set the root of PhantasyEngine
set(PH_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Make all projects compile to the same directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compiler flag functions
# ------------------------------------------------------------------------------------------------

# Sets the compiler flags for the given platform
function(phSetCompilerFlags)
	message("-- [PhantasyEngine]: Setting compiler flags")

	if(MSVC)
		# Visual Studio flags
		# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
		# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
		# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
		# /EHsc = TODO: Add explanation
		# /GR- = Disable RTTI
		# /arch:AVX = Enable (require) Intel AVX instructions for code generation
		# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
		# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
		set(PH_CMAKE_CXX_FLAGS "/W4 /wd4201 /Zi /EHsc /GR- /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
		# /Od = "disables optimization, speeding compilation and simplifying debugging"
		set(PH_CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")
		# /DEBUG = "creates debugging information for the .exe file or DLL"
		set(PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
		# /O2 = Optimize code for fastest speed
		# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
		# /DSFZ_NO_DEBUG = defines the "SFZ_NO_DEBUG" macro, which disables sfz_assert_debug()
		set(PH_CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")

		if(PH_CUDA_SUPPORT)
			# Define SFZ_CUDA
			set(PH_CMAKE_CXX_FLAGS "${PH_CMAKE_CXX_FLAGS} /DSFZ_CUDA")

			# Function for processing flags for Xcompiler support
			function(process_flags flags_in processed_flags_out)
				string(STRIP ${flags_in} TMP1)
				string(REPLACE "  " " " TMP2 ${TMP1})
				string(REPLACE "  " " " TMP3 ${TMP2})
				string(REPLACE " " "," TMP4 ${TMP3})
				set(${processed_flags_out} ${TMP4} PARENT_SCOPE)
			endfunction()

			# Process flags
			process_flags(${PH_CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS_PROCESSED)
			process_flags(${PH_CMAKE_CXX_FLAGS_DEBUG} CMAKE_CXX_FLAGS_DEBUG_PROCESSED)
			process_flags(${PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO} CMAKE_CXX_FLAGS_RELWITHDEBINFO_PROCESSED)
			process_flags(${PH_CMAKE_CXX_FLAGS_RELEASE} CMAKE_CXX_FLAGS_RELEASE_PROCESSED)

			# Set CUDA flags
			# sm_52 <==> Maxwell cards (high end), (GTX 970M, GTX 980 Ti, etc)
			# sm_61 <==> Pascal cards (first gen), (1070, 1080, Titan X, etc)
			set(PH_CMAKE_CUDA_FLAGS "-D_WINDOWS -expt-relaxed-constexpr -gencode arch=compute_52,code=sm_52 -Xcompiler=${CMAKE_CXX_FLAGS_PROCESSED}")
			set(PH_CMAKE_CUDA_FLAGS_DEBUG "-O0 -g -lineinfo -Xcompiler=${CMAKE_CXX_FLAGS_DEBUG_PROCESSED}")
			set(PH_CMAKE_CUDA_FLAGS_RELWITHDEBINFO "-O3 -use_fast_math -g -lineinfo -Xcompiler=${CMAKE_CXX_FLAGS_RELWITHDEBINFO_PROCESSED}")
			set(PH_CMAKE_CUDA_FLAGS_RELEASE "-O3 -use_fast_math -Xcompiler=${CMAKE_CXX_FLAGS_RELEASE_PROCESSED}")
		endif()

	elseif(EMSCRIPTEN)
		message("  -- Emscripten flags are a bit hardcoded and not super reliable, beware! TODO: Fix.")
		# Emscripten flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -fno-rtti = Disable RTTI
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -s USE_SDL=2 = Use SDL2 library
		# -s TOTAL_MEMORY=1073741824 = 1GiB heap, change this value if you need more
		# -DPH_STATIC_LINK_RENDERER = Link renderer statically instead of dynamically
		# --preload-file resources = Load "resources" directory into generated javascript
		# -DSFZ_NO_DEBUG = Used by sfzCore to disable assertions and such on release builds
		set(PH_CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-rtti -fno-strict-aliasing -s USE_SDL=2 -s TOTAL_MEMORY=1073741824 -s WASM=0 -s DEMANGLE_SUPPORT=1 -DPH_STATIC_LINK_RENDERER --preload-file res --preload-file res_compgl")
		set(PH_CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
		set(PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(PH_CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")

		if(PH_CUDA_SUPPORT)
			message(FATAL_ERROR "[PhantasyEngine]: CUDA not supported on this platform")
		endif()

	elseif(IOS)
		set(PH_CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-rtti -fno-strict-aliasing -DSFZ_IOS -DPH_STATIC_LINK_RENDERER")
		set(PH_CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
		set(PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(PH_CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")

		if(PH_CUDA_SUPPORT)
			message(FATAL_ERROR "[PhantasyEngine]: CUDA not supported on this platform")
		endif()

	elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
		# macOS flags
		# -Wall -Wextra = Enable most warnings
		# -std=c++14 = Enable C++14 support
		# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
		# -fno-rtti = Disable RTTI
		# -fno-strict-aliasing = Disable strict aliasing optimizations
		# -DSFZ_NO_DEBUG = Used by sfzCore to disable assertions and such on release builds
		set(PH_CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -fno-rtti -fno-strict-aliasing")
		set(PH_CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
		set(PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(PH_CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")

		if(PH_CUDA_SUPPORT)
			message(FATAL_ERROR "[PhantasyEngine]: CUDA not supported on this platform")
		endif()

	else()
		message(FATAL_ERROR "[PhantasyEngine]: Compiler flags not set for this platform, exiting.")
	endif()

	# Set flags to parent scope
	set(CMAKE_CXX_FLAGS ${PH_CMAKE_CXX_FLAGS} PARENT_SCOPE)
	set(CMAKE_CXX_FLAGS_DEBUG ${PH_CMAKE_CXX_FLAGS_DEBUG} PARENT_SCOPE)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO} PARENT_SCOPE)
	set(CMAKE_CXX_FLAGS_RELEASE ${PH_CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
	if (PH_CUDA_SUPPORT)
		set(CMAKE_CUDA_FLAGS ${PH_CMAKE_CUDA_FLAGS} PARENT_SCOPE)
		set(CMAKE_CUDA_FLAGS_DEBUG ${PH_CMAKE_CUDA_FLAGS_DEBUG} PARENT_SCOPE)
		set(CMAKE_CUDA_FLAGS_RELWITHDEBINFO ${PH_CMAKE_CUDA_FLAGS_RELWITHDEBINFO} PARENT_SCOPE)
		set(CMAKE_CUDA_FLAGS_RELEASE ${PH_CMAKE_CUDA_FLAGS_RELEASE} PARENT_SCOPE)
	endif()

endfunction()

# Prints the currently set compiler flags
function(phPrintCompilerFlags)
	message("-- [PhantasyEngine]: Printing compiler flags:")
	message("  -- CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
	message("  -- CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
	message("  -- CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
	message("  -- CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})
	if (PH_CUDA_SUPPORT)
		message("-- [PhantasyEngine]: CUDA flags:")
		message("  -- CMAKE_CUDA_FLAGS: " ${CMAKE_CUDA_FLAGS})
		message("  -- CMAKE_CUDA_FLAGS_DEBUG: " ${CMAKE_CUDA_FLAGS_DEBUG})
		message("  -- CMAKE_CUDA_FLAGS_RELWITHDEBINFO: " ${CMAKE_CUDA_FLAGS_RELWITHDEBINFO})
		message("  -- CMAKE_CUDA_FLAGS_RELEASE: " ${CMAKE_CUDA_FLAGS_RELEASE})
	endif()
endfunction()

# 3rd-party dependencies
# ------------------------------------------------------------------------------------------------

# Adds the Dependency-SDL2 dependency. By default downloads from Github, but if PH_SDL2_ROOT is
# set that version will be used instead. The following variables will be set:
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES} and ${SDL2_RUNTIME_FILES}
function(phAddSDL2)

	if (PH_SDL2_ROOT)
		message("-- [PhantasyEngine]: Adding Dependency-SDL2 from: \"${PH_SDL2_ROOT}\"")
		add_subdirectory(${PH_SDL2_ROOT} ${CMAKE_BINARY_DIR}/sdl2)

	else()
		message("-- [PhantasyEngine]: Downloading Dependency-SDL2 from GitHub")

		FetchContent_Declare(
			sdl2
			GIT_REPOSITORY https://github.com/PhantasyEngine/Dependency-SDL2.git
			GIT_TAG 1a44fa61353e86f48efc17e31d575a11382c59d6
		)
		FetchContent_GetProperties(sdl2)
		if(NOT sdl2_POPULATED)
			FetchContent_Populate(sdl2)
			add_subdirectory(${sdl2_SOURCE_DIR} ${CMAKE_BINARY_DIR}/sdl2)
		endif()
	endif()

	set(SDL2_FOUND ${SDL2_FOUND} PARENT_SCOPE)
	set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS} PARENT_SCOPE)
	set(SDL2_LIBRARIES ${SDL2_LIBRARIES} PARENT_SCOPE)
	set(SDL2_RUNTIME_FILES ${SDL2_RUNTIME_FILES} PARENT_SCOPE)

endfunction()

# Adds sfzCore. By default downloads from GitHub, but if PH_SFZ_CORE_ROOT is set that version will
# be used instead. The following variables will be set:
# ${SFZ_CORE_FOUND}, ${SFZ_CORE_INCLUDE_DIRS}, ${SFZ_CORE_LIBRARIES}
function(phAddSfzCore)

	if (PH_SFZ_CORE_ROOT)
		message("-- [PhantasyEngine]: Adding sfzCore from: \"${PH_SFZ_CORE_ROOT}\"")
		add_subdirectory(${PH_SFZ_CORE_ROOT} ${CMAKE_BINARY_DIR}/sfzCore)

	else()
		message("-- [PhantasyEngine]: Downloading sfzCore from GitHub")

		FetchContent_Declare(
			sfzCore
			GIT_REPOSITORY https://github.com/PetorSFZ/sfzCore.git
			GIT_TAG 72c4b0e86fff6d5bbdfc5ab8d56caeec6f9055e6
		)
		FetchContent_GetProperties(sfzCore)
		if(NOT sfzCore_POPULATED)
			FetchContent_Populate(sfzCore)
			add_subdirectory(${sfzcore_SOURCE_DIR} ${CMAKE_BINARY_DIR}/sfzCore)
		endif()
	endif()

	set(SFZ_CORE_FOUND ${SFZ_CORE_FOUND} PARENT_SCOPE)
	set(SFZ_CORE_INCLUDE_DIRS ${SFZ_CORE_INCLUDE_DIRS} PARENT_SCOPE)
	set(SFZ_CORE_LIBRARIES ${SFZ_CORE_LIBRARIES} PARENT_SCOPE)
	if(SFZ_CORE_OPENGL_FOUND)
		set(SFZ_CORE_OPENGL_FOUND ${SFZ_CORE_OPENGL_FOUND} PARENT_SCOPE)
		set(SFZ_CORE_OPENGL_INCLUDE_DIRS ${SFZ_CORE_OPENGL_INCLUDE_DIRS} PARENT_SCOPE)
		set(SFZ_CORE_OPENGL_LIBRARIES ${SFZ_CORE_OPENGL_LIBRARIES} PARENT_SCOPE)
		set(SFZ_CORE_OPENGL_RUNTIME_FILES ${SFZ_CORE_OPENGL_RUNTIME_FILES} PARENT_SCOPE)
	endif()

endfunction()

# Adds the bundled externals, this is currently stb and dear-imgui.
# stb: ${STB_FOUND}, ${STB_INCLUDE_DIRS}
# dear-imgui: ${IMGUI_FOUND}, ${IMGUI_INCLUDE_DIRS}, ${IMGUI_LIBRARIES}
# tinygltf: ${TINYGLTF_FOUND}, ${TINYGLTF_INCLUDE_DIRS}
function(phAddBundledExternals)

	message("-- [PhantasyEngine]: Adding stb target")
	add_subdirectory(${PH_ROOT}/externals/stb ${CMAKE_BINARY_DIR}/stb)
	set(STB_FOUND ${STB_FOUND} PARENT_SCOPE)
	set(STB_INCLUDE_DIRS ${STB_INCLUDE_DIRS} PARENT_SCOPE)

	message("-- [PhantasyEngine]: Adding dear-imgui target")
	add_subdirectory(${PH_ROOT}/externals/dear-imgui ${CMAKE_BINARY_DIR}/dear-imgui)
	set(IMGUI_FOUND ${IMGUI_FOUND} PARENT_SCOPE)
	set(IMGUI_INCLUDE_DIRS ${IMGUI_INCLUDE_DIRS} PARENT_SCOPE)
	set(IMGUI_LIBRARIES ${IMGUI_LIBRARIES} PARENT_SCOPE)

	message("-- [PhantasyEngine]: Adding tinygltf target")
	add_subdirectory(${PH_ROOT}/externals/tinygltf ${CMAKE_BINARY_DIR}/tinygltf)
	set(TINYGLTF_FOUND ${TINYGLTF_FOUND} PARENT_SCOPE)
	set(TINYGLTF_INCLUDE_DIRS ${TINYGLTF_INCLUDE_DIRS} PARENT_SCOPE)

endfunction()

# PhantasyEngine targets
# ------------------------------------------------------------------------------------------------

# Adds the interface and engine targets
function(phAddPhantasyEngineTargets)
	message("-- [PhantasyEngine]: Adding PhantasyEngine targets (engine and interface)")

	# Adding interface
	add_subdirectory(
		${PH_ROOT}/interface
		${CMAKE_BINARY_DIR}/PhantasyEngine-interface
	)
	set(PH_INTERFACE_FOUND ${PH_INTERFACE_FOUND} PARENT_SCOPE)
	set(PH_INTERFACE_INCLUDE_DIRS ${PH_INTERFACE_INCLUDE_DIRS} PARENT_SCOPE)
	set(PH_INTERFACE_LIBRARIES ${PH_INTERFACE_LIBRARIES} PARENT_SCOPE)

	# Adding engine
	add_subdirectory(
		${PH_ROOT}/engine
		${CMAKE_BINARY_DIR}/PhantasyEngine-engine
	)
	set(PHANTASY_ENGINE_FOUND ${PHANTASY_ENGINE_FOUND} PARENT_SCOPE)
	set(PHANTASY_ENGINE_INCLUDE_DIRS ${PHANTASY_ENGINE_INCLUDE_DIRS} PARENT_SCOPE)
	set(PHANTASY_ENGINE_LIBRARIES ${PHANTASY_ENGINE_LIBRARIES} PARENT_SCOPE)

endfunction()

# Renderers
# ------------------------------------------------------------------------------------------------

# Adds the Renderer-CompatibleGL target, static for iOS and Emscripten, dynamic for everything else
function(phAddRendererCompatibleGL)
	message("-- [PhantasyEngine]: Adding Renderer-CompatibleGL target")

	# Emscripten and iOS needs statically linked renderer
	if(EMSCRIPTEN OR IOS)
		message("  -- Static renderer")
		set(PH_RENDERER_COMPATIBLE_GL_STATIC true)
		set(PH_RENDERER_COMPATIBLE_GL_STATIC ${PH_RENDERER_COMPATIBLE_GL_STATIC} PARENT_SCOPE)
	else()
		message("  -- Dynamic renderer")
	endif()

	# Adding renderer
	add_subdirectory(
		${PH_ROOT}/renderers/CompatibleGL
		${CMAKE_BINARY_DIR}/Renderer-CompatibleGL
	)
	set(PH_RENDERER_COMPATIBLE_GL_FOUND ${PH_RENDERER_COMPATIBLE_GL_FOUND} PARENT_SCOPE)
	set(PH_RENDERER_COMPATIBLE_GL_LIBRARIES ${PH_RENDERER_COMPATIBLE_GL_LIBRARIES} PARENT_SCOPE)
	set(PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR ${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR} PARENT_SCOPE)

endfunction()

# Linking
# ------------------------------------------------------------------------------------------------

# Links SDL2 to the specified target
function(phLinkSDL2 linkTarget)
	target_include_directories(${linkTarget} PUBLIC ${SDL2_INCLUDE_DIRS})
	target_link_libraries(${linkTarget} ${SDL2_LIBRARIES})
endfunction()

# Links sfzCore to the specified target
function(phLinkSfzCore linkTarget)
	target_include_directories(${linkTarget} PUBLIC ${SFZ_CORE_INCLUDE_DIRS})
	target_link_libraries(${linkTarget} ${SFZ_CORE_LIBRARIES})
endfunction()

# Links the bundled externals to the specified target
function(phLinkBundledExternals linkTarget)
	target_include_directories(${linkTarget} PUBLIC
		${STB_INCLUDE_DIRS}
		${IMGUI_INCLUDE_DIRS}
		${TINYGLTF_INCLUDE_DIRS}
	)
	target_link_libraries(${linkTarget}
		${IMGUI_LIBRARIES}
	)
endfunction()

# Links PhantasyEngine "engine" and "interface" to the specified target
function(phLinkPhantasyEngine linkTarget)
	target_include_directories(${linkTarget} PUBLIC
		${PH_INTERFACE_INCLUDE_DIRS}
		${PHANTASY_ENGINE_INCLUDE_DIRS}
	)
	target_link_libraries(${linkTarget}
		${PH_INTERFACE_LIBRARIES}
		${PHANTASY_ENGINE_LIBRARIES}
	)
endfunction()

# Links the Renderer-CompatibleGL to the specified target, static or dynamic depending on the value
# of PH_RENDERER_COMPATIBLE_GL_STATIC (set automatically by phAddRendererCompatibleGL()).
function(phLinkRendererCompatibleGL linkTarget)
	if(PH_RENDERER_COMPATIBLE_GL_STATIC)
		message("-- [PhantasyEngine]: Statically linking Renderer-CompatibleGL")
		target_link_libraries(${linkTarget} ${PH_RENDERER_COMPATIBLE_GL_LIBRARIES})
	else()
		message("-- [PhantasyEngine]: Dynamically linking Renderer-CompatibleGL")
		add_dependencies(${linkTarget} ${PH_RENDERER_COMPATIBLE_GL_LIBRARIES})
	endif()
endfunction()

# Copy files functions
# ------------------------------------------------------------------------------------------------

# Windows only, copies runtime DLLs to the output binary directory (${CMAKE_BINARY_DIR}).
#
# This function takes a variable number of parameters (i.e. vararg), each parameter is a path to
# a DLL to be copied in to your binary directory.
#
# Commonly you should call this with ${SDL2_RUNTIME_FILES} and ${SFZ_CORE_OPENGL_RUNTIME_FILES}
# (if you are using OpenGL).
function(phMsvcCopyRuntimeDLLs)
	if(MSVC)
		message("-- [PhantasyEngine]: Copying following DLLs to binary directory:")
		foreach(dllPath ${ARGV})
			message("  -- ${dllPath}")
		endforeach()

		foreach(dllPath ${ARGV})
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
			file(COPY ${dllPath} DESTINATION ${CMAKE_BINARY_DIR}/Release)
		endforeach()

	endif()
endfunction()

# iOS only, copies resources directories to .app after a build of specified target is completed.
#
# This function should be seen as a iOS specific replacement for phCreateSymlinkScript() rather than
# phMsvcCopyRuntimeDLLs(). See phCreateSymlinkScript() for more information.
function(phIosCopyRuntimeDirectories copyTarget)
	if(IOS)
		message("-- [PhantasyEngine]: Copying following directories to iOS .app:")
		foreach(dirPath ${ARGV})
			if(${dirPath} STREQUAL ${copyTarget})
				continue()
			endif()
			message("  -- ${dirPath}")
			set(DIRS_TO_COPY ${DIRS_TO_COPY} ${dirPath})
		endforeach()

		foreach(dirPath ${DIRS_TO_COPY})
			get_filename_component(dirName ${dirPath} NAME)
			add_custom_command(TARGET ${copyTarget} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_directory
				${dirPath} $<TARGET_FILE_DIR:${copyTarget}>/${dirName}
			)
		endforeach()
	endif()
endfunction()

# Symlink script generation
# ------------------------------------------------------------------------------------------------

# Creates a shell script in the output binary directory (${CMAKE_BINARY_DIR}) which creates symlinks
# to the specified directories. This is useful in order to symlink in runtime directories,
# containing things such as assets and shaders, into the build directory.
#
# This function takes a variable number of parameters (i.e. vararg), each parameter is a path to
# a directory to be symlinked in to your build directory.
#
# For PhantasyEngine you are normally expected to have a "res" directory where you keep all your
# runtime assets, this directory can then be symlinked in using this function.
function(phCreateSymlinkScript)

	if(MSVC)
		message("-- [PhantasyEngine]: Creating \"create_symlinks.bat\" for the following directories:")
		foreach(symlinkPath ${ARGV})
			message("  -- ${symlinkPath}")
		endforeach()

		# Directories
		set(SYMLINK_FILE "${CMAKE_BINARY_DIR}/create_symlinks.bat")
		set(DEBUG_DIR "${CMAKE_BINARY_DIR}/Debug")
		set(RELWITHDEBINFO_DIR "${CMAKE_BINARY_DIR}/RelWithDebInfo")
		set(RELEASE_DIR "${CMAKE_BINARY_DIR}/Release")

		foreach(symlinkPath ${ARGV})

			# Get name of directory to symlink
			get_filename_component(SYMLINK_DIR_NAME ${symlinkPath} NAME)

			# Append symlink commands to file
			file(APPEND ${SYMLINK_FILE} ": Create symlinks for \"${SYMLINK_DIR_NAME}\"\n")
			file(APPEND ${SYMLINK_FILE} "mklink /D \"${CMAKE_BINARY_DIR}/${SYMLINK_DIR_NAME}\" \"${symlinkPath}\"\n")
			file(APPEND ${SYMLINK_FILE} "mklink /D \"${DEBUG_DIR}/${SYMLINK_DIR_NAME}\" \"${symlinkPath}\"\n")
			file(APPEND ${SYMLINK_FILE} "mklink /D \"${RELWITHDEBINFO_DIR}/${SYMLINK_DIR_NAME}\" \"${symlinkPath}\"\n")
			file(APPEND ${SYMLINK_FILE} "mklink /D \"${RELEASE_DIR}/${SYMLINK_DIR_NAME}\" \"${symlinkPath}\"\n")
			file(APPEND ${SYMLINK_FILE} "\n")
		endforeach()

	else()
		message("-- [PhantasyEngine]: Creating \"create_symlinks.sh\" for the following directories:")
		foreach(symlinkPath ${ARGV})
			message("  -- ${symlinkPath}")
		endforeach()

		# Directories
		set(SYMLINK_FILE "${CMAKE_BINARY_DIR}/create_symlinks.sh")
		set(DEBUG_DIR "${CMAKE_BINARY_DIR}/Debug")
		set(RELWITHDEBINFO_DIR "${CMAKE_BINARY_DIR}/RelWithDebInfo")
		set(RELEASE_DIR "${CMAKE_BINARY_DIR}/Release")

		# Append create directories commands to file
		file(APPEND ${SYMLINK_FILE} "# Create Debug, Release and RelWithDebInfo directories\n")
		file(APPEND ${SYMLINK_FILE} "# (These are used for some IDE's, such as Xcode. Not for makefiles.)\n")
		file(APPEND ${SYMLINK_FILE} "mkdir ${DEBUG_DIR}\n")
		file(APPEND ${SYMLINK_FILE} "mkdir ${RELWITHDEBINFO_DIR}\n")
		file(APPEND ${SYMLINK_FILE} "mkdir ${RELEASE_DIR}\n")
		file(APPEND ${SYMLINK_FILE} "\n")

		foreach(symlinkPath ${ARGV})

			# Get name of directory to symlink
			get_filename_component(SYMLINK_DIR_NAME ${symlinkPath} NAME)

			# Append symlink commands to file
			file(APPEND ${SYMLINK_FILE} "# Create symlinks for \"${SYMLINK_DIR_NAME}\"\n")
			file(APPEND ${SYMLINK_FILE} "ln -s ${symlinkPath} ${CMAKE_BINARY_DIR}/${SYMLINK_DIR_NAME}\n")
			file(APPEND ${SYMLINK_FILE} "ln -s ${symlinkPath} ${DEBUG_DIR}/${SYMLINK_DIR_NAME}\n")
			file(APPEND ${SYMLINK_FILE} "ln -s ${symlinkPath} ${RELWITHDEBINFO_DIR}/${SYMLINK_DIR_NAME}\n")
			file(APPEND ${SYMLINK_FILE} "ln -s ${symlinkPath} ${RELEASE_DIR}/${SYMLINK_DIR_NAME}\n")
			file(APPEND ${SYMLINK_FILE} "\n")
		endforeach()
	endif()

endfunction()
