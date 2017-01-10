# sfzCore

sfzCore is a base library for SkipIfZero graphical applications, replacing the now deprecated SkipIfZero Common library. It aims to provide high quality common components, while still keeping code bloat to a minimum. Among other things sfzCore contains:

* Replacements for parts of STL (allocators, containers, etc) that aim to be simpler, faster and more low-level
* Matrix and Vector primitives, along with some useful math functions
* Geometric primitives (AABB, OBB, Spheres, etc) with some intersection tests
* A simple game loop implementation
* Some wrappers around SDL to make it more intuitive to handle user input
* An ini parser
* Cross-platform IO functions
* (Optionally) OpenGL wrappers, utilities and shaders

## Building

It is advisable to let CMake generate the wanted build solution in a directory called `build` inside the project folder as this folder is ignored by git. The CMake variable `SFZ_CORE_BUILD_TESTS` determines whether the tests should be built or not. Do note that the library is currently only regularly tested on Windows, so building on other platforms may not work. If you find any problems please create an issue so they can be addressed.

### Requirements
- __CMake__
- __SDL2__
- Windows: __Visual Studio 2015__ or newer

### Flags

Different parts of sfzCore will be activated depending on what flags are used. Currently the following flags are available:

* `SFZ_CORE_BUILD_TESTS`: Includes and builds the tests
* `SFZ_CORE_OPENGL`: Includes the OpenGL part of the library

Flags can be set using `-DNAME_OF_FLAG=TRUE` when generating a project, or by calling `set(NAME_OF_FLAG TRUE)` before including sfzCore in a CMake file.

### Windows

#### Installing dependencies

##### SDL2
Download and install the SDL2 development libraries for Visual C++ from the official website. Then create an environment variable called `SDL2` and point it to the root of the SDL2 installation.

#### Generating Visual Studio solution

* Create a directory called `build` in the root sfzCore directory
* Open CMD inside the `build` directory (shift + right click -> "Open command window here")
* Run the following command:


	// With tests
	cmake .. -G "Visual Studio 14 2015 Win64" -DSFZ_CORE_BUILD_TESTS=TRUE

	// Without tests
	cmake .. -G "Visual Studio 14 2015 Win64"

	// Optional OpenGL flag
	-DSFZ_CORE_OPENGL=TRUE

### Mac OS X

Mac OS X is currently untested (but should hopefully compile). Dependencies can be installed using homebrew. Xcode project can be generated with the following commands (starting from the root directory):

	mkdir build

	cd build

	// With tests
	cmake .. -GXcode -DSFZ_CORE_BUILD_TESTS=TRUE

	// Without tests
	cmake .. -GXcode

	// Optional OpenGL flag
	-DSFZ_CORE_OPENGL=TRUE

## Linking

### CMake
To link sfzCore in your CMake project the easiest solution is to copy the wanted version into a subdirectory of your project. First define any flags for options you want (such as OpenGL support), then use the following command in your `CMakeLists.txt` to add the `${SFZ_CORE_INCLUDE_DIRS}` and `${SFZ_CORE_LIBRARIES}` variables:

	add_subdirectory(path_to_SkipIfZero_Core_dir)

Now you can link sfzCore with the following commands:

	include_directories(${SFZ_CORE_INCLUDE_DIRS})

	target_link_libraries(some_executable ${SFZ_CORE_LIBRARIES})

In addition to sfzCore, the `${SFZ_CORE_INCLUDE_DIRS}` and `${SFZ_CORE_LIBRARIES` variables expose the internal libraries used by sfzCore (such as SDL2).

## License
Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.

Libraries used by sfzCore fall under various licenses, see their respective LICENSE file for more info.
