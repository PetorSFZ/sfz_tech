# sfzCore

sfzCore is a base library for SkipIfZero graphical applications, replacing the now deprecated SkipIfZero Common library. It aims to provide high quality common components, while still keeping code bloat to a minimum. Among other things sfzCore contains:

* Replacement for parts of STL (allocators, containers, strings, smartpointers, etc) that aim to be simpler, faster and more low-level
* Instance based allocators used for all memory allocation
* Matrix, Vector and Quaternion primitives (Designed to also be usable in CUDA device code)
* Math functions useful for graphics
* Geometric primitives (AABB, OBB, Spheres, etc) with some intersection tests
* A simple game loop implementation
* Some wrappers around SDL to make it more intuitive to handle user input
* An ini parser
* Cross-platform IO functions
* (Optionally) OpenGL wrappers, utilities and shaders

## Building

It is advisable to let CMake generate the wanted build solution in a directory called `build` inside the project folder as this folder is ignored by git. Do note that the library is currently only regularly tested on Windows, so building on other platforms may not work. If you find any problems please create an issue so they can be addressed.

### Requirements
- __CMake__
- Windows: __Visual Studio 2015__ or newer

### Flags

Different parts of sfzCore will be activated depending on what flags are used. Currently the following flags are available:

* `SFZ_CORE_BUILD_TESTS`: Includes and builds the tests
* `SFZ_CORE_OPENGL`: Includes the OpenGL part of the library

Flags can be set using `-DNAME_OF_FLAG=TRUE` when generating a project, or by calling `set(NAME_OF_FLAG TRUE)` before including sfzCore in a CMake file.

### Windows

#### Generating Visual Studio solution

* Create a directory called `build` in the root sfzCore directory
* Open CMD inside the `build` directory (shift + right click -> "Open command window here")
* Run the following command:


	cmake .. -G "Visual Studio 14 2015 Win64" -DSFZ_CORE_BUILD_TESTS=TRUE -DSFZ_CORE_OPENGL=TRUE

	// Or alternatively if you dont want OpenGL
	cmake .. -G "Visual Studio 14 2015 Win64" -DSFZ_CORE_BUILD_TESTS=TRUE

	// Or if you don't want tests
	cmake .. -G "Visual Studio 14 2015 Win64"


### Mac OS X

Mac OS X is currently untested (but should hopefully compile). Dependencies can be installed using homebrew. Xcode project can be generated with the following commands (starting from the root directory):

	mkdir build

	cd build

	cmake ..  -GXcode -DSFZ_CORE_BUILD_TESTS=TRUE -DSFZ_CORE_OPENGL=TRUE

## Linking

### CMake
To link sfzCore in your CMake project the easiest solution is to copy the wanted version into a subdirectory of your project. First define any flags for options you want (such as OpenGL support), then use the following command in your `CMakeLists.txt`:

	add_subdirectory(path_to_SkipIfZero_Core_dir)

This will give you access to three CMake variables:

* `${SFZ_CORE_INCLUDE_DIRS}`: The include directories (sfzCore and dependencies such as SDL2, GLEW, etc)
* `${SFZ_CORE_LIBRARIES}`: The sfzCore libraries and sfzCore dependencies (such as SDL2)
* `${SFZ_CORE_DLLS}`: A list with paths to all runtime DLLs needed to run sfzCore (such as `SDL2.dll`)

As mentioned these variables not only include sfzCore, but also exposes the internal libraries used by sfzCore (such as SDL2). So by using them you are also using the necessary dependencies. You can then link sfzCore with the following commands:

	include_directories(${SFZ_CORE_INCLUDE_DIRS})

	target_link_libraries(some_executable ${SFZ_CORE_LIBRARIES})

	# Copy some DLLs
	file(COPY ${SFZ_CORE_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Debug)

## License
Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.

Libraries used by sfzCore fall under various licenses, see their respective LICENSE file for more info.
