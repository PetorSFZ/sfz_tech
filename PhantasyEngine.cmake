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

# Options
# ------------------------------------------------------------------------------------------------

# PH_CUDA_SUPPORT: Will build with CUDA support if defined

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

	elseif(IOS)
		set(PH_CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-rtti -fno-strict-aliasing -DSFZ_IOS -DPH_STATIC_LINK_RENDERER")
		set(PH_CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
		set(PH_CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
		set(PH_CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")

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
