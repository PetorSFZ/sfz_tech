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

The core part of sfzCore uses no dependencies (but some of the extra libraries does) and compiles cleanly on most compilers, including MSVC, Clang and Emscripten.

In addition to the core part of `sfzCore` there are are also optional, experimental helper libraries for various rendering API's. These libraries should not be considered stable, and change often. Use at your own risk. In order to use them specific variables need to be defined before including the sfzCore project. These libraries are:

* `lib-core`: The core sfzCore library, no dependencies, always included
* `lib-opengl`: OpenGL helper library, provides and uses included `GLEW`, define `SFZ_CORE_OPENGL` before including sfzCore.

## Building

### Windows

#### Visual Studio 2019

Simply open this directory as a folder.

#### Visual Studio 2017

As a pre-requisite __CMake__ need to be installed and available in the path. In order to generate the Visual Studio solution you only have to run `gen_visualstudio2017.bat` in the `gen_scripts` directory (double clicking is enough, no need to open a command prompt).

#### Emscripten (Web)

* Install Make for Windows (available in chocolatey)
* Install Emscripten, run:
  * `emsdk update`
  * `emsdk install latest`
  * `emsdk activate latest`
* To use emscripten in a terminal first run `emsdk_env.bat`, then `emcc` is available.
* Set the `EMSCRIPTEN` system variable properly
* `cmake -DCMAKE_TOOLCHAIN_FILE="%EMSCRIPTEN%\cmake\Modules\Platform\Emscripten.cmake" .. -G "Unix Makefiles" `

In order to run locally in a browser you need to host the files. Recommend using python http-here:

* `pip install http-here`
* `python3 -m http.server --bind 127.0.0.1`


### macOS

#### Xcode (macOS)

As a pre-requisite __Xcode__ and __CMake__ need to be installed. CMake can easily be installed using __Homebrew__. Only the latest Xcode version available is actively tested, older versions are not guaranteed to work.

In order to generate the Xcode project you only have to run `gen_xcode.sh` in the `gen_scripts` directory from a terminal.

#### Xcode (iOS)

The same pre-requisites as for a macOS build, but in addition [Polly](https://github.com/ruslo/polly) need to be installed. The following environment variables need to be set:

~~~sh
export POLLY_ROOT="<path to polly install>"
export POLLY_IOS_DEVELOPMENT_TEAM="<Your Apple Developer Team ID>"
export POLLY_IOS_BUNDLE_IDENTIFIER="<Your Apple Developer App ID>"
~~~

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

There are two recommended ways of including sfzCore in your project, including it directly or downloading it using [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html).

To include sfzCore directly just copy the whole project into your project's directory. Then add the following line to your `CMakeLists.txt`:

~~~cmake
#set(SFZ_CORE_OPENGL true) # Define SFZ_CORE_OPENGL if you want lib-opengl
add_subdirectory(path_to_sfzCore_dir)
~~~

Using `FetchContent` sfzCore can be added with the following CMake commands:

~~~cmake
#set(SFZ_CORE_OPENGL true) # Define SFZ_CORE_OPENGL if you want lib-opengl
FetchContent_Declare(
	sfzCore
	GIT_REPOSITORY https://github.com/PetorSFZ/sfzCore.git
	GIT_TAG <commit tag for version you want>
)
FetchContent_GetProperties(sfzCore)
if(NOT sfzCore_POPULATED)
    FetchContent_Populate(sfzCore)
    add_subdirectory(${sfzcore_SOURCE_DIR} ${CMAKE_BINARY_DIR}/sfzCore)
endif()
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

If including `lib-opengl` by defining `SFZ_CORE_OPENGL` the following variables will also be available:

- `${SFZ_CORE_OPENGL_FOUND}`: True, used to check if lib-opengl is available
- `${SFZ_CORE_OPENGL_INCLUDE_DIRS}`: The include directories
- `${SFZ_CORE_OPENGL_LIBRARIES}`: The libraries
- `${SFZ_CORE_OPENGL_RUNTIME_FILES}`: Runtime libraries (i.e. `.dll`'s) that need to be copied into build directory

### iOS

When compiling for iOS `SFZ_IOS` must be defined, otherwise you will get compile errors.

## Usage

### Context creation

sfzCore stores all of its global singletons inside a context. Such a contex must be set before sfzCore can be used, this can be accomplished by including `#include <sfz/Context.hpp>` and calling `sfz::setContext(sfz::getStandardContext());` in the beginning of your program before you start using sfzCore. The use of contexts makes it easier to use sfzCore with dynamic libraries. See the `Context.hpp` header for more documentation.

## License

Licensed under zlib, this means that you can basically use the code however you want as long as you give credit and don't claim you wrote it yourself. See LICENSE file for more info.
