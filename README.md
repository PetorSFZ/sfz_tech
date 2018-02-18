# sfzCore

sfzCore is a base library for most Skipifzero applications, replacing the now deprecated SkipIfZero Common library. It aims to provide high quality common components, while still keeping code bloat to a minimum. Among other things sfzCore contains:

* Replacement for parts of STL (allocators, containers, strings, smartpointers, etc) that aim to be simpler, faster and more low-level
* Instance based allocators used for all memory allocation
* Matrix, Vector and Quaternion primitives (Designed to also be usable in CUDA device code)
* Math functions useful for graphics
* Geometric primitives (AABB, OBB, Spheres, etc) with some intersection tests
* An ini parser
* Cross-platform IO functions
* (Optionally) Header-only CUDA wrappers and utilities

sfzCore uses no dependencies and compiles cleanly on most compilers, including MSVC, Clang and Emscripten.

## Building

### Windows

#### Visual Studio 2015

As a pre-requisite __Visual Studio 2015__ and __CMake__ need to be installed. CMake need to be available in the path. Later versions of Visual Studio will likely work, but are not actively tested.

In order to generate the Visual Studio solution you only have to run `gen_visualstudio2015.bat` in the `gen_scripts` directory (double clicking is enough, no need to open a command prompt).

#### Visual Studio 2017

Same as for Visual Studio 2015, except `gen_visualstudio2017.bat` should be used instead. Builds using the `v140` (Visual Studio 2015) platform toolset by default, so that also needs to be installed (available through the `Tools/Get Tools and Features` menu). Should probably build fine using newer toolset, but this is not regularly tested.

### macOS

#### Xcode

As a pre-requisite __Xcode__ and __CMake__ need to be installed. CMake can easily be installed using __Homebrew__. Only the latest Xcode version available is actively tested, older versions are not guaranteed to work.

In order to generate the Xcode project you only have to run `gen_xcode.sh` in the `gen_scripts` directory from a terminal.

#### Emscripten (Web)

In addition to the dependencies of the __Xcode__ build, __emscripten__ also need to be installed. This can be accomplished by the following steps:

1. `brew install emscripten`
   1. Run `emcc` once
   2. Modify `~/emscripten`
      1. LLVM should be found at `/usr/local/opt/emscripten/libexec/llvm/bin`
      2. Comment out `BINARYEN_ROOT`
2. Set `EMSCRIPTEN` path variable
   1. I.e., edit `.bash_profile`
   2. Example `EMSCRIPTEN=/usr/local/Cellar/emscripten/1.37.18/libexec`, but you have to change version to the latest one you have.

The emscripten makefiles can then be generated using either `gen_emscripten_debug.sh` or `gen_emscripten_release.sh` in the `gen_scripts` directory. After sfzCore has been built using `make` the tests can be run by using `node sfzCoreTests.js`.

## Linking sfzCore

There are two recommended ways of including sfzCore in your project, including it directly or downloading it using `DownloadProject`. The latter is recommended if you don't intend to modify sfzCore, as it will be more convenient to update to newer versions.

To include sfzCore directly just copy the whole project into your project's directory. Then add the following line to your `CMakeLists.txt`:

~~~cmake
add_subdirectory(path_to_sfzCore_dir)
~~~

To use `DownloadProject` you first have to include it into your project, it is available [here](https://github.com/Crascit/DownloadProject). Then sfzCore can be added with the following CMake commands:

~~~cmake
download_project(
	PROJ                sfzCore
	PREFIX              externals
	GIT_REPOSITORY      https://github.com/PetorSFZ/sfzCore.git
	GIT_TAG             <GIT_HASH_FOR_VERSION_YOU_WANT>
	UPDATE_DISCONNECTED 1
	QUIET
)
add_subdirectory(${sfzCore_SOURCE_DIR})
~~~

Regardless of how you included it the following CMake variables will be available afterwards:

- `${SFZ_CORE_FOUND}`: True, used to check if sfzCore is available


- `${SFZ_CORE_INCLUDE_DIRS}`: The include directories
- `${SFZ_CORE_LIBRARIES}`: The sfzCore libraries

You can the simply link the library using the normal:

~~~cmake
target_include_directories(some_executable ${SFZ_CORE_INCLUDE_DIRS})
target_link_libraries(some_executable ${SFZ_CORE_LIBRARIES})
~~~

## Usage

### Context creation

sfzCore stores all of its global singletons inside a context. Such a contex must be set before sfzCore can be used, this can be accomplished by including `#include <sfz/Context.hpp>` and calling `sfz::setContext(sfz::getStandardContext());` in the beginning of your program before you start using sfzCore. The use of contexts makes it easier to use sfzCore with dynamic libraries. See the `Context.hpp` header for more documentation.

## License

Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.
