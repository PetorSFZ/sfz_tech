# sfzCore

New work in progress base library for graphical applications. The goal is to have replacements for some STL functionality (allocators, containers, etc). It also aims to replace SkipIfZero Common when finished, but it has not yet been decided if OpenGL or Vulkan should be used.

## Building

It is advisable to let CMake generate the wanted build solution in a directory called `build` inside the project folder as this folder is ignored by git. The CMake variable `SFZ_CORE_BUILD_TESTS` determines whether the tests should be built or not. Do note that the library is currently only regularly tested on Windows, so building on other platforms may not work. If you find any problems please create an issue so they can be addressed.

### Requirements
- __CMake__
- __SDL2__
- __SDL2_Mixer__
- Windows: __Visual Studio 2015__ or newer

### Windows

#### Installing dependencies

##### SDL2 & SDL2 Mixer
Download and install the SDL2 development libraries for Visual C++ from the official website. Then create an environment variable called `SDL2` and point it to the root of the SDL2 installation. Then do the same for the SDL2_Mixer library. Note that the `SDL2` variable should contain the path to both libraries.

#### Generating Visual Studio solution

* Create a directory called `build` in the root sfzCore directory
* Open CMD inside the `build` directory (shift + right click -> "Open command window here")
* Run the following command:


	// With tests
	cmake .. -G "Visual Studio 14 2015 Win64" -DSFZ_CORE_BUILD_TESTS:BOOLEAN=TRUE

	// Without tests
	cmake .. -G "Visual Studio 14 2015 Win64"


### Mac OS X

Mac OS X is currently untested (but should hopefully compile). Dependencies can be installed using homebrew. Xcode project can be generated with the following commands (starting from the root directory):

	mkdir build

	cd build

	// With tests
	cmake .. -GXcode -DSFZ_CORE_BUILD_TESTS:BOOLEAN=TRUE

	// Without tests
	cmake .. -GXcode


## Linking

### CMake
To link sfzCore in your CMake project the easiest solution is to copy the wanted version into a subdirectory of your project. Then use the following command in your `CMakeLists.txt` to add the `${SFZ_CORE_INCLUDE_DIRS}` and `${SFZ_CORE_LIBRARIES}` variables:

	add_subdirectory(path_to_SkipIfZero_Core_dir)

Now you can link sfzCore with the following commands:

	include_directories(${SFZ_CORE_INCLUDE_DIRS})

	target_link_libraries(some_executable ${SFZ_CORE_LIBRARIES})

If you also want to build the tests (but really, why would you in this situation?) you also need to define `SFZ_CORE_BUILD_TESTS` before adding the directory.

In addition to sfzCore, the `${SFZ_CORE_INCLUDE_DIRS}` and `${SFZ_CORE_LIBRARIES` variables expose the internal libraries used by sfzCore (such as SDL2).

## License
Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.

Libraries used by sfzCore fall under various licenses, see their respective LICENSE file for more info.
